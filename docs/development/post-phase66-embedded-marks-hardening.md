# Post-Phase-66: embedded native cut-marks hardening

## Candidate and scope

Uncommitted changes on `ff5b1301b6f02518fb70b3dacf49c5fc8fbfd7ef` in
`/home/yavdr/vdr-suite-marks-hardening`. The unchanged commit is **not accepted**.
No Phase 67 work, commits, pushes, merges or changes to `/home/yavdr/vdr-suite`.

## Cause and correction

The previous read path used the configured embedded SuiteBridge transport, but
all marks mutations required an external backend-agent lease and assignment.
The embedded observation runtime does not enroll as that external agent.

A configured embedded SuiteBridge now executes new marks operations through its
existing native `NMARKS` v2 transport. No separate service, enrollment or operator
provider-ownership setup is required for this configured path. Existing external
assignments keep their original route; no operation is reassigned between paths.
If the embedded journal cannot initialize, the route fails closed.

`EmbeddedRecordingMarksRepository` stores a control-plane journal in the Suite
database. It stores command identity and execution evidence, never recording
marks files. Starting is persisted before native execution. Exact replay uses the
same full request, command identity and original provider epoch. VDR enforces its
replay ledger, recording-in-use check, revision check and native marks mutation.
A changed request under an existing operation ID is rejected.

Success requires typed native post-revision evidence for that exact command and
an authoritative native readback with the matching recording key and revision.
Verified results survive daemon restarts. Pending accepted operations only read
back; uncertain transport results reuse the original fenced native command.
A rejection of a replay after an unknown result cannot establish that the original
had no effect: it remains uncertain and is never issued under a fresh identity.
No direct `marks`/`marks.vdr` writes or RESTfulAPI mutation fallback were added.

## UI

Set, move, delete and remove-all execute from a single click. Cut retains its
confirmation. No visible `Auftrag prüfen` control. Verification continues
automatically using the same operation identity, slowing to five-second polling
after the initial quick checks. Destruction cancels pending timers. Definitive
lease/capability/revision/authorization failures release the edit controls after
refreshing native state. Controls and mark entries occupy a compact wrapping row.

## Validation on yaVDR

Passed:

- `make test-recording-native-editing-contracts`
- `make test-security-recording-marks`
- `make test-ci-frontend`
- `python3 tools/check_architecture.py`
- Additional final focused `make test-embedded-recording-marks-runtime`
- Additional final `node web/frontend/tests/test_recordings2_marks_editor.js`
- `make daemon`
- `git diff --check`

The embedded regression includes all mutation shapes, immediate API verification,
exact replay, conflicting operation payloads, concurrent requests, missing
capability, rejected execution, mismatched readback, lost response, disk journal
reopen, VDR restart fencing and unavailable journal. UI regression exercises the
production detail composition and checks direct reset, automatic verification
beyond the initial retry window, terminal lease errors and lifecycle cleanup.

## Real runtime acceptance

Pending installation approval. Automatic approval review rejected replacement of
the installed daemon/frontend and the daemon restart. No runtime files were
installed and no service was restarted by this workstream at this point.

Read-only baseline: VDR PID 17363, Suite daemon PID 20085, separate backend agent
inactive. Native `NMARKS CAP 2 modify` is enabled. Browser session is authenticated.
The selected recording `Historienfilm/Der Untergang` initially has zero marks.
The acceptance sequence will set multiple marks, navigate, move, delete, reload,
remove all, reload and set again, then remove the test marks to restore zero.
No cut job will be started just to validate cut confirmation.
