# MU.5 Administration Architecture Acceptance

## Status

Completed architecture acceptance candidate.

Baseline live `main` audited before this slice:

```text
fb5169d38530374e3b05bcbe5431c34ab5ae102c
```

No parallel open pull request existed when the slice started.

## Purpose

MU.5 exists to freeze the Human Account / backend access administration
architecture before MU.6 runtime work.

The accepted architecture is
[ADR-0067: Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md).

This slice is documentation/architecture only. It does not implement Account,
grant, credential, session or frontend runtime.

## Live-code evidence

### Human Account persistence is read/bootstrap-oriented

`HumanAccountRepository` already owns:

- explicit `security_human_accounts` persistence;
- Account-to-User-Actor invariants;
- bootstrap-time `ensureAccountInActiveTransaction(...)`;
- lookup by Account ID / Actor ID;
- list/read semantics.

It does not yet own general create/update/activate/deactivate administration
semantics or a mutable Account revision.

That is the proven MU.6 runtime gap.

### Grant persistence already exists

`SecurityPermissionGrantRepository` already owns the canonical
`security_actor_permission_grants` table and provides:

- `findActiveGrantsForActor(...)`;
- `ensureGrant(actor, permission, backend)`;
- `revokeGrant(actor, permission, backend)`.

Therefore MU.7 must add administration policy/service semantics above that
repository, not a second ACL/grant database.

The low-level repository accepts bounded strings. ADR-0067 consequently requires
the administration service to own the supported product permission/scope
allowlist before exposing grant mutation.

### Existing Account authorization is read-only

The current Account security boundary exposes only:

```text
accounts.view@*
```

for the stable read-only `/api/v1/accounts` collection.

No `accounts.create`, Account lifecycle mutation, grant administration,
credential administration or session administration Public-v1 runtime exists on
the audited baseline.

### Existing browser-session lifecycle exposed a deactivation gap

`HumanPasswordBrowserAuthenticator` resolves the explicit Human Account and
rejects login when effective Account state is inactive.

By contrast, `BrowserSessionAuthenticator` resolves an existing session from
the browser-session and issuing-credential lifecycle and does not currently
consult `security_human_accounts.active`.

A future implementation that only sets `security_human_accounts.active=0`
would therefore block new password logins while potentially leaving an already
issued browser session usable.

ADR-0067 closes that architecture gap before runtime work by requiring both:

- immediate browser-session revocation during Account deactivation; and
- Human Account active-state validation in Human Account browser-session
  resolution.

### Mutation complexity must remain proportional

ADR-0063 classifies local authoritative mutations as Class A and explicitly
forbids copying Timer-specific orchestration without a concrete distributed or
ambiguous failure model.

Human Account metadata/lifecycle, local grant ensure/revoke and local
credential/session revoke remain Suite-database authoritative synchronous
mutations.

ADR-0067 therefore requires revision/precondition/idempotency semantics where
needed without introducing TimerAssignment, Agent command, durable asynchronous
operation or saga machinery.

## Accepted MU.5 decisions

### Permission vocabulary

Global read permissions:

```text
accounts.view
accounts.grants.view
accounts.credentials.view
accounts.sessions.view
```

Global mutation permissions:

```text
accounts.create
accounts.modify
accounts.activate
accounts.deactivate
accounts.grants.modify
accounts.credentials.revoke
accounts.sessions.revoke
```

All administration authorization requests use global scope `*`.

A backend-scoped `role.admin@backend` does not imply global Human Account
administration.

### First bounded Account creation model

MU.6 may create a later Human Account only as one atomic Suite-security
transaction containing:

- User Actor;
- Human Account;
- human-password credential;
- unique login verifier;
- secret-free accountability evidence.

No role/backend permission is granted automatically.

The initial password is request-only and yescrypt-backed. A bounded durable
create idempotency binding prevents duplicate Accounts after response loss
without persisting password material.

### Lifecycle and session semantics

Mutable Human Accounts gain an opaque revision/strong-ETag contract.

Display-name, activate and deactivate mutations are revision-checked.

Deactivation:

- keeps Human Account and generic Actor identity separate;
- blocks new Human Account login;
- revokes existing browser sessions/canonical browser-session lifecycle state;
- makes Human Account browser-session resolution fail closed when current
  Account state is inactive;
- does not revive old sessions when the Account is later reactivated.

### Final usable administrator invariant

A usable administrator requires, in one transactionally checked state:

- active Human Account;
- valid active User Actor binding;
- active `role.admin@*`;
- at least one active/unexpired/unrevoked human-password credential;
- at least one verifier resolving to such a credential.

Deactivation, admin-grant revoke, final credential revoke or any later
equivalent administration mutation must fail if it would leave zero usable
administrators.

### Grant administration

MU.7 reuses the existing normalized grant repository and adds only the product
administration service/read model required to:

- expose supported grants;
- derive an opaque normalized grant-set revision;
- ensure/revoke supported desired-state grant tuples;
- reject unsupported permission/scope combinations;
- apply final-admin protection to `role.admin@*`.

### Credential/session administration

MU.8 may expose safe lifecycle metadata and terminal revoke actions only.

Verifier hashes, reusable credentials, browser cookies, CSRF secrets and
bootstrap material remain non-readable.

Remote password reset/rotation is not introduced. ADR-0066 local audited
recovery remains authoritative.

## Successor

With ADR-0067 accepted, MU.5 is complete.

The next bounded runtime slice is:

```text
MU.6 — Human Account lifecycle administration
```

MU.6 must remain limited to the Account lifecycle boundary accepted by ADR-0067.
Grant administration remains MU.7, credential/session administration remains
MU.8 and the browser administration UI remains MU.9.

Phase 70 remains not started.
