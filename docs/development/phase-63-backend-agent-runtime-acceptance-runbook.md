# Phase 63 Backend Agent runtime acceptance runbook

This runbook is the binding real-yaVDR acceptance path for the bounded Phase-63
Backend Agent foundation.  It is intentionally fail-closed and must be run
only after all required GitHub Actions jobs are green for the exact PR head.

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

The lost-response credential-rotation branch remains covered by focused
automated tests because deliberately dropping a committed HTTPS response on a
production yaVDR host is not a safe live acceptance action.

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
HEAD=<exact accepted head>
FIRST_AGENT_ID=<revoked Agent identifier>
REPLACEMENT_AGENT_ID=<active replacement identifier>
CREDENTIAL_GENERATION=<rotated generation>
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
failed rotation/revocation/replacement, changed VDR-native fingerprints or
secret-like log evidence.  It never enables
`curl --insecure`, prints the Agent identity file or dumps process environment.
