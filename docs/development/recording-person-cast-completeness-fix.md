# Recording Person Cast Completeness Fix

## Status

- Date: 2026-07-25
- Repository: `hotzenplotz5/vdr-suite`
- Baseline before the fix: `630635f903c251c1e175683eeca7847802dbb063`
- Related handoff:
  [`vdr-plugin-suite-bridge/docs/RECORDING-METADATA-HANDOFF.md`](../../vdr-plugin-suite-bridge/docs/RECORDING-METADATA-HANDOFF.md)
- Live VDR acceptance: pending rebuild, installation and metadata refresh

## Problem

The native TVScraper recording metadata path was already authoritative and
persistent, as required by the recording-metadata handoff. However, the
SuiteBridge recording contract accepted only twelve people.

The real `Pulp Fiction` recording proved that this limit was not a presentation
limit:

- TVScraper provider ID `680` contains John Travolta as Vincent Vega;
- the Suite recording-person table contained exactly twelve entries;
- those entries matched the twelve lowest TVScraper `actor_id` values;
- John Travolta has `actor_id = 8891` and was discarded before persistence.

TVScraper's legacy `GetActors()` query does not define a cast-order `ORDER BY`.
Therefore truncating the returned vector after twelve entries can remove a lead
actor even when that actor is first in the provider's intended cast order.

## Decision

Recording metadata transport and persistence must keep a substantially complete,
bounded person set. UI presentation is a separate concern and must not determine
which people are searchable.

The bounded recording contract is changed to:

| Boundary | Previous | New |
| --- | ---: | ---: |
| SuiteBridge recording people | 12 | 128 |
| SuiteBridge recording JSON payload | 7,679 bytes | 65,535 bytes |
| Backend native metadata people | 12 | 128 |
| Backend native metadata parser payload | 8,192 bytes | 65,535 bytes |
| SVDRP transport reply bound | 8,192 bytes | 131,072 bytes |

The EPG metadata contract remains unchanged. This fix applies to recording
metadata imported through `RMETA`.

## Implementation impact

The existing recording adapter already stops at
`SuiteBridgeRecordingMetadata::kMaxPeople`. Raising the shared recording
contract limit therefore allows the adapter to pass the larger TVScraper
person set without a second import path.

The persistent repository already inserts every person supplied by the
resolver and stores deterministic ordinals. No schema migration is required.

The frontend remains provider-neutral. It may choose its own presentation
strategy, but the stored recording-person index retains the imported people for
search.

## Regression coverage

The focused contract tests now model a 52-person `Pulp Fiction` payload with
John Travolta beyond the former twelve-person boundary and prove that:

1. SuiteBridge serializes the complete bounded payload;
2. the payload is larger than the former 7,680-byte contract capacity;
3. the backend parser accepts more than 8,192 bytes;
4. all 52 people survive parsing;
5. John Travolta and the character name `Vincent Vega` remain present;
6. 129 people are still rejected by the explicit 128-person bound;
7. payloads beyond 65,535 bytes remain rejected rather than truncated;
8. the SVDRP transport accepts a reply larger than the former 8,192-byte bound.

Focused tests executed in an isolated source reconstruction:

```text
g++ -std=c++17 -Wall -Wextra -pedantic -I. \
  suitebridge_artwork_reference.cpp \
  suitebridge_recording_identity.cpp \
  suitebridge_recording_metadata.cpp \
  suitebridge_recording_metadata_contract.cpp \
  tests/test_suitebridge_recording_metadata_contract.cpp \
  -o /tmp/test_suitebridge_recording_metadata_contract

/tmp/test_suitebridge_recording_metadata_contract

g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude \
  src/VdrRecordingNativeIdentity.cpp \
  src/SuiteBridgeRecordingMetadataResolver.cpp \
  tests/test_suite_bridge_recording_metadata_cast_completeness.cpp \
  -o /tmp/test_suite_bridge_recording_metadata_cast_completeness

/tmp/test_suite_bridge_recording_metadata_cast_completeness
```

Both focused binaries completed successfully.

The complete repository build and the transport integration test still need to
run on the VDR-Suite checkout because this execution environment has no GitHub
checkout and no installed VDR development headers.

## Required real-VDR acceptance

After updating the checkout on the VDR host:

```text
cd /home/yavdr/vdr-suite
git pull --ff-only origin main

make -C vdr-plugin-suite-bridge test-recording-metadata-contract
make test-suite-bridge-recording-metadata-resolver
make test-suite-bridge-recording-metadata-cast-completeness
make test-suite-bridge-svdrp-recording-metadata-transport
make test-docs
git diff --check
make daemon
```

Build and install the SuiteBridge plugin with the repository's normal staged
installation workflow, restart VDR and the VDR-Suite daemon, then refresh the
native metadata entry for the `Pulp Fiction` recording.

Acceptance requires:

- `vdr_recording_native_person` contains John Travolta;
- the stored character name is `Vincent Vega`;
- recording-person search for `John Travolta` returns `Pulp Fiction`;
- the result survives a daemon restart;
- an oversized or malformed payload remains rejected;
- EPG metadata, recordings, timers and remote-control behavior remain unchanged.
