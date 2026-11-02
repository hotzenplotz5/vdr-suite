# MLD 6.6: First-run package integration (MLD.F2.5)

Branch: `work/mld66-skippy-feedback-20261027`.
This is an **MLD recipe integration contract**, not an MLD package install
or a verified skippy runtime result. The MLD BitBake recipe is **not stored
in this repository**; it must explicitly adopt these source-level hooks.
The MLD recipe remains an adapter to the [shared distribution first-run staging contract](../../docs/planning/distribution-first-run-setup.md). Ubuntu/yaVDR installation and upgrades are still unchanged **at this source checkpoint**; implementing automatic onboarding for them is required next, not an intentional permanent exclusion.

## What the MLD recipe must do

The native MLD package build/install context (not the browser) owns all
privileged operations. It already installs the *existing* Suite binaries:
daemon, Backend Agent, enrollment tool, Agent administrator, and
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
