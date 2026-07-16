# SB.2 Capability Contract

The Suite bridge capability catalogue is immutable, VDR-independent and
transport-neutral.

Capability schema version: `1`

| Capability | State | Meaning |
| --- | --- | --- |
| `lifecycle` | `available` | Deterministic plugin lifecycle is implemented and tested. |
| `status-events` | `planned` | Native VDR status observation is not implemented yet. |
| `snapshots` | `planned` | Immutable VDR snapshots are not implemented yet. |
| `local-contract` | `planned` | No external Backend Agent transport contract exists yet. |
| `mutations` | `disabled` | Native write operations are deliberately unavailable. |

The capability model must not imply availability before the corresponding
implementation slice and acceptance tests have been completed.
