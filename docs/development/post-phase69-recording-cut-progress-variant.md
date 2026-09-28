# Post-Phase-69 Recording Cut Progress and Variant Hardening

Status: **non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

## Reported gap

Native VDR cutting already completes correctly, but the Recording detail gave no
durable indication that the cut was still active. Users could only infer
completion after the edited Recording appeared elsewhere.

The edited result was also presented as an unrelated Recording even though the
native cut read model already knows the exact edited-result identity.

## Authority and progress truth

ADR-0059 remains binding:

- VDR owns native marks and cutting.
- VDR-Suite starts the cut only through the existing protected native cut
  operation.
- acknowledgement is not completion.
- the original Recording remains intact.
- VDR-Suite must not invent percentage progress.

The existing RCUT/native readback already exposes the truthful lifecycle facts
needed by the UI:

- `handlerUsage`;
- `editedDestinationExists`;
- `editedRecordingFound`;
- `editedRecordingKey`.

There is no reliable native percent/frames/bytes progress contract. Therefore
this slice intentionally uses an **indeterminate progress indicator** while the
native cutter is active or the edited destination is being finalized.

## Exact edited Recording projection

When `editedRecordingFound` is true, the REST runtime resolves
`editedRecordingKey` back against the current canonical Recording collection.
The result is projected only when exactly one current Recording has the same
native identity key.

The browser therefore receives a normal canonical Recording projection for the
edited result without:

- guessing VDR paths;
- deriving names in JavaScript;
- matching by title/duration;
- introducing a second Recording identity model.

The original detail renders that exact result as **Schnittfassung** and can
open it through the existing `VdrSuiteRecordings2.openRecording(...)` owner.

## Original deletion

Cut completion never deletes or replaces the source Recording.

The variant surface tells the user that the original can be removed through
the existing **Aufnahmeaktionen -> In Papierkorb verschieben** workflow. That
workflow retains its own validation, confirmation, execution and readback
contract. This slice deliberately does not make deletion a cut side effect and
does not add another destructive mutation owner.

## Lifecycle

The Recording detail polls the existing read-only cut state while any of these
conditions is true:

- the local protected cut operation is still pending;
- the persisted operation journal reports a pending cut for this source;
- a verified cut result exists but its canonical Recording projection has not
  caught up yet.

Handler usage or an existing destination alone does not establish a Suite cut.
The progress/result surface belongs directly to the existing detail root, outside
the playback/marks subtree hidden by the hero visibility owner. It remains
visible in detail, playback, metadata and Recording-action modes. The explicit
cut confirmation receives focus and is scrolled into view. Local pending state
renders indeterminate progress synchronously, before the POST/readback resolves.

Polling stops once the exact edited Recording projection is available and the
protected operation has settled. Reloading the detail during a running cut
re-enters the same read-only monitoring path.

## Regression boundary

Automated coverage must prove:

- running cuts are visible with an indeterminate progress element;
- no percentage field/value is fabricated;
- transient status loss keeps the same operation identity and continues
  reconciliation rather than dispatching a new cut;
- exact cut-result Recording projection is derived from
  `editedRecordingKey`;
- the confirmed cut result appears as `Schnittfassung`;
- opening the variant delegates to the existing Recordings 2 owner;
- the original remains present; cut completion never suggests deletion;
- the separate Recording action obtains backend permission before confirmation;
- existing marks and playback ownership remain unchanged.

Phase 70 remains not started.

## Runtime investigation, 2026-09-28

The installed editor fingerprint was
`261315b13834e8235a2efc6ba451f8b8c94335d1610f1456965db151bd34baf5`,
matching `626ee33814e5b983c5d8a62bf899fe68ddad8276`.
Before changes, the PR was at `bcb5e2fbc3b88b9f5bad99916104f3793fffafde`:
`92c2d96` had already removed the blocking preview-read dependency and
`bcb5e2f` adjusted the corresponding timer regression.

For the reported 13:32 CEST attempt, nginx records GET cut-state requests for
backend `default`, Recording ID `576`, but no POST cut request. VDR records
RCUT source key `f8a789ed0abb6e2a720338db43d445e7`, ready with handler usage zero.
At 13:32:44/48/51 the separate Recording actions validation/execution requests
appear; accountability records `recordings.delete`, and VDR records the source
being renamed from `.rec` to `.del` at 13:32:51. The native marks reads and
deletion name the same Recording. There is no corresponding new embedded cut
command, agent cut reservation, or NCUT start in this interval, so there is no
edited-result key to attribute to this attempt. A later timeout-bounded RCUT
read reports source not found; filesystem and LSTR no longer contain it.

This proves a real trash operation, not just a missing cache projection. The
logs do not retain request bodies or browser click history, and cannot establish
why the cut confirmation was not submitted. No claim of a cutter deleting a
different source is supported by this evidence.

A separate reproducible projection defect used the numeric Recording ID before
native identity in `forgetRecording`. The runtime cache demonstrated reuse of
ID `576` for another native Recording after deletion. A late delete completion
could consequently hide its successor. The regression now proves that matching
uses backend plus native identity, removes the original even if its list ID
changed, and retains a different Recording reusing the old ID. This is a
projection fix, not evidence that the reported native trash target was wrong.

## Premature completion and deletion permission, 2026-09-28

VDR journal evidence for Million Dollar Baby: the native cutter started at
14:06:04 CEST and ended at 14:08:13. RCUT reported source
`7b250cd1fd1ae0e5235eb69c2b55a210`, handler usage 36, and edited result
`51712793791949d50c62cce6dbae58a2` already present while cutting. A trash
preview occurred at 14:07:06; actual source deletion occurred at 14:09:32,
after cutter completion. The embedded journal stored verified result identity.
The completion predicate checked existence/identity but omitted handler usage;
it could therefore verify before native completion, while the independent
trash safety check still correctly blocked the active source.

Completion now requires the exact result plus zero handler usage. Embedded
replay and read-only reconciliation recheck previously persisted verification;
the UI also retains progress for a journal-owned cut with an active handler.
No handler activity alone establishes ownership of a Suite cut.

The cut banner has no deletion suggestion. Opening the separate trash editor
checks backend permission with the existing validation and dry-run endpoints.
The delete button stays disabled on denial and preserves the backend reason.
Execution rechecks permission before the single explicit confirmation. Detached
owners cannot confirm delayed responses; requests retain the frozen source
identity and backend. Regression tests cover denial, changed permission,
cancellation, duplicate clicks, owner detachment and exact source identity.

This correction changes daemon and frontend code. Deploy the daemon and the
existing concatenated Recordings 2 bundle plus actions/editor assets; no
SuiteBridge rebuild or VDR restart is required.


## Real-system follow-up: Recording action reconciliation

Real yaVDR acceptance also exposed an older Recording-action cache fan-out that
was independent of the cutter itself but became visible while exercising the
new cut/delete flow.

The generic successful-action callback still requested eight complete
Recording-cache reconciliation passes. Worse, successful dry-run previews also
reached the same post-success callback. Opening and confirming a destructive
action could therefore enqueue repeated full Recording inventory rebuilds before
and after the real mutation.

The corrected contract is:

- dry-run Recording actions never trigger post-mutation cache work;
- a committed DELETE removes only the affected backend-native Recording from
  the persistent Recording cache and the in-memory snapshot immediately;
- that committed DELETE publishes one backend-scoped Recording presentation
  invalidation so Home can drop the card without waiting for a full inventory
  rebuild;
- one asynchronous authoritative Recording inventory read remains as
  reconciliation;
- Rename/Move keep their single synchronous identity refresh and no longer
  schedule an additional eight-pass fan-out.

The same acceptance restart also exposed that the Global Search legacy EPG
people backfill was executed on every daemon start. It is now guarded by a
persistent schema-version marker. Existing installations that already have the
pre-versioned people table are marked migrated without repeating the expensive
legacy scan; a genuinely older installation without that table performs the
backfill once and then records the marker.


## Real-system startup profiling follow-up

Real yaVDR startup profiling on the production database showed that the first
Global Search migration fix reduced HTTP-listener readiness from roughly
55 seconds to roughly 16 seconds, but did not eliminate the remaining startup
stall.

The production database contained roughly 1.7 million
`suite_metadata_genre_assignments` rows while all Genre schema migration
markers through version 12 were already present. The Genre schema initializer
still issued the historical `DELETE FROM suite_metadata_genre_assignments ...
AND NOT EXISTS(schema-version)` migration statements on every daemon start.

Those migrations are now gated in C++ before any destructive migration SQL is
issued. Completed schema versions return immediately and therefore do not ask
SQLite to consider the large assignment table at startup. A repository restart
regression test traces SQL and requires zero EPG browse-classification DELETE
statements after the migration markers have been persisted.
