# RenkuOS Codex stress-test report

Tested native x86-64 RenkuOS package, displayed version `0.171.0-alpha.5`,
upstream snapshot `336b19ab26e0d4e36b16d8a21d668632cd800207` plus port changes.
No kernel changes were made in this testing pass.

## Issues corrected or mitigated

- Exec and device-login banners used the source version placeholder. Display
  paths now use the requested native version. The rebuilt exec banner and
  `--version` were checked on the VM.
- Five old Terminal teams from direct-exec launcher trials consumed about 96%
  of guest CPU. Those specific teams were terminated. The current launcher
  forks before exec; three new launches did not show the busy-main-thread
  behavior and their test terminals shut down. The exact Terminal internals
  behind the old spin were not diagnosed.
- Native Rust archive creation failed repeatedly with `Bad address`, including
  two consecutive core archive failures. A compiler-only buffered-write
  interceptor mitigates this workload: two full builds succeeded, the second
  after forcing core and dependent recompilation. The latter took 9m40s with
  normal CPU load. The underlying OS/file-mapping fault remains undiagnosed;
  this is a tested build mitigation, not a kernel fix or a universal I/O shim.

## Verification results

- Invalid CLI arguments/configuration returned errors without a panic;
  interactive, resume, and fork startup with nonterminal input refused startup.
- 24 version/login-status launches, four at a time, returned expected codes.
  The 150-second timing was under the old runaway Terminal load and is not a
  normal performance measurement.
- Nine native clipboard conversion test runs passed. The earlier desktop
  crash dialogs corresponded to development-test assertions; the current
  failure could not be reproduced.
- 1,000 malformed bitmap archives were handled without a crash.
- App-server EOF shutdown and restart after forced process termination passed.
- Live protocol recovery passed on the installed rebuilt binary and again on
  the second build: initialization, malformed JSON, 100 unknown methods,
  a subsequent valid thread/list, and clean EOF shutdown.
- Three native file-lock unit tests passed. Twenty separate child-process
  kill/reacquire rounds left no stale exclusive lock.
- Installed managed daemon: start, idempotent repeated start, restart with a
  new PID, running version query, and stop passed in a separate test home.
- Real authenticated model response and packaged delegated shell command
  returned successfully without the compiler interceptor at runtime.
- Eight nonzero 32 MiB mapped-file copy rounds passed with and without the
  interceptor; wrapped output hashes matched. This probe did not reproduce
  the original compiler fault and does not establish its root cause.

## Limits

This is a finite robustness pass, not proof of absence of all defects or a
security audit. Long-duration soak, every clipboard representation, every
network failure mode, and a complete upstream test suite were not covered.
Unauthenticated requests retried then returned HTTP 401 on both RenkuOS and
an upstream Linux build; this shared behavior was recorded, not changed.
Internal workspace/daemon protocol versions retain the source `0.0.0`
placeholder while the native display/package version is `0.171.0-alpha.5`.

The native build helper `build-native.sh` scopes the compiler interceptor to
Cargo. Native tests/probes and more detailed build evidence are in README.md.
Guest build logs: `/boot/home/codex-buffered-build.log` and
`/boot/home/codex-buffered-repeat-build.log`.
