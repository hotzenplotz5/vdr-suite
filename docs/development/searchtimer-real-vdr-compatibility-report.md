# SearchTimer Real VDR Compatibility Report

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)
- [SearchTimer Completeness Audit](searchtimer-completeness-audit.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)

---

## Purpose

This report documents the first successful real VDR SearchTimer end-to-end validation.

The test was executed against a real VDR RESTfulAPI instance with epgsearch support enabled.

---

## Validated Flow

The real smoke helper validated this full path:

    VDR-Suite -> RESTfulAPI SearchTimer Command Executor -> RESTfulAPI -> epgsearch -> real VDR

Validated operations:

- GET /searchtimers.json connectivity.
- POST /searchtimers create.
- GET /searchtimers.json readback after create.
- PUT /searchtimers/<id> update.
- GET /searchtimers.json readback after update.
- DELETE /searchtimers/<id> cleanup.
- GET /searchtimers.json readback after delete.

---

## Real RESTfulAPI Behavior Found

### Create Response

The real RESTfulAPI create operation successfully creates a SearchTimer.

However, the response does not reliably return an Id text body.

The real smoke helper resolves the created SearchTimer id by reading back /searchtimers.json and matching the unique test search string.

### Update Response

The real RESTfulAPI update operation successfully updates a SearchTimer.

However, the response does not reliably return an Id text body.

The production executor preserves the requested backend-native id when the update response does not include an id.

### Boolean Readback

Some boolean-like fields may be serialized by RESTfulAPI as either numeric or JSON boolean values.

The smoke helper accepts both forms for real readback checks.

---

## Validated Fields

| Field | Create | Readback | Update |
| --- | --- | --- | --- |
| search | yes | yes | yes |
| directory | yes | yes | yes |
| priority | yes | yes | yes |
| lifetime | yes | yes | not changed in update check |
| margin_start | yes | yes | not changed in update check |
| margin_stop | yes | yes | not changed in update check |
| use_vps | yes | yes | not changed in update check |
| use_time | yes | yes | not changed in update check |
| start_time | yes | accepted by real VDR | yes |
| stop_time | yes | accepted by real VDR | yes |
| use_duration | yes | yes | yes |
| duration_min | yes | accepted by real VDR | yes |
| duration_max | yes | accepted by real VDR | yes |
| use_dayofweek | yes | accepted by real VDR | yes |
| dayofweek | yes | accepted by real VDR | yes |
| avoid_repeats | yes | yes | yes |
| allowed_repeats | yes | yes | yes |
| repeats_within_days | yes | accepted by real VDR | yes |
| compare_title | yes | accepted by real VDR | yes |
| compare_subtitle | yes | accepted by real VDR | yes |
| compare_summary | yes | accepted by real VDR | yes |
| compare_categories | yes | accepted by real VDR | yes |
| compare_time | yes | accepted by real VDR | yes |

---

## Invalid Value Found During Testing

The initial smoke-test payload used dayofweek = 62.

Real RESTfulAPI / epgsearch rejected this with HTTP 406.

The Live-derived epgsearch logic accepts only the range -127..6 for dayofweek.

The smoke helper now uses valid values:

- create: dayofweek = 1
- update: dayofweek = 2

---

## Safety Behavior

The smoke helper is intentionally safe by default.

The Makefile target only builds the helper and prints --help.

Real VDR modification requires explicit execution with --run.

Example:

    VDR_SUITE_RESTFULAPI_HOST=127.0.0.1 VDR_SUITE_RESTFULAPI_PORT=8002 /tmp/vdr_suite_searchtimer_real_smoke --run

The helper attempts cleanup by deleting the created SearchTimer.

---

## Result

The real VDR smoke helper reached:

    Result: SUCCESS

This confirms that SearchTimer create, readback, update and delete now work end-to-end against a real VDR with RESTfulAPI and epgsearch.

---

## Consequences

Further field enrichment should be validated with the real smoke helper or an extended compatibility helper.

The next implementation phases should prefer real VDR behavioral validation over mock-only assumptions.

---

## Next Recommended Phase

Phase 47.58 - SearchTimer blacklist enrichment


## Back

- [Back to Development Index](index.md)
- [Back to SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)
- [Back to SearchTimer Completeness Audit](searchtimer-completeness-audit.md)
