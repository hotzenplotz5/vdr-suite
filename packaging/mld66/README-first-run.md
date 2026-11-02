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
   a working HTTPS Control Plane origin. An optional trusted CA
   certificate path may be supplied when the system trust store does
   not already trust that origin. For the reported MLD layout,
   `/etc/vdr/channels.conf` is a link to the selected channel list;
   this is supported. **Do not choose a fake origin, public HTTP endpoint
   or a guessed URL.**
2. Ensure the package's installed Daemon really loads its staged
   `/etc/default/vdr-suite-daemon` file and the Agent unit runs as its
   existing reviewed **non-root** `vdr` account with access to its
   private `/var/lib/vdr-suite/backend-agent` state directory. The
   daemon and Agent must use the *same* Control Plane SQLite database
   path. The MLD recipe must own these service/account/path facts;
   this repository has not verified its external BitBake metadata.
3. During the **trusted MLD first-run install phase**, once the VDR and
   Suite daemon services are running and the verified HTTPS origin is
   reachable, invoke the already staged binary
   `/usr/sbin/vdr-suite-backend-agent-autostart --installer-onboard`
   **as root, without user interaction**. This single call checks
   the local topology, prepares an enrollment only when genuinely absent,
   starts the existing Agent only for pending first enrollment, waits for
   authenticated Agent/health/channel readback, and enables its
   existing unit for future boots. An existing active Agent is checked
   instead of re-enrolled.
4. On a first-run failure, preserve the pending enrollment/identity,
   never reissue a token speculatively, and retry the same installer-owned
   entrypoint only after the original precondition is corrected. Since
   no persistent MLD recipe hook is yet present here, reboot/interrupted
   resume is **not** proven. Do not print secrets or ask skippy to run
   root commands.
5. After the first admin is created or signs in, the existing browser
   retrieves `/api/backends/default/health` and displays the
   opt-in `localBackendSetup.status`. `ready` requires a fresh
   identity, lease, backend health, and channel observation. The browser
   does not start services, issue enrollments, or elevate permissions.

### Example package-build wiring (illustrative)

The package build system substitutes the actually verified HTTPS origin
and channel source from its **trusted package recipe metadata**. This
is not an instruction for end users and should not run on a live machine.

```sh
make stage-mld-first-run-config \
  DESTDIR="$MLD_PACKAGE_STAGING_ROOT" \
  MLD_FIRST_RUN_CONTROL_PLANE_URL="$MLD_LOCAL_HTTPS_CONTROL_PLANE_ORIGIN" \
  MLD_FIRST_RUN_CHANNELS_CONF_PATH=/etc/vdr/channels.conf \
  MLD_FIRST_RUN_CA_CERTIFICATE_PATH="$MLD_INSTALLED_CA_PATH"
```

The regular `make install` deliberately does **not** invoke this target.
The MLD recipe must adopt the post-install/first-boot handoff itself; the
repo cannot edit a BitBake recipe it does not contain. Do not confuse
packaging-stage PASS with functional MLD setup or real Live-TV acceptance.

## Blocking/verification notes

- The Agent requires an actual, trusted **HTTPS** Control Plane; the
  browser's plain HTTP `:18080` origin is not automatically usable for
  Agent enrollment. A working MLD TLS front end and trust/CA configuration
  must be verified before invoking this hook. Do not disable TLS checks.
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
  systemd unit, package dependency, branch, or PR is introduced here.
