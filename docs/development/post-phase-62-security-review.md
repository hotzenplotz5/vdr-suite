# Post-Phase-62 Security Review

## Status

Phase 62 remains completed. This document records the security impact of later runtime changes without rewriting the historical Phase-62 acceptance fingerprint.

The bounded route-derived series-artwork settings hardening is implemented in Draft PR #133. It closes an accountability-scope precision gap without reopening Phase 62, changing Legacy Basic compatibility or starting Phase 63.

## Historical Phase-62 acceptance

The final Phase-62 runtime acceptance remains the durable completion evidence for the accepted candidate:

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

PR #117 was merged into `main` as merge commit `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`. Phase 62 is not active work and is not reopened by later feature development.

## Relevant post-Phase-62 changes

The following completed work landed after the Phase-62 closeout:

- PR #118: TVScraper genre classification and refresh fixes, merge commit `06f727ed2f8d604b5dadd75e997c666a5f6a8dfe`;
- PR #123: EPG artwork resolution under public base paths, merge commit `d2bdceedde62467cb45037ea49584ac10a07ceb2`;
- PR #132: guarded external series-artwork fallback, TMDB/TVmaze providers, secure per-backend settings and TVScraper identity/artwork ordering, merge commit `441e5febf7d3ab0121a585ce1176a8e5a7c67ce0`;
- direct frontend correction `96b9737895cb737e7cf75be10c462b0e7ffa74a8` with regression test `2d04a963054e9925f6b8cb12392b188a89e11f07`.

Draft PR #133 is a later bounded hardening change. It does not alter provider behavior, stored settings semantics or the historical Phase-62 runtime fingerprint.

## Security assessment of the settings mutation

The `POST /api/backends/<backend>/settings/series-artwork` contract is integrated into the Phase-62 security model:

- it is classified as a protected mutation;
- it requires `backend.settings.series-artwork.modify` for the selected backend;
- the authorization and accountability backend is derived from the authoritative route rather than trusted from the JSON body;
- fixed Admin grants include the permission and fixed Read-only grants deny it;
- browser-session callers require the server-owned CSRF token;
- allow and deny decisions use append-only pre-dispatch accountability;
- authorized responses receive `operation.succeeded` or `operation.failed` outcome evidence using the route-derived backend scope;
- malformed nested or percent-encoded backend route segments fail closed instead of falling back to a body-provided scope;
- unknown POST routes continue to fail closed outside Legacy Basic compatibility.

The request passed to the API handler remains the original request. The handler therefore retains its independent route/body backend mismatch rejection and cannot mutate a backend selected only by the body.

The frontend sends same-origin credentials and the browser-session CSRF header. The TMDB token is not returned by the settings GET response.

Managed TMDB tokens are stored beneath the dedicated secret root. The implementation requires a private absolute directory, opens path components without following symlinks, writes a temporary `0600` file, synchronizes it and atomically renames it. Tokens, cookies, Authorization headers and request/response bodies are not copied into accountability events.

The external artwork delivery path validates managed roots, refuses symlink traversal and validates supported image content before public delivery.

## Focused validation

The dedicated `test-security-series-artwork-route-scope` target is part of `test-security` and therefore part of the repository CI test graph. It covers:

- Admin authorization using the route backend;
- fixed Read-only denial;
- denial when a caller has permission only for the body backend but the route selects another backend;
- missing and invalid browser CSRF denial with route-scoped accountability;
- route/body mismatch authorization and audit scope while preserving the handler mismatch check;
- query-string handling;
- fail-closed handling for malformed backend route segments;
- route-scoped `operation.succeeded` and `operation.failed` evidence.

The existing security architecture checker, full security test graph, daemon build, packaging, frontend and documentation checks remain required on the final Draft-PR head.

## Result

No authentication bypass, authorization bypass, CSRF bypass, Read-only-role bypass, cross-backend write path or API/audit token disclosure is known in the post-Phase-62 work.

The earlier precision issue is closed by deriving the authorization and accountability scope from the route before dispatch. A route/body mismatch may still produce a controlled handler failure, but both the pre-dispatch decision and the post-dispatch outcome remain attributed to the route-selected backend.

The historical Phase-62 acceptance remains valid as the completion evidence for its accepted candidate. It must not be presented as a byte-for-byte runtime acceptance of later daemon builds because PR #132 and Draft PR #133 changed daemon routing, authorization classification or protected-mutation handling.

This bounded hardening does not retire Legacy Basic, does not change TVScraper and does not start or reopen a numbered runtime phase.

## Related documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](current-status.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Completed Phases](completed-phases.md)
