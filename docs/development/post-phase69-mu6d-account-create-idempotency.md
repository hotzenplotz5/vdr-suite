# MU.6D — Atomic Account CREATE + Durable Idempotency

Status: **COMPLETED — real yaVDR acceptance passed; PR #411 merged.**

MU.6D adds the bounded Human Account creation surface defined by ADR-0067.
It does not add backend grants, role assignment, credential administration,
session administration or browser-admin UI.

## Public-v1 contract

```text
POST /api/v1/accounts
Content-Type: application/json
Idempotency-Key: <caller-owned bounded key>

{
  "loginName": "...",
  "displayName": "...",
  "password": "..."
}
```

The request object is closed: exactly `loginName`, `displayName` and
`password` are accepted. `If-Match` is not part of Account CREATE.

Successful create and exact replay return:

- `201 Created`;
- `Location: /api/v1/accounts/{accountId}`;
- the normal secret-free Public-v1 Account representation;
- a strong Account ETag.

The response never returns the submitted password, password hash, credential
identity, session identity or permission grants.

## Authorization and browser security

The mutation requires:

```text
accounts.create@*
```

The scope is global because Human Account identity is Suite-global. Therefore a
global `role.admin@*` may satisfy the permission while a backend-scoped
`role.admin@default` does not. Browser requests remain behind the central
SecurityHttpGate CSRF boundary. `role.read-only@*` blocks the mutation.

## Atomic authority

`HumanAccountCreationService` owns the synchronous Class-A transaction and
reuses the established security persistence:

- `SecurityIdentityProvisioningRepository`;
- `HumanAccountRepository`;
- `CredentialVerifierRepository`;
- `AccountabilityEventRepository`.

One successful transaction creates:

1. a server-generated User Actor;
2. one active `human-password` credential;
3. one unique login verifier;
4. one active Human Account with revision 1;
5. one durable create-idempotency binding;
6. secret-free accountability evidence.

No role or backend permission grant is created by MU.6D.

## Password boundary

The initial password is request-only. The service uses the same yescrypt
`$y$` hashing mechanism as the accepted First Admin / recovery authority and
wipes transient password/hash buffers. Plaintext passwords are not persisted,
returned or included in idempotency state.

## Durable create idempotency

`security_human_account_create_idempotency` is keyed by authenticated actor
plus `Idempotency-Key`. It stores only:

- actor ID;
- idempotency key;
- normalized non-secret request fields `loginName` and `displayName`;
- resulting Account ID.

It deliberately does not store the password, a reversible secret or a
password-derived request fingerprint.

Semantics:

- same actor + same key + same non-secret fields -> replay the original Account
  result without hashing or reprocessing the supplied password;
- same actor + same key + different non-secret fields -> `409
  idempotency_conflict`;
- conflicting login name -> `409 operation_conflict`;
- no second Human Account is created on an exact replay.

## HTTP failure contract

| Condition | HTTP |
| --- | ---: |
| anonymous / invalid authentication | 401 |
| missing authority / CSRF / read-only role | 403 |
| malformed JSON or Idempotency-Key | 400 |
| unsupported media type | 415 |
| invalid closed request object | 422 |
| login conflict | 409 |
| idempotency conflict | 409 |
| security/account persistence unavailable | 503 |

## Deliberate exclusions

MU.6D does **not** implement:

- automatic `role.admin` or `role.read-only` assignment;
- backend-specific access grants;
- grant administration (MU.7);
- password rotation / credential administration (MU.8);
- session administration (MU.8);
- browser administration UI (MU.9);
- Phase 70 work.

## Acceptance state

Local acceptance and real yaVDR HTTP acceptance passed on the supported system.

The accepted runtime evidence proved:

- Public-v1 Account CREATE returned `201 Created`, `Location` and strong Account ETag;
- the response remained secret-free;
- exact Idempotency-Key replay returned the original Account without creating a second Account;
- a different retry password did not replace or reprocess the original credential;
- changing non-secret request fields under the same Idempotency-Key returned `409 idempotency_conflict`;
- CREATE assigned no role or backend permission grants;
- the newly created Human Account could authenticate with its original password;
- `GET /api/v1/channels?backendId=default` returned `403` for that grant-less Account;
- deactivation through the MU.6C lifecycle boundary succeeded and revoked the Account's active browser session;
- the acceptance Account remains only as an inactive `mu6d_accept_*` artifact because Account DELETE is intentionally not part of MU.6D;
- the normal systemd daemon was restored and final HTTP readiness passed.

MU.6D is **COMPLETED — real yaVDR acceptance passed; PR #411 merged**.
MU.7 grant administration is the active successor workstream; MU.7A is an implementation candidate with local and real yaVDR acceptance pending.
