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

This document defines the Phase 47.15 validation workflow for real RESTfulAPI and epgsearch SearchTimer payloads.

The goal is to validate backend payload shape before expanding the VDR-Suite SearchTimer domain model.

---

## Capture Command

Run this command on the yaVDR system where RESTfulAPI exposes epgsearch SearchTimers:

    python3 tools/capture_searchtimer_payload.py --url http://127.0.0.1:8002/searchtimers.json

The script stores the captured payload here:

    docs/development/validation/searchtimer-real-payload.json

---

## Current Expected Minimal Fields

The current mapper requires:

    id
    search
    use_as_searchtimer

The current mapper ignores all additional backend fields.

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

No SearchTimer domain expansion should happen before a real payload is captured and reviewed.

Phase 47.16 may expand the domain only for fields proven by real payloads.

---

## Verification

This document is valid when these checks pass:

    make test-docs
    make test-phase
