# Phase 63 Backend Agent runtime acceptance runbook

This runbook defines three fail-closed real-yaVDR acceptance paths for Phase 63:

1. the historical clean-state lifecycle path that proved Slice 1 enrollment,
   rotation, revocation and replacement;
2. the upgrade-safe `backend-health` ingestion path for PR #139 and later
   ingestion-only changes on a host that already has an enrolled, active Agent; and
3. the upgrade-safe Channel observation path for PR #141 and later Channel
   ingestion changes using a root-controlled fixture copied from native VDR state.

All paths may run only after all required GitHub Actions jobs are green for the
exact PR head. The upgrade-safe path is binding for PR #139 because the real
yaVDR host already contains the accepted Slice-1 Agent identity and history.

## Upgrade-safe PR #139 coverage

The upgrade-safe harness proves:

- the checked-out branch and exact PR head match the requested candidate;
- installed daemon, Agent, enrollment and administration binaries match the
  binaries built from that exact checkout;
- the public Control-Plane URL is HTTPS, certificate verification remains
  enabled and the deployed public base path reaches the protected Backend API;
- the existing active Agent identity is preserved and remains read-only;
- the existing Agent publishes the bounded `backend-health` observation domain;
- a complete baseline already exists and a later producer sequence advances;
- the committed observation cursor survives a daemon restart while the Agent is
  stopped;
- the same existing Agent reconnects after the daemon restart;
- an equivalent replay is acknowledged idempotently without advancing the
  cursor;
- a deliberate sequence gap returns `resync-required` without advancing or
  corrupting the cursor;
- the same existing Agent resumes exact-next ingestion after the rejected gap;
- selected VDR-native state fingerprints for timers, configuration, remote,
  SearchTimer configuration and recording-directory identities are unchanged;
- VDR, daemon and Agent services are active at the end;
- retained logs contain no enrollment token, Agent credential secret or
  Authorization material.

The harness does **not** revoke, replace or re-enroll the production Agent. The
lost-response delivery branch and the client-side resynchronization branch remain
covered by focused automated tests because deliberately dropping a committed
HTTPS response or corrupting the live producer sequence is not safe production
acceptance behaviour.

## Upgrade-safe preconditions

- Execute from an already opened root shell on the real yaVDR host.
- Use the established checkout `/home/yavdr/vdr-suite`.
- The worktree must be clean and on the exact candidate branch/head.
- VDR, `vdr-suite-daemon.service` and
  `vdr-suite-backend-agent.service` must be active.
- The existing protected Agent identity must be present. The harness refuses to
  create, revoke, replace or delete Agent identity/history.
- The exact candidate daemon, Agent, enrollment and administration binaries must
  already be built and installed.
- The Control-Plane URL must be the public HTTPS VDR-Suite URL without a
  trailing slash, including a deployed base path such as
  `https://host/vdr-suite`. For a private CA, pass its certificate path.
- The evidence directory must not already exist.

## Upgrade-safe execution contract

Run the exact candidate through:

```text
make phase63-backend-health-ingestion-runtime-acceptance \
  PHASE63_EXPECTED_BRANCH=<exact branch> \
  PHASE63_EXPECTED_HEAD=<exact PR head> \
  PHASE63_CONTROL_PLANE_URL=<HTTPS public origin> \
  PHASE63_CA_CERTIFICATE_PATH=<optional private CA path> \
  PHASE63_EVIDENCE_DIR=<new root-only evidence directory>
```

A successful PR #139 run ends with:

```text
PHASE_63_BACKEND_HEALTH_INGESTION_UPGRADE_ACCEPTANCE=PASS
HEAD=<exact accepted head>
CONTROL_PLANE_URL=<verified HTTPS public origin>
AGENT_ID=<preserved active Agent identifier>
INITIAL_PRODUCER_SEQUENCE=<observed sequence>
ADVANCED_PRODUCER_SEQUENCE=<later sequence>
BACKEND_HEALTH_OBSERVATION_REPLAY=yes
BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=yes
OBSERVATION_CURSOR_RESTART_PERSISTED=yes
EXISTING_AGENT_IDENTITY_PRESERVED=yes
VDR_NATIVE_STATE_UNCHANGED=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=<root-only evidence directory>
```

## Upgrade-safe Channel observation coverage

The Channel observation harness is binding for PR #141. It proves the exact PR
head without mutating VDR-native Channel state and without replacing the existing
active Agent.

The harness:

- verifies the branch and full commit, rebuilds daemon, Agent, enrollment and
  administration candidates from that exact checkout, and only then byte-compares
  them with the installed binaries before any runtime configuration change;
- preserves the existing Agent ID, credential ID and credential generation;
- copies the native `channels.conf` to a temporary root-controlled fixture under
  the Agent state directory;
- enables only `channels-conf` plus the `channels` observation domain in a
  temporary Agent configuration;
- observes the initial complete Channel snapshot and bounded fact count through
  `vdr-suite-backend-agent-admin --status`;
- changes only the fixture's display name for one Channel and requires a newer
  snapshot generation with the same fact count;
- stops the Agent and proves equivalent replay plus deliberate sequence-gap
  `resync-required` handling without cursor or fact movement;
- proves the committed Channel cursor and fact count survive a daemon restart;
- proves the same Agent creates a newer Channel lineage after reconnect and
  recovers after the deliberate gap;
- runs the existing real-VDR read-only RESTfulAPI regression before and after;
- hashes native `channels.conf`, Timer/configuration files and recording
  directory identities before and after;
- restores the original Agent configuration and removes the fixture even on
  failure;
- keeps evidence root-only and rejects retained logs containing credentials,
  enrollment tokens or Authorization material.

It does not enroll, revoke, replace or rotate the Agent. It does not write the
native `channels.conf`, manually inspect SQLite, call a Channel mutation route or
retain the original Agent configuration in the evidence directory.

### Channel acceptance preconditions

- Execute from an already opened root shell on the real yaVDR host.
- Use the established checkout `/home/yavdr/vdr-suite` on the exact PR #141 head.
- The worktree must be clean.
- VDR, `vdr-suite-daemon.service` and
  `vdr-suite-backend-agent.service` must be active.
- The existing protected Agent identity and configuration must be present.
- The exact candidate daemon, Agent, enrollment and administration binaries must
  already be built and installed.
- `/var/lib/vdr/channels.conf` must be a regular file. A different native source
  may be supplied explicitly through `PHASE63_CHANNELS_CONF_SOURCE`.
- The evidence directory and temporary fixture path must not already exist.

### Channel acceptance execution contract

Run the exact candidate through:

```text
make phase63-channel-observation-runtime-acceptance \
  PHASE63_EXPECTED_BRANCH=agent/phase63-channel-observation-runtime \
  PHASE63_EXPECTED_HEAD=<exact PR head> \
  PHASE63_CONTROL_PLANE_URL=<HTTPS public origin> \
  PHASE63_CA_CERTIFICATE_PATH=<optional private CA path> \
  PHASE63_EVIDENCE_DIR=<new root-only evidence directory>
```

A successful run ends with:

```text
PHASE_63_CHANNEL_OBSERVATION_UPGRADE_ACCEPTANCE=PASS
HEAD=<exact accepted head>
AGENT_ID=<preserved active Agent identifier>
INITIAL_CHANNEL_FACT_COUNT=<accepted native fixture fact count>
CHANNEL_BASELINE=yes
CHANNEL_FIXTURE_TRANSITION=yes
CHANNEL_OBSERVATION_REPLAY=yes
CHANNEL_OBSERVATION_GAP_RESYNC=yes
CHANNEL_CURSOR_RESTART_PERSISTED=yes
CHANNEL_RECOVERY_AFTER_RESYNC=yes
EXISTING_AGENT_IDENTITY_PRESERVED=yes
CREDENTIAL_GENERATION_PRESERVED=yes
VDR_NATIVE_STATE_UNCHANGED=yes
VDR_READ_ONLY_REGRESSION=yes
ORIGINAL_CONFIGURATION_RESTORED=yes
VDR_ACTIVE=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=<root-only evidence directory>
```

## Historical clean-state lifecycle path

The historical Slice-1 path remains available as
`phase63-backend-agent-runtime-acceptance` for a disposable clean Backend with no
Agent identity or history. It proves controlled enrollment, online/stale/offline
lease transitions, reconnect, credential rotation, revoked Agent denial,
replacement Agent enrollment and retained history. It also exercises
`backend-health` baseline, equivalent replay, sequence gap, daemon restart and
replacement cursor behaviour when run against the current runtime.

This clean-state path is intentionally destructive for the selected test Backend:
it revokes the first Agent and enrolls a replacement Agent. It must not be used
for PR #139 acceptance on the established production Backend.

A successful clean-state run ends with:

```text
PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS
PHASE_63_BACKEND_HEALTH_INGESTION_RUNTIME_ACCEPTANCE=PASS
HEAD=<exact accepted head>
FIRST_AGENT_ID=<revoked Agent identifier>
REPLACEMENT_AGENT_ID=<active replacement identifier>
CREDENTIAL_GENERATION=<rotated generation>
BACKEND_HEALTH_OBSERVATION_INGESTED=yes
BACKEND_HEALTH_OBSERVATION_REPLAY=yes
BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=yes
OBSERVATION_CURSOR_RESTART_PERSISTED=yes
OBSERVATION_REPLACEMENT_CURSOR=yes
VDR_NATIVE_STATE_UNCHANGED=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=<root-only evidence directory>
```

## Failure behaviour

Both harnesses reject a non-root shell, branch/head mismatch, dirty worktree,
installed binary mismatch, inactive required services, insecure or malformed
Control-Plane URL, TLS verification failure, a missing public API route, missing
observation baseline, replay mismatch, cursor movement after replay or sequence
gap, cursor loss after daemon restart, changed VDR-native fingerprints or
secret-like evidence.

The upgrade-safe harness additionally rejects a missing existing identity, an
inactive existing Agent, any Agent identity change across restart/recovery and a
producer sequence that does not advance. Its failure trap restores daemon and
Agent service availability but never edits the database, identity file or Agent
configuration.

Neither harness enables `curl --insecure`, prints the protected identity file,
dumps process environments or requires manual SQLite inspection.
