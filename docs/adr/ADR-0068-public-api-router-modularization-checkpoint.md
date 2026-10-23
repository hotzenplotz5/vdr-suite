# ADR-0068: Public API Router Modularization Checkpoint After Recording R3

## Status

**CLOSED — behavior-preserving source refactor merged in [PR #517](https://github.com/hotzenplotz5/vdr-suite/pull/517) on 2026-10-22 with exact-head and post-merge CI SUCCESS.**

Date: 2026-10-21

Decision authority: approved Public API v1 development sequence for `main`.

The implementation/merge proof is recorded below. ADR-0068 certifies source modularization and CI, **not** a productive daemon deployment, new hardware acceptance, complete OpenAPI conformance, unrelated API development or reopening Phase 69. See
[the single-branch implementation record](../development/adr-0068-single-branch-read-dispatch-progress.md)
and [the contract reconciliation](../development/adr-0068-public-legacy-contract-reconciliation.md).

## Implementation closeout (2026-10-22)

- Single branch `work/adr0068-router-modularization-20261022`; final PR-head [`867f43b67`](https://github.com/hotzenplotz5/vdr-suite/commit/867f43b67e88f6b9c904db5b98e99ce1b79ff784); merge [`6c05493ad`](https://github.com/hotzenplotz5/vdr-suite/commit/6c05493ad53c82e19fef4a2bc70e09bc1603e0c8) into `main`.
- All four required PR-head checks and all four post-merge checks passed; [CI links and contract scope](../CURRENT.md#adr-0068-closeout--merged-and-source-ci-accepted-2026-10-22).
- Existing GET domain routing, ordered POST domain routing, callback/admission lifecycle and source guards were extracted without intended contract changes. The [60 OpenAPI-declared method/path contract entries](../development/adr-0068-public-legacy-contract-reconciliation.md) are an audited **partial** inventory, not exhaustive coverage.
- No installed daemon or hardware acceptance is inferred. E1 begins **after this closeout** as independent functional work; Phase 70 remains unstarted.

## Context

`ApiRouter` and especially `PublicApiRuntime` have grown as multiple
Public-v1 domains were added to VDR-Suite. Their shared infrastructure and
route-specific concerns are increasingly difficult to review and test independently.

At the same time, the public Recording mutation boundary is actively changing.
R1 introduced public `rec_` identity resolution through the already established
`PublicRecordingPlaybackTargetResolver` and
`PublicRecordingIdentityRepository`; R2 and R3 must still establish
correct public RENAME/DELETE and MOVE contracts, authoritative revisions,
readback and persistent identity continuity. A simultaneous large router
reorganization would obscure whether regressions arise from a domain
contract change or moving code.

The existing foundations are authoritative, including:

- [ADR-0014](ADR-0014-recording-identity-strategy.md): Recording identity;
- [ADR-0042](ADR-0042-safe-mutation-revision-idempotency-contract.md): mutation safety;
- [ADR-0048](ADR-0048-public-api-versioning-error-compatibility-contract.md): Public-v1 compatibility;
- [ADR-0063](ADR-0063-mutation-complexity-proportionality-reuse.md): proportional reuse.

A refactor may reorganize these implementations, but cannot create a
second identity/security/mutation owner.

## Decision: mandatory implementation order

The agreed working sequence on `main` is:

```text
R1 -- public Recording rec_ identity / read-only Validate and Dry-run
  -> R2 -- public RENAME and DELETE, revision, validation, readback, identity lifecycle
  -> R3 -- public MOVE, logical destination, verified readback and rec_ rebind
  -> CHECKPOINT -- behavior-preserving ApiRouter / PublicApiRuntime modularization
  -> E1 -> E2 -> M1 -> M2 -> S1 -> P1
```

No large-scale `ApiRouter` or `PublicApiRuntime` refactoring may begin
before R1–R3 are semantically stabilized with appropriate tests and
acceptance evidence. Small directly necessary route changes for R1–R3
remain permitted. A completed feature commit by itself does not
satisfy a hardware or rollout gate.

**The checkpoint is deliberate and mandatory before the next large
Public API v1 development wave**, not a proposal to interrupt R2/R3.
`docs/CURRENT.md` owns volatile progress; this ADR records only the
accepted architectural decision.

## Proposed responsibility boundaries

The target is to decompose the existing Public-v1 handling by domain,
with exact final filenames chosen from the actual post-R3 source tree:

```text
PublicApiRuntime / Public API router
    |-- PublicDiscoveryRoutes
    |-- PublicBackendRoutes
    |-- PublicChannelRoutes
    |-- PublicRecordingRoutes
    |-- PublicEpgRoutes
    |-- PublicTimerRoutes
    |-- PublicSearchTimerRoutes
    |-- PublicAccountRoutes
    |-- PublicDeviceRoutes
    '-- PublicMediaRoutes
```

These are **desired ownership boundaries, not already existing classes
or a mandate to create ten new services/files**. Only extract cohesive
route registration, parsing, response assembly and request orchestration
which already exist. Shared HTTP/Public-v1 helpers may be consolidated
if their behavior is identical. Existing domain services, public
identity repositories, durable operation state, security gates and
backend adapters remain their sole authorities.

Start with an exact post-R3 inventory of routes, request/response
shapes, status codes, headers, security mappings and registration
lifecycle, then choose the smallest coherent extraction slices.
Avoid making the router checkpoint another large feature project.

## Non-negotiable behavior-preservation contract

**No functional change is allowed as part of this checkpoint:**

1. **No new API functions** and no removed functions.
2. **No changed URL, HTTP method, query/body schema or JSON payload.**
3. **No changed HTTP statuses, Problem Details, ETag/If-Match,
   Idempotency-Key, cache or correlation/request headers.**
4. **No changed authentication, backend grant scope, CSRF,
   revocation, actor ownership or audit behavior.**
5. **No new domain logic, database, identity authority, executor,
   mutable resource owner or backend fallback.**
6. **No silent client compatibility, packaging or daemon bootstrap
   lifecycle change.**
7. Legacy/private routes remain in place and maintain their exact
   contracts; never re-route a legacy mutation to a public new owner.

Where an extraction would require a behavior change, defer that change
into its own later domain slice with separate review and acceptance.

## Acceptance and review gates

Before extraction:

- R1/R2/R3 source semantics and test baselines established on the
  exact current `main`; specific hardware acceptance tracked
  separately from CI;
- complete registered route inventory, including paths not yet
  enumerated in the partial OpenAPI specification;
- route matrix for method/path, body, success and error schemas,
  status and headers, security/grants, frontend/VIDAA references;
- explicit legacy compatibility baseline.

For every extraction increment:

- golden request/response and negative security contract tests,
  including grants, CSRF, stale revisions and identity provenance;
- no changed URL, status, JSON contract, ordering, paging,
  header or error semantics;
- existing unit/integration/security tests plus GitHub CI at the
  exact implementation commit;
- no production Recording mutation or hardware action to prove
  a purportedly behavior-preserving source move without a
  separate approval.

Checkpoint completion requires a reviewable source tree in which
route responsibilities are separated and old contracts compare
equivalently. New features may resume only after the checkpoint is
finished or an explicit user decision changes the sequence.

## Consequences

- Recording-v1 domain behavior is stabilized first; refactoring
  does not mix domain migration with infrastructure relocation.
- Future EPG/metadata/search-timer/resume slices can use smaller
  domain-organized Public-v1 route surfaces.
- Short-term router size is tolerated to avoid concurrent change risk.
- No new API service, repository or branch is created by this ADR.

## References

- [Strict execution roadmap](../planning/roadmap.md)
- [Public-v1 coverage roadmap](../planning/public-v1-coverage-roadmap.md)
- [Recording-v1 Source Audit and R1–R3 Roadmap](../development/public-v1-source-audit-20261021.md)
- [Volatile status](../CURRENT.md)
- [R2 Issue #507](https://github.com/hotzenplotz5/vdr-suite/issues/507)
