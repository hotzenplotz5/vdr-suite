# Debian package compatibility gate

This directory contains **experimental packaging metadata**, not release-approved builds. The VDR ABI dependency must be resolved from the target distribution's VDR packaging before publishing the plugin. The current `vdr` dependency alone is **not sufficient** to guarantee compatibility; `dpkg-shlibdeps` also cannot verify the VDR plugin API.

Before any package is released: determine distro-specific VDR virtual ABI package, validate built `libvdr-suitebridge.so.<APIVERSION>`, set strict dependency, generate canonical staging ownership lists, test conffiles and maintainer script lifecycle, build with `dpkg-buildpackage` in an isolated chroot and run lintian.

Do not upload to Launchpad or install on production hosts based solely on these metadata files.
