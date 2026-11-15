#!/usr/bin/env python3
"""Generate rusty_v8 bindings with a native Haiku bindgen installation."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="patched rusty_v8 source root")
    parser.add_argument("build", type=Path, help="GN output with project.json")
    parser.add_argument("output", type=Path, help="destination Rust bindings")
    parser.add_argument("--bindgen", default="bindgen", help="bindgen executable")
    args = parser.parse_args()
    source, build, output = (p.resolve() for p in (args.source, args.build, args.output))
    defines = subprocess.check_output(
        [sys.executable, "tools/get_bindgen_args.py", "--gn-out", str(build)], cwd=source
    ).decode().split("\0")
    command = [args.bindgen, "--generate-cstr"]
    for option, patterns in (
        ("--rustified-enum", (".*UseCounterFeature", ".*ModuleImportPhase", ".*Intercepted")),
        ("--bitfield-enum", (".*GCType", ".*GCCallbackFlags")),
        ("--allowlist-item", ("v8__.*", "cppgc__.*", "RustObj", "memory_span_t",
                              "const_memory_span_t", "ExternalConstOneByteStringResource")),
        ("--blocklist-item", ("cppgc.*Visitor", "RustObj.*Trace")),
    ):
        for pattern in patterns:
            command.extend((option, pattern))
    command.extend(("src/binding.hpp", "-o", str(output), "--", "-x", "c++", "-std=c++20",
                    "-nostdinc++", "-Iv8/include", "-I.",
                    "-isystembuildtools/third_party/libc++",
                    "-isystemthird_party/libc++/src/include",
                    "-isystemthird_party/libc++abi/src/include"))
    command.extend(value for value in defines if value)
    subprocess.run(command, cwd=source, check=True, env=os.environ.copy())


if __name__ == "__main__":
    main()
