# MU.8A — Safe Credential / Session Metadata

Status: **IMPLEMENTATION CANDIDATE — acceptance pending.**

MU.8A is the first bounded runtime slice of ADR-0067 credential/session
administration. It deliberately exposes read-only lifecycle metadata before any
remote revoke mutation is opened.

## Public contract

```text
GET /api/v1/accounts/{accountId}/credentials
GET /api/v1/accounts/{accountId}/sessions
```

Credential reads require `accounts.credentials.view@*`.
Session reads require `accounts.sessions.view@*`.

A global `role.admin@*` may satisfy those permissions through the existing
AuthorizationService role expansion. A backend-scoped `role.admin@backend`
does not become global Human Account administration authority.

## Secret-free read model

Credential items expose only credential ID, credential type, active/expired/
revoked lifecycle flags, optional expiry and creation timestamp. Internal
`browser-session` Credentials are excluded from this collection.

Session items expose only session ID, device ID, `issuedFromCredentialId`,
active/expired/revoked lifecycle flags, expiry, optional last-seen timestamp and
creation timestamp.

The repository projection joins the canonical Session, browser-session
Credential and issuing Credential lifecycle. A revoked issuer therefore fences
the projected Session without a second lifecycle authority.

Password verifier hashes, reusable browser tokens/cookies, session-secret
hashes and CSRF-secret hashes are not selected into the MU.8A repository model
and cannot be serialized by the Public-v1 representation.

## Concurrency

MU.8A is read-only. It introduces no Credential/Session revision and no
`If-Match` contract. The Human Account revision is not reused for these
resources. Responses use `Cache-Control: no-store` and intentionally carry no
ETag.

## Mutation boundary

**No credential or session revoke mutation is part of MU.8A.**

In particular, `accounts.credentials.revoke@*` and
`accounts.sessions.revoke@*` remain later MU.8 work together with a dedicated
strong-precondition contract. Human-password revocation must also reuse the
canonical final-usable-administrator invariant and issuer-session fencing.

Remote password reset/rotation, browser administration UI, TV/app pairing and
Profiles are not part of MU.8A.

## Existing lifecycle reused

MU.8A reads the existing `security_credentials`, `security_sessions` and
`security_browser_session_credentials` authority and preserves the existing
`issued_from_credential_id` issuer fencing. No second identity/session store
is introduced.

## Acceptance

The implementation candidate is guarded by:

```text
make test-security-human-account-credential-session-read
make test-security-public-account-security-metadata
python3 tools/check_mu8a_credential_session_metadata.py
```

Real yaVDR acceptance remains pending. No MU.8 completion claim is made by this
candidate.
