# ADR-0068 increment 2 — Public-v1 Discovery routes (2026-10-21)

Baseline: `main` at `b8220196eb7b6d7d67ccbc46162f38b75cd0a233`; increment 1 / PR #509 has passed CI and merged. This is **source-only restructuring**, without changing runtime contracts.

## Exact responsibility boundary

- `api/rest/src/PublicDiscoveryRoutes.cpp` now owns existing Discovery GET body projection: original `contractRoot` and `platformCapabilities` expressions, same ordered capability list, and the **same configured lookup callbacks on the original `PublicApiRuntime` instance**. This module introduces no mutable owner, callback cache, credential handling or second router.
- `PublicApiRuntime::tryHandleGet` retains both exact-path GET dispatch checks. The existing single `jsonResponse` formats status 200, `application/json; charset=utf-8`, `Cache-Control: no-store`, `X-Content-Type-Options: nosniff`, and request/correlation headers. The new module only returns a body string.
- `tryHandlePost`, `tryHandleDelete`, `tryHandleUnsupportedMethod` (405 and Allow), unknown `/api/v1/*` path handling (404), and the prior SecurityHttpGate authentication/authorization ownership are unchanged.
- The existing 46 registration/reset pairs and callback mutexes retain their current instance, locks and shutdown lifecycle. No public Recording/Timer, Identity, Service, database, executor, frontend or yaVDR state is modified.
- The existing `REST_PUBLIC_API_SRC` Make group now owns both C++ translation units, reused by every formerly single-source standalone C++ test. Phase-69 compatibility source assertions read both source files without relaxing any expected tokens.

## Frozen contract checks

| Route | GET semantics | Other methods and unknown paths |
| --- | --- | --- |
| `/api/v1` | 200 JSON: build version, compatibility policy, authenticated boolean and links; existing standard success headers | Previously defined 405/Allow and unknown-child 404 unchanged |
| `/api/v1/capabilities` | 200 JSON: exact ordered dynamic callback availability and compatibility metadata; existing standard success headers | Previously defined 405/Allow and path fallthrough unchanged |

The pre-existing `test_public_api_contract_runtime.cpp` covers authentication presentation, success headers, dynamic capability toggles, 405/Allow and unknown-route Problem Details. New assertions compare the full body, headers, content type and status with and without irrelevant query suffixes for both paths. No R2/R3 hardware acceptance is repeated.

ADR-0068 is **not yet complete**. Subsequent domain extractions of public Backend/EPG/Channel/Timer/Account/Device handling must preserve the same security, method and frontend contract baseline. E1 remains gated behind the complete checkpoint.
