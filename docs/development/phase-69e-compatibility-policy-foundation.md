# Phase 69.E — Compatibility Policy Foundation

## Status

**IMPLEMENTED CANDIDATE — first bounded 69.E slice.**

Baseline:

```text
main = 8e3d56f2b99f0f52412d8849645b463de5267366
69.D = COMPLETED
69.E = ACTIVE
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.D Closeout](phase-69d-closeout.md)
- [Phase 69 Public API Kickoff and Runtime Progress](phase-69-public-api-kickoff.md)

## Root cause and slice choice

The live repository already has the transport prerequisite for deprecation
metadata: `ApiResponse` carries arbitrary response headers and the HTTP server
propagates them. Phase 69.B deliberately left `Deprecation`, `Sunset` and
successor `Link` values unassigned until 69.E had an owning compatibility
policy.

The public contract root already advertises `apiVersion=v1` and
`supportedApiMajors=["v1"]`, while `/api/v1/capabilities` already uses
versioned capability items. What is missing is a machine-readable statement of
the compatibility rules themselves.

Therefore this first 69.E slice publishes the accepted ADR-0048 policy through
the existing public discovery resources. It does **not** deprecate or remove any
legacy alias yet. Alias retirement can only start after this policy is visible
and contract-tested.

## Public discovery contract

`GET /api/v1` retains its existing identity and links and now includes:

```json
{
  "compatibilityPolicy": {
    "version": 1,
    "responseEvolution": "additive",
    "breakingChanges": "new-major",
    "unknownRequestFields": "reject",
    "legacyUnversioned": "transition"
  }
}
```

The policy means:

- response representations may gain additive fields within v1;
- removing or changing supported v1 semantics is a breaking change and normally
  requires a new public API major;
- request objects remain closed by default, so unknown request fields are
  rejected unless an endpoint documents an extension point;
- the unversioned `/api/...` surface remains transitional compatibility and is
  not another supported public API major.

`GET /api/v1/capabilities` adds two versioned platform capabilities:

```text
public-api.compatibility-policy / version 1 / available
public-api.deprecation-metadata / version 1 / available
```

It also publishes the compatibility policy version, supported API majors,
lifecycle states and supported deprecation metadata headers:

```text
supported -> deprecated -> sunset-announced -> removed
Deprecation / Sunset / Link
```

`deprecatedAliases` is currently an empty array. That is deliberate: this
slice establishes policy and negotiation only; it does not silently classify an
existing route as deprecated.

## Compatibility matrix

| Surface | Current server contract | Status |
| --- | --- | --- |
| Public API v1 | `/api/v1` and explicitly stabilized v1 resources | Supported / canonical |
| Public API v2 | not advertised in `supportedApiMajors` | Unsupported |
| Unversioned `/api/...` | retained migration/compatibility surface | Transitional, not a public API major |
| Agent / Media / OSD / plugin schemas | separately versioned protocols | Not inferred from public API v1 |

The server package version is diagnostic build identity. Clients must use
`supportedApiMajors` and versioned capabilities rather than infer compatibility
from the package version.

## Deprecation boundary

Canonical `/api/v1` root and capability responses do not receive
`Deprecation`, `Sunset` or successor `Link` headers from this slice.

A later 69.E slice may deprecate a concrete legacy alias only after proving:

1. a canonical v1 successor exists;
2. authorization and representation semantics are explicitly mapped;
3. mutation aliases, if any, preserve the same durable operation/idempotency
   identity and never use speculative fallback;
4. the first deprecated release and successor are documented;
5. any `Sunset` date is an explicit policy decision, never derived from package
   version or guessed by the client.

## Non-scope

This slice does not change Home, Recording discovery/metadata fanout, LiveTV,
Live Preview, EPG aggregation, Timer mutation, SuiteBridge/Agent transport,
snapshot polling or any native VDR effect.

It adds no new public route and does not remove, redirect or deprecate an
existing route.

No real-yaVDR native-effect acceptance is required for this public discovery
contract.
