#!/usr/bin/env python3
"""Run the native updater handoff against a local, controlled successor."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

if len(sys.argv) != 3:
    raise SystemExit("usage: updater-handoff-check.py /absolute/path/to/codex /absolute/temp/directory")
binary = Path(sys.argv[1]).resolve(strict=True)
temp_root = Path(sys.argv[2]).resolve(strict=True)
fixture = Path(__file__).resolve().with_name("updater-handoff-fixture.sh")

with tempfile.TemporaryDirectory(prefix="codex-handoff-", dir=temp_root) as temp:
    home = Path(temp)
    root = home / "packages/app-server-daemon"
    # The public updater does not list Haiku releases. This label only admits
    # our shell fixture through its eligibility check; it runs no Linux code.
    release = "1.2.3-x86_64-unknown-linux-musl"
    successor = root / "releases" / release / "bin/codex"
    successor.parent.mkdir(parents=True)
    shutil.copyfile(fixture, successor)
    successor.chmod(0o755)
    (root / "current").symlink_to(Path("releases") / release)
    (root / "auto-update-version").write_text(release)
    marker = home / "marker"
    env = dict(os.environ, CODEX_HOME=str(home), CODEX_TEST_HANDOFF_MARKER=str(marker))
    process = subprocess.Popen(
        [str(binary), "app-server", "daemon", "pid-update-loop"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    try:
        try:
            stdout, stderr = process.communicate(timeout=35)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()
            raise
        if process.returncode != 0:
            raise RuntimeError(f"updater exited {process.returncode}: {stdout} {stderr}")
        lines = marker.read_text().splitlines()
        if lines[1:4] != ["app-server", "daemon", "pid-update-loop"]:
            raise RuntimeError(f"unexpected successor arguments: {lines}")
        if int(lines[4]) != process.pid:
            raise RuntimeError(f"PID changed: started {process.pid}, successor {lines[4]}")
        print(f"updater handoff preserved PID and arguments: {process.pid}")
    finally:
        # exec bypasses the old socket guard. Clean up only the uniquely named
        # socket created for this temporary home, never an arbitrary link target.
        alias = home / "app-server-daemon/daemon-updater.sock"
        if alias.is_symlink():
            target = Path(os.readlink(alias))
            private = Path("/tmp").resolve() / f"codex-daemon-{os.geteuid()}"
            if target.parent == private and re.fullmatch(r"[0-9a-f]{64}", target.name):
                target.unlink(missing_ok=True)
                target.with_suffix(".lock").unlink(missing_ok=True)
