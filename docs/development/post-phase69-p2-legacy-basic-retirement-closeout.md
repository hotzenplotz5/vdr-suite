# Post-Phase-69 P2 Legacy Basic Retirement Closeout

## Status

Completed.

This closeout records the durable end state after the P2 Human Account / First
Admin / bootstrap-recovery sequence and the accepted Legacy Basic deployment
migration.

## Accepted sequence

The retirement boundary was crossed in two bounded steps:

1. PR #403 accepted the guarded real-yaVDR migration/rollback sequence and the
   corrected pre-P2 database handling.
2. PR #404 removed the Legacy Basic runtime compatibility implementation after
   that deployment evidence existed.

Accepted repository checkpoints:

```text
PR #403 merge:
c8597d23d729054853a56e3dd61aba6c4ee51bd9

PR #404 head:
c1b047b76bed13f4fc0d556b6a3f63f383a06a7d

PR #404 merge/main:
fb25d95de7aea23c8dfbc9e5ab67b1cd9fad46d6

PR #404 hosted CI:
VDR-Suite CI #9670
run 36836143208
result SUCCESS
```

The retained real-runtime acceptance evidence for the migration gate is
documented in
[Legacy Basic Retirement Runtime Acceptance](p2-legacy-basic-retirement-runtime-acceptance.md).
That acceptance proved the bounded historical transition
`legacy-basic -> enforced -> legacy-basic -> enforced` while preserving Human
Account login and persistent identity.

## Current runtime authority

Legacy Basic is retired runtime history, not a current authentication authority.

Current supported human/browser authentication authorities are:

- Human Account password authentication with browser sessions;
- optional Managed Basic where deliberately configured.

The retirement removal means:

- no Legacy Basic deployment mode or authenticator remains;
- stale `VDR_SUITE_SECURITY_MODE`, `VDR_SUITE_BASIC_AUTH` and
  `VDR_SUITE_LEGACY_BASIC_*` values do not reactivate compatibility authority;
- startup does not provision a Legacy Basic identity;
- browser/general HTTP and HbbTV paths have no Legacy Basic fallback;
- older preserved defaults-file lines may remain on disk but are inert;
- the historical rollback sequence is evidence only and is not a current
  runtime rollback mechanism.

Older unclaimed/pre-P2 installations use the accepted local trusted First Admin
bootstrap/claim path instead of a compatibility credential.

## Real yaVDR removal deployment

The merged #404/main removal candidate was installed on the supported real
yaVDR target and accepted with:

```text
source main:
fb25d95de7aea23c8dfbc9e5ab67b1cd9fad46d6

installed/running daemon SHA256:
cbe42ba3c4fd060ea4a54238d568e9dc659ac6e7158f06a2e529cfee4044b6a8

service:
active

HTTP readiness:
200 on port 18080

VDR restarted:
no

Backend Agent restarted:
no
```

The existing `/etc/default/vdr-suite-daemon` fingerprint was unchanged across
that installation. This confirms the removal candidate did not require rewriting
the preserved operator defaults file.

## Closure decision

The cross-cutting **Legacy Basic retirement** product milestone is closed.

Historical Phase-62 and P1/P2 audit documents may continue to describe the
transitional state that existed at their acceptance time. Active status,
roadmap, handoff and architecture-gap documents must describe the current
post-retirement authority instead of copying that historical state forward.

## Successor boundary

This closeout does **not** start Phase 70. Phase 70 remains not started until its
dedicated runtime ADR is accepted.

The still-open non-numbered productization work includes:

- Account and Backend Access Administration;
- Broad Timer Product UI after its access-administration prerequisite;
- Audit / Security / Operations product surfaces;
- first-party client-family rollout;
- release-grade Debian/Ubuntu packaging;
- federation/pairing and later Profile/device work under their accepted
  architecture boundaries.

No single one of those product milestones is made active merely by completing
Legacy Basic retirement. A successor runtime slice still requires its own
binding requirement, accepted-code gap and bounded implementation contract.
