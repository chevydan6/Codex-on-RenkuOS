# RenkuOS native build notes

Displayed version: 0.171.0-alpha.5. This experimental port is based on
upstream development commit 336b19ab26e0d4e36b16d8a21d668632cd800207,
not the exact upstream release tag.

This is an experimental Haiku/RenkuOS port. The native build has been exercised
on x86-64 RenkuOS with Rust 1.94.1, the system C/C++ toolchain, `protoc`, and
the system Zstandard development package. Fetch the Cargo dependencies before
using `--offline`.

For the tested native build mitigation, run from the repository root:

```sh
third_party/renkuos/build-native.sh
```

Keep `TMPDIR` on a volume with enough free space and set `CARGO_TARGET_DIR` as
needed. The helper builds the compiler-only write interceptor and scopes it to
Cargo. Two full buffered builds passed, including forced core recompilation;
the helper's native smoke check also passed. The underlying OS fault remains
unresolved. See [STRESS-TEST-REPORT.md](STRESS-TEST-REPORT.md) for the test audit.

The underlying build invocation, from `codex-rs`, is:

```sh
ZSTD_SYS_USE_PKG_CONFIG=1 CARGO_BUILD_JOBS=1 CARGO_INCREMENTAL=0 \
  CARGO_PROFILE_DEV_DEBUG=0 cargo build --locked --offline \
  --ignore-rust-version -p codex-cli --bin codex
```

The system Zstandard library avoids a symbol conflict between the bundled static
library and RenkuOS shared libraries. `--ignore-rust-version` is required for
the tested Rust version, which is older than this checkout's pinned toolchain.
The resulting debug executable is large and linking requires several GiB of
memory. A build with 12 GiB of guest RAM has linked successfully.

The tested executable dynamically links against `libzstd.so.1`, OpenSSL 3,
the GCC C++ runtime, and Haiku system libraries. The interactive terminal
uses crossterm's native `/dev/tty` backend on Haiku; the default event backend
stopped receiving input after the first key on the tested system.
The CLI's direct ELF dependencies also include `libbe`, `libtranslation`,
`libbsd`, `libroot`, and `libnetwork`; the code-mode host additionally needs
`libexecinfo.so.1.1`. These libraries resolved on the tested guest.
Install the HaikuPorts `ripgrep` package to provide the `rg` search command.
The packaged native client completed OpenAI device authentication using a
Google-linked ChatGPT account. A real model request returned successfully, and
a second real request ran a delegated shell command and reported its output.
The device login was completed by the account owner.
The experimental managed-daemon package uses `codex-package.json`, `bin/`,
`codex-resources/`, and `codex-path/` directories. The latter contains an
executable wrapper for the installed `/boot/system/bin/rg`. The native daemon
passed start, restart, stop, automatic startup, and a local mock-model session
with a delegated shell command. Concurrent start and status requests reported
the existing daemon without starting another. Its socket security has not been independently
validated, and the flat package still needs `codex --no-daemon`.
On the tested guest, a separate probe found that an exited child disappeared
from Haiku's team API and failed `kill(pid, 0)` before its parent collected
the exit status. Native process-management tests verify that mismatched
numeric start times do not stop a reused PID and that ambiguous older records
are preserved. These are narrow lifecycle checks, not a security audit.
The native app server also rejected a control-socket parent directory with
world-writable permissions in a separate negative test.
The generic updater re-exec test fails on Haiku because the test harness runs
it on a worker thread. A separate probe showed that `exec` succeeded on the
process's main thread but returned `Operation not allowed` from a worker.
The native dispatcher now runs on the main thread, and a regression probe
confirmed that it can replace the process. An isolated handoff test exercised
the real updater loop and verified that a controlled shell successor received
the same PID and the expected arguments. The test used a Linux-labeled dummy
release directory solely to exercise the existing eligibility check; it ran
no Linux binary. Native Haiku releases are not eligible for the upstream
public updater, and a real update from an official service remains unverified.
The regression probe is `cargo run -p codex-arg0 --example haiku-main-thread`
from `codex-rs`; success replaces the probe with a shell that exits zero.
Run the separate handoff probe on RenkuOS, passing the native executable and a
temporary directory on a volume with free space:

```sh
python3.10 third_party/renkuos/updater-handoff-check.py \
  /path/to/bin/codex /CodexBuild/tmp
```

It creates and removes its own isolated test home and uses
`updater-handoff-fixture.sh` as the successor.

RenkuOS currently has no Codex native sandbox backend. A command that requests
one must be rejected rather than presented as protected. Some platform features,
including clipboard images, have not been fully ported or validated. This is not a
release-ready build.
Interactive launches now display a no-sandbox warning and require typing `yes`
before continuing with full filesystem access; refusal or nonterminal input
cancels startup. This leaves the normal command-approval policy intact. Login,
version, and noninteractive subcommands do not use the interactive prompt;
noninteractive sandbox requests remain fail-closed.
A daemon-backed terminal mock-model test requested a workspace-write sandbox;
the delegated shell command was rejected and the model received the reason.
On the tested guest, a screenshot copied to the clipboard appears as an
`image/bitmap` BMessage containing an archived `BBitmap`. A native test covers
conversion of that representation to PNG and reading it from the clipboard.
The terminal also submitted a pasted screenshot to a local mock model service;
the service verified a 1280-by-800 PNG in the model request.
The source also handles Tracker file references, preferring a readable image
file over a bitmap preview; a native conversion test verifies the precedence
and fallback. A terminal test also pasted a Tracker-style file reference and
submitted the resulting 1280-by-800 PNG to a local mock model service. Other
clipboard representations have not been validated. Run the native conversion
test from the repository root with:

```sh
clang++ third_party/renkuos/clipboard_image_test.cpp \
  codex-rs/tui/src/clipboard_paste/haiku.cpp -lbe -ltranslation \
  -o /tmp/codex-clipboard-image-test
/tmp/codex-clipboard-image-test
```
The distributed archive was transferred back to the guest and extracted. Its
daemon started and stopped, and its terminal submitted a Tracker-style image
and completed a delegated shell command using a local mock model service.
The tested installation provides `chroot` but no `jail` or `unshare` command,
and its development headers expose no sandbox, pledge, or Landlock interface.
Those observations do not establish a way to enforce Codex's filesystem and
network policy; they are not a substitute for a sandbox backend.
Shell commands use the tested `en_US.UTF-8` locale on Haiku, where `C.UTF-8`
was unavailable on the tested RenkuOS image.

The experimental native code-mode host uses the V8 source patch in
`rusty-v8-haiku.patch`, applied to the `rusty_v8` revision pinned by this
workspace. It also needs native GN, Ninja, Clang, bindgen, libexecinfo and the
V8 checkout's pinned dependencies. The platform adaptation draws on the
[HaikuPorts Node.js 24 patchset](https://github.com/haikuports/haikuports/blob/master/net-libs/nodejs/patches/nodejs-24.21.0.patchset).
Build V8 with its sandbox enabled. A local
build produced `librusty_v8.a` and `src_binding.rs`; set `RUSTY_V8_ARCHIVE`
and `RUSTY_V8_SRC_BINDING_PATH` to those files to build the host. Use the
native Clang linker (`-C linker=clang`) with `-lnetwork -lbsd -lexecinfo`:
the tested Rust default linker did not preserve initialized C++ thread-local
data. Install `codex-code-mode-host` alongside `codex`. The host passed its 45
library tests and nine stdio integration tests on the tested guest. These tests
do not establish that the V8 sandbox or Codex's OS sandbox is secure.
The two generated inputs have also been preserved in a separate developer
archive. After transferring copies back to the guest, the host was rebuilt
from those copies and completed a local mock-model delegated command. This
does not verify building V8 from a clean source checkout.

For the tested guest, the Rust sysroot was
`/boot/home/config/develop/tools/rust`. The tools directory contained `bin/bindgen`,
`bin/rustfmt`, and `bin/python3` pointing to the native programs, and `lib`
pointing to `/boot/system/lib`. Its V8 checkout also had the `fadec` dependency
at `v8/third_party/fadec/src` pinned to
`c9f78f532b9004de278489019ab2c6c28ae9746e`, with `third_party/fadec` pointing
to `../v8/third_party/fadec` and `third_party/rust-toolchain` pointing to the
native Rust sysroot. Apply the patch from the root of that fully populated
checkout. These paths and tool versions are examples from the tested guest;
the V8 dependency checkout and patch have not been verified from a clean install.

For reference, the generated V8 build directory used these GN arguments:

```gn
is_debug = true
use_custom_libcxx = true
v8_enable_sandbox = true
v8_enable_external_code_space = true
v8_enable_pointer_compression = true
v8_enable_v8_checks = false
clang_base_path = "/boot/system"
treat_warnings_as_errors = false
rust_sysroot_absolute = "/boot/home/config/develop/tools/rust"
rust_bindgen_root = "/CodexBuild/v8-tools"
clang_version = "23"
rustc_version = "1.94.1"
```

The configured build was resumed with
`RUSTC_BOOTSTRAP=1 ninja -C /CodexBuild/v8-probe/debug/gn_out -j 2 rusty_v8`.
This records the tested configuration; it is not a verified clean-build recipe.

To build the host from an already-built native V8 archive, use the following
command from `codex-rs`:

```sh
RUSTY_V8_ARCHIVE=/path/to/librusty_v8.a \
RUSTY_V8_SRC_BINDING_PATH=/path/to/src_binding.rs \
CARGO_PROFILE_DEV_DEBUG=0 \
RUSTFLAGS='-C linker=clang -C link-arg=-lnetwork -C link-arg=-lbsd -C link-arg=-lexecinfo' \
  cargo build --locked --offline --ignore-rust-version \
  -p codex-code-mode-host --bin codex-code-mode-host
```

Set `TMPDIR` to a filesystem with several GiB free for the final CLI link.
The guest's default `/tmp` filled its boot volume during one build; using the
larger build volume for temporary files let the link complete.
During some Rust test builds, archive creation intermittently failed with
`Bad address`. The guest system log recorded a virtual-memory soft fault with
`Device/File/Resource busy` near a failure, while a memory probe reported
roughly 9 GB free. The cause has not been established. Retrying has allowed
individual dependencies to build, but a clean uninterrupted build has not
been demonstrated.
An offline CLI build in a fresh target directory did complete after one retry
of that error. Its executable passed `--version` and a local mock-model session
that delegated a shell command through the separately built code-mode host.
Placed in an isolated package, that fresh-target executable also installed,
started, restarted, and stopped its managed daemon successfully.
The same executable accepted a Tracker-style clipboard image in the terminal
and passed the controlled updater handoff check. In a daemon-backed mock-model
session it rejected a delegated command when a workspace-write sandbox was
requested, and the command's expected output was absent.
This used the existing dependency cache and is not a clean V8 source build.
The source-change archive was also applied to a separately reconstructed
upstream tree on the guest. Its offline CLI build completed after three
archive-error retries, and the resulting executable passed `--version` and a
mock-model delegated-command test with the separately built code-mode host.

A subsequent V8 build used the same patched, populated source checkout but a
new output directory. `gn gen` and all 2,450 Ninja steps completed, producing
`librusty_v8.a`. The host was rebuilt against that archive and the previously
generated Rust bindings; it passed a protocol handshake and a local mock-model
session that executed a delegated shell command. The build and session logs are
`/boot/home/codex-v8-clean-build.log`,
`/boot/home/codex-v8-clean-host-build.log`, and
`/boot/home/v8-clean-host-e2e.log` on the tested guest. This verifies a fresh
output directory, not a fresh V8 dependency checkout or sandbox security.
As a separate patch preflight, the 28 existing files modified by the V8 patch
were reconstructed from the checked-out Git revisions of their respective
repositories. The patch applied cleanly, and all 29 resulting files (including
the new file) matched the tested checkout byte for byte. This checks the patch,
not completeness of the other dependency checkouts.
For a broader source reconstruction, the root and 19 available submodules
were exported from their local Git revisions, along with the pinned fadec
checkout. After applying the patch and supplying the native toolchain symlinks,
GN generation and all 2,450 Ninja steps succeeded in fresh directories. A
host linked to that archive and the previously generated bindings completed a
local mock-model session with a delegated shell command. The guest logs are
`/boot/home/codex-v8-reconstructed-build.log`,
`/boot/home/codex-v8-reconstructed-host.log`, and
`/boot/home/v8-reconstructed-host-e2e.log`. Fetching the dependencies on a
fresh installation has not been verified. The RenkuOS VM subsequently fetched
the exact root revision, all 19 submodule revisions used in the reconstructed
build, and the pinned fadec revision from their upstream repositories using
shallow Git fetches. The guest log is `/boot/home/v8-upstream-fetch-check.log`;
the root, 19 dependencies, and fadec fetched trees were also compared with the
locally exported revisions, and all 21 Git tree IDs matched.
This fetch check does not include provisioning the native build tools. The
bindings were subsequently
generated from the reconstructed checkout using the native bindgen CLI, the
arguments and filters from `build.rs`, and GN's JSON project definitions. A
host built with both the reconstructed archive and these new bindings passed
the delegated-command mock session; guest logs are
`/boot/home/codex-v8-new-binding-host.log` and
`/boot/home/v8-new-binding-e2e.log`.
To reproduce the binding generation, run `gn gen` with `--ide=json`, then:

```sh
LIBCLANG_PATH=/boot/system/lib /boot/system/bin/python3.10 \
  /path/to/generate-v8-bindings.py /path/to/rusty_v8 /path/to/gn_out \
  /path/to/src_binding.rs --bindgen /path/to/bindgen
```

The helper's output matched the bindings used in that session byte for byte.

A later robustness pass exercised invalid CLI arguments, missing credentials,
noninteractive startup, 24 concurrent/repeated version and login-status
commands (four at a time), and 1,000 malformed bitmap archives. The commands
and malformed image probe completed without a crash; the concurrent run took
about 150 seconds on the loaded VM. Unauthenticated model requests retried and
ultimately returned HTTP 401; upstream Linux Codex showed the same behavior.
An existing native build intermittently failed while creating a Rust archive
with `Bad address`, with a guest VM soft-fault `Device/File/Resource busy`
message around the same time. The connection between these messages is not
proven. The malformed image probe is `clipboard_malformed_probe.cpp` and can
be built with the same flags as the clipboard image test above.
The subsequent version-display fix was not installed: the native rebuild
failed twice creating `libcodex_core` with `Bad address` after earlier failures
in three smaller archives. The VM log associated a file-backed page-fault
`Device/File/Resource busy` message with the compiler thread. The exact cause
and a reliable user-space workaround remain unconfirmed; the previously
working packaged binary was retained.

The isolated stdio shutdown probe (`protocol_shutdown_probe.py`) observed clean
exit after malformed input followed immediately by EOF and after empty EOF.
It also killed a server and successfully started and closed a fresh server in
the same test home. Since immediate EOF produced no response, these results
verify shutdown behavior only, not protocol validation or session recovery.

The live protocol recovery probe (`protocol_recovery_probe.py`) initialized the
server, sent malformed JSON and an unknown method with stdin kept open, and
successfully issued `thread/list` afterward. The unknown method returned a
JSON-RPC error and the process exited zero after EOF.

An experimental compiler-only write interceptor (`compiler_write_buffer.c`)
copies up to 64 KiB into heap memory before calling the underlying `write`.
A mapped-file copy passed with matching SHA-256 hashes, and a native Cargo build
with `LD_PRELOAD=/CodexBuild/codex-write-buffer.so` passed creation of the core
archive that had failed twice without it. The full build and repeatability
checks are still pending. This is not part of the installed Codex runtime.
The live recovery probe was expanded with assertions and 100 invalid method
requests with growing payloads. All 100 returned errors, a subsequent valid
`thread/list` succeeded, and the server exited zero.
A stronger mapped-write probe (`archive_copy_probe.c`) filled a 32 MiB file with
nonzero data, mapped it, and copied it eight times. Both the unmodified process
and the buffered process passed all eight rounds; the buffered output matched
the source SHA-256. This probe does not reproduce the compiler failure and
therefore establishes data integrity only, not that the workaround is causal.
The buffered Cargo build subsequently passed app-server and reached the TUI.

A guest `top` sample identified five old Terminal teams from the direct-exec
launcher trials consuming approximately 96% of guest CPU. Their team IDs were
115974, 115979, 115990, 116026, and 116031. Terminating those specific leftovers
removed the CPU load and increased linker throughput. The installed launcher
uses `fork` before executing Terminal; direct process replacement from the
Tracker-launched team must not be reintroduced without verifying startup and
idle CPU behavior. Earlier stress timings were collected under this load and
must not be treated as normal performance measurements.
The buffered native CLI build completed successfully (`codex-buffered-build.log`,
31m08s, including the CPU-starved period). This is one successful full build,
not proof of repeatability or the underlying fault's exact cause. Without
LD_PRELOAD at runtime, the new binary passed the 100-invalid-request recovery
probe, a real model response, and a packaged delegated shell command producing
`RENKU_REGRESSION_OK`. Both `--version` and the exec banner reported
`0.171.0-alpha.5`. The native package executable was replaced with this build.
The native file-lock module passed its three unit tests. A separate
`lock_process_probe.rs` ran 20 rounds in which a child process held an
exclusive lock, the parent verified contention, killed the child, then
successfully acquired and released the lock. No stale lock remained.

To try the compiler-only workaround on RenkuOS, build the interceptor with:

```sh
gcc -shared -fPIC third_party/renkuos/compiler_write_buffer.c \
  -o /CodexBuild/codex-write-buffer.so
```

Then prefix the native Cargo command above with
`LD_PRELOAD=/CodexBuild/codex-write-buffer.so`. Scope this variable to the build
command. The library is experimental and has only been tested with this native
build workload; it is not a general replacement for the system `write` API.
A repeated-launch probe invoked the current desktop launcher three times. All
launcher parents returned zero and three new Terminal teams appeared. A CPU
sample showed none of those teams busy; terminating the three test terminals
removed them, and a subsequent sample showed normal CPU load. This samples
current launcher behavior; it does not independently establish the root cause
of the older direct-exec Terminal spin.
The second buffered build again passed core archive creation and reached
app-server after forcing a core source timestamp change.
The installed package passed an isolated managed-daemon lifecycle probe:
start succeeded, repeated start reported `alreadyRunning`, restart launched a
new PID, version reported `running`, and stop reported `stopped`. The probe
uses a separate Codex home and cleans up its daemon in a finally block.
The display/package version is `0.171.0-alpha.5`; internal workspace/daemon
protocol version fields retain the source checkout's `0.0.0` placeholder.
