#!/bin/bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_dir=$(cd -- "$script_dir/../.." && pwd)
case "$(uname -s)" in
    Haiku) ;;
    *) echo 'This helper must run natively on Haiku/RenkuOS.' >&2; exit 1 ;;
esac
build_support_dir="${CODEX_RENKU_BUILD_SUPPORT_DIR:-${TMPDIR:-/tmp}/codex-renku-build-support}"
mkdir -p "$build_support_dir"
shim="$build_support_dir/compiler-write-buffer.so"
gcc -shared -fPIC "$script_dir/compiler_write_buffer.c" -o "$shim"
cd "$repo_dir/codex-rs"
export ZSTD_SYS_USE_PKG_CONFIG=1
export CARGO_BUILD_JOBS="${CARGO_BUILD_JOBS:-1}"
export CARGO_INCREMENTAL=0
export CARGO_PROFILE_DEV_DEBUG=0
LD_PRELOAD="$shim${LD_PRELOAD:+:$LD_PRELOAD}" \
    cargo build --locked --offline --ignore-rust-version -p codex-cli --bin codex "$@"
