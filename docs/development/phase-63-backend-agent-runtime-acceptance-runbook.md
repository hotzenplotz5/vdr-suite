# Phase 63 Backend Agent runtime acceptance runbook

This runbook is the binding real-yaVDR acceptance path for the bounded Phase-63
Backend Agent lifecycle and `backend-health` observation-ingestion runtime. It is
intentionally fail-closed and must be run only after all required GitHub Actions
jobs are green for the exact PR head.

## Coverage

The harness proves:

- installed daemon, Agent, enrollment and administration binaries match the
  exact checked-out candidate;
- the public Control-Plane URL is HTTPS, certificate verification is not
  disabled and its configured public base path reaches the protected Backend API;
- controlled enrollment, protocol negotiation, capability publication and
  heartbeat establish the derived `online` state;
- stopping the Agent produces deterministic `stale` and then `offline` state;
- restart reconnects the same Agent;
- a `backend-health` complete snapshot and later exact-next change are accepted;
- an equivalent replay is acknowledged idempotently without advancing the cursor;
- a deliberate sequence gap returns `resync-required` without advancing the cursor;
- the committed observation cursor survives a daemon restart;
- the Agent recovers after the deliberate gap with a fresh fenced lineage;
- credential rotation advances the persisted credential generation;
- a revoked Agent cannot reconnect;
- a replacement Agent receives a distinct identity while revoked history is
  retained and exactly one active Agent remains;
- selected VDR-native state fingerprints for timers, configuration, remote,
  SearchTimer configuration and recording-directory identities are unchanged;
- VDR, daemon and replacement Agent services are active at the end;
- retained logs contain no enrollment token, credential secret or
  Authorization material; the deterministic scanner reports only redacted
  file/line/category evidence if it rejects a log.

The lost-response credential-rotation and observation-delivery branches remain
covered by focused automated tests because deliberately dropping a committed
HTTPS response on a production yaVDR host is not a safe live acceptance action.
The live harness nevertheless exercises an equivalent replay and a deliberate
sequence gap through a root-only helper that reads protected identity material
internally, never prints it and verifies that the cursor remains unchanged.

## Preconditions

- Execute from an already opened root shell on the real yaVDR host.
- Use the established checkout `/home/yavdr/vdr-suite`.
- The worktree must be clean and on the exact candidate branch/head.
- VDR and `vdr-suite-daemon.service` must be active.
- No prior Backend Agent history or identity may exist for the selected
  Backend.  The first acceptance run refuses to overwrite or silently reuse
  Agent state.
- The Control-Plane URL must be the public HTTPS VDR-Suite URL without a
  trailing slash, including any deployed public base path such as
  `https://host/vdr-suite`.  The preflight rejects a missing API route before
  any Agent state is created.  For a private CA, pass its certificate path
  explicitly.

## Execution contract

Build and install only the exact candidate targets, then run:

```text
make phase63-backend-agent-runtime-acceptance \
  PHASE63_EXPECTED_BRANCH=<exact branch> \
  PHASE63_EXPECTED_HEAD=<exact PR head> \
  PHASE63_CONTROL_PLANE_URL=<HTTPS public origin> \
  PHASE63_CA_CERTIFICATE_PATH=<optional private CA path> \
  PHASE63_EVIDENCE_DIR=<new root-only evidence directory>
```

A successful run ends with:

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

## Failure behavior

The harness rejects a non-root shell, branch/head mismatch, dirty worktree,
binary mismatch, unavailable production database, inactive VDR/daemon,
pre-existing Agent state, insecure/non-HTTPS origin, TLS verification failure,
a missing or unexpected public Backend API route, missing state transitions,
missing observation baseline, replay mismatch, cursor movement after replay or
sequence gap, cursor loss after daemon restart, failed
rotation/revocation/replacement, changed VDR-native fingerprints or secret-like
log evidence. It never enables
`curl --insecure`, prints the Agent identity file or dumps process environment.
