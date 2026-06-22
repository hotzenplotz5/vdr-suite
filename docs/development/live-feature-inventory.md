# Live / EPGSearch Feature Inventory

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

Phase 48.0 starts the Live goldstandard analysis.

This document inventories Live/EPGSearch capabilities before implementing new VDR-Suite features.

The goal is to avoid guessing and derive the next roadmap from actual Live and EPGSearch service capabilities.

---

## Source Finding

Live integrates EPGSearch service capabilities that go beyond basic EPG search.

Relevant EPGSearch service areas:

- Search
- Search results
- SearchTimer list
- SearchTimer add
- SearchTimer modify
- SearchTimer delete
- Query SearchTimer
- Query ad-hoc search
- Extended EPG category list
- Channel group list
- Blacklist list
- Directory list
- Timer conflict list
- Conflict-check advice
- Short directory list
- Expression evaluation against an event

---

## Capability Inventory

| Area | Live / EPGSearch capability | VDR-Suite status | Gap |
| --- | --- | --- | --- |
| Basic EPG search | query, mode, channel, title/subtitle/description fields | partially present | needs Live-level matrix |
| Search results | result list from EPGSearch | partially present through EPG query | format and semantics gap |
| SearchTimer list | service-level SearchTimerList | present | mostly covered |
| SearchTimer create | AddSearchTimer | present | real VDR covered |
| SearchTimer update | ModSearchTimer | present | real VDR covered |
| SearchTimer delete | DelSearchTimer | present | real VDR covered |
| SearchTimer query preview | QuerySearchTimer | partially present | needs exact compatibility check |
| Ad-hoc query search | QuerySearch | partially present | needs dedicated capability |
| Extended EPG categories | ExtEPGInfoList | missing or not first-class | high-value gap |
| Channel groups | ChanGrpList | missing or not first-class | medium-value gap |
| Blacklists | BlackList | write fields exist; list capability unclear | gap |
| Directories | DirectoryList / ShortDirectoryList | partially present via recordings/actions | needs dedicated capability |
| Timer conflicts | TimerConflictList | missing | high-value gap |
| Conflict-check advice | IsConflictCheckAdvised | missing | high-value gap |
| Expression evaluation | Evaluate(expr,event) | missing | advanced gap |

---

## Strategic Finding

After Phase 47, VDR-Suite is strong in:

- SearchTimer CRUD
- Timer lifecycle
- recordings read/actions foundation
- read-only real VDR regression
- unified real VDR regression

The remaining Live-level value is concentrated in:

1. EPGSearch result semantics
2. extended EPG metadata/categories
3. timer conflict visibility
4. SearchTimer/query previews
5. directories/channel groups/blacklists as first-class capabilities
6. expression/evaluation helpers

---

## Recommended Phase 48 Roadmap

### Phase 48.1 - EPGSearch capability matrix

Document exact mapping:

- EPGSearch service
- RESTfulAPI endpoint
- VDR-Suite domain/service/controller status
- real VDR test status
- backend-neutrality

### Phase 48.2 - Timer conflict domain foundation

Create backend-neutral conflict domain before adapter-specific implementation.

### Phase 48.3 - Extended EPG category capability

Expose extended EPG metadata as a capability and query primitive.

### Phase 48.4 - SearchTimer query preview parity

Align QuerySearchTimer / QuerySearch semantics with VDR-Suite preview behavior.

---

## Decision

Do not immediately implement new Live features.

First complete a capability matrix for EPGSearch-derived features, because those capabilities define the next high-value gaps.

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)

