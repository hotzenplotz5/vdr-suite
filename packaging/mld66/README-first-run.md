# MLD 6.6: First-run package integration (MLD.F2.5/F2.6)

Canonical source: **`main`** (MLD integration consolidated 10 November 2026).
The former `work/mld66-skippy-feedback-20261027` branch is retained as
historical test evidence, not a package source. The proven libcurl proxy
and Basic-auth fixes, repeat-safe first-run, shared staging, and direct LAN
HTTP media-cookie fix are now on `main`. Skippy reported working Live-TV
(direct and MLD-embedded) and recording playback, while Pit confirmed
Agent synchronization and functioning timer creation/deletion on their
previously assembled test builds. An MLD package built from the new
consolidated `main` has **not** yet been hardware-tested. This remains
an **MLD recipe integration contract**. The MLD BitBake recipe is **not stored
in this repository**; it must explicitly adopt these source-level hooks.
The MLD recipe remains an adapter to the [shared distribution first-run staging contract](../../docs/planning/distribution-first-run-setup.md). Debian/Ubuntu/yaVDR already include the common units and first-install package hooks. Their first-boot runtime behavior, like MLD's, still needs real system verification.

## Consolidated main acceptance (10 November 2026)

The owning MLD BitBake recipe must point to the tested `main` revision,
not the old work branch. Its package must contain the shared first-run
service/path, the `vdr-suite-backend-agent-autostart` helper, and the
MLD `vdr:vdr` Agent account. Keep the MLD VDR service itself unchanged.

Focused source tests (run without waiting for CI):

```sh
make test-mld-agent-first-run test-systemd-unit-contract
make test-backend-agent-client
node web/frontend/tests/test_searchtimer_workflows_runtime.js
```

Do not mistake successful Agent enrollment for complete interactive
functionality. Fresh MLD installations still need validated automatic
`vdr.live` and `vdr.timer` provider ownership and a measured media
transcode performance profile for encoder mode `auto`. Browser SearchTimer
creation in `main` remains staged/inactive; direct editing/activation needs
a separate authorized feature completion, not replacement by legacy UI.

## F2.6 MLD root-VDR versus Agent service account (05 November 2026)

Pit confirmed on the MLD test system that the installed Agent unit says
`User=root` and `Group=root`, although a `vdr` user was later added
manually. The improved first-run check names the exact rejection:
`AGENT_BOOT_PREFLIGHT=BLOCKED: installed Agent service is not ready for initial setup: service-user`.
The VDR itself runs under root by MLD policy and must **not** be changed.

The candidate BitBake `.bbappend` now explicitly provisions the separate
`vdr:vdr` **Agent service account** using Yocto `useradd.bbclass`;
installs the unchanged reviewed `packaging/systemd/vdr-suite-backend-agent.service`
(`User=vdr`, `Group=vdr`) into `${systemd_system_unitdir}`;
and stages the protected root-owned `/var/lib/vdr-suite/backend-agent`
directory with mode 0700, ready for the existing first-run helper's verified
ownership handoff. It does not modify MLD `vdr.service`, create enrollment
tokens or grant the Agent root privileges. The previously added executable
first-run helper remains included.

**External recipe integration:** The owner must merge `inherit useradd` and
the `USERADD_*`/`GROUPADD_*` settings with any existing MLD account
definitions, check whether another MLD package already owns the `vdr`
account, and verify `${systemd_system_unitdir}`, `${S}`, package ownership
and `SRCREV` in the real BitBake environment. Since the external recipe
cannot be read here, these facts are not claimed as verified. The contained
staging regression checks the exact staged helper, reviewed unit, private
directory and unchanged MLD root-VDR, **not** the real `useradd`
preinstall, BitBake package manager or first boot. Source regression passing
alone does not mean an MLD test package was built.

## F2.6 missing autostart helper: targeted BitBake overlay candidate

A new MLD live diagnostic (03 November 2026, 22:05 CET) establishes:
`vdr-suite-first-run.service` is installed and its job is reached,
but systemd skips it because
`ConditionPathExists=/usr/sbin/vdr-suite-backend-agent-autostart` is unmet.
`FIRST_RUN_HELPER=NOT_INSTALLED` confirms that the executable is absent.
The agent consequently remains disabled and unenrolled. This is **not**
evidence for changing MLD's channels.conf symlink or SVDRP transports.

A concrete minimal package correction is prepared at
[`meta/recipes-vdr/vdr-suite/vdr-suite_%.bbappend`](meta/recipes-vdr/vdr-suite/vdr-suite_%.bbappend).
It appends the normal BitBake `do_install` task to install the **existing**
VDR-Suite helper from `${S}/tools/vdr_suite_backend_agent_autostart.py`,
assigns it to `${PN}` through `FILES`, and declares required Python
runtime modules. The package build **fails** if the pinned `SRCREV` lacks
that source file or the target `sbindir` differs from `/usr/sbin`.
There are no new enrollment commands, alternate binaries, or ad-hoc
actions for MLD users.

**Integration boundary:** The `.bbappend` is a candidate in the VDR-Suite
source tree and is not read by the external MLD build system until the
maintainer places/adapts it in the owning `MLD-6/meta-mld` layer.
The actual MLD recipe and its `S`, `SRCREV`, `FILES` package split
and effective Python package names are inaccessible and **not verified**.
A shell-only staged-payload test can verify the copy semantics but is
not a BitBake package build. The next acceptance must inspect the
built package's executable `/usr/sbin/vdr-suite-backend-agent-autostart`
and its interpreter dependencies, then observe the first-run systemd job
on a fresh MLD image. **MLD_RECIPE_APPLIED=NO;
MLD_PACKAGE_BUILD=NOT_RUN; REAL_FIRST_RUN=NOT_VERIFIED.**

## What the MLD recipe must do

The native MLD package build/install context (not the browser) owns all
privileged operations. **The actual installed MLD recipe payload has not been
verified.** That recipe must include the existing Suite binaries: daemon,
Backend Agent, enrollment tool, Agent administrator, and
`vdr-suite-backend-agent-autostart`. It must also stage the existing
`backend-agent.conf` and `vdr-suite-daemon` environment file, plus the
reviewed `vdr-suite-backend-agent.service` unit.

1. After the usual `make install-runtime DESTDIR=...` and
   `make install-systemd DESTDIR=...` staging steps, run the **opt-in**
   `make stage-mld-first-run-config` target (compatibility adapter for the shared
   `make stage-distribution-first-run-config`) on that same separate
   `DESTDIR` with the actual installed MLD channel-selection path and
   the real VDR channels.conf. The local standard uses the already
   installed daemon on `http://127.0.0.1:18080` and does not need TLS
   certificates. A remote Control Plane still requires HTTPS and, if
   needed, an existing trusted CA. For the reported MLD layout,
   `/etc/vdr/channels.conf` is a link to the selected channel list;
   this is supported. **Never use an arbitrary remote HTTP endpoint.**
2. Ensure the package's installed Daemon really loads its staged
   `/etc/default/vdr-suite-daemon` file and the Agent unit runs as its
   existing reviewed **non-root** `vdr` account with access to its
   private `/var/lib/vdr-suite/backend-agent` state directory. The
   daemon and Agent must use the *same* Control Plane SQLite database
   path. The MLD recipe must own these service/account/path facts;
   this repository has not verified its external BitBake metadata.
3. Install the shared systemd units `vdr-suite-first-run.service` and
   `vdr-suite-first-run.path` via the existing `make install-systemd` contract
   and preserve the packaged daemon unit's `Wants` dependency. Once VDR and
   the Suite daemon are started, the daemon starts the common one-shot
   `/usr/sbin/vdr-suite-backend-agent-autostart --auto-first-run`
   **automatically as root, without user interaction**. If the MLD recipe
   provides a native install/first-boot hook instead, it must call the
   **same** shared `--installer-onboard` entrypoint, not implement
   another enrollment path. That shared helper checks
   the local topology, prepares an enrollment only when genuinely absent,
   starts the existing Agent only for pending first enrollment, waits for
   authenticated Agent/health/channel readback, and enables its
   existing unit for future boots. An existing active Agent is checked
   instead of re-enrolled.
4. On a first-run failure, preserve the pending enrollment/identity,
   never reissue a token speculatively, and retry the same installer-owned
   entrypoint only after the original precondition is corrected. Since
   no real MLD package installation has been tested, reboot/interrupted
   resume remains **unproven**. Do not print secrets or ask skippy to run
   root commands.
5. After the first admin is created or signs in, the existing browser
   retrieves `/api/backends/default/health` and displays the
   opt-in `localBackendSetup.status`. `ready` requires a fresh
   identity, lease, backend health, and channel observation. The browser
   does not start services, issue enrollments, or elevate permissions.

### Example package-build wiring (illustrative)

The package build uses the local Loopback origin when no explicit HTTPS
origin is supplied. It must still supply the actual MLD VDR channels.conf path. This
is not an instruction for end users and should not run on a live machine.

```sh
make stage-mld-first-run-config \
  DESTDIR="$MLD_PACKAGE_STAGING_ROOT" \
  MLD_FIRST_RUN_CHANNELS_CONF_PATH=/etc/vdr/channels.conf \
  MLD_FIRST_RUN_CA_CERTIFICATE_PATH="$MLD_INSTALLED_CA_PATH"
```

The regular `make install` deliberately does **not** invoke this target.
The MLD recipe must install the shared units or supply an equivalent native
first-boot hook; the repo cannot edit the external BitBake recipe it does
not contain. Do not confuse
packaging-stage PASS with functional MLD setup or real Live-TV acceptance.

## Blocking/verification notes

- The same-host Agent can now reach the packaged Daemon through the exact
  built-in loopback endpoint `http://127.0.0.1:18080`. This does **not**
  allow external HTTP Agents. Remote Agent links still require HTTPS.
  The existing MLD browser URL is not used to discover Agent credentials.
- The helper currently requires the packaged Agent unit to run as the
  non-root `vdr` account, with known systemd fragment and no unreviewed
  Agent drop-ins. If MLD chooses a different service account, that is an
  explicit platform integration gap; do not run the Agent as root.
- Real MLD package file paths, service permissions, and the BitBake
  post-install execution order remain **unverified**. `vdr.service`
  and `vdr-suite-daemon.service` must actually be running for the
  installer-owned activation.
- `--installer-onboard` resumes a still-pending enrollment without
  creating another token. If a crash leaves a protected identity behind
  while the packaged Agent is inactive, it checks the existing unit and
  identity before starting **that same Agent**, then waits for an
  authenticated lease and Channel observation. It refuses unknown
  units or mismatched identities; it never re-enrolls.
- `MLD_FIRST_RUN_CA_CERTIFICATE_PATH` is optional when the MLD trust
  store already validates its HTTPS origin. Otherwise the recipe can
  supply an **existing trusted CA certificate path**, preserved by
  staging on repetition; an existing different configured CA path is
  never replaced. Staging does not issue certificates or weaken TLS.
- Host reboot and skippy's browser/live-TV acceptance are MLD.F2.6.
- No new Agent, HTTP mutation route, privileged browser workflow,
  package dependency, branch or PR is introduced here. The shared first-run
  one-shot and path units are new but use the existing Agent entrypoint.

## F2.6 verified integration boundary (03 November 2026)

- **MLD recipe directory (maintainer-reported, not independently fetched):** [`MLD-6/meta-mld`, `wrynose-ng`, `meta/recipes-vdr/vdr-suite`](https://gitlab.com/MLD-6/meta-mld/-/tree/wrynose-ng/meta/recipes-vdr/vdr-suite?ref_type=heads), recorded on 19 October in [`mld66-pit-runtime-evidence-20261019.md`](../../docs/development/mld66-pit-runtime-evidence-20261019.md). The `.bb` filename/content, effective recipe revision, package install, systemd activation, service account and database mapping remain **unverified** because this GitLab directory cannot currently be read. The separately public [`mld-dev`](https://gitlab.com/MLD-6/mld-dev) `scarthgap` KAS entrypoint is not proof of the actual `wrynose-ng` recipe payload.
- **External GitLab changes:** NONE. No recipe update, MLD BitBake package build, or MLD first-boot activation has been performed or claimed.
- **Available shared source:** `make install-systemd` stages the daemon, Backend Agent, first-run one-shot and `.path` units. `make stage-mld-first-run-config` delegates to the same validated staging implementation used by other distributions; `--auto-first-run` reuses the existing enrollment and Agent activation. These are source contracts, **not** confirmation of actual recipe inclusion.
- **Debian/Ubuntu/yaVDR:** commit [`46828dd6`](https://github.com/hotzenplotz5/vdr-suite/commit/46828dd62806dc571f17ddd7c2a91e09a559b07c) adds a systemd unit reload ahead of first-install starts in both package hooks. Package regression passed in [VDR-Suite CI run 41592697774](https://github.com/hotzenplotz5/vdr-suite/actions/runs/41592697774). Actual systemd start, enrollment, and channel readback on a VDR host remain unverified.
- **Outstanding MLD adapter work:** read the maintainer-reported `vdr-suite` recipe in `wrynose-ng` and verify that it installs the shared unit and helper files and that the Daemon's `Wants` plus the MLD package/boot ordering actually trigger the one-shot. Apply only missing recipe wiring in the owning GitLab repository when accessible; do not copy enrollment logic.
- **Read-only diagnostics portability:** `tools/mld66-readonly-inventory.sh` and `tools/mld66-live-readonly-diagnostics.sh` now prefer the installed native `svdrpsend` CLI for optional `CAPS 1` checks and fall back to the MLD `svdrpsend.sh` variant only when the native executable is absent. The live diagnostics additionally expose `vdr-suite-first-run.service`/`.path` states. Productive SuiteBridge traffic remains on the existing native SVDRP socket path; no service or enrollment change is implied.
- **Acceptance:** `SOURCE_CONTRACT=AVAILABLE`, `DEBIAN_PACKAGE_REGRESSION=PASS`, `MLD_RECIPE_INTEGRATION=NOT_VERIFIED`, `MLD_PACKAGE_BUILD=NOT_RUN`, `AUTO_FIRST_BOOT=NOT_VERIFIED`, `REAL_VDR_LIVE_TV=NOT_VERIFIED`. A passing disposable package install with service starts suppressed is not a first-boot test.
