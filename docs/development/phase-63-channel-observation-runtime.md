# Phase 63 — Channel Observation Runtime

## Status

**Draft runtime implementation for the merged Channel observation contract.**

This slice implements the read-only `channels` observation domain defined by
`phase-63-channel-observation-ingestion.md`. It is based on the merged contract
commit:

```text
main commit: 2567407e6e1c6d098804f887875e7f3cbf9cba60
main tree: c4afb5d9c413401476e642c8799178a8e7676d33
```

The implementation remains opt-in. The packaged default continues to publish
only `backend-health`.

## Explicit local source

The only source implemented by this slice is the local VDR
`channels.conf` file selected through:

```text
ADAPTERS=channels-conf
OBSERVATION_DOMAINS=channels
CHANNELS_CONF_PATH=/var/lib/vdr/channels.conf
```

`channels-conf` and `channels` must be configured together. The Agent does not
probe, auto-select or fall back to RESTfulAPI, SVDRP, SuiteBridge or an existing
Suite cache. A missing, symlinked, malformed, oversized or ambiguous source
fails closed and does not advance the accepted Channel cursor.

## Source interpretation

The bounded parser accepts the standard 13-field VDR Channel record and group
separator syntax. It derives the native Channel identity from source, network
ID, transport-stream ID, service ID and optional radio ID. When both network and
transport-stream IDs are zero, the VDR frequency/polarization fallback is used.
Channel number, name, provider and group remain mutable facts and never become
identity.

The parser enforces:

- a one-MiB file limit;
- a 4,096-Channel limit;
- bounded lines, IDs and display fields;
- regular-file input without symbolic-link following;
- exact field count and bounded numeric parsing;
- valid primary video PID and CAID syntax;
- unique native Channel IDs;
- deterministic sorting and canonical JSON;
- a deterministic resource revision derived from the complete canonical fact
  set.

Source failure is never represented as an empty snapshot.

## Client publication

The Agent publishes a complete snapshot only when the canonical source revision
changes. It uses an independent Channel lineage in the protected identity file:

- backend generation;
- snapshot generation;
- producer sequence;
- last accepted resource revision.

Before dispatch, the exact request envelope is atomically written with mode
0600 to:

```text
<IDENTITY_PATH>.channels.pending.json
```

An ambiguous transport result retains the envelope. The same Agent process
retries those exact bytes before sending the next heartbeat, so the referenced
heartbeat revision cannot silently change. Acceptance or equivalent replay is
persisted before the pending file is removed. A backend-generation replacement
or explicit resynchronization starts a newer snapshot lineage and removes stale
pending evidence.

The current source publisher intentionally uses complete snapshots for changed
`channels.conf` revisions. The Control Plane also implements exact-next
`changeBatch` validation and persistence so later local adapters can publish
explicit upserts/removals without changing the protocol.

## Control-Plane route and validation

The authenticated machine route is:

```text
POST /api/agent/v1/observations/channels
```

It uses the existing technical Agent authentication, current Agent instance,
backend-generation fence, active lease, exact heartbeat revision and declared
read-only capability. The Channel route alone permits a bounded 512-KiB body;
all existing Agent routes retain their previous 16-KiB limit.

The strict parser rejects duplicate object keys, unknown fields, unsupported
JSON types, malformed UTF-8, numeric overflow, duplicate Channel IDs,
overlapping upsert/removal identities and invalid fact fields.

## Agent-owned persistence

Accepted facts live in the Suite-owned table:

```text
backend_agent_channel_facts
```

The key is `(backend_id, channel_id)`. Every row retains Agent, Agent instance,
backend generation, snapshot generation, producer sequence, capture time and
resource revision provenance.

Receipt, cursor and fact changes commit in one repository-owned transaction.
A complete snapshot replaces only the Agent-owned Channel facts for its Backend.
A change batch applies exact upserts and explicit removals. Removal of an
unknown identity is rejected without advancing facts or cursor.

This table is deliberately separate from `VdrChannelCacheRepository` and
`vdr_channel_cache`. The existing direct-adapter cache, frontend read paths and
provider authority remain unchanged.

## Administrative readback

`vdr-suite-backend-agent-admin --status` now reports a bounded
`channelObservation` object with cursor metadata and `factCount`. Acceptance and
operations therefore do not require manual SQLite inspection or exposure of
Channel payloads, credentials or private provider URLs.

## Automated verification

The runtime test graph covers:

- standard `channels.conf` parsing and native identity fallback;
- deterministic serialization and strict JSON rejection;
- explicit adapter/domain configuration pairing;
- initial complete snapshot publication;
- unchanged-source suppression;
- newer snapshot publication after a controlled source change;
- exact pending-envelope retry after an ambiguous transport result;
- identity version migration and independent Channel cursor persistence;
- complete snapshot persistence and readback;
- order-independent equivalent replay;
- exact-next upsert/removal;
- unknown removal rejection without cursor/fact advance;
- sequence-gap `resync-required`;
- existing backend-health behavior and transaction rollback;
- daemon, Agent, enrollment and administration builds.

## Real yaVDR acceptance gate

This Draft is not Ready for review or merge until guarded exact-head acceptance
on the real yaVDR host proves all of the following:

- the checkout branch and full commit match the approved Draft head;
- installed daemon, Agent, enrollment and administration binaries match the
  candidate build;
- the existing Agent ID, credential ID and credential generation are preserved;
- VDR, daemon and Agent services are active before and after acceptance;
- the opt-in source is an explicit root-controlled fixture copied from the local
  `channels.conf`, not a mutation of VDR-native state;
- a baseline and one controlled fixture transition are visible through
  `vdr-suite-backend-agent-admin --status`;
- equivalent retry, deliberate gap/resync and restart persistence are proven;
- Channel fact count and cursor lineage survive restart;
- direct-adapter Channel behavior remains unchanged;
- VDR-native Channel, Timer, setup and recording fingerprints remain unchanged;
- retained evidence is mode 0700, root-owned and secret-free.

No enrollment, revocation, Agent replacement, VDR Channel mutation, public
provider URL or manual SQLite inspection is part of acceptance. The guarded
implementation is registered as:

```text
make phase63-channel-observation-runtime-acceptance \
  PHASE63_EXPECTED_BRANCH=agent/phase63-channel-observation-runtime \
  PHASE63_EXPECTED_HEAD=<exact PR head> \
  PHASE63_CONTROL_PLANE_URL=<HTTPS public origin> \
  PHASE63_EVIDENCE_DIR=<new root-only evidence directory>
```

The harness restores the original Agent configuration and removes its private
Channel fixture before reporting success.

## Exclusions

This runtime does not add Channel commands, editing, ordering, creation,
deletion or enablement. It does not implement Timer/Phase 64, EPG, recordings,
provider ownership, provider selection, frontend migration, public Agent URLs,
streaming, media sessions, OSD or `BackendNode.online` authority changes.
