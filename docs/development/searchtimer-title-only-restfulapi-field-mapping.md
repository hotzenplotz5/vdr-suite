# SearchTimer Title-Only RESTfulAPI Field Mapping

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Automation Safety Review](searchtimer-automation-safety-review.md)

---

## Purpose

Phase 53.0 fixes RESTfulAPI SearchTimer field mapping for title-only searches.

A request such as query Amerika with compareTitle=true, compareSubtitle=false and compareSummary=false must be sent to RESTfulAPI as title-only search.

---

## Fixed Mapping

Before this phase, the RESTfulAPI request body always emitted:

- use_title=true
- use_subtitle=true
- use_description=true

After this phase, the request body maps the public request fields:

- use_title follows compareTitle
- use_subtitle follows compareSubtitle
- use_description follows compareSummary

---

## Example

For a title-only request:

- query: Amerika
- compareTitle: true
- compareSubtitle: false
- compareSummary: false

the RESTfulAPI body now contains:

- search=Amerika
- use_title=1
- use_subtitle=0
- use_description=0

---

## Safety Boundary

This phase changes only field mapping.

It does not add:

- automation scheduling
- automatic execution
- new REST endpoints
- backend write enablement switches
- production mutation policy changes

---

## Verification

Targeted verification:

- make test-restful-api-search-timer-command-executor
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 53.1 - SearchTimer title-only request parser contract

The next phase should strengthen request parser tests so external JSON bodies preserve title/subtitle/summary flags before reaching the executor.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
