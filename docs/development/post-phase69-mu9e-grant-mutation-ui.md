# MU.9E — Backend Grant Mutation UI

Status: **IMPLEMENTATION MERGED — PR #429 / FOCUSED YAVDR PASS / POST-MERGE CI #9763 GREEN**

Parent workstream: [Post-Phase-69 Multiuser Productization Workstream](post-phase69-multiuser-workstream.md)

Architecture authority: [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)

Backend contract authority: [MU.7 Backend Access / Permission Grant Administration](post-phase69-mu7-account-grant-administration.md)

Predecessor: [MU.9D Human-password Credential Revoke UI](post-phase69-mu9d-credential-revoke-ui.md)

## Purpose

MU.9E exposes the already accepted MU.7 desired-state grant mutation in the
existing selected Human Account Settings surface without creating a frontend
permission authority.

It adds two views of the same stable Public-v1 `setAccountGrant(options)`
contract:

- revoke an already returned active grant tuple;
- ensure an explicitly entered permission plus backend/resource scope tuple.

## Public-v1 and concurrency contract

The canonical client sends exactly `permission`, `backendId`, and `active`
in the POST body. The browser supplies `accountId` plus the current strong
grant-set ETag as `If-Match`, keeps `credentials: same-origin`, and forwards
active browser CSRF.

Grant-set revision is independent from the Human Account ETag. MU.9E therefore
uses `grantsEtag`, never `accountEtag`, for grant mutation.

## Server-owned policy boundary

MU.7 deliberately keeps the product grant allowlist in the server-side
administration service. The current Public-v1 surface has no accepted
permission-catalog discovery resource.

MU.9E therefore does **not** hardcode that allowlist in JavaScript. Existing
grant tuples use server-returned permission/scope values. Ensure accepts an
explicit permission and scope and lets the authoritative server reject
unsupported/invalid tuples with `422`.

The administrator still requires global `accounts.grants.modify@*`.
`backendId` remains target data, not the administrator's own authorization
scope.

## Mutation safety

Each mutation:

1. uses the currently loaded grant-set strong ETag;
2. restores the browser session when available;
3. forwards browser CSRF;
4. sends exactly one `setAccountGrant(...)` request;
5. never automatically retries a stale `412`;
6. refreshes the complete selected Account overview after success or failure.

Failures stay explicit and fail-closed: `401`, `403`, final-admin `409`,
stale `412`, unsupported/invalid tuple `422`, and missing revision `428`.

## Deliberate exclusions

No Account CREATE, password setup/reset/rotation, Credential creation, pairing,
Profiles, client-owned grant policy, or Phase 70 work is part of MU.9E.
Phase 70 remains **NOT STARTED**.

## Acceptance gate

The focused candidate must prove ensure/revoke over only
`setAccountGrant(...)`, grant-set ETag ownership, browser CSRF, no mutation
retry after `412`, visible fail-closed errors, full selected-Account refresh,
absence of a copied browser permission allowlist, MU.9A-D regressions, frontend
ownership, phase consistency, and no Daemon build/install/restart/runtime
mutation.


## Merge and post-merge closeout

Focused real-yaVDR acceptance passed on implementation head
`13a387c66c777585f7f6207da1478af30a9419f9` without daemon installation,
restart or runtime mutation. PR #429 merged that accepted tree to `main` as
`0bb84d5702508110960b4f8004f3fc9a31a1a744`.

GitHub-hosted runner degradation cancelled queued jobs before they received a
runner during the initial CI attempts; those cancellations are not product-test
failures. Post-merge VDR-Suite CI run #9763, final attempt 3, completed
successfully on the merge commit with architecture, packaging, Make-audit,
frontend, fast-regression/daemon-build and documentation jobs green.

MU.9E owns no successor status. MU.9F Account CREATE UI is selected separately
as the next bounded browser-administration slice.
