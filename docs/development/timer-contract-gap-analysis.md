# Timer Contract Gap Analysis

## Navigation

- [Development Index](index.md)
- [Current Architecture State](current-architecture-state.md)
- [ADR-0026: External Orchestration Layer Above VDR](../adr/ADR-0026-external-orchestration-layer-above-vdr.md)
- [ADR-0027: VDR-First Implementation With Future Media Federation](../adr/ADR-0027-vdr-first-implementation-with-future-media-federation.md)

---

## Purpose

This document records the timer contract gap between VDR-Core, RESTfulAPI, LIVE and the current VDR-Suite timer read domain.

The goal is to avoid building timer create, update and delete operations on an incomplete or inaccurate read contract.

---

## Current VDR-Suite Timer Domain

After Phase 44.1, `VdrTimer` contains:

- `id`
- `channelId`
- `channelName`
- `eventId`
- `title`
- `subtitle`
- `aux`
- `startTime`
- `endTime`
- `priority`
- `lifetime`
- `enabled`
- `recording`
- `pending`

This is sufficient for basic timer listing and overview use cases.

It is not yet sufficient for full timer operation support.

---

## Source Reference Summary

### VDR-Core

VDR `cTimer` exposes timer state and behavior around:

- `Id()`
- `Flags()`
- `Channel()`
- `Day()`
- `WeekDays()`
- `Start()`
- `Stop()`
- `Priority()`
- `Lifetime()`
- `File()`
- `Aux()`
- `Remote()`
- `Event()`
- `Recording()`
- `Pending()`
- `Skip()`
- `SetEventFromSchedule()`

Relevant flags include:

- `tfActive`
- `tfInstant`
- `tfVps`
- `tfRecording`
- `tfSpawned`
- `tfAvoid`

VDR remains the source of truth for timer semantics.

### RESTfulAPI

RESTfulAPI timer listing exposes fields such as:

- `id`
- `index`
- `flags`
- `start`
- `stop`
- `priority`
- `lifetime`
- `event_id`
- `weekdays`
- `day`
- `channel`
- `filename`
- `channel_name`
- `is_pending`
- `is_recording`
- `is_active`
- `aux`

RESTfulAPI create/update builds a VDR timer line:

- `flags:channel:weekdays@day:start:stop:priority:lifetime:file:aux`

RESTfulAPI delete is safety-relevant because deleting a recording timer calls `Skip()` and processes record controls before deletion.

### LIVE

LIVE is the practical VDR timer workflow reference.

LIVE timer editing uses:

- active state
- channel
- title
- remote server name
- date/day
- weekdays
- start/stop
- priority/lifetime
- directory/title split using `~`
- VPS monitoring
- EPGSearch event-id or event-time monitoring
- TVScraper and EPGSearch data in `aux`
- permission checks before editing timers

LIVE also disables event-id and event-time monitoring choices for repeating weekday timers.

### EPGSearch / SearchTimers

RESTfulAPI exposes SearchTimer endpoints separately from basic timers.

SearchTimers are not part of the basic VDR timer read domain and should not be mixed into the first timer operation model.

They should be treated as a later EPGSearch-specific integration block.

---

## Field Gap Table

| Concept | VDR-Core | RESTfulAPI | LIVE usage | VDR-Suite Phase 44.1 | Decision |
| --- | --- | --- | --- | --- | --- |
| Timer identity | `Id()` | `id`, `index` | `timerid` | `id` | Keep. |
| Backend identity | external to VDR timer | REST adapter context | remote/local context | backend layer | Keep outside `VdrTimer` for now. |
| Channel identity | `Channel()->GetChannelID()` | `channel` | channel widget by id | `channelId` | Keep. |
| Channel name | `Channel()->Name()` | `channel_name` | UI display | `channelName` | Keep. |
| Event identity | `Event()->EventID()` | `event_id` | EPG monitoring | `eventId` | Keep. |
| File/title | `File()` | `filename` | title plus optional directory | `title` | Keep display field, later split operation request. |
| Aux data | `Aux()` | `aux` | EPGSearch/TVScraper XML | `aux`, `subtitle` fallback | Keep raw `aux`; do not interpret yet. |
| Start | `Start()` / `StartTime()` | `start` | edit field | `startTime` | Rename later or document HHMM vs timestamp before write support. |
| Stop | `Stop()` / `StopTime()` | `stop` | edit field | `endTime` | Rename later or document HHMM vs timestamp before write support. |
| Priority | `Priority()` | `priority` | edit field | `priority` | Keep. |
| Lifetime | `Lifetime()` | `lifetime` | edit field | `lifetime` | Keep. |
| Active state | `Flags() & tfActive` | `is_active`, `flags` | active radio | `enabled` | Keep; retain raw flags later. |
| Recording state | `Recording()` | `is_recording` | safety-relevant | `recording` | Keep. |
| Pending state | `Pending()` | `is_pending` | state display | `pending` | Keep. |
| Raw flags | `Flags()` | `flags` | VPS and active logic | missing | Add before operations. |
| VPS | `tfVps` | via `flags` | monitoring option | missing | Derive later from flags. |
| Instant timer | `tfInstant` | via `flags` | not first-class in current UI | missing | Later read-only metadata. |
| Spawned timer | `tfSpawned` | via `flags` | EPGSearch-related behavior | missing | Later read-only metadata. |
| Avoid timer | `tfAvoid` | via `flags` | pattern/search behavior | missing | Later read-only metadata. |
| Day/date | `Day()` | `day` | date field | missing | Add before create/update. |
| Weekdays | `WeekDays()` | `weekdays` | repeat timer workflow | missing | Add before create/update. |
| Remote server | `Remote()` | not consistently exposed | `remoteServerName` | missing | Later multi-server field, do not block local timers. |
| Directory/title split | `File()` convention | `filename` | split at `~` | not modeled | Add to operation request, not necessarily read domain. |
| Monitoring mode | aux + flags | aux + flags | none/vps/event_id/event_time | missing | Later EPGSearch phase. |
| SearchTimer | EPGSearch plugin | `/searchtimers` | EPGSearch UI | missing | Separate phase, not basic timer CRUD. |

---

## Decisions For Next Phases

### Phase 44.2 Outcome

The read contract is now good enough for timer overview, but not good enough for timer operations.

Before create, update or delete execution, VDR-Suite needs an explicit timer operation request model.

### Required Before Timer Write Support

Add or model:

- raw `flags`
- `day`
- `weekdays`
- HHMM start/stop semantics
- directory/title write mapping
- active/VPS flag behavior
- delete safety for recording timers
- read-only backend policy
- capability checks for timer write support

### Not Required Yet

Do not implement in the first timer CRUD block:

- SearchTimers
- EPGSearch monitoring modes
- TVScraper aux interpretation
- pattern timers
- spawned timers
- generic IPTV timer abstraction

These belong to later EPGSearch, metadata or media federation phases.

---

## Recommended Next Phase

Recommended next implementation phase:

```text
Phase 44.3 - Timer Operation Request Model
```

Scope:

- introduce a VDR-specific timer operation request structure
- model fields required by RESTfulAPI create/update
- keep request construction separate from `VdrTimer` read domain
- avoid execution and HTTP transport in this phase

---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
