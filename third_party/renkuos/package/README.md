# Native RenkuOS install package

Artifact: `codex_renkuos-0.171.0~alpha.5-1-x86_64.hpkg`.
Double-click it in Tracker and install through HaikuDepot, or run
`pkgman install /path/to/package.hpkg`.

The package installs `apps/Codex/` and an Applications-menu shortcut. Its
launcher discovers the CLI relative to its own native app image, so it works
without the development `/CodexBuild` tree. The CLI, code-mode host, package
metadata, and ripgrep wrapper retain their original relative layout. Runtime
package dependencies are declared in PackageInfo. No account credentials or
compiler workaround library are included. Interactive startup retains the
explicit no-sandbox consent prompt. The displayed version labels this port;
it is based on upstream development commit 336b19a, not the exact alpha tag.

`launcher.cpp` builds with the native C++ compiler. `set_icon.cpp` links against
libbe and libtranslation and sets Haiku mini/large icon attributes from the
PNG. Run it on the staged launcher before `package create` so those attributes
are preserved inside the package. The installed icon was inspected in Tracker.
Artwork provenance is in ICON-SOURCE.txt. The package remains an experimental
community port.

Install verification: pkgman resolved the declared dependencies and activated
the package; the installed binary reported 0.171.0-alpha.5 and passed the
100-invalid-request protocol recovery probe. The installed GUI launcher opened
a Terminal displaying the required consent prompt. The VM desktop shortcut
now points to `/boot/system/apps/Codex/Codex`.
