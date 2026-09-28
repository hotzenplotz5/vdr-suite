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
- VDR reports active Recording handler usage;
- the edited destination exists but the edited Recording has not yet appeared;
- VDR found the edited Recording but the current canonical Recording projection
  has not caught up yet.

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
- the original remains present and deletion is only referenced through the
  existing safe Recording action workflow;
- existing marks and playback ownership remain unchanged.

Phase 70 remains not started.
