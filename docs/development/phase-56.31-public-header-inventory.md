# Phase 56.31 - Public Header Inventory

## Navigation

- [Development Index](index.md)
- [Phase 56.29 - Packaging Boundary Guide](phase-56.29-packaging-boundary-guide.md)
- [Phase 56.30 - Package Candidate Source Inventory](phase-56.30-package-candidate-source-inventory.md)

---

## Status

Completed.

---

## Purpose

Phase 56.31 records the first public/private header inventory for package candidates.

Phase 56.30 mapped implementation source files. This phase maps header ownership so later packaging work can avoid exposing daemon-only, REST-implementation, test-support or mutation-sensitive headers as public API.

This phase does not move headers and does not create packages.

---

## Header Inventory Command

The authoritative local inventory command is:

```bash
find core api apps -type f \( -name '*.h' -o -name '*.hpp' \) | sort
```

For package-focused review, use:

```bash
find core/runtime/include core/http/include core/recordings/include core/vdr/include api/rest/include core/daemon/include -type f \( -name '*.h' -o -name '*.hpp' \) | sort
```

If a directory does not exist in a local checkout, skip it and keep the package candidate marked as `header-audit-pending`.

---

## Header Classification Labels

| Label | Meaning |
| --- | --- |
| `public-candidate` | May become installed public header after dependency audit. |
| `private-internal` | Repository-internal header only. |
| `rest-implementation` | REST controller/parser implementation header, not core library API. |
| `daemon-only` | Application/runtime composition header. |
| `test-support` | Mock/fake/test utility header. |
| `tooling` | Operator or smoke helper header. |
| `split-required` | Header currently belongs to a mixed area and needs narrower ownership. |
| `header-audit-pending` | Exact local header list still needs confirmation before packaging. |

---

## Runtime Headers

Package candidate:

```text
vdr-suite-runtime-core
```

Directory:

```text
core/runtime/include
```

Initial classification:

| Header area | Classification |
| --- | --- |
| runtime diagnostics value types | public-candidate |
| runtime diagnostics summary builder interfaces | public-candidate |
| runtime logger interfaces | public-candidate |
| console/null logger concrete headers | public-candidate or private-internal after dependency review |

Notes:

- Runtime headers are a strong first public candidate because the source group is already small.
- Install only headers that do not require daemon startup or REST controller dependencies.

---

## HTTP Headers

Package candidates:

```text
vdr-suite-http-client
vdr-suite-http-listener-internal
vdr-suite-test-support-http
```

Directory:

```text
core/http/include
```

Verified public client candidates:

| Header | Classification | Reason |
| --- | --- | --- |
| `core/http/include/HttpRequest.h` | public-candidate | Plain request value type. |
| `core/http/include/HttpResponse.h` | public-candidate | Plain response value type. |
| `core/http/include/IHttpClient.h` | public-candidate | Backend-neutral client interface. |

Expected package split:

| Header area | Classification |
| --- | --- |
| request/response/client interface headers | public-candidate |
| `BasicHttpClient` header | public-candidate after dependency audit |
| listener headers | private-internal / daemon-adjacent |
| mock client headers | test-support |
| test server headers | test-support |

Notes:

- HTTP client headers must not require listener or test-server headers.
- `MockHttpClient` and `TestHttpServer` may be documented as developer utilities, but they must not be production dependencies of `vdr-suite-http-client`.

---

## Recording Core Headers

Package candidates:

```text
vdr-suite-recording-core
vdr-suite-metadata-core
vdr-suite-dashboard-readmodel
vdr-suite-recording-action-validation
```

Directory:

```text
core/recordings/include
```

Initial classification:

| Header area | Classification |
| --- | --- |
| recording domain/value objects | public-candidate |
| recording repository/service headers | public-candidate after persistence dependency audit |
| metadata repository/service headers | public-candidate or shared persistence dependency |
| dashboard facade/read-model headers | private-internal until dashboard package is split |
| job repository/service headers | private-internal |
| recording action validation request/result headers | public-candidate for validation-only package |
| recording action execution/mutation headers | private-internal until safety policy package exists |
| recording action JSON serializers | public-candidate only if reused outside REST; otherwise private-internal |
| worker/rectools adapter headers | tooling/private-internal |

Notes:

- Metadata currently overlaps recording core and needs a package ownership decision.
- Mutation execution headers must not be installed as public headers until permissions and safety docs are complete.

---

## VDR Domain and Service Headers

Package candidates:

```text
vdr-suite-vdr-domain
vdr-suite-vdr-adapter-api
vdr-suite-vdr-service
vdr-suite-epg-core
vdr-suite-searchtimer-readonly
vdr-suite-snapshot-read
```

Directory:

```text
core/vdr/include
```

Known public domain candidates:

| Header area | Classification |
| --- | --- |
| `VdrStatus` | public-candidate |
| `VdrChannel` | public-candidate |
| `VdrEvent` | public-candidate |
| `VdrTimer` | public-candidate |
| `VdrRecording` | public-candidate |
| capability value objects | public-candidate |
| backend-neutral query/result value objects | public-candidate |

Adapter API candidates:

| Header area | Classification |
| --- | --- |
| adapter interface headers | public-candidate |
| backend registry contract headers | public-candidate after dependency audit |
| capability report service headers | public-candidate or private-internal pending dependency audit |
| concrete RESTfulAPI adapter headers | private-internal / concrete backend package |
| mock adapter headers | test-support |
| adapter factory headers | private-internal until composition ownership is clear |

Service candidates:

| Header area | Classification |
| --- | --- |
| VDR read/overview service headers | public-candidate |
| VDR recording query service headers | public-candidate |
| EPG query/search headers | public-candidate |
| SearchTimer preview/read-only headers | public-candidate |
| SearchTimer discovery headers | public-candidate or concrete backend integration depending on provider |
| snapshot read headers | public-candidate after cache ownership audit |
| change-feed read headers | public-candidate or private-internal pending API contract decision |

Internal VDR headers:

| Header area | Classification |
| --- | --- |
| SearchTimer create/update/delete execution | private-internal |
| workflow execution and production policy gates | private-internal |
| backend write allowlist/switch headers | private-internal |
| readback verification headers | private-internal until mutation package policy exists |
| polling coordinator headers | daemon-adjacent/private-internal |
| snapshot cache mutation/update headers | private-internal |
| live transport implementation headers | daemon-adjacent/private-internal |
| test live transport headers | test-support |

Notes:

- `core/vdr/include` is still `split-required` as a directory.
- Later work should split install-header manifests by domain, adapter API, service, read-only SearchTimer, snapshot-read, internal mutation and test support.

---

## REST Headers

Package candidates:

```text
vdr-suite-rest-api-contracts
vdr-suite-rest-implementation
```

Directory:

```text
api/rest/include
```

Initial classification:

| Header area | Classification |
| --- | --- |
| controller headers | rest-implementation |
| request parser headers | rest-implementation |
| route/router headers | rest-implementation / application-adjacent |
| REST query parameter helpers | rest-implementation utility |
| endpoint schemas and examples | public API contract documentation, not C++ public headers |

Notes:

- REST controller/parser headers are not public C++ library headers by default.
- Public REST API contracts should be documented as endpoint schemas/examples, not by installing controller headers.

---

## Daemon Headers

Package candidate:

```text
vdr-suite-daemon
```

Directory:

```text
core/daemon/include
```

Initial classification:

| Header area | Classification |
| --- | --- |
| runtime configuration headers | daemon-only |
| daemon runtime headers | daemon-only |
| daemon app/application headers | daemon-only |
| startup/runtime wiring headers | daemon-only |

Notes:

- Daemon headers must not be installed as public library API.
- Public libraries may be dependencies of the daemon. The daemon must not become a dependency of public libraries.

---

## Apps and Tooling Headers

Directories:

```text
apps
tools
```

Initial classification:

| Header area | Classification |
| --- | --- |
| CLI/application entry headers | tooling/application |
| smoke harness helper headers | tooling |
| destructive real-backend helper headers | tooling with destructive-operation warning |
| generated helper headers | private-internal unless explicitly promoted |

Notes:

- Tooling headers are not production library headers.
- Destructive helper headers must be labelled and isolated from default packages.

---

## Initial Public Header Package Map

| Package candidate | Header classification | Status |
| --- | --- | --- |
| `vdr-suite-runtime-core` | runtime diagnostics/logger headers | public-candidate |
| `vdr-suite-http-client` | `HttpRequest`, `HttpResponse`, `IHttpClient`, basic client header | public-candidate |
| `vdr-suite-recording-core` | recording domain/repository/service headers | public-candidate after persistence audit |
| `vdr-suite-metadata-core` | metadata repository/service headers | split decision required |
| `vdr-suite-vdr-domain` | VDR domain value object headers | public-candidate |
| `vdr-suite-vdr-adapter-api` | adapter interface/capability headers | public-candidate |
| `vdr-suite-vdr-service` | read/overview/query service headers | public-candidate after dependency audit |
| `vdr-suite-epg-core` | EPG query/search headers | public-candidate |
| `vdr-suite-searchtimer-readonly` | preview/read-only/dry-run headers | public-candidate |
| `vdr-suite-snapshot-read` | snapshot read headers | public-candidate after cache ownership audit |
| `vdr-suite-rest-api-contracts` | docs/schema/examples, not C++ controller headers | public contract candidate |
| `vdr-suite-daemon` | daemon app/runtime/config headers | daemon-only |
| `vdr-suite-test-support-http` | mock/test server headers | test-support |
| `vdr-suite-test-support-vdr` | mock/fake/test transport headers | test-support |

---

## Immediate Header Split Targets

1. Create a generated or documented public-header manifest per package candidate.
2. Keep `core/http/include` split between client, listener and test-support headers.
3. Split `core/vdr/include` by domain, adapter API, read services, mutation internals, polling/snapshot internals and test support.
4. Split recording action headers into validation-only candidates, mutation internals and REST parser headers.
5. Keep `api/rest/include` out of public C++ library packages unless a specific reusable non-controller helper is promoted.
6. Keep `core/daemon/include` daemon-only.

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents header inventory only.

---

## Back

- [Back to Development Index](index.md)
