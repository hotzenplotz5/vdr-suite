# MU.7 — Backend Access / Permission Grant Administration

Status: **CANDIDATE — LOCAL ACCEPTANCE PENDING**

MU.7 implements the bounded grant-administration contract accepted by ADR-0067.
It reuses the canonical `security_actor_permission_grants` authority and does
not create a second ACL or policy store.

## Public-v1 contract

Grant-set read:

```text
GET /api/v1/accounts/{accountId}/grants
```

The response is secret-free and contains:

- target `accountId`;
- bound `actorId`;
- sorted active supported grant tuples;
- a strong ETag derived from the normalized effective supported grant set.

Each exposed tuple contains only:

```json
{
  "permission": "channels.view",
  "backendId": "default"
}
```

Grant desired-state mutation:

```text
POST /api/v1/accounts/{accountId}/grants
Content-Type: application/json
If-Match: <strong grant-set ETag>

{
  "permission": "channels.view",
  "backendId": "default",
  "active": true
}
```

The mutation body is closed. `active:true` means ensure the tuple is active;
`active:false` means ensure it is absent/revoked.

## Administration authorization

Grant-set read requires:

```text
accounts.grants.view@*
```

Grant mutation requires:

```text
accounts.grants.modify@*
```

These are Suite-global administration permissions. The `backendId` in the
managed tuple is target data and never becomes the administrator's own
authorization scope.

Consequences:

- `role.admin@*` may satisfy both administration permissions;
- `role.admin@default` does not satisfy them;
- browser mutations require central SecurityHttpGate CSRF validation;
- `role.read-only@*` continues to block grant mutation;
- anonymous access fails closed.

## Product grant allowlist

The low-level repository accepts bounded strings, but Public-v1 administration
does not.

MU.7 exposes only explicitly allowlisted, currently enforced product
permissions plus the established roles:

- `role.admin`, `role.read-only`;
- Channel/Timer access and mutation permissions;
- recording/media playback and recording mutation permissions currently
  enforced by SecurityHttpGate;
- Live TV/media playback;
- remote/OSD permissions;
- SearchTimer permissions;
- recording metadata assignment;
- Teletext/HbbTV/session-control permissions.

Internal or architecture-only markers are not publicly administrable, including:

- `authentication.access`;
- `security.permissions.resolve`;
- `unmapped.*`;
- arbitrary unknown strings.

MU.7 also does not expose direct creation of the `accounts.*` administration
permission vocabulary as managed target grants. Global Human Account
administration remains represented by the established administrator role or
other separately provisioned authority.

Every managed tuple requires an explicit safe backend/resource scope: `*` or
a bounded `[A-Za-z0-9._-]` identifier.

## Grant-set revision and concurrency

Grant concurrency is intentionally independent from the Human Account revision.

The service:

1. filters active grants through the product allowlist;
2. sorts by `permission`, then `backendId`;
3. hashes the normalized sequence with SHA-256;
4. exposes only an opaque `grant-set:<digest>` resource revision through a
   strong ETag.

Grant changes therefore do not change the Account ETag.

Mutation semantics follow ADR-0067:

- matching revision + different desired state -> mutate;
- already-desired tuple -> success without duplicate/revival;
- stale revision + tuple already in requested final state -> terminal success;
- stale revision + state change still required -> `412 revision_conflict`;
- repeated revoke of absent/revoked tuple remains terminal;
- no MutationOperation, Agent job or saga is introduced.

## Final usable administrator protection

Revoking `role.admin@*` is checked inside the same `BEGIN IMMEDIATE`
transaction as the grant mutation.

If the target is the final usable Human Account administrator, the mutation
returns `409 operation_conflict` and the grant remains active.

The invariant reuses the same accepted MU.6A definition and
`HumanAccountAdministrationRepository` query rather than creating a second
administrator-count rule.

## HTTP failure contract

| Condition | HTTP |
| --- | ---: |
| anonymous / invalid authentication | 401 |
| missing authority / CSRF / read-only role | 403 |
| malformed JSON or malformed/wrong-resource ETag | 400 |
| unsupported media type | 415 |
| unsupported/invalid grant tuple | 422 |
| missing If-Match | 428 |
| unknown Account | 404 |
| stale grant-set revision while change is still required | 412 |
| final usable administrator role revoke | 409 |
| persistence/authority unavailable | 503 |

## Reference client

The JavaScript Public-v1 reference client exposes:

- `getAccountGrants(options)`;
- `setAccountGrant(options)`.

The client transports ETags, If-Match/If-None-Match and caller-provided CSRF
headers. The server remains the sole permission/scope policy authority.

## Deliberate exclusions

MU.7 does **not** implement:

- credential metadata/revoke;
- session metadata/revoke;
- password reset/rotation;
- browser administration UI;
- Profiles;
- TV/app pairing;
- federation administration;
- Phase 70 work.

Those remain MU.8/MU.9 or later explicit slices.

## Acceptance state

Implementation is present on `work/mu7-grant-administration`.

Status remains **CANDIDATE — LOCAL ACCEPTANCE PENDING** until the focused
service, Public-v1, SecurityHttpGate, reference-client, architecture,
documentation and productive daemon-link checks pass on the supported yaVDR
system.
