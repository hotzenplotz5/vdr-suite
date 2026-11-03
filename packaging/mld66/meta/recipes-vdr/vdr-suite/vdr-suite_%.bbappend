# F2.6 MLD 6.6 package fix for the skipped first-run systemd service.
# Maintainer integration candidate: copy into the matching MLD meta-mld layer.
# This file is NOT active while stored in the VDR-Suite source repository.
# No second enrollment implementation or host-side changes.
#
# systemd reports ConditionPathExists=/usr/sbin/vdr-suite-backend-agent-autostart
# not met on the target even though vdr-suite-first-run.service is installed.
# The existing script is the canonical auto-first-run entrypoint.
do_install:append() {
    if [ ! -f "${S}/tools/vdr_suite_backend_agent_autostart.py" ]; then
        echo "MLD_FIRST_RUN_HELPER=BLOCKED_SOURCE_MISSING (check S and SRCREV)" >&2
        exit 1
    fi
    if [ "${sbindir}" != "/usr/sbin" ]; then
        echo "MLD_FIRST_RUN_HELPER=BLOCKED_SBINDIR_MISMATCH" >&2
        exit 1
    fi
    install -d "${D}${sbindir}"
    install -m 0755 "${S}/tools/vdr_suite_backend_agent_autostart.py" \
        "${D}${sbindir}/vdr-suite-backend-agent-autostart"
}

# Ensure the installed helper belongs to the main package, even when
# the existing MLD recipe uses explicit FILES ownership for its binaries.
FILES:${PN}:append = " ${sbindir}/vdr-suite-backend-agent-autostart"

# The helper imports stdlib modules (json, subprocess, tempfile, urllib...).
# Explicit Python runtime availability is required on minimal MLD images.
RDEPENDS:${PN}:append = " python3-core python3-modules"
