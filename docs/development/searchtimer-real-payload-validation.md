# SearchTimer Real Payload Validation

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)
- [Completed Phases](completed-phases.md)

## Back

- [Development Index](index.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)

---

## Purpose

This document records the Phase 47.15 validation workflow for real RESTfulAPI and epgsearch SearchTimer payloads.

The original gate was to validate backend payload shape before expanding the VDR-Suite SearchTimer domain model.

That gate has passed. The document remains as the historical payload-capture reference for later Phase 50 user workflow work.

---

## Capture Command

Run this command on the yaVDR system where RESTfulAPI exposes epgsearch SearchTimers:

    python3 tools/capture_searchtimer_payload.py --url http://127.0.0.1:8002/searchtimers.json

The script stores the captured payload here:

    docs/development/validation/searchtimer-real-payload.json

---

## Original Expected Minimal Fields

The original mapper required:

    id
    search
    use_as_searchtimer

The original mapper ignored all additional backend fields until real payload evidence justified domain expansion.

---

## Validation Questions

The captured payload should answer:

- Does the backend return a top-level searchtimers array?
- Are id, search and use_as_searchtimer always present?
- Which additional fields are present on real epgsearch SearchTimers?
- Are additional fields stable enough for domain expansion?
- Which fields are optional, empty or backend-version dependent?

---

## Domain Expansion Gate

This gate is complete.

Real payload validation enabled later SearchTimer domain expansion, real VDR compatibility checks and native fuzzy backend capability validation.

Current follow-up direction is Phase 50.0 - SearchTimer user workflow foundation.

---

## Verification

This document is valid when these checks pass:

    make test-docs
    make test-phase
