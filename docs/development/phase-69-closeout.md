# Phase 69 Closeout — Public API and Client Compatibility Hardening

## Status

**Phase 69 is completed.**

```text
phase69=COMPLETED
69.A=COMPLETED
69.B=COMPLETED
69.C=COMPLETED
69.D=COMPLETED
69.E=COMPLETED
69.F=COMPLETED
next=Phase 70 - Recommendation and Content Knowledge Graph [NOT STARTED]
```

Closeout baseline:

```text
main=4171574f76eeb2d1a7aad9861a81de90451b820a
last accepted Phase-69 runtime/client slice=PR #381
accepted head=f8034648907a02965c41cbf49be24b14f3e38356
merge/main=4171574f76eeb2d1a7aad9861a81de90451b820a
CI #9357 / run 36369528246 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69 Kickoff and implementation record](phase-69-public-api-kickoff.md)
- [Phase 69.B Closeout](phase-69b-closeout.md)
- [Phase 69.C Closeout](phase-69c-closeout.md)
- [Phase 69.D Closeout](phase-69d-closeout.md)
- [Phase 69.E Closeout](phase-69e-closeout.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)

Phase 70 is **not** started by this closeout. The Strict Roadmap requires its
own accepted runtime ADR before implementation.

## Completion decision

The live post-PR-381 audit finds no remaining bounded Phase-69 runtime/client
gap.

The stable public-v1 inventory contains exactly eight method/resource contracts,
and the accepted independent JavaScript reference seam covers that exact set:

```text
8 stable public-v1 contracts
8 reference-covered contracts
0 uncovered stable contracts
0 extra inferred public contracts
```

All explicit browser route fallback probing inventoried during 69.E/69.F is
retired. The bundled browser remains on richer pre-v1/private contracts where
the semantics are deliberately non-equivalent; no route substitution was used
to fabricate compatibility.

Domains still classified `missing-public-v1` — ProgramEvent/EPG, Recordings,
metadata/persons, global search, SearchTimer and Genre — remain intentionally
outside the Phase-69 stable set. Their absence is not unfinished Phase 69. A
future promotion requires its own identity, representation and compatibility
decision.

## 69.A-F completion map

| Slice | Completed responsibility | Durable evidence |
| --- | --- | --- |
| 69.A | Public route/resource inventory and bounded `/api/v1` root/capability foundation. | Phase-69 kickoff accepted checkpoints. |
| 69.B | Request/correlation metadata, stable Problem Details and HTTP error semantics. | [69.B Closeout](phase-69b-closeout.md). |
| 69.C | ETags, conditional reads, strong If-Match, durable Idempotency-Key and productive Timer CREATE lifecycle. | [69.C Closeout](phase-69c-closeout.md), including exact-head real-yaVDR acceptance for PR #361. |
| 69.D | Deterministic collections, keyset pagination and explicit federated partial-result semantics. | [69.D Closeout](phase-69d-closeout.md). |
| 69.E | Machine-readable compatibility/deprecation policy plus complete retained legacy-route classification. | [69.E Closeout](phase-69e-closeout.md). |
| 69.F | Client error/fallback hardening, stable external-client boundary and executable reference coverage of all declared public-v1 contracts. | Accepted PR #369 through #381 chain below. |

## Accepted 69.F implementation chain

| PR | Slice | Accepted head | Merge/main | Hosted CI |
| --- | --- | --- | --- | --- |
| #369 | Client error + mutation fallback safety | `061b0f1909a228c57d88c3b53f4f50d0275b1bba` | `587352abfb4ffb04628831a2fbc3b528be3c722b` | #9314 / `36328486580` SUCCESS 6/6 |
| #370 | SearchTimer fallback removal | `eaa1cb5c715fdf7970058c6f9b290fe96264bf2d` | `b7ec2a6cf44e5b6b3e4652370a73e27aa071e772` | #9317 / `36331179868` SUCCESS 6/6 |
| #371 | Same-handler read alias fallback removal | `bc3c31df17f923fe0877068fa550bc802c50e8d1` | `83bd7c3522d4cdaef6d24b529fc46c44994275b9` | #9321 / `36333516626` SUCCESS 6/6 |
| #372 | Client contract matrix | `97739a82b59913edf268bc33beece1b074f58aec` | `ca7ddce8c1f4dcd94960c87d49c87533c5b0630b` | #9324 / `36335165236` SUCCESS 6/6 |
| #373 | Public Backend collection | `096a4d2f1e011b876f80a38ef7e7b66ca04f0d83` | `64798ee1bd51694d70a01432d6260b4f84dfa26b` | #9335 / `36338659080` SUCCESS 6/6 |
| #374 | Home EPG single-route hardening | `03b4783a0cd9e53e65f9dad3e1c0bf611a35cb84` | `fbaab982964c838e589dfce8adb62e14013e0e07` | #9337 / `36341522122` SUCCESS 6/6 |
| #375 | Timer live single-route hardening | `dfba6b0459c3521fe534900c03b383f539fa55ce` | `19077e7110fdb8111aead43e15b6962767ac6234` | #9339 / `36343460569` SUCCESS 6/6 |
| #376 | Public-v1 discovery reference client | `67d103443ec247eb7c7e3d395386510ae33b8fe4` | `88b7e01d5fb327fea3f54d43f31202dc7e303d98` | #9341 / `36344690345` SUCCESS 6/6 |
| #377 | Public-v1 Channel reference client | `959ac28a5f88f34282a7644c31c1982ff6beface` | `7704d3826aa426a4cb5e48c4ba07e372614b86fe` | #9344 / `36346772858` SUCCESS 6/6 |
| #378 | Public-v1 TimerAssignment collection client | `c996f4c057f0d5b912c00f65d11630cf606cf8c4` | `7a2c8f651ed895148d5520f2ac5820781b917e49` | #9346 / `36364060778` SUCCESS 6/6 |
| #379 | Public-v1 TimerAssignment item client | `0762ea868d1c9ba93378064107731af9660ac705` | `28eb3301215bea17958c3c1b2f49f1e8c7705445` | #9348 / `36366447485` SUCCESS 6/6 |
| #380 | Public-v1 Timer CREATE admission client | `1b033576fab3c5322c14edc6405d5c6948dc50f3` | `afd0e04708622d99a3ee82532e7d8608495d3f6b` | #9352 / `36367430072` SUCCESS 6/6 |
| #381 | Public-v1 durable Operation client | `f8034648907a02965c41cbf49be24b14f3e38356` | `4171574f76eeb2d1a7aad9861a81de90451b820a` | #9357 / `36369528246` SUCCESS 6/6 |

## Phase-69 acceptance gate

| Gate | Result |
| --- | --- |
| Declared stable domain set exists under `/api/v1` | PASS — eight explicitly inventoried contracts |
| Stable HTTP/error semantics | PASS — 69.B |
| Revisions, preconditions and idempotency | PASS — 69.C |
| Restart/concurrency/native-effect correctness where the mutation path requires it | PASS — 69.C durable runtime + PR #361 exact-head real yaVDR acceptance |
| Deterministic collection/pagination/partial-result semantics | PASS — 69.D |
| Compatibility/deprecation policy is machine-readable and testable | PASS — 69.E |
| Clients do not depend on private Agent/provider/plugin shapes | PASS — 69.F matrix/reference seam |
| Breaking-change guards protect the public surface | PASS — inventory, architecture and matrix guards |
| Supported legacy aliases remain rollback-safe | PASS — aliases remain server-retained; 69.F removed client probing without deleting the compatibility routes |
| Media/OSD/Broadcast Companion planes remain separate | PASS — no Phase-69 slice redefined those transports |
| Independent reference seam covers the stable set exactly | PASS — 8/8, no extras |
| Final accepted runtime/client head hosted CI | PASS — PR #381 CI #9357 / run `36369528246`, 6/6 |

## Client boundary at closeout

Independent clients may build against the deliberately stable reference seam
for:

- public root and capability discovery;
- actor-filtered Backend discovery;
- explicit-source federated Channel collection reads;
- backend-scoped TimerAssignment collection/item reads;
- preconditioned/idempotent Timer CREATE admission;
- actor-owned durable Operation reads.

The reference client deliberately provides no automatic mutation retry and no
automatic Operation polling.

The bundled browser remains free to use richer first-party transition/private
routes where its semantics are not equivalent. Browser session/CSRF lifecycle,
cache helpers, Live overlay/SSE and other first-party support surfaces do not
become public merely because Phase 69 is complete.

## Acceptance boundary

The final 69.F slices after the already accepted productive Timer CREATE runtime
are client-only or documentation/guard work. They do not alter VDR native
effects, SuiteBridge, Agent execution, Home, LiveTV or media playback.

The only Phase-69 native-effect acceptance required by this closeout is already
recorded by 69.C for the exact productive Timer CREATE candidate. The Phase-69
closeout itself therefore requires exact-head hosted CI, documentation and
architecture/status guards; repeating real-yaVDR mutation acceptance would add
no new evidence.

## Successor boundary

```text
Phase 69 - Public API and Client Compatibility Hardening [COMPLETED]
  -> Phase 70 - Recommendation and Content Knowledge Graph [NOT STARTED]
```

This closeout is **not** authorization to implement Phase 70. The Strict Roadmap
requires a dedicated accepted runtime ADR before Phase-70 implementation.
Cross-cutting productization work such as app/client rollout, access
administration and release packaging remains governed by its own roadmap and
does not reopen Phase 69.
