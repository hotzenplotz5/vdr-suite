# SearchTimer yaVDR Smoke-Test Execution Report

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)

---

## Purpose

This report records the first successful yaVDR-side real SearchTimer smoke test after the SearchTimer workflow safety chain and non-mutating VDR-Suite real-test mode were introduced.

The report separates two surfaces:

1. Direct VDR RESTfulAPI on port 8002.
2. VDR-Suite API real-test endpoint `/api/searchtimers/real-test`.

---

## Environment

Observed local services:

- `127.0.0.1:8002` is the VDR RESTfulAPI endpoint.
- `127.0.0.1:8080` is a PHP/Rectools dashboard endpoint, not VDR-Suite.
- No VDR-Suite API process was observed during this smoke run.

This means direct RESTfulAPI validation is available, while the VDR-Suite real-test endpoint still requires a running VDR-Suite API harness or daemon process.

---

## Direct RESTfulAPI Connectivity

Command:

    curl -sS -i --max-time 5 http://127.0.0.1:8002/searchtimers.json

Observed result:

    HTTP/1.1 200 OK
    {"searchtimers":[],"count":0,"total":0}

Conclusion:

- RESTfulAPI is reachable.
- The SearchTimer endpoint is available.
- The initial SearchTimer list was empty.

---

## Direct RESTfulAPI Real Lifecycle Smoke Test

Command:

    make searchtimer-real-vdr-smoke-helper
    VDR_SUITE_RESTFULAPI_HOST=127.0.0.1 \
    VDR_SUITE_RESTFULAPI_PORT=8002 \
    /tmp/vdr_suite_searchtimer_real_smoke --run

Observed result:

    PASS CONNECT /searchtimers.json - status=200
    PASS CREATE - searchtimer created through RESTfulAPI
    PASS READBACK after create - status=200
    PASS UPDATE - searchtimer updated through RESTfulAPI
    PASS READBACK after update - status=200
    PASS DELETE cleanup - searchtimer deleted through RESTfulAPI
    PASS READBACK after delete - status=200
    Result: SUCCESS

Additional field checks passed for create and update, including directory, priority, lifetime, margins, VPS/time/duration flags, repeat handling, series recording, blacklist mode, match mode, extended EPG info, favorites, pause-on-recordings, switch-minutes-before and delete-after-recordings fields.

Conclusion:

- The existing direct RESTfulAPI SearchTimer command executor can create a real SearchTimer.
- It can read back the created SearchTimer.
- It can update the real SearchTimer.
- It can read back the updated SearchTimer.
- It can delete the real SearchTimer.
- The cleanup readback confirms removal.

---

## Cleanup Verification

Command:

    curl -sS http://127.0.0.1:8002/searchtimers.json

Observed result:

    {"searchtimers":[],"count":0,"total":0}

Conclusion:

- The smoke test left no SearchTimer behind.
- Cleanup succeeded.
- The real VDR/RESTfulAPI state returned to the empty baseline.

---

## VDR-Suite Real-Test Endpoint Observation

Command shape:

    python3 tools/run_searchtimer_yavdr_real_test.py \
      --run \
      --base-url http://127.0.0.1:8080 \
      --backend home-vdr \
      --print-json

Observed result:

    httpStatus=401
    WWW-Authenticate: Basic realm="VDR-Rectools Dashboard"

Conclusion:

- Port 8080 is not the VDR-Suite API in this yaVDR environment.
- It is protected by the VDR-Rectools dashboard Basic Auth layer.
- The VDR-Suite real-test endpoint was not reached through this port.
- This is an environment routing/runtime issue, not a SearchTimer workflow failure.

---

## Current Safety State

The VDR-Suite SearchTimer workflow remains non-mutating through its real-test endpoint:

- production policy gate remains closed
- expected final dispatch stage is `production-policy-gate-closed`
- expected `executorInvocationAttempted` is `false`
- operator warnings remain visible
- audit trail remains visible

Direct RESTfulAPI mutation is proven to work through the dedicated real smoke helper, but VDR-Suite production mutation remains intentionally blocked.

---

## Follow-Up

The next implementation step should provide a local VDR-Suite API smoke harness or daemon/runtime surface that can actually serve:

    POST /api/searchtimers/real-test

against the local ApiRouter without accidentally hitting the Rectools/PHP endpoint on port 8080.

This will allow the Phase 50.36 smoke script to test the VDR-Suite safety endpoint itself, not only the direct RESTfulAPI backend.

---

## Back

- [Back to SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [Back to Development Index](index.md)
- [Back to README](../../README.md)
