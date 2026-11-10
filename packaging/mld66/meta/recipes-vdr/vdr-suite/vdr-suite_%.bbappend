# F2.6 MLD 6.6 package integration for the isolated Backend Agent account.
# Maintainer integration candidate: copy into the matching MLD meta-mld layer.
# This file is NOT active while stored in the VDR-Suite source repository.
# No second enrollment implementation or host-side changes.
#
# systemd reports ConditionPathExists=/usr/sbin/vdr-suite-backend-agent-autostart
# not met on the target even though vdr-suite-first-run.service is installed.
# The existing script is the canonical auto-first-run entrypoint.
#
# On MLD the VDR service itself runs as root. This does not require running
# the Backend Agent as root: the shared first-run and identity checks expect
# a dedicated vdr:vdr Agent service account. The package must provision it
# before attempting automatic first-run; never alter vdr.service.
#
# Yocto useradd.bbclass creates the service group/user during package
# installation. If the owning MLD recipe already provisions accounts,
# its useradd metadata must be merged rather than replaced.
inherit useradd
USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM:${PN} = "--system vdr"
USERADD_PARAM:${PN} = "--system --gid vdr --home-dir /var/lib/vdr-suite/backend-agent --no-create-home --shell /bin/false vdr"

do_install:append() {
    if [ ! -f "${S}/tools/vdr_suite_backend_agent_autostart.py" ]; then
        echo "MLD_FIRST_RUN_HELPER=BLOCKED_SOURCE_MISSING (check S and SRCREV)" >&2
        exit 1
    fi
    if [ ! -f "${S}/packaging/systemd/vdr-suite-backend-agent.service" ]; then
        echo "MLD_AGENT_UNIT=BLOCKED_SOURCE_MISSING (check S and SRCREV)" >&2
        exit 1
    fi
    if [ "${sbindir}" != "/usr/sbin" ]; then
        echo "MLD_FIRST_RUN_HELPER=BLOCKED_SBINDIR_MISMATCH" >&2
        exit 1
    fi
    if [ -z "${systemd_system_unitdir}" ]; then
        echo "MLD_AGENT_UNIT=BLOCKED_UNITDIR_MISSING" >&2
        exit 1
    fi
    # The daemon's service account is deliberately unchanged (root on MLD).
    # Replace only the MLD-installed Agent unit with the reviewed upstream
    # unit whose User=vdr and Group=vdr match the First-Run identity contract.
    install -d "${D}${sbindir}"
    install -m 0755 "${S}/tools/vdr_suite_backend_agent_autostart.py" \
        "${D}${sbindir}/vdr-suite-backend-agent-autostart"
    install -d "${D}${systemd_system_unitdir}"
    install -m 0644 "${S}/packaging/systemd/vdr-suite-backend-agent.service" \
        "${D}${systemd_system_unitdir}/vdr-suite-backend-agent.service"
    # First-run enrollment needs this clean private directory before the
    # Agent's StateDirectory lifecycle has ever been entered. The helper
    # changes ownership from root to vdr only after checking its state.
    install -d -m 0700 "${D}${localstatedir}/lib/vdr-suite/backend-agent"
}

# Ensure the installed helper belongs to the main package, even when
# the existing MLD recipe uses explicit FILES ownership for its binaries.
FILES:${PN}:append = " ${sbindir}/vdr-suite-backend-agent-autostart ${systemd_system_unitdir}/vdr-suite-backend-agent.service ${localstatedir}/lib/vdr-suite/backend-agent"

# The helper imports stdlib modules (json, subprocess, tempfile, urllib...).
# Explicit Python runtime availability is required on minimal MLD images.
RDEPENDS:${PN}:append = " python3-core python3-modules"
