# Phase 62 Slice 2M — Explicit Safe POST Classification

## Status

Slice 2M is complete on Draft PR #117.

Repository implementation, CI, guarded yaVDR installation and real-runtime
acceptance passed on 2026-08-01 at:

```text
43b516c7e2adb96bfde415abc8c665f77a541643
```

---

## Purpose

HTTP method alone does not determine whether a request mutates state.

Slice 2M explicitly classifies the existing validation, preview and planning
POST routes that execute no backend write. This allows browser sessions to use
them without weakening the fail-closed boundary for every other POST.

---

## Exact Safe Routes

Recording validation and preview:

```text
POST /api/recordings/actions/validate
POST /api/vdr/recordings/actions/validate
POST /api/recordings/actions/preview
POST /api/vdr/recordings/actions/preview
```

SearchTimer validation and planning:

```text
POST /api/searchtimers/validate
POST /api/vdr/searchtimers/validate
POST /api/searchtimers/plan
POST /api/vdr/searchtimers/plan
```

Query strings are accepted because classification uses the exact normalized
request path.

Trailing-slash variants are not aliases and remain fail-closed.

---

## Security Contract

Each exact safe route requires an authenticated request context.

Accepted authentication mechanisms remain:

- Legacy Basic compatibility identity;
- configured Managed Basic identity;
- valid browser session with normal lifecycle and precedence rules.

A browser session does not need a CSRF token for these routes because they do
not mutate state.

A safe POST:

- does not set `protectedMutation`;
- does not require a mutation permission;
- does not consume Admin or Read-only role expansion;
- does not append an authorization decision event merely for successful safe
  dispatch;
- does not bypass authentication or browser-grant persistence availability.

This is an exact allowlist, not a general rule that POST is safe.

---

## Routes That Remain Fail-Closed

The classification deliberately excludes:

```text
POST /api/searchtimers/execute
POST /api/vdr/searchtimers/execute
POST /api/searchtimers/real-test
POST /api/vdr/searchtimers/real-test
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
POST /api/epg/cache/refresh
POST /api/epgsearch/native-fuzzy/refresh
POST /api/vdr/epgsearch/native-fuzzy/refresh
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
```

Execution, cache refresh and administration have state, operator or backend
write implications and require separate permissions and CSRF contracts.

---

## Focused Tests

The shared browser-gate fixture avoids repeating identity, SQLite and browser
credential setup:

```text
core/security/tests/SecurityHttpGateBrowserTestFixture.h
```

The focused runtime-independent gate test is:

```text
core/security/tests/test_safe_post_security.cpp
```

The architecture guard is:

```text
tools/check_safe_post_security.py
```

Together they prove:

- all eight exact aliases;
- Legacy Basic allowance;
- browser-session allowance without CSRF or grants;
- non-mutation classification;
- no successful-dispatch authorization evidence;
- query-string normalization;
- trailing-slash denial;
- continued fail-closed treatment of execute, real-test, refresh and
  administration routes;
- frontend mutation wrappers still do not inject CSRF into safe POSTs.

---

## Real yaVDR Acceptance

Slice 2M ran in the same guarded installation window as Slice 2L:

```text
Repository head:
43b516c7e2adb96bfde415abc8c665f77a541643

GitHub Actions:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30703009976

Verified backup:
/var/backups/vdr-suite-phase62-slice2lm-20260801T141451Z-43b516c7e2ad

Installed/running daemon SHA-256:
02a701c3bc8279808b3303c23d19080dc7ccc9c522d2ca28c90b996618456a03

Installed deferred loader SHA-256:
c32999adc0aca8ee815ceebde8982ad50f4c206d93d4864ac949146dc8190bd7
```

The safe-POST profile passed:

```text
tests_passed=27
tests_failed=0
runtime_http_requests=26
safe_routes=8
```

The full combined 2L/2M batch passed:

```text
runtime_acceptance_total_tests=85
runtime_acceptance_total_http_requests=80
```

The checksum-protected evidence is stored at:

```text
/var/backups/vdr-suite-phase62-slice2lm-20260801T141451Z-43b516c7e2ad/runtime-acceptance-batch
```

The pass verified unauthenticated denial, Legacy Basic and browser-session
allowance, query-string variants, trailing-slash fail-closed behavior and
continued denial of excluded execute, real-test and refresh routes.

Resource state, daemon PID, installed fingerprints, SQLite integrity and backup
checksums remained unchanged. Acceptance browser sessions were revoked and
revoked-cookie replay was denied.

```text
real_recording_mutations=0
real_searchtimer_updates=0
real_searchtimer_deletes=0
```

---

## Completion

Repository implementation, focused tests, full CI, guarded installation,
real-runtime acceptance, cleanup and durable evidence all passed.

PR #117 remains open, Draft and unmerged.
