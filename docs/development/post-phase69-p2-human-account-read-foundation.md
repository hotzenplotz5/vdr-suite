# P2 Human Account Read Foundation

## Status

Implemented on the P2 candidate branch after acceptance of ADR-0065.

Baseline main:

```text
2b3be73cfe53dd50356563e10daa08b1d6178a07
```

## Scope

This is the first runtime slice after the P1 identity audit.

It adds an explicit Human Account persistence/read foundation inside the existing Suite security authority. It does not create a second identity database.

## Persistence

The additive table is:

```text
security_human_accounts
  account_id
  actor_id UNIQUE -> security_actors(actor_id)
  display_name
  active
  created_at
  updated_at
```

A Human Account row is explicit. Existing generic `ActorType::User` principals are not synthesized into accounts.

Database triggers reject account bindings to non-`user` Actors and prevent a bound Human Account Actor from later being retyped away from `user`.

## Read model

`HumanAccountRepository` and `HumanAccountReadService` expose only:

- stable Account ID;
- bound Actor ID;
- display name;
- effective active state.

Effective active state fails closed when either the Account is inactive or its bound Actor is inactive/revoked.

No password verifier, credential, session secret, CSRF material or permission Grant is part of the Account read model.

## Explicit non-goals

No public `/api/v1/accounts` route is added in this slice.

Also not included:

- Account creation/update/delete HTTP APIs;
- password/bootstrap lifecycle;
- Profile persistence;
- device pairing;
- grant administration;
- security-default migration;
- Phase 70 work.

The immediate successor may expose the established read model through an authenticated/authorized Public-v1 Account contract without reading SQLite directly from HTTP code.
