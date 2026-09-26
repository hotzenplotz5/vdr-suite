# SuiteBridge control-plane closeout

## Status

ADR-0064 latency-isolation objective: **closed after the ETYPES Background
Provider migration**.

This closeout does not remove SVDRP and does not migrate native mutations.
It records which operations intentionally remain on the typed SVDRP path and
why that is the safer architecture at the current evidence boundary.

## What the local control plane now solves

The accepted implementation isolates the production failure coupling that
motivated ADR-0064:

```text
Critical Control
  Live + native probe

Interactive Control/Read
  Legacy OSD

External Plugin Interactive
  HbbTV + Teletext

Background Provider
  RMETA + META + ARTW + ETYPES
```

A slow TVScraper/provider request no longer occupies the same execution worker
as Live liveness or Legacy OSD control. External plugin Service() calls remain
serialized and TVScraper-backed work remains serialized. No generic provider
parallelism was introduced.

## Why Timer mutations stay on SVDRP

Timer CREATE/DELETE/MODIFY already have stronger mutation authority than the
transport itself:

- durable operation/command identity;
- backend generation and provider/capability fencing;
- assignment/revision fences;
- durable starting/outcome evidence;
- authoritative readback/reconciliation;
- explicit `outcome_unknown` semantics;
- no blind redispatch after possible native execution;
- bounded `cTimers::GetTimersWrite(..., 1000)` acquisition in the native
  callbacks.

They are short and infrequent compared with the provider workloads that caused
the observed head-of-line failure. No measured runtime incident currently
justifies adding a second transport selection boundary to these mutations.

SVDRP is therefore the selected production transport for the Timer mutation
family at this closeout. This is not a missing ADR-0064 implementation step.

## Why Recording editing stays together on SVDRP

Recording editing has a stronger coupling than a simple read-versus-write
label suggests.

`RMARKS` resolves the Recording under a bounded Recording-list read lock,
copies the native facts it needs, releases that list lock, and then loads and
serializes the marks file.

`NMARKS` resolves the same Recording and may load, save or delete the same
marks file while applying the revision fence and native readback.

`RCUT` inspects the same marks file plus cutter/RecordingsHandler state.

`NCUT` repeats those native preconditions and may enqueue
`RecordingsHandler.Add(ruCut, ...)`.

Today all four operations execute through the shared SuiteBridge SVDRP handler.
That serialization prevents a local-control read worker from racing an SVDRP
marks/cut mutation at the marks-file/cut-state boundary.

Moving only RMARKS/RCUT to `InteractiveControlRead` would therefore create a
new concurrency mode that the current native contracts do not prove safe.
Moving the whole Recording editing family would require mutation transport
selection, pre-dispatch fallback proof, post-dispatch unknown-outcome handling
and a shared serialization decision. No demonstrated latency problem currently
justifies that larger risk.

The complete Recording editing family remains on typed SVDRP until a future
bounded architecture slice proves otherwise.

## Diagnostic-only command

`MCOMPARE` remains diagnostic-only SVDRP because the live audit found no
normal productive owner. It is not migrated merely to reduce the count of
SVDRP commands.

## Closeout invariant

The local operation registry must not silently grow native Timer or Recording
editing operations. A future migration must deliberately change the
architecture guard and provide, for the affected family:

1. a measured/productive reason to move;
2. exact pre-dispatch transport-selection semantics;
3. no replay after timeout or uncertain dispatch;
4. the existing durable receipt/readback/fencing authority;
5. the native VDR lock/lifetime contract;
6. a proof for any new cross-operation concurrency;
7. regression coverage that preserves the PR #358 Home playback lifecycle
   contract and the ADR-0064 Live/control isolation contract.

## Phase 69 relation

This closeout removes the internal transport architecture as a prerequisite for
the next Phase-69 mutation work. Phase 69 may continue using the accepted
Timer/Recording mutation authorities without first migrating those mutations
off SVDRP.

No public route, public schema, actor authorization, Timer ownership, Recording
ownership or retry authority changes here.
