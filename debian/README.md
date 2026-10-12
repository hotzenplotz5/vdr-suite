# Experimental Debian source packaging

This root `debian/` directory is canonical for `dpkg-buildpackage`. `packaging/debian/` is historical planning material.

NOT RELEASE-READY: the first pristine chroot build, package file audits, copyright and license review, VDR ABI and distribution compatibility and upgrade tests remain outstanding. The helper `dh_vdrplugin_depends` is expected to set `${vdr:Depends}` only on distros that provide it. Verify plugin output directory against `vdr.pc` and `.install` globs for each target.

Service auto-enable/auto-start is disabled in `debian/rules` for experiments. No DB migration occurs in maintainer scripts. Do not deploy on production yaVDR.
