# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Read it before
repeating repository-wide analysis or real-runtime acceptance. A new chat alone
is not a reason to start over.

Trust completed items marked **VERIFIED** unless a directly relevant repository,
binary, configuration, database, routing or behaviour fingerprint changed.

## Canonical reading

- [Current project truth](CURRENT.md)
- [Current project status](development/current-status.md)
- [Phase 62 Slice 2V closeout](development/phase-62-slice-2v-browser-session-idle-expiry.md)
- [Phase 62 runtime evidence](development/phase-62-runtime-evidence.md)
- [Phase 62 gap matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Security and identity architecture](architecture/security-identity-foundation.md)
- [Strict roadmap](planning/roadmap.md)
- [Phase map](planning/phase-map.md)
- [Parity and frontend gap roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture audit gap matrix](planning/architecture-audit-gap-matrix.md)
- [Completed phases](development/completed-phases.md)
- [Phase 61 closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 runtime closeout](development/post-phase-61-platform-runtime-closeout.md)
- [ADR index](adr/index.md)
- [Agent workflow rules](../AGENTS.md)

The Slice-2V closeout, runtime evidence, current state and current-status files
are the newest authorities for Phase 62 repository, CI and real-runtime truth.
The historical Slice-3A checkpoint remains authoritative only for public-origin
and Nginx routing history.

## Stable project position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella track:
Phase 58 - Frontend and Live Parity

Completed platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 state:
active and incomplete

Phase 63-67 runtime:
not advanced
```

## Active workstream

```text
Repository: hotzenplotz5/vdr-suite
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Remote branch: phase-62-security-identity-foundation
Pull request: #117
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
PR state: open, Draft, unmerged, mergeable

Repository, source CI and real-runtime accepted through:
Slice 2V - Browser-Session Idle Expiry and throttled last_seen

Accepted implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Source CI:
VDR-Suite CI #6779
Run ID 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Canonical documentation closeout series:
starts at 45f1cc78d2c98f6db4d039a5ea7189f51bbcf8e9

Final closeout CI:
pending on the current branch head

Active implementation after Slice 2V:
none selected
```

PR #117 must remain open and Draft. Do not mark it ready, merge it, enable
auto-merge, force-push, rewrite branch history or mutate review state without
explicit approval.

PR #118 is the separate TVScraper bugfix workstream and is currently paused. Do
not mix PR #118 or its daemon fingerprint into Phase 62 commits or evidence.

## Installed real-runtime truth

**VERIFIED on 2026-08-02 after the successful Slice-2V acceptance:**

```text
Daemon unit:
vdr-suite-daemon.service

Installed executable:
/usr/sbin/vdr-suite-daemon

Installed/running daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Restored daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Final service PID observed by acceptance:
86549

Runtime evidence:
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25

Runtime report SHA-256:
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86
```

The service was active. The accepted Phase-62 daemon remained installed and
running. The original configuration was restored exactly. The runtime-only
systemd drop-in was removed and the idle test environment was absent from the
final unit. Process IDs and service timestamps remain volatile.

## Cumulative accepted Phase 62 foundation

**VERIFIED in source CI and, where required, on the real yaVDR runtime:**

- canonical actor, device, session, credential, authentication, request and
  correlation context;
- persistent actor/device/session/credential lifecycle;
- optional Managed Basic authentication without managed defaults;
- persistent browser-session verifiers with independent session and CSRF
  secrets;
- strict cookie parsing and browser-cookie precedence without Basic fallback;
- exact actor permission and backend-scope grants;
- fixed exact-scope Admin and Read-only role semantics;
- memory-only Webfrontend CSRF ownership and exact route injection;
- protected Remote, Timer, Channel Move, Recording and SearchTimer mutation
  families;
- explicit Safe POST classification for accepted validation and preview routes;
- protected Native Fuzzy refresh, global stale-probe deletion and query-scoped
  cache refresh;
- configurable immutable absolute browser-session lifetime;
- append-only pre-dispatch accountability;
- browser-session issue and revoke outcome accountability with compensation;
- request-time issuing-credential lifecycle binding;
- optional per-actor effective browser-session concurrency limit with deny-new
  semantics;
- optional browser-session idle timeout with additive `last_seen_at`;
- one repository-owned idle calculation for cookie and CSRF paths;
- fixed 60-second activity-write throttle;
- idle-expired sessions excluded from the effective concurrency count;
- mutation-safe real-runtime acceptance harnesses with guarded rollback.

## Slice 2V runtime acceptance

**VERIFIED on 2026-08-02 at
`e84415fadb2587ff744ff8927f1f0113920ece2f`:**

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS

Configured idle timeout:
300 seconds

Activity-write interval:
60 seconds

Ordinary GET before idle expiry:
HTTP 200

Ordinary GET after idle expiry:
HTTP 401 session_expired

Protected mutation after idle expiry:
HTTP 401 session_expired

last_seen writes inside accepted interval:
1

Absolute expiry unchanged:
yes

Replacement logout:
HTTP 204

Revoked replacement-cookie replay:
HTTP 401 credential_revoked

Acceptance lifecycle active rows after cleanup:
0

SQLite quick check:
ok

SQLite foreign-key check:
empty

Accountability secret-free:
yes

VDR domain mutations:
0

Final service state:
active

Runtime drop-in:
removed

Idle test environment:
not set
```

Do not repeat this runtime acceptance merely because a chat changes. Repeat only
when a directly relevant daemon, configuration, schema or lifecycle contract
changes.

## Public-origin truth

**VERIFIED historical installed contract:**

```text
/vdr-suite          -> 308 /vdr-suite/frontend/
/vdr-suite/         -> 308 /vdr-suite/frontend/
/vdr-suite/frontend -> 308 /vdr-suite/frontend/

Public root /api/*:
yaVDR-owned

Public Suite API prefix:
/vdr-suite/api/*

Internal daemon API prefix:
/api/*
```

The active repository-managed Nginx snippet is
`/etc/nginx/snippets/vdr-suite.conf`. The public prefix is stripped before
proxying to the daemon and cookie paths are rewritten to `/vdr-suite/`.

Do not repeat public-origin installation or routing acceptance unless a directly
relevant Nginx, frontend, daemon or configuration fingerprint changes.

## Anti-loop boundary

Do not repeat any completed work unless its directly relevant fingerprint or
contract changed:

1. repository-wide Phase 62 analysis;
2. public-origin and Nginx installation or acceptance;
3. accountability transaction-serialization root-cause analysis;
4. persistent browser-grant acceptance;
5. fixed-role and accepted route-family runtime acceptance;
6. Safe POST inventory and classification;
7. absolute browser-session lifetime acceptance;
8. browser issue/revoke outcome acceptance;
9. issuing-credential lifecycle acceptance;
10. concurrent browser-session limit acceptance;
11. browser-session idle-expiry and throttled-activity acceptance;
12. credential rotation or password recovery;
13. TVScraper work from the separate paused PR #118.

## Credential and secret restrictions

The Managed-Basic plaintext password used for earlier acceptance is unavailable.
Do not rotate or reprovision credentials implicitly.

Never print, store or commit:

- Authorization headers;
- plaintext passwords or password hashes;
- cookies or complete cookie values;
- CSRF tokens;
- raw session or verifier secrets;
- login response bodies containing secrets;
- process environments.

Preserve only non-secret status, paths, request outcomes and approved artifact
fingerprints.

## Volatile recheck rule

Recheck only state needed for the next approved operation:

1. local branch, HEAD, upstream and clean status before local mutation;
2. remote head and PR Draft/base/CI state before repository mutation;
3. installed daemon, configuration and loader fingerprints before runtime
   replacement;
4. service, listener and database state only around an approved runtime change.

A changed PID alone does not invalidate completed acceptance.

## Remaining Phase 62 gaps

After Slice 2V, the accepted POST inventory remains complete. Still open:

- physical browser-session cleanup and retention;
- operation outcomes beyond browser lifecycle operations;
- stronger transaction coupling or Outbox semantics;
- common revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- protected audit reads, export, redaction and retention;
- compatibility-retirement readiness and final Phase 62 closeout.

No next implementation slice is selected by the Slice-2V runtime acceptance.

## Exact next action

1. require all five GitHub Actions jobs on the final Slice-2V closeout head;
2. keep the refreshed PR #117 description while the PR remains open and Draft;
3. only after full green closeout CI, perform a fresh post-2V gap analysis;
4. select exactly one bounded next Phase-62 slice;
5. do not combine selection with implementation or runtime mutation.

Do not begin cleanup, retention, eviction, session administration, broader
security administration, Outbox, Android or Phase 63-67 runtime before Slice 2V
is fully closed.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing or next-action truth
changes. Preserve durable non-secret evidence, record exact fingerprints when
they decide whether acceptance must repeat, and keep the next permitted action
explicit.
