# SB.2 Capability Contract

The Suite Bridge capability catalogue is immutable, VDR-independent and
transport-neutral.

Capability schema version: `1`

| Capability | State | Meaning |
| --- | --- | --- |
| `lifecycle` | `available` | Deterministic plugin lifecycle is implemented and live accepted. |
| `status-events` | `available` | Native VDR status callbacks are observed through bounded counters. |
| `snapshots` | `available` | Immutable diagnostic snapshots with continuity fields are implemented. |
| `local-contract` | `available` | The read-only `SNAP` local contract is implemented and live accepted. |
| `mutations` | `disabled` | Native write operations are deliberately unavailable. |

The catalogue order is stable for deterministic serialization. Consumers must
nevertheless identify entries by the stable capability ID rather than treating
array position as the semantic identity.

SB.9 exposes this catalogue through:

```text
PLUG suitebridge CAPS [discovery-schema]
```

Capability discovery has its own schema version and reports the capability,
snapshot and local-contract schema versions independently. Plugin software
version alone is not a compatibility contract.

An unknown or absent capability is treated as unavailable. An absent or disabled
`mutations` capability is a hard write prohibition. Capability discovery never
constitutes user authorization and does not enable a mutation by itself.
