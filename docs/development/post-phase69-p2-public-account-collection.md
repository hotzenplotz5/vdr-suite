# P2 Public Human Account Collection

## Status

Implemented as the bounded successor to the accepted Human Account persistence/read foundation.

Baseline main:

```text
1d4ec2bc4e481a8abbed4ab9fdcd65f9c3c40f74
```

## Contract

The stable read-only endpoint is:

```text
GET /api/v1/accounts
```

It requires authentication plus the global permission:

```text
accounts.view@*
```

A backend-scoped `accounts.view` grant does not authorize this global collection. A wildcard-scoped administrator role may satisfy the permission through the existing AuthorizationService role vocabulary; a backend-scoped administrator role may not.

The response exposes only:

- `accountId`;
- `actorId`;
- `displayName`;
- effective `active`.

No password verifier, credential, browser-session material, CSRF material, permission-grant payload or profile state is exposed.

## Pagination

The collection follows the existing Public-v1 keyset conventions:

- default limit: 50;
- maximum limit: 100;
- stable order: `accountId asc`;
- opaque `ac1_` cursor;
- no offset pagination;
- no collection ETag.

The runtime validates strictly increasing Account IDs returned by the callback boundary.

## Ownership

`PublicApiRuntime` owns only the stable HTTP representation and callback boundary.

The daemon composes the callback from `HumanAccountReadService`. HTTP code does not query SQLite and does not synthesize Accounts from generic User Actors.

The existing Suite database remains the one Identity Authority:

```text
security_human_accounts
        |
        v
HumanAccountRepository
        |
        v
HumanAccountReadService
        |
        v
PublicApiRuntime callback
        |
        v
GET /api/v1/accounts
```

## Security

`SecurityHttpGate` classifies the Account collection as an authenticated read and evaluates `accounts.view@*` before dispatch. The decision uses the existing `AuthorizationService` and existing append-only accountability path.

Legacy compatibility wildcard access remains governed by the existing compatibility mode. This slice does not change authentication defaults.

## Public-v1 inventory

The Account collection is the ninth stable Public-v1 method/resource contract and receives explicit coverage in the existing Phase-69-derived route inventory, client-contract matrix and reference client. Historical Phase-69 per-slice cardinality guards now protect their eight-contract baseline additively rather than prohibiting future stable additions.

## Non-goals

This slice does not add:

- `/api/v1/accounts/{accountId}`;
- Account create/update/delete;
- password or credential administration;
- grants administration;
- Profiles;
- device pairing;
- first-admin bootstrap;
- security-default migration;
- Phase 70 work.
