# MU.9 — Account and Access Administration UI Closeout

Status: **COMPLETED — MU.9A-F MERGED**

MU.9 closes the browser administration surface required by ADR-0067 without
creating browser-owned authorization policy.

## Accepted implementation

- MU.9A Account Administration Read UI — PR #418.
- MU.9B Account Lifecycle Mutation UI — PR #420.
- MU.9C Session Revoke UI — PR #426.
- MU.9D Human-password Credential Revoke UI — PR #428.
- MU.9E Backend Grant Mutation UI — PR #429.
- MU.9F Account CREATE UI — PR #430.

MU.9F passed focused real-yaVDR acceptance on
`87c167c5af5817ef77d6e4f0b226aa5092c9963c`. PR CI #9764 and post-merge
main CI #9765 completed with all six jobs green. The merged main checkpoint is
`7e44578fc9771134944bc5361c6ad7897c906390`.

## Product boundary proved by MU.9

The Settings owner now exposes the accepted server contracts for Account read,
Account CREATE, display-name/activation lifecycle mutation, backend grant
ensure/revoke, Human-password Credential revoke and Session revoke. Strong
ETag/If-Match fencing remains resource-owned where required, browser CSRF is
preserved, stale preconditions are not retried implicitly, final-usable-admin
protection remains server-owned, and Account CREATE assigns no grants
automatically.

The browser does not own a permission catalog or authorization policy.

## Remaining debt is not MU.9

MU.8A safe Credential/Session metadata remains merged with its separately
documented real-runtime acceptance debt. Closing MU.9 does not erase or satisfy
that debt.

Password reset/rotation, general Credential creation, Device/app pairing,
Profiles/personalization, audit/operations product surfaces and Phase 70 are
outside MU.9. No evidence currently justifies inventing MU.9G.

## Successor rule

MU.9 completion removes the account/backend-access administration prerequisite
from the broad Timer Product UI. Device/app pairing remains the separately
planned MU.10 successor in the Multiuser workstream. Phase 70 remains a
separate not-started numbered phase and requires its own accepted runtime ADR.
