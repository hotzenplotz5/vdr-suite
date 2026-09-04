# Post-Phase-66 Native Recording Editing

## Status

**Authorized bounded post-Phase-66 workstream. Source and architecture audit complete enough to begin the first implementation vertical. Runtime acceptance is not yet claimed.**

This document owns the implementation scope and evidence boundary for native VDR recording marks and cutting. It does not create a new numbered phase or slice.

Starting point:

```text
main: 11f8b162160fa129cf71b628d270a4fa377e4f33
merged predecessor: PR #267 - Post-Phase-66: complete Series metadata artwork
working branch: work/post-phase66-native-recording-editing
```

## Roadmap classification

The strict roadmap assigns the next numbered runtime phase to:

```text
Phase 67 - Broadcast Companion Services: Teletext and HbbTV
```

Native recording marks/cutting therefore does **not** start Phase 67 and must not be assigned an invented phase or slice number.

The present work is a bounded post-Phase-66 Recording workflow/correctness capability that composes already accepted foundations:

- VDR-native Recording identity and ownership;
- RecordingAction safety/capability/permission contracts;
- Phase-63 Agent/provider fencing and protected-write semantics;
- Phase-65 Recording playback/timeline ownership;
- Recordings 2 detail/frontend composition.

No Teletext, HbbTV, Legacy OSD, public-API-hardening or recommendation work is authorized by this document.

## Production VDR target and source evidence

The production reference remains VDR `2.7.9`, API version `11`. Existing accepted VDR-Suite runtime evidence already records real yaVDR execution on this exact VDR version.

The upstream VDR `2.7.9` release archive is the required source baseline for native compilation/runtime acceptance. The audited implementation areas are:

```text
recording.h / recording.c
  cMark
  cMarks
  cRecording::HasMarks()
  cRecording::DeleteMarks()
  cRecording::IsEdited()
  cRecording::IsInUse()
  cIndexFile

menu.c
  cReplayControl mark handling
  cMenuRecordingEdit

cutter.h / cutter.c
  cCutter
  cRecordingsHandler
  RecordingsHandler

status.h
  cStatus::MsgMarksModified(...)
```

Current public source mirrors may display a newer VDR source revision. Such a mirror is useful for source navigation only and is not allowed to silently replace `2.7.9` as production truth. Before the native implementation is accepted, the exact installed VDR `2.7.9` headers/source and resulting plugin build on the real yaVDR host must confirm the used APIs and semantics.

## Native marks authority

VDR is the canonical owner of recording marks. VDR-Suite must not create a second marks database.

For current TS recordings VDR stores marks in:

```text
<recording-directory>/marks
```

Legacy PES recordings use:

```text
<recording-directory>/marks.vdr
```

The relevant implementation is `cMarks`/`cMark`.

### Position model

The native semantic position is a **recording frame index**.

`cMark::ToText()` serializes the frame position through VDR's `IndexToHMSF(...)`; `cMark::Parse()` accepts the VDR textual mark representation through `HMSFToIndex(...)`.

The ordinary textual representation is therefore recording-time based, conceptually:

```text
hh:mm:ss.ff [optional comment]
```

but the in-process authority is the frame index, not an arbitrary browser second value and not a private Suite time base.

For public/client presentation VDR-Suite may expose derived time values for usability, but it must retain the exact native frame position and return the post-normalization VDR position as authority.

### Index and I-frame normalization

For TS recordings VDR uses the native `index` file. `cMarks::Align()` constructs a `cIndexFile` for the recording and aligns every mark to `GetClosestIFrame(...)` before sorting.

Consequences:

- VDR-Suite must not pretend that every requested decimal second is a valid mark;
- a UI request derived from the current playback position is a requested position, not the final authoritative mark position;
- SuiteBridge must let VDR convert/align to the native Recording FPS/index semantics;
- the response/readback must expose the exact normalized native frame/timecode;
- marks remain ordered by VDR after alignment.

### Loading, external changes and saving

`cMarks::Load()` binds a marks instance to the Recording and FPS and loads the native file. `cMarks::Update()` observes marks-file modification time, reloads changed content, aligns it and sorts it. `cMarks::Save()` persists the canonical file and updates the stored modification time.

The normal VDR replay controller also calls `marks.Update()` when no local unsaved mark modification owns the state. This is the key interoperability property:

```text
VDR OSD writes marks
  -> native marks file
  -> VDR-Suite next read sees the same marks

VDR-Suite writes through native VDR marks owner
  -> native marks file
  -> normal VDR replay sees the same marks
```

No synchronization database or file-copy shadow is required.

## Native OSD mark operations

The ordinary VDR replay controller establishes the user-facing native semantics that VDR-Suite should preserve.

### Add/delete at current position

VDR obtains the current replay index, then:

- removes the exact mark when one exists at that position; or
- adds a new native mark otherwise;
- marks the marks set modified;
- publishes `cStatus::MsgMarksModified(&marks)`.

### Move

VDR permits mark movement while paused and only for an existing mark at the current position. Movement is frame based and is constrained so the mark cannot cross its adjacent marks. Replay movement uses the native index path rather than inventing a second timeline.

VDR-Suite does not need to reproduce OSD key semantics literally, but native mutations must preserve the same ordering and normalized-position invariants.

### Delete all

VDR provides `cMarks::DeleteMarksFile(...)` / `cRecording::DeleteMarks()` for removing native marks. A Suite implementation must use VDR-owned semantics and must not unlink an arbitrary caller-supplied path from HTTP code.

## Native cutting architecture

VDR-Suite must use VDR's Recording handler, not implement its own cutter and not call `rm`/`mv`/`cp` based editing.

The native start path is:

```text
RecordingsHandler.Add(ruCut, recording->FileName())
```

`cRecordingsHandler` owns the queue/serialization and creates `cCutter` internally.

This is preferable to constructing `cCutter` directly because the handler owns:

- pending versus active edit state;
- collision prevention for source and destination;
- creation of the edited destination name;
- serialized cutting work;
- result Recording registration;
- error/cancel cleanup of incomplete output;
- usage reporting through `GetUsage(...)`;
- aggregate active state through `Active()`;
- cancellation through `Del(...)`.

### Output and original Recording

VDR creates a **new edited Recording**. The native edited-name convention is observable through `cRecording::IsEdited()`, which identifies the edited Recording by the `%` prefix on the recording-name component.

The original Recording remains the original. VDR-Suite must not automatically delete or replace it after a successful cut.

### In-use safety

`cRecording::IsInUse()` includes at least:

```text
ruTimer  - the Recording is currently being recorded
ruReplay - the Recording is currently replayed
RecordingsHandler.GetUsage(...) - native cut/move/copy activity
```

The first VDR-Suite implementation therefore fails closed for marks mutation/cut start when the Recording is in use. No exception for a growing/current Recording is introduced in this workstream without separate source-backed design and acceptance.

### Cut prerequisites

Before native cut dispatch the owner must re-read native state and reject at least:

- Recording not found under the expected backend-native identity;
- stale Backend/Agent/provider generation or identity;
- Recording currently in use;
- no native marks;
- marks that do not form a usable cutting sequence according to VDR;
- another native Recording handler operation using the same source/destination;
- missing/incompatible native capability;
- read-only Backend or missing permission;
- stale expected marks revision/fingerprint;
- an existing edited destination when VDR requires an explicit overwrite decision.

The implementation must not infer success merely from local transport success.

## Existing VDR-Suite capability and gap

VDR-Suite already models:

```text
RecordingActionType::Cut
RecordingWorkflowService::createCutJob(...)
recording.action.cut
recording.permission.cut
```

The LIVE-reference capability set includes Cut.

However the production RESTfulAPI RecordingAction executor supports only:

```text
Move
Rename
Delete
```

and its tests intentionally prove `Cut` unsupported.

This means the gap is not the existence of a generic `CUT` enum. The missing capability is the **native VDR marks/cutter owner path plus truthful readback and frontend integration**.

RESTfulAPI must not be extended with invented filesystem-side cutting merely to make the enum executable.

## Ownership decision

The smallest architecture-consistent owner topology is:

```text
Recordings 2 / first-party playback owner
  -> authenticated VDR-Suite Recording-edit API
  -> Control Plane authorization + backend/native Recording identity
  -> protected operation / idempotency / expected marks revision
  -> current Backend Agent + explicitly owned SuiteBridge provider
  -> typed native Recording-edit command/read contract
  -> SuiteBridge
  -> cRecording / cMarks / RecordingsHandler
  -> authoritative native readback
```

Rules:

- browser code never opens or writes `marks`;
- HTTP handlers never spawn a cutter or shell command;
- RESTfulAPI filesystem mutation is not used as a substitute for native VDR ownership;
- SuiteBridge owns only the bounded VDR-process-local operation;
- Agent owns local provider/generation/dispatch fencing, not Recording domain policy;
- Control Plane owns authorization, idempotency, operation outcome and reconciliation;
- VDR remains final authority for marks and cutter state.

The Phase-63/64 protected-write model is reused rather than bypassed:

```text
operation / intent
  -> authorization and backend/provider eligibility
  -> idempotency / expected native marks revision
  -> resource ownership and backend-generation fences
  -> durable starting-before-dispatch boundary
  -> bounded typed native execution
  -> authoritative readback
  -> verified success / verified no-effect / outcome unknown
  -> reconciliation before unsafe retry
```

A future command cannot be authorized merely because another SuiteBridge command exists. Recording editing requires its own typed capability and schema.

## Native Recording identity

Existing Recording mutation evidence establishes:

```text
backendId + backendNativeId
```

as the routing identity for VDR Recording mutations.

For the current VDR backend `backendNativeId` is the native absolute VDR Recording file name.

The public client is not allowed to turn this into arbitrary path authority. Before every native read/mutation SuiteBridge must resolve the requested identity against VDR's current Recording inventory and use the Recording object returned by VDR. A caller-supplied path that is not the exact expected current Recording identity fails closed.

## First coherent implementation vertical

The first vertical is intentionally smaller than a full video editor but complete enough for real native use.

### Read contract

For one current Recording, return:

- backend/native Recording identity used for verification;
- marks revision/fingerprint derived from the current canonical native marks state;
- Recording FPS/index availability needed to interpret marks;
- ordered marks containing exact `positionFrame`, VDR timecode and derived display seconds;
- native marks state: none / present / invalid when VDR cannot establish a usable canonical set;
- Recording in-use state;
- native cut usage/state where VDR can truthfully report it;
- edited-result identity when reliably discoverable.

### Marks mutation contract

Support bounded operations through the same native authority:

- add mark at a requested playback/native position;
- delete an exact existing mark;
- move an exact existing mark to a requested position;
- replace the complete native marks set only when protected by the exact expected marks revision;
- reset/delete native marks.

Every successful mutation returns authoritative readback from VDR. The final normalized frame positions in that readback are the client truth.

### Cut contract

Support explicit user-triggered native cut start through `RecordingsHandler.Add(ruCut, ...)`.

The operation must prevent duplicate start, retain unknown-outcome semantics after possible dispatch, and obtain native state/readback before finalizing the Suite operation.

A later status refresh may report native pending/active/finished/error state only to the extent VDR exposes it reliably. VDR-Suite must not fabricate percentage progress if the native API does not provide trustworthy progress.

### Frontend contract

The feature belongs in the existing Recordings 2 detail flow.

The first vertical provides a compact **Schnitt / Schnittmarken** section with:

- native marks list;
- exact formatted mark positions;
- add mark at the current canonical Recording playback position;
- jump to a mark using the existing persistent playback owner/seek semantics;
- delete mark;
- bounded mark move/edit control;
- clear indication of alternating kept/removed native sections;
- explicit `Schneiden` action with confirmation/state feedback.

It must reuse:

```text
Recordings 2
  -> recordings2-browser-view
  -> VdrSuiteRecordings2Playback.createPanel(...)
  -> one persistent Recording playback owner
```

No second player, second MediaSession owner or private video timeline is introduced.

## Marks revision and replay safety

Because the OSD and VDR-Suite intentionally share one native file, optimistic concurrency is mandatory.

The native read owner must produce a stable revision/fingerprint over the canonical current marks representation plus the current Recording identity/facts required to prevent stale mutation.

Every mutation carries the expected revision. Immediately before changing VDR state SuiteBridge re-reads current native marks and rejects a mismatch.

This closes the race:

```text
Suite reads marks A
VDR OSD changes marks -> B
stale Suite mutation based on A
```

The stale Suite request must fail without overwriting B.

The same rule applies to exact mark delete/move: the expected source frame must still exist under the expected marks revision.

## MARKAD optional integration

MARKAD is not part of the core marks/cut authority and is not a dependency.

The supported design direction is:

```text
MARKAD available and explicitly supported
  -> optional "Werbung automatisch erkennen"
  -> MARKAD writes native VDR marks
  -> normal native marks readback
  -> user reviews/edits the same marks
  -> separate explicit native VDR cut action
```

There is no `MARKAD marks` database.

Before enabling this optional action on the real host the implementation/acceptance must determine:

- installed plugin/tool presence and exact version;
- whether and when it runs automatically;
- exact manual invocation semantics for that installed version;
- whether existing manual marks are overwritten or merged;
- available status/progress semantics;
- safe policy for preserving user-edited marks.

MARKAD's own optional cutting modes are **not** a replacement for the VDR-native `RecordingsHandler` path in this workstream.

If MARKAD is absent, native manual marks and VDR cutting remain fully functional.

## Automated acceptance requirements

The implementation must prove at least:

### Native marks

- no marks;
- one/multiple native marks read in canonical order;
- exact native frame/timecode serialization;
- I-frame/index normalization is reflected in readback;
- add/delete/move/reset mutate only the expected Recording;
- stale marks revision rejects without effect;
- invalid Recording/native identity fails closed;
- wrong Backend/provider/generation fails closed;
- in-use Recording mutation fails closed;
- OSD/native-file change is visible on next Suite read;
- no parallel proprietary marks persistence exists.

### Native cut

- no-marks cut rejects;
- in-use Recording rejects;
- start uses `RecordingsHandler.Add(ruCut, ...)` rather than direct `cCutter`/shell/filesystem implementation;
- duplicate start is rejected/recognized without a second native job;
- source/original is preserved;
- edited Recording is detected from native Recording state;
- possible-dispatch timeout becomes outcome-unknown/reconciliation rather than blind retry;
- current Backend/Agent/provider identity is revalidated before dispatch.

### Product integration

- existing Recording listing/details/playback do not regress;
- production Recordings 2 detail creates the editing surface;
- add-at-current-position consumes the existing canonical playback owner position;
- jump-to-mark reuses existing seek ownership;
- the frontend does not call SuiteBridge/SVDRP/filesystem directly;
- protected mutations use the production Control Plane/Agent/SuiteBridge owner path.

## Real yaVDR acceptance boundary

No real acceptance may be claimed until executed on the actual yaVDR host and exact candidate.

The real acceptance must prove:

1. an existing native VDR marks file is shown identically in VDR-Suite;
2. a mark created in VDR-Suite becomes visible to normal VDR replay/OSD;
3. a mark created/changed in normal VDR becomes visible in VDR-Suite;
4. a dedicated test Recording can be cut from VDR-Suite via the native VDR handler;
5. the original Recording remains intact and the edited Recording follows native VDR naming/registration semantics;
6. no VDR/daemon crash and no Recording corruption occurs;
7. optional MARKAD behavior is tested only when the installed version is present and safely testable; otherwise it is recorded as N/A.

The native-build gate must also record:

```text
VDR version
VDR API version
candidate commit
installed SuiteBridge object hash
installed daemon/Agent candidate identity
marks test Recording identity
pre/post native marks state
cut source/result Recording identities
```

## Non-goals for this workstream

- no Phase 67 work;
- no generic video editor;
- no proprietary marks database;
- no browser-side marks file access;
- no shell cutter;
- no direct `cCutter` ownership by VDR-Suite when `RecordingsHandler` owns the native lifecycle;
- no automatic original deletion/replacement;
- no irreversible MARKAD auto-cut;
- no fabricated cut percentage;
- no support for active/growing Recording editing until separately source-audited and authorized;
- no promotion of transitional internal endpoints to the Phase-69 public API contract.
