# Phase 69.E — Retained Legacy Route Classification

## Status

**IMPLEMENTED CANDIDATE — second bounded 69.E slice.**

Baseline:

```text
main = 42c2d9fe0bf453f0dceddb77e85eaf4704ab6d1e
PR #366 / first 69.E compatibility-policy foundation = MERGED
69.D = COMPLETED
69.E = ACTIVE
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.E Compatibility Policy Foundation](phase-69e-compatibility-policy-foundation.md)
- [Phase 69 Public API Kickoff and Runtime Progress](phase-69-public-api-kickoff.md)

## Root cause and slice choice

The live inventory contains 124 route literals: 6 declared public-v1 literals and
118 retained unversioned route literals. None of the existing same-handler
legacy aliases has a stabilized public-v1 successor.

ADR-0048 permits deprecation metadata only after a real canonical replacement
exists and its semantics are mapped. The live repository has no
`/api/v1/search-timers`. The federated `/api/v1/channels` contract is
deliberately not the same contract as `/api/vdr/channels`, and
`/api/v1/timer-assignments` is not a successor for native
`/api/vdr/timers`.

Therefore this slice classifies the retained surface without deprecating,
redirecting or removing any route. It extends the existing
`tools/check_phase69_public_api_inventory.py` authority rather than creating a
second route registry.

## Complete retained-route classification

The machine-guarded inventory divides the 118 retained unversioned route
literals into exactly:

- 27 same-handler alias groups;
- 54 alias-member route literals;
- 64 standalone transition route literals;
- 0 deprecated aliases.

A same-handler alias group means only that both current pre-v1 paths enter the
same server-side handler/service branch. It does not make either spelling a
stable public API and does not prove equivalence to a future public-v1
representation.

The 27 currently proven groups are:

| Group | Routes | Contains mutation path |
| --- | --- | --- |
| `recording-action-validate` | `/api/recordings/actions/validate` ↔ `/api/vdr/recordings/actions/validate` | yes |
| `recording-action-execute` | `/api/recordings/actions/execute` ↔ `/api/vdr/recordings/actions/execute` | yes |
| `recording-action-preview` | `/api/recordings/actions/preview` ↔ `/api/vdr/recordings/actions/preview` | yes |
| `channel-move` | `/api/vdr/channels/move` ↔ `/api/vdr/channels/actions/move` | yes |
| `searchtimer-preview-cache-refresh` | `/api/searchtimers/preview/cache/refresh` ↔ `/api/vdr/searchtimers/preview/cache/refresh` | yes |
| `searchtimer-real-test` | `/api/searchtimers/real-test` ↔ `/api/vdr/searchtimers/real-test` | yes |
| `searchtimer-execute` | `/api/searchtimers/execute` ↔ `/api/vdr/searchtimers/execute` | yes |
| `searchtimer-plan` | `/api/searchtimers/plan` ↔ `/api/vdr/searchtimers/plan` | yes |
| `searchtimer-validate` | `/api/searchtimers/validate` ↔ `/api/vdr/searchtimers/validate` | yes |
| `searchtimer-update` | `/api/searchtimers/update` ↔ `/api/vdr/searchtimers/update` | yes |
| `searchtimer-delete` | `/api/searchtimers/delete` ↔ `/api/vdr/searchtimers/delete` | yes |
| `searchtimer-root` | `/api/searchtimers` ↔ `/api/vdr/searchtimers` | yes |
| `native-fuzzy-refresh` | `/api/epgsearch/native-fuzzy/refresh` ↔ `/api/vdr/epgsearch/native-fuzzy/refresh` | yes |
| `native-fuzzy-stale-probe-delete` | `/api/epgsearch/native-fuzzy/stale-probes/delete` ↔ `/api/vdr/epgsearch/native-fuzzy/stale-probes/delete` | yes |
| `vdr-overview` | `/api/vdr` ↔ `/api/vdr/overview` | no |
| `recording-folder` | `/api/vdr/recordings/folder` ↔ `/api/vdr/recordings/folders` | no |
| `recording-metadata-image` | `/api/recordings/metadata/image` ↔ `/api/vdr/recordings/metadata/image` | no |
| `recording-metadata` | `/api/recordings/metadata` ↔ `/api/vdr/recordings/metadata` | no |
| `persons` | `/api/persons` ↔ `/api/vdr/persons` | no |
| `recording-person-search` | `/api/recordings/persons/search` ↔ `/api/vdr/recordings/persons/search` | no |
| `timer-conflicts-live` | `/api/vdr/timer-conflicts/live` ↔ `/api/vdr/timers/conflicts/live` | no |
| `searchtimer-discovery` | `/api/searchtimers/discovery` ↔ `/api/vdr/searchtimers/discovery` | no |
| `searchtimer-automation-preview` | `/api/searchtimers/automation/preview` ↔ `/api/vdr/searchtimers/automation/preview` | no |
| `searchtimer-preview` | `/api/searchtimers/preview` ↔ `/api/vdr/searchtimers/preview` | no |
| `runtime-summary` | `/api/runtime/summary` ↔ `/api/runtime/diagnostics/summary` | no |
| `runtime-diagnostics` | `/api/runtime` ↔ `/api/runtime/diagnostics` | no |
| `global-search` | `/api/search` ↔ `/api/vdr/search` | no |

Every other retained unversioned literal is listed explicitly in
`LEGACY_TRANSITION_ROUTE_LITERALS`. The inventory guard fails if a retained
route appears without one of these classifications, appears in both classes, or
if an alias group silently acquires an unproven public-v1 successor.

`deprecatedAliases = []` therefore remains the only truthful capability value
for this slice.

## Successor and equivalence boundary

No alias group in this slice names a `publicV1Successor`. Moving one group from
`transition` to `deprecated` requires:

1. a stabilized canonical public-v1 successor already exists;
2. request, response, stable identity, authorization, backend scope and failure
   semantics are explicitly mapped;
3. a mutating alias reaches the same durable operation/idempotency identity and
   cannot be retried speculatively after an ambiguous outcome;
4. the first deprecated release and successor are documented;
5. any `Sunset` date is an explicit release-policy decision.

This is why SearchTimer duplicates are classified but not deprecated. Creating
`/api/v1/search-timers` merely to manufacture a successor would invert the
required architecture proof.

## First-party fallback inventory

The bundled client still has two catch-all helpers,
`requestJsonWithFallback()` and `requestJsonWithFallbacks()`. They retry the
next path for any rejected request; they do not first prove a route-unsupported
condition.

There are 15 fallback call sites in `web/frontend/api/client-api.js`. One
probes `/api/vdr/searchtimers/live`, which is not present in the server route
inventory, before trying retained SearchTimer routes.

The four definite state-changing SearchTimer mutation fallbacks are
`fetchClientSearchTimerExecute()`, `fetchClientSearchTimerCreateAction()`,
`fetchClientSearchTimerUpdateAction()` and
`fetchClientSearchTimerDeleteAction()`. They are architecture debt recorded by
this slice, not behavior approved by it. Broad client migration belongs to
69.F.

A separate GET-only fallback remains in `web/frontend/epg-cache.js`:
`/api/epg/now-next?from=-1` falls back to `/api/vdr/events` after any
non-success response rather than only a proven unsupported-route result. The
guard records that fact without touching Home or EPG runtime.

## Guards and non-scope

`tools/check_phase69_public_api_inventory.py` now owns the complete retained
classification. `tools/check_phase69e_legacy_route_classification.py` proves
same-handler membership, complete counts, no invented v1 successor,
`deprecatedAliases = []`, and the current client fallback inventory.

This slice changes no HTTP route behavior, representation, authorization,
backend scope, durable operation identity, native VDR effect or client retry
behavior. It does not change Home, Home rails, Recording discovery/metadata
fanout, LiveTV, Live Preview, Now/Next refresh, EPG aggregation,
SuiteBridge/Agent transport or Timer orchestration.

No real-yaVDR native-effect acceptance is required. Focused architecture/docs
guards plus hosted CI are sufficient.
