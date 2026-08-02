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
- [Phase 62 Slice 2W selection](development/phase-62-slice-2w-browser-session-retention-cleanup.md)
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

The Slice-2W selection document, Slice-2V closeout, runtime evidence, current
state and current-status files are the newest Phase 62 authorities.

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

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

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

Accepted through:
Slice 2V - Browser-Session Idle Expiry and throttled last_seen

Accepted Slice-2V implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Accepted Slice-2V source CI:
VDR-Suite CI #6779
Run ID 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Accepted Slice-2V closeout head:
cf31b2b67f73f12718601ced5468a59a1183adcb

Accepted Slice-2V closeout CI:
VDR-Suite CI #6799
Run ID 30742295881
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742295881

Selected next bounded slice:
Slice 2W - Browser-Session Terminal Retention Cleanup

Slice-2W state:
selection and contract documented; implementation not started
```

PR #117 must remain open and Draft. Do not mark it Ready, merge it, enable
auto-merge, rebase, force-push, rewrite branch history or mutate review state
without explicit approval.

PR #118 is the separate paused TVScraper bugfix workstream. Do not mix its code
or daemon fingerprint into Phase 62 commits or evidence.

## Installed real-runtime truth

**VERIFIED on 2026-08-02 after successful Slice-2V acceptance:**

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

Runtime evidence:
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25

Runtime report SHA-256:
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86
```

The accepted Phase-62 daemon remained installed and active. The original
configuration was restored exactly. The runtime-only systemd drop-in was removed
and the idle test environment was absent from the final unit. Process IDs and
service timestamps remain volatile.

## Fully accepted Slice 2V

**VERIFIED:**

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS
ordinary_get_before_idle=HTTP 200
ordinary_get_after_idle=HTTP 401 session_expired
protected_mutation_after_idle=HTTP 401 session_expired
last_seen_writes_inside_interval=1
absolute_expiry_unchanged=yes
replacement_logout=HTTP 204
revoked_cookie_replay=HTTP 401 credential_revoked
test_lifecycle_active_rows=0
sqlite_quick_check=ok
sqlite_foreign_key_check=empty
accountability_secret_free=yes
vdr_domain_mutations=0
final_service_state=active
runtime_dropin=removed
idle_test_environment=not-set
```

Do not repeat Slice-2V runtime acceptance merely because a chat changes. Repeat
only when a directly relevant daemon, configuration, schema or lifecycle
contract changes.

## Selected Slice 2W contract

Exactly one next slice is selected:

```text
Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup
```

Configuration:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay
fixed batch size  256
```

Ownership and trigger:

- one bounded pass during Security Runtime initialization;
- after schema and configuration validation;
- before authenticators/gates are declared ready;
- no periodic thread, scheduler or request-path cleanup.

Deletion boundary:

- select only browser lifecycles terminal beyond retention;
- terminal sources are explicit revocation, absolute expiry and idle expiry when
  the accepted idle policy is enabled;
- recheck eligibility inside one `BEGIN IMMEDIATE` transaction;
- append secret-free cleanup accountability;
- delete the browser verifier;
- delete its canonical browser session only if unreferenced;
- delete its credential only if it is exactly type `browser-session` and
  unreferenced;
- preserve actor, device, issuing credential, grants, roles and accountability;
- any enabled-policy failure leaves the Security Runtime fail closed.

Explicitly excluded:

- cleanup triggered solely by issuer revocation;
- automatic eviction for concurrency limits;
- session listing, logout-all or administration API/UI;
- generic identity or credential cleanup;
- periodic scheduling;
- Outbox, Android, Android TV or Phase 63-67 runtime.

No Slice-2W implementation or runtime mutation has occurred.

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
`/etc/nginx/snippets/vdr-suite.conf`. Do not repeat public-origin acceptance
unless a directly relevant routing, frontend, daemon or configuration
fingerprint changes.

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

## Anti-loop boundary

Do not repeat completed repository analysis, public-origin work, accepted route
migration, Slice-2R lifetime, Slice-2S outcomes, Slice-2T issuer binding,
Slice-2U concurrency or Slice-2V idle acceptance unless a directly relevant
fingerprint or contract changed.

## Remaining Phase 62 gaps

Beyond the selected Slice 2W, still open are:

- broader operation outcomes and stronger transaction coupling;
- common revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- protected audit reads, export, redaction and retention;
- compatibility retirement and final Phase 62 closeout.

## Exact next action

1. require all five GitHub Actions jobs on the documentation-only Slice-2W
   selection head;
2. keep PR #117 open and Draft;
3. after green selection CI, implement only the selected Slice-2W configuration,
   bounded cleanup transaction/service, startup integration, focused tests,
   architecture guard and Make-test registration;
4. do not combine implementation with scheduling, administration APIs, issuer
   cascade, automatic eviction, broader security administration, Outbox,
   Android or Phase 63-67 runtime.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing or next-action truth
changes. Preserve durable non-secret evidence and keep the next permitted action
explicit.
