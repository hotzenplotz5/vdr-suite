# Post-Phase-69 Multiuser Administration Usability

Status: **CANDIDATE**

This is a cross-cutting Multiuser productization slice after the completed MU.9
Account/access administration UI and before MU.10 Device/app pairing. It is not
MU.9G, does not reopen the accepted MU.9 contract, and does not start Phase 70.

## Problem

MU.9 made the accepted Account, Grant, Credential and Session administration
contracts reachable from the browser, but reachability alone did not make the
surface usable for a normal administrator.

The concrete product failures were:

- Grant creation required free-text `permission` input;
- Grant scope required free-text backend/scope input;
- Account, Actor, Credential and Session identifiers dominated the primary presentation;
- the surface did not explain the intended sequence from Account creation to access assignment and sign-in;
- technical security vocabulary was presented as the primary user vocabulary.

This made a technically complete administration surface hard to operate without repository/API knowledge.

## Decision

The existing Settings owner remains the administration surface. This slice changes information architecture and presentation, not security authority.

The selected Account view is organized as:

1. **Account** — active/inactive state and display name;
2. **Access** — role/permission plus where it applies;
3. **Sign-in** — active credential state;
4. **Devices** — active browser/device sessions.

A setup summary reports which of those steps are complete for the selected Account. Technical identifiers remain available under an explicit "Technical details" disclosure instead of being the primary presentation.

## Server-owned grant options

ADR-0067 remains binding: the browser must not own or invent the permission catalogue.

The existing `GET /api/v1/accounts/{accountId}/grants` response is extended additively with server-generated option metadata:

- `supportedPermissions` — the authoritative supported permission values;
- `supportedPermissionOptions` — presentation metadata for those same server-owned permission values;
- `supportedScopeKinds` — the supported scope classes.

The security service remains the source of the allowlist. The public runtime only serializes that server-owned data. The browser may localize a `presentationKey`, but it cannot make an unsupported permission selectable.

No new public endpoint or second authorization catalogue is introduced.

## Backend selection

When the server advertises backend-scoped grants, the browser obtains concrete backend choices through the existing stable Public-v1 backend collection.

The browser therefore does not ask an administrator to type a backend ID for normal operation.

Global scope is exposed only when the server advertises the corresponding scope kind.

If option discovery is unavailable, the UI fails closed for **new** Grant creation: existing Grants remain visible and revocable, but arbitrary free-text Grant creation is not restored as a fallback.

## Human-facing presentation

The browser may translate server-supplied presentation keys into human-facing labels such as "Live-TV abspielen" or "Timer erstellen". These translations are presentation only:

- they do not add permission values;
- they do not change authorization;
- unknown future server options remain representable;
- the wire permission code stays visible under technical details.

## Preserved safety contracts

This slice does not change:

- Grant desired-state mutation semantics;
- opaque Grant-set ETag / `If-Match` fencing;
- browser CSRF forwarding;
- final usable administrator protection;
- Account lifecycle fencing;
- Credential/Session revoke fencing;
- fail-closed authorization;
- audit/event semantics.

## Explicit non-goals

This slice does **not** add:

- password reset or password rotation;
- general Credential creation;
- custom role definitions or browser-owned presets;
- Device/app pairing;
- Profiles/household personalization;
- automatic authorization policy;
- Phase 70 work.

MU.10 remains the planned Device/app pairing successor after this usability slice is accepted.

## Acceptance direction

The slice is acceptable only when focused tests prove all of the following:

- the Grant permission control is a selector populated from server metadata;
- the scope control is a selector populated from server scope metadata and the Public-v1 backend collection;
- no permission catalogue is copied into `settings-account-admin.js`;
- missing option metadata cannot fall back to arbitrary free-text mutation;
- the setup guide is present;
- technical identifiers are placed behind the technical-details disclosure;
- existing MU.9 mutation/revoke/create fencing tests remain green;
- Phase 70 remains not started.
