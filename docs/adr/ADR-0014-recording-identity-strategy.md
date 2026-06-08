# ADR-0014: Recording Identity Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Accepted

## Context

VDR-Suite now has snapshot-backed read APIs for status, channels, timers, events and recordings.

The next architectural step is selective refresh and change-feed support. Recordings are especially sensitive because they are not safely identified by one technical field alone.

External VDR backends such as RESTfulAPI expose useful recording data, including:

```text
number
file_name
relative_file_name
inode
channel_id
event_start_time
event_duration
event_title
duration
filesize_mb
hash
```

However, these values have different meanings and stability properties:

```text
number
        Current backend list position. It can change after reloads, sorting, deletion or rescans.

file_name / relative_file_name
        Current backend address. It can change after move or rename operations.

inode
        Local filesystem identity hint. It can change after copy, restore, re-encode, migration or cross-device moves.

hash
        Backend-provided change fingerprint. It can indicate metadata or content changes, but it is not a permanent recording identity.

channel_id / event_start_time / event_duration / event_title
        Useful semantic origin hints. They can identify the likely original broadcast, but may be incomplete, wrong or duplicated.
```

VDR-Suite must support future scenarios such as:

- multiple VDR backends
- multiple houses or locations
- remote VDR-Suite federation
- recording move and rename operations
- recording deletion and restore
- recording repair and re-encode workflows
- imports from VDR-Rectools or external sources
- partial snapshot refresh
- change feeds for frontends

Using only one backend field as the durable recording identity would create long-term architecture debt.

## Decision

VDR-Suite will treat recording identity as a composed architecture concept.

A recording must not be identified durably by backend list number alone.

A recording must not be identified durably by file path alone.

A recording must not be identified durably by inode alone.

A recording must not be identified durably by backend hash alone.

Instead, VDR-Suite distinguishes three identity-related concepts:

```text
RecordingAddress
RecordingStableFingerprint
RecordingChangeFingerprint
```

## RecordingAddress

`RecordingAddress` describes where the backend currently exposes the recording.

Typical inputs:

```text
backendId
file_name
relative_file_name
```

The address is allowed to change.

Examples:

```text
backendId=house-a
file_name=/srv/vdr/video/Movie/Foo/2026-06-01.20.15.1-0.rec
```

A changed address does not necessarily mean the recording is new.

It may mean:

- recording was moved
- recording was renamed
- recording was copied
- backend changed its video directory
- path normalization changed

## RecordingStableFingerprint

`RecordingStableFingerprint` describes the best available semantic identity of a recording within a backend.

Typical inputs:

```text
backendId
channel_id
event_start_time
event_duration
normalized_event_title
normalized_recording_name
duration
```

The stable fingerprint is used for best-effort matching across address changes.

It is intentionally not treated as globally perfect.

It can have collisions, especially for repeated broadcasts, manual recordings, incomplete EPG data or imported recordings.

VDR-Suite must therefore use it as a matching hint, not as the only database key for destructive operations.

## RecordingChangeFingerprint

`RecordingChangeFingerprint` describes whether the representation of a recording changed.

Typical inputs:

```text
backend hash
filesize_mb
duration
mtime if available in a future backend
metadata version if available in a future backend
```

For RESTfulAPI, the `hash` field is a useful backend-provided change fingerprint when sync mode is active.

A changed `RecordingChangeFingerprint` means VDR-Suite should refresh or reprocess the recording metadata.

It does not mean the recording is a different logical recording.

## Backend Scope

All identity concepts are scoped by backend identity.

Two different backends may expose recordings with the same title, event start time or file name.

Therefore, VDR-Suite must include `backendId` when creating any recording address, stable fingerprint or change fingerprint.

This follows the backend identity strategy and avoids single-VDR assumptions.

## Matching Rules

Future snapshot reconciliation should follow these rules:

```text
same backendId + same address
        Treat as the same backend-visible recording address.

same backendId + changed address + similar stable fingerprint
        Treat as likely move or rename candidate.

same backendId + same address + changed change fingerprint
        Treat as changed recording metadata or content.

same backendId + missing old address + no stable match
        Treat as removed or unavailable.

new address + no stable match
        Treat as new recording candidate.
```

Destructive operations must remain address-based and permission-checked at the time of execution.

A stable fingerprint alone must not be used to delete, move or modify a recording.

## RESTfulAPI Implications

RESTfulAPI already exposes useful recording identity inputs.

VDR-Suite should use available RESTfulAPI fields instead of requiring a new RESTfulAPI recording ID immediately.

Relevant RESTfulAPI fields include:

```text
file_name
relative_file_name
inode
channel_id
event_start_time
event_duration
event_title
duration
filesize_mb
hash
```

The `hash` field is useful as a change fingerprint when available, but it must not be treated as durable identity.

No RESTfulAPI change is required by this ADR.

## Snapshot and Change Feed Implications

Snapshot reconciliation should store both address and fingerprint data.

A future snapshot change feed can then report changes such as:

```text
recording.added
recording.removed
recording.changed
recording.moved_or_renamed
```

The change feed should describe what VDR-Suite inferred, while keeping the raw backend identity inputs available for diagnostics.

## Consequences

Benefits:

- avoids path-only identity
- avoids backend-list-number identity
- supports move and rename detection
- supports multiple VDR backends
- supports future federation
- supports selective recording refresh
- uses existing RESTfulAPI fields without requiring immediate upstream changes
- keeps destructive operations safer by separating identity hints from executable addresses

Trade-offs:

- matching logic is more complex
- stable fingerprint collisions are possible
- imported or manually created recordings may have incomplete EPG identity data
- reconciliation must be tested carefully
- future database models may need to store multiple identity-related fields

## Non-Goals

This ADR does not implement:

- database schema changes
- `RecordingIdentity` value object
- snapshot reconciliation code
- recording move detection implementation
- REST API response changes
- RESTfulAPI changes
- destructive recording operations
- duplicate-resolution UI

This ADR records the architecture rule only.

## Follow-Up Work

Future phases may introduce:

- `RecordingAddress` value object
- `RecordingStableFingerprint` value object
- `RecordingChangeFingerprint` value object
- recording reconciliation tests
- snapshot change feed recording events
- REST API fields for recording identity diagnostics
- persistence schema for backend-scoped recording identity
- safe move/rename inference rules
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
