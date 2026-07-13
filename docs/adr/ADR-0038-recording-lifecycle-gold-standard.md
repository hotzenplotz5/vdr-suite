# ADR-0038: Recording Lifecycle Gold Standard

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Recording Action Transport Mapping](ADR-0024-recording-action-transport-mapping.md)

---

## Status

Accepted

## Context

VDR-Suite exposes Recording actions across local and remote VDR backends. The existing action foundation already supports validation, dry-run execution, backend permissions, explicit confirmation, backend-native Recording identities, RESTfulAPI transport mapping and post-action cache readback.

The initial sharp Delete path used RESTfulAPI's existing Recording delete endpoint. That endpoint performs VDR-compatible soft deletion by renaming an active `.rec` Recording to `.del` and moving the object from `Recordings` to `DeletedRecordings`.

A direct soft-delete mutation alone is not sufficient as the final VDR-Suite contract.

The current Live implementation establishes a higher minimum safety baseline. Before deleting an active Recording it can:

- stop conflicting RecordingHandler operations,
- stop replay of the same Recording,
- deactivate an active local Timer,
- handle active remote Timers,
- preserve the VDR lock order,
- move the Recording into `DeletedRecordings`,
- restore a deleted Recording through `cRecording::Undelete()`,
- permanently remove a deleted Recording only through a separate purge operation.

EPGSearch also observes Recording start and stop events. It keeps Timer, SearchTimer, Event and Recording filename state and may classify an interrupted Recording as incomplete. Therefore deleting a currently active SearchTimer Recording without controlled Timer handling can affect the EPGSearch done list, avoid-repeats behavior and SearchTimer follow-up processing.

VDR-Suite is intended to become the safest and most complete VDR management layer, not merely a frontend over the lowest-level backend mutation.

## Decision

VDR-Suite adopts a Recording lifecycle model with three distinct domain operations:

```text
TRASH
RESTORE
PURGE
```

Their meanings are:

```text
TRASH
  active .rec Recording -> VDR DeletedRecordings / .del

RESTORE
  deleted .del Recording -> active Recordings / .rec

PURGE
  irreversible physical removal of a deleted Recording
```

`DELETE` may remain as a legacy transport term, but the VDR-Suite product and domain language uses `TRASH` for reversible VDR soft deletion.

### Minimum parity rule

Sharp `TRASH` execution may not be considered complete unless it meets at least the current Live and EPGSearch safety standard.

The minimum native backend behavior is:

1. resolve the exact backend-owned Recording identity,
2. inspect active RecordingHandler usage,
3. inspect replay of the same Recording,
4. inspect an active local Recording Timer,
5. inspect an active remote Recording Timer,
6. inspect SearchTimer/EPGSearch origin and consequences when available,
7. require explicit policy approval for every disruptive consequence,
8. revalidate the state immediately before mutation,
9. deactivate or stop approved active dependencies,
10. execute VDR-native soft delete,
11. update `Recordings` and `DeletedRecordings` under the correct lock order,
12. trigger Recording and Timer invalidation,
13. confirm the resulting state through backend readback.

A backend that cannot prove this minimum behavior must expose a reduced capability and must not be presented as full safe-trash support.

### Gold-standard preflight

VDR-Suite must expose a structured preflight result before a disruptive Recording mutation.

The result must distinguish at least:

- Recording state,
- backend and access mode,
- backend-native identity,
- active replay,
- RecordingHandler usage,
- active local Timer,
- active remote Timer,
- SearchTimer/EPGSearch origin when known,
- required disruptive actions,
- warnings,
- blocking errors,
- a backend state revision or equivalent expectation token.

A dry run that only reports that backend execution was skipped is not sufficient as the final preflight contract.

### Explicit consequence approval

The user-facing confirmation and execution request must explicitly authorize disruptive consequences such as:

- stopping replay,
- deactivating a local Timer,
- modifying a remote Timer,
- interrupting a SearchTimer Recording.

VDR-Suite must not silently broaden the requested operation.

### Optimistic concurrency

The state observed during preflight must be compared with the state immediately before execution.

If the relevant Recording, Timer, replay or backend revision changed, execution must fail with a conflict result and require a new preflight and confirmation.

This prevents a Recording that starts after validation from being trashed under stale assumptions.

### Native atomicity boundary

Backend-native VDR coordination belongs inside the VDR-side adapter or plugin because it owns the VDR locks and direct access to:

- `Recordings`,
- `DeletedRecordings`,
- `Timers`,
- `cRecordControls`,
- replay control,
- RecordingHandler operations,
- remote Timer handling.

VDR-Suite must not emulate one atomic Trash operation through an unsafe sequence of independent Timer and Recording HTTP mutations.

VDR-Suite remains responsible for:

- permissions,
- multi-site policy,
- preflight presentation,
- explicit confirmation,
- expected-state tokens,
- audit information,
- cache synchronization,
- user-visible result handling.

### Deleted Recording identity

VDR-Suite distinguishes:

```text
recordingId
  stable VDR-Suite identity

backendNativeId
  current backend-owned .rec or .del identity
```

A transient Recording list index is never an accepted mutation identity.

The initial RESTfulAPI Restore transport may use the exact `.del` path, but the public VDR-Suite model must retain enough stable identity information to correlate an active Recording with its deleted and restored forms.

### Restore contract

Restore is a separate validated operation.

Before Restore, the backend must verify at least:

- the `.del` Recording still exists,
- it is present in `DeletedRecordings`,
- the target `.rec` path does not conflict,
- the backend is writable,
- the Recording has not already been purged,
- the expected state has not changed.

After Restore, VDR-Suite must verify that the Recording reappears in the active Recording model and disappears from the deleted model.

### Purge contract

Purge is not part of the initial Recording Trash completion.

Purge must have:

- a separate backend capability,
- a separate permission,
- a stronger confirmation path,
- explicit irreversible wording,
- a dedicated native operation using VDR's physical removal semantics,
- independent tests.

Normal Recording write access does not automatically imply Purge permission.

### Capability model

Backends should distinguish capabilities equivalent to:

```text
recordings.trash
recordings.trash.preflight
recordings.restore
recordings.purge
recordings.active-timer-safety
recordings.remote-timer-safety
recordings.epgsearch-awareness
```

The exact serialized names may be finalized in the capability-contract implementation slice.

## Consequences

- The existing VDR-Suite Trash UI remains useful as a workflow foundation, but its current low-level RESTfulAPI Delete transport is not the final Gold Standard contract.
- The RESTfulAPI integration needs a native preflight and safe atomic Trash execution path that reaches at least current Live behavior.
- Deleted Recording listing and Restore remain separate, reviewable functionality.
- Purge stays deliberately deferred.
- Active local, remote and SearchTimer Recording tests become mandatory before declaring the Recording lifecycle complete.
- Backends with weaker support remain usable, but their reduced safety capability must be visible and enforced.

## Required validation matrix

The Recording lifecycle is not complete until real-system validation covers at least:

1. inactive normal Recording -> Trash -> Restore,
2. Recording currently being replayed,
3. active local Timer Recording,
4. active remote Timer Recording where available,
5. active EPGSearch/SearchTimer Recording,
6. state change between preflight and execution,
7. read-only backend rejection,
8. Restore target conflict,
9. deleted Recording already purged by VDR,
10. daemon cache and frontend readback after Trash and Restore,
11. SSE/change-state invalidation for Recording and Timer consequences.

## Follow-up

1. define the native RESTfulAPI preflight and safe Trash contract,
2. compare the implementation directly with current VDR, Live and EPGSearch sources,
3. rebase the tested DeletedRecordings list/Restore patch onto current upstream,
4. add VDR-Suite backend adapter contracts and capability reporting,
5. run the required real-system validation matrix,
6. add the VDR-Suite deleted Recordings browser and Restore workflow,
7. design Purge only as a later independent phase.

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
