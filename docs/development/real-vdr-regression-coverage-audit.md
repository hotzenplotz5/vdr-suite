# Real VDR Regression Coverage Audit

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [SearchTimer Completeness Re-Audit](searchtimer-completeness-reaudit.md)

---

## Purpose

This document audits which VDR-facing functionality has real VDR regression coverage.

The goal is to stop adding new SearchTimer or VDR features until the older non-SearchTimer VDR functionality is also validated against a real VDR RESTfulAPI instance.

---

## Current Real VDR Helper Inventory

The repository already has these real or semi-real smoke helpers:

| Helper target | Scope | Safety class | Current role |
| --- | --- | --- | --- |
| restfulapi-connectivity-smoke | one configurable RESTfulAPI path | read-only | basic connectivity check |
| searchtimer-real-vdr-smoke-helper | SearchTimer create/update/delete/readback | write, self-cleaning | full SearchTimer payload validation |
| restfulapi-real-delete-smoke-helper | Recording delete action | destructive unless restricted to test recording | safety-critical helper |
| restfulapi-real-move-smoke-helper | Recording move action | modifies recording paths unless restricted to test recording | safety-critical helper |

---

## Coverage Status Before This Audit

| Area | Existing unit/API tests | Existing real VDR coverage | Gap |
| --- | --- | --- | --- |
| RESTfulAPI connectivity | partial | yes, one path | needs multi-endpoint read-only helper |
| VDR status | yes | no dedicated real VDR regression | missing |
| Channels | yes | no dedicated real VDR regression | missing |
| Events / EPG | yes | no dedicated real VDR regression | missing |
| Recordings read | yes | no dedicated real VDR regression | missing |
| Timers read | yes | no dedicated real VDR regression | missing |
| Timer actions | yes | no safe real VDR lifecycle helper yet | missing |
| Recording actions | yes | delete/move helpers exist | needs safety audit before routine use |
| SearchTimer | yes | yes, full payload helper | covered |

---

## Required Regression Strategy

The next real VDR regression layer must be read-only first.

It should test these endpoints without modifying the VDR:

- /api/info.json or equivalent RESTfulAPI info endpoint
- /status.json
- /channels.json
- /events.json
- /recordings.json
- /timers.json

The helper should report:

- HTTP status
- body size
- whether a JSON-like body was returned
- optional count fields if present
- PASS/FAIL per endpoint
- final SUCCESS/FAILURE summary

This helper must not create, update, delete, move or modify anything.

---

## Safety Rules For Future Real VDR Tests

### Read-only tests

Allowed by default:

- connectivity
- status
- channels
- events
- recordings list
- timers list
- capabilities

### Write tests

Allowed only with explicit safeguards:

- unique marker
- self-cleaning cleanup path
- id fallback handling
- cleanup on exception
- final grep/readback to ensure no test object remains

### Destructive recording tests

Allowed only with explicit test fixtures:

- never operate on arbitrary real recordings
- require a marker directory or test recording name
- refuse to run without a strong safety flag
- print all target paths before action
- verify expected target before and after
- keep delete tests separate from move tests

---

## Historical Recommendation

Phase 47.67 originally recommended adding a real VDR read-only regression helper.

That recommendation is complete.

Current state:

- The read-only real VDR regression helper exists.
- The SearchTimer real VDR smoke path exists.
- The Timer lifecycle real VDR smoke path exists.
- Phase 47.71 added the unified `make real-vdr-regression` command.
- Phase 50.0 can build on this validation foundation for SearchTimer user workflow work.

---

## Back

- [Back to Development Index](index.md)
- [Back to SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [Back to SearchTimer Completeness Re-Audit](searchtimer-completeness-reaudit.md)

## Phase 47.71 Unified Real VDR Regression Command

Phase 47.71 added a unified `make real-vdr-regression` command.

Included helpers:

- real VDR read-only regression
- SearchTimer real VDR smoke test
- Timer lifecycle real VDR smoke test

Excluded by design:

- recording move smoke
- recording delete smoke

Recording actions are excluded from the unified command until a safe, reproducible test-recording fixture exists. Their helpers remain available manually and protected by marker and execution gates.

Required environment:

- VDR_SUITE_RESTFULAPI_HOST, optional, default 127.0.0.1
- VDR_SUITE_RESTFULAPI_PORT, optional, default 8002
- VDR_SUITE_TIMER_CHANNEL, required, real VDR channel id for timer lifecycle validation
