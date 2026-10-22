# ADR-0068 — Public-v1 and legacy source-contract reconciliation

Status: **ADR-0068 source closeout accepted — PR #517 merged 2026-10-22, four CI tracks SUCCESS at PR head and merge head.** This is a partial audit of 60 declared OpenAPI method/path entries, not exhaustive route, schema or runtime coverage.
Single implementation branch: `work/adr0068-router-modularization-20261022`. Single PR: [#517](https://github.com/hotzenplotz5/vdr-suite/pull/517).

## Provenance and boundaries

- Source of the table: the current **partial audited** `api/openapi.json` at branch commit `21d6ac560d31118663b3babdace69f5dc7c02dc5`. Exactly **60 method/path entries** were read; this is **not a declaration that OpenAPI covers every registered route**.
- The source owners below refer to dispatcher/registration boundaries, not new domain service, identity or mutation authorities. The exact auth text and documented 2xx/header requirements are taken from OpenAPI; actual negative status/CSRF/ETag/cursor behavior remains asserted by existing HTTP/security suites and `SecurityHttpGate`.
- Source blocks were moved without editing their original POST bodies. The **first-match** POST handler order is NativeTimer → DevicePairing → AccountCredentialSession → DeviceAdministration → AccountAdministration → TimerWorkflow → read-only method fallback. `tools/check_public_v1_openapi.py` asserts both dispatched helper order and route marker presence inside the helper spans.
- The **sole PublicApiRuntime DELETE dispatcher** still handles `/api/v1/native-timers/{timerId}`, including the original exact-backend, actor, strong If-Match, idempotency and durable-admission requirements. No additional DELETE behavior was introduced.
- `PublicIdentityAdmissionLifecycle.cpp` moves Account/Device pairing, lifecycle, grant, credential, session and native Timer mutation-admission **callback wiring only**; callback slots, locks, repository ownership, daemon registration and reset semantics remain unchanged.
- The Recording R1/R2/R3 mutation HTTP boundary remains in `ApiRouterPublicRecordingActions.cpp`; recording identity and action executors are **not moved** by this checkpoint.
- Device media-session HTTP composition is owned separately by `RecordingMediaHttpRuntime.cpp`; the public route inventory must not falsely infer those handlers are implemented inside `PublicApiRuntime`.

## Audited Public-v1 method and response matrix

This preserves the *existing OpenAPI-declared* successful HTTP codes, mandatory request headers and authentication expectations. Full error/Problem Details schemas, optional headers, request bodies and additional status codes stay in [the source specification](../../api/openapi.json). `—` means not declared mandatory in that field, not that other checks are absent.

| Method | Path | Registration owner | Declared 2xx | Required request headers | Declared authorization boundary |
| --- | --- | --- | --- | --- | --- |
| GET | `/api/v1` | PublicApiRuntime GET/read | 200 | — | Anonymous discovery |
| GET | `/api/v1/accounts` | PublicApiRuntime GET/read | 200 | — | accounts.view@* |
| POST | `/api/v1/accounts` | PublicApiRuntime POST domain | 201 | Idempotency-Key | accounts.create@* |
| GET | `/api/v1/accounts/{accountId}` | PublicApiRuntime GET/read | 200 | — | accounts.view@* |
| POST | `/api/v1/accounts/{accountId}` | PublicApiRuntime POST domain | 200 | If-Match | accounts.modify@* OR accounts.activate@* OR accounts.deactivate@* depending on exactly one requested field |
| GET | `/api/v1/accounts/{accountId}/credentials` | PublicApiRuntime GET/read | 200 | — | accounts.credentials.view@*; Suite-global Human Account administration |
| GET | `/api/v1/accounts/{accountId}/credentials/{credentialId}` | PublicApiRuntime GET/read | 200 | — | accounts.credentials.view@*; Suite-global Human Account administration |
| POST | `/api/v1/accounts/{accountId}/credentials/{credentialId}` | PublicApiRuntime POST domain | 200 | If-Match | accounts.credentials.revoke@*; Suite-global Human Account administration; protected mutation with browser-session CSRF where relevant |
| GET | `/api/v1/accounts/{accountId}/grants` | PublicApiRuntime GET/read | 200 | — | accounts.grants.view@* |
| POST | `/api/v1/accounts/{accountId}/grants` | PublicApiRuntime POST domain | 200 | If-Match | accounts.grants.modify@* |
| GET | `/api/v1/accounts/{accountId}/sessions` | PublicApiRuntime GET/read | 200 | — | accounts.sessions.view@*; Suite-global Human Account administration |
| GET | `/api/v1/accounts/{accountId}/sessions/{sessionId}` | PublicApiRuntime GET/read | 200 | — | accounts.sessions.view@*; Suite-global Human Account administration |
| POST | `/api/v1/accounts/{accountId}/sessions/{sessionId}` | PublicApiRuntime POST domain | 200 | If-Match | accounts.sessions.revoke@*; Suite-global Human Account administration; protected mutation with browser-session CSRF where relevant |
| GET | `/api/v1/backends` | PublicApiRuntime GET/read | 200 | — | Positive permission grants determine visible backend scopes; no backends.view permission exists. |
| GET | `/api/v1/capabilities` | PublicApiRuntime GET/read | 200 | — | Anonymous discovery |
| GET | `/api/v1/channels` | PublicApiRuntime GET/read | 200 | — | channels.view for every explicitly requested backendId |
| GET | `/api/v1/device-pairings` | PublicApiRuntime GET/read | 200 | — | device.pairing.view@* |
| POST | `/api/v1/device-pairings` | PublicApiRuntime POST domain | 201 | — | Anonymous bootstrap only; no Actor grants; no durable Device authority acquired. |
| GET | `/api/v1/device-pairings/{pairingRequestId}` | PublicApiRuntime GET/read | 200 | — | EITHER short-lived matching pairing token for bootstrap status OR authenticated global device.pairing.view@* admin read; never both implied by one credential. |
| POST | `/api/v1/device-pairings/{pairingRequestId}` | PublicApiRuntime POST domain | 200 | If-Match | device.pairing.decide@* plus protected browser CSRF where applicable |
| POST | `/api/v1/device-pairings/{pairingRequestId}/credential` | PublicApiRuntime POST domain | 201 | X-VDR-Suite-Pairing-Token | Short-lived request-specific pairing token AND approved, unexpired, unconsumed pairing state. |
| GET | `/api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle` | PublicApiRuntime GET/read | 200 | — | devices.lifecycle.view@* |
| POST | `/api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle` | PublicApiRuntime POST domain | 200 | If-Match | devices.credentials.revoke@* |
| POST | `/api/v1/devices/{deviceId}/credentials/{credentialId}/rotate` | PublicApiRuntime POST domain | 200 | If-Match | admin Browser Session, devices.credentials.rotate@*, protected mutation CSRF; Device authorization alone is explicitly forbidden. |
| GET | `/api/v1/devices/{deviceId}/grants` | PublicApiRuntime GET/read | 200 | — | devices.grants.view@* |
| POST | `/api/v1/devices/{deviceId}/grants` | PublicApiRuntime POST domain | 200 | If-Match | devices.grants.modify@* with protected mutation CSRF where relevant |
| GET | `/api/v1/devices/{deviceId}/lifecycle` | PublicApiRuntime GET/read | 200 | — | devices.lifecycle.view@* |
| POST | `/api/v1/devices/{deviceId}/lifecycle` | PublicApiRuntime POST domain | 200 | If-Match | devices.revoke@* |
| GET | `/api/v1/epg/now-next` | PublicApiRuntime GET/read | 200 | — | epg.view@backendId; exactly one authorized backend; permission checked in SecurityHttpGate and runtime scope |
| POST | `/api/v1/epg/timer-intents` | PublicApiRuntime POST domain | 202 | If-Match, Idempotency-Key | Verified paired Device credential with exact timers.create@backendId grant; Browser sessions are not allowed; independent backend write-policy check |
| GET | `/api/v1/epg/window` | PublicApiRuntime GET/read | 200 | — | epg.view@backendId OR media.live.play@backendId; exact backend scope |
| GET | `/api/v1/genres` | PublicApiRuntime GET/read | 200 | — | recordings.view@backendId; exactly one authorized backend; permission checked in SecurityHttpGate and runtime scope |
| GET | `/api/v1/genres/recordings` | PublicApiRuntime GET/read | 200 | — | recordings.view@backendId; exactly one authorized backend; permission checked in SecurityHttpGate and runtime scope |
| POST | `/api/v1/live-playback-sessions` | RecordingMediaHttpRuntime | 201 | — | Verified Device credential, current owner and media.live.play@backendId; browser credentials and media.recording.play alone denied. |
| GET | `/api/v1/live-playback-sessions/{sessionId}` | RecordingMediaHttpRuntime | 200 | — | Verified Device credential, current owner and media.live.play@backendId; browser credentials and media.recording.play alone denied. |
| POST | `/api/v1/live-playback-sessions/{sessionId}/stop` | RecordingMediaHttpRuntime | 200 | — | Verified Device credential, current owner and media.live.play@backendId; browser credentials and media.recording.play alone denied. |
| GET | `/api/v1/native-timers` | PublicApiRuntime GET/read | 200 | — | timers.view@backendId; exact backend scope |
| GET | `/api/v1/native-timers/{timerId}` | PublicApiRuntime GET/read | 200 | — | timers.view@backendId; exact backend scope |
| DELETE | `/api/v1/native-timers/{timerId}` | PublicApiRuntime DELETE (NativeTimer) | 202 | If-Match, Idempotency-Key | Device credential required, exact timers.delete@backendId grant and audited protected mutation |
| POST | `/api/v1/native-timers/{timerId}/modifications` | PublicApiRuntime POST domain | 202 | If-Match, Idempotency-Key | Device credential and exact timers.modify@backendId; audited mutation |
| GET | `/api/v1/operations/{operationId}` | PublicApiRuntime GET/read | 200 | — | authenticated actor must own the durable Operation |
| POST | `/api/v1/recording-playback-sessions` | RecordingMediaHttpRuntime | 201 | — | Verified VDR-Suite Device credential; active, owned Device session; media.recording.play@backendId for create/mutations (read rechecks current Device credential, session ownership and backend playback grant in the owner registry). Browser session, Basic authentication, or recordings.view alone do not authorize this API. |
| GET | `/api/v1/recording-playback-sessions/{sessionId}` | RecordingMediaHttpRuntime | 200 | — | Verified VDR-Suite Device credential; active, owned Device session; media.recording.play@backendId for create/mutations (read rechecks current Device credential, session ownership and backend playback grant in the owner registry). Browser session, Basic authentication, or recordings.view alone do not authorize this API. |
| POST | `/api/v1/recording-playback-sessions/{sessionId}/stop` | RecordingMediaHttpRuntime | 200 | — | Verified VDR-Suite Device credential; active, owned Device session; media.recording.play@backendId for create/mutations (read rechecks current Device credential, session ownership and backend playback grant in the owner registry). Browser session, Basic authentication, or recordings.view alone do not authorize this API. |
| POST | `/api/v1/recording-playback-sessions/{sessionId}/subtitles` | RecordingMediaHttpRuntime | 200 | — | Verified VDR-Suite Device credential; active, owned Device session; media.recording.play@backendId for create/mutations (read rechecks current Device credential, session ownership and backend playback grant in the owner registry). Browser session, Basic authentication, or recordings.view alone do not authorize this API. |
| GET | `/api/v1/recording-playback-sessions/{sessionId}/tracks` | RecordingMediaHttpRuntime | 200 | — | Verified VDR-Suite Device credential; active, owned Device session; media.recording.play@backendId for create/mutations (read rechecks current Device credential, session ownership and backend playback grant in the owner registry). Browser session, Basic authentication, or recordings.view alone do not authorize this API. |
| GET | `/api/v1/recording-playback-sessions/{sessionId}/trickplay` | RecordingMediaHttpRuntime | 200 | — | Verified VDR-Suite Device credential; active, owned Device session; media.recording.play@backendId for create/mutations (read rechecks current Device credential, session ownership and backend playback grant in the owner registry). Browser session, Basic authentication, or recordings.view alone do not authorize this API. |
| GET | `/api/v1/recording-playback-sessions/capabilities` | RecordingMediaHttpRuntime | 200 | — | Verified VDR-Suite Device credential and active Device identity only; media.recording.play is checked separately on Session create, and no grant is assigned by this read. |
| GET | `/api/v1/recordings` | PublicApiRuntime GET/read | 200 | — | recordings.view@backendId; exactly one authorized backend; permission checked in SecurityHttpGate and runtime scope |
| POST | `/api/v1/recordings/actions/execute` | ApiRouterPublicRecordingActions | 200 | — | exact recordings.rename / recordings.move / recordings.delete@backendId grant and browser CSRF; legacy/private routes unaffected |
| POST | `/api/v1/recordings/actions/validate` | ApiRouterPublicRecordingActions | 200 | — | backend-scoped recordings.rename / recordings.move / recordings.delete as selected by action; browser CSRF required; no device/legacy impersonation |
| GET | `/api/v1/search-timers` | PublicApiRuntime GET/read | 200 | — | verified Device and searchtimers.view@backend |
| POST | `/api/v1/search-timers` | PublicApiRuntime POST domain | 202 | Idempotency-Key | verified Device and exact searchtimers.create@backend; no Browser/legacy fallback |
| GET | `/api/v1/search-timers/{searchTimerId}/preview` | PublicApiRuntime GET/read | 200 | — | verified Device and searchtimers.view@backend plus epg.view@backend |
| GET | `/api/v1/search-timers/draft-preview` | PublicApiRuntime GET/read | 200 | — | verified Device, searchtimers.view@backend and epg.view@backend |
| GET | `/api/v1/search/epg` | PublicApiRuntime GET/read | 200 | — | Requires authenticated Actor/Device credential and epg.view@backendId; exactly one authorized backend; verified by SecurityHttpGate and PublicApiRuntime, no browser-private search fallback. |
| GET | `/api/v1/search/recordings` | PublicApiRuntime GET/read | 200 | — | Requires authenticated Actor/Device credential and recordings.view@backendId; exactly one authorized backend; verified by SecurityHttpGate and PublicApiRuntime, no browser-private search fallback. |
| GET | `/api/v1/timer-assignments` | PublicApiRuntime GET/read | 200 | — | timers.view@backend |
| GET | `/api/v1/timer-assignments/{timerAssignmentId}` | PublicApiRuntime GET/read | 200 | — | timers.view@backend |
| POST | `/api/v1/timer-assignments/{timerAssignmentId}` | PublicApiRuntime POST domain | 202 | If-Match, Idempotency-Key | timers.create@backend plus protected-mutation security (including browser-session CSRF when applicable) |

## Legacy and first-party API isolation

`ApiRouter::handlePost` in `api/rest/src/ApiRouter.cpp` is **unchanged** by this branch. Preserve distinct contracts, controller selection and fallback behavior; do **not** re-route them through the Public-v1 mutation delegates.

| Legacy/first-party POST family | Existing owner / behavior boundary |
| --- | --- |
| `/api/recordings/actions/*`, `/api/vdr/recordings/actions/*` | Legacy Recording validation/execution/preview controllers |
| `/api/vdr/timers/actions/create`, `/update`, `/delete` | VDR Timer action controller/adapter |
| `/api/vdr/channels/move`, `/api/vdr/channels/actions/move` | VDR Channel Move controller |
| `/api/epg/cache/refresh` | EPG cache refresh controller |
| `/api/searchtimers/preview/cache/refresh`, `/api/vdr/searchtimers/preview/cache/refresh` | SearchTimer cache refresh controller |
| `/api/searchtimers/{real-test,execute,plan,validate,update,delete}` and `/api/vdr/searchtimers/{...}` | Existing SearchTimer controller and command executor |
| `/api/searchtimers`, `/api/vdr/searchtimers` | Legacy SearchTimer create controller |
| `/api/epgsearch/native-fuzzy/refresh`, `/api/vdr/epgsearch/native-fuzzy/refresh` | Existing EPGSearch native-fuzzy refresh controller |
| `/api/epgsearch/native-fuzzy/stale-probes/delete` and VDR alias | Existing stale-probe administration controller |

The legacy `ApiRouter::handleGet` paths are likewise unchanged. Browser-only `/api/...` routes are not implicitly stable independent-client `/api/v1` contracts; see [Phase-69F client matrix](phase-69f-client-contract-matrix.md).

## Behavioral parity and release gates

1. **Security** — preserve `SecurityHttpGate`, actor/backend grant scopes, Device vs Human admin vs pairing-token distinctions, owner checks, CSRF and revocation. No new authorization path.
2. **Writes** — retain exact ETag/If-Match, Idempotency-Key, durable operation admission, stale revisions, status/Location, error/problem and audit behavior. Do not exercise productive recordings, timers or service restart for source refactor acceptance.
3. **Reads** — retain If-None-Match/304, cursors and authorization scope, JSON/Problem Details, stable sorting, cache state and provider lookup side effects. EPG/channel/backend/timer/recording read owners remain as previously separated.
4. **Build** — `REST_PUBLIC_API_SRC` includes the callback source for both daemon and standalone tests; every moved method is defined once. Architecture and OpenAPI source guards must follow the **new file** rather than accept weaker generic text searches.
5. **Release** — run the complete final-head CI graph at PR #517, verify the precise branch and changing `main` heads, merge only with no outstanding required failures, then verify actual post-merge `main` and CI. E1 starts only after this checkpoint.

This matrix is a **source/API contract review aid**, not a substitute for golden HTTP responses, negative security tests, CI, or hardware deployment evidence.

## Post-merge provenance and audit limits (2026-10-22)

[PR #517](https://github.com/hotzenplotz5/vdr-suite/pull/517), final head [`867f43b67`](https://github.com/hotzenplotz5/vdr-suite/commit/867f43b67e88f6b9c904db5b98e99ce1b79ff784), merge [`6c05493ad`](https://github.com/hotzenplotz5/vdr-suite/commit/6c05493ad53c82e19fef4a2bc70e09bc1603e0c8), exact CI links in [CURRENT](../CURRENT.md#adr-0068-closeout--merged-and-source-ci-accepted-2026-10-22). The pre-merge release-gate list above is retained as historical review evidence; its CI and merge steps have passed. This method/path inventory is **not** an assertion that OpenAPI exhaustively enumerates all runtime routes, error/JSON schemas or their execution. No new live yaVDR installation or hardware acceptance is claimed.
