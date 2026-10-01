# P2 First-Admin Bootstrap Boundary Audit

## Baseline

Audited against live `main`:

```text
2937873b856c5f802ef5030504cedff66aee8e22
```

The Public-v1 Account collection is already on `main`. This slice deliberately does not modify its runtime, API, client or Phase-69 compatibility surfaces.

## Current deployment facts

The packaged daemon unit sources:

```text
EnvironmentFile=-/etc/default/vdr-suite-daemon
```

The packaged defaults file contains operational daemon settings but no first-admin claim lifecycle.

`SecurityConfiguration` still falls back to:

```text
VDR_SUITE_SECURITY_MODE -> legacy-basic
```

and the compatibility credential remains available when no migration configuration is supplied.

## Current persistence facts

The Suite security database already owns:

- Actors;
- Human Accounts;
- Devices;
- Sessions;
- Credentials;
- Basic credential verifiers;
- permission grants;
- accountability events.

`SecurityIdentityProvisioningRepository` can create identity rows and `CredentialVerifierRepository` persists one-way password verifiers.

These are reusable primitives, but neither repository defines first-admin claim ownership, single-use bootstrap lifetime, claim atomicity or local recovery policy.

## Missing product lifecycle

Current main does not provide a canonical lifecycle equivalent to:

```text
fresh install
 -> unclaimed
 -> local operator bootstrap issuance
 -> short-lived setup proof
 -> first Human Account + admin grants
 -> claimed
 -> bootstrap permanently invalid
```

There is also no canonical local Human Account recovery contract that can replace a lost credential without revealing the stored verifier.

## Root cause

The remaining blocker to retiring the compatibility default is not missing password verification.

It is missing lifecycle authority around:

- whether the server is claimed;
- who may issue setup authority;
- how bootstrap material expires and becomes single-use;
- how first Human Account creation is atomic;
- how administrator grants are established;
- how an operator recovers access;
- how upgrades avoid locking out existing installations.

Therefore changing `legacy-basic` today would be a deployment behavior change without a safe replacement path.

## First justified runtime successor

With ADR-0066 accepted, the smallest runtime slice is persistent claim/bootstrap state in the existing Suite database with:

- explicit unclaimed/claimed determination;
- short-lived bootstrap verifier metadata;
- single-use/expiry state;
- no plaintext bootstrap secret persistence;
- no HTTP endpoint yet;
- no authentication-default change yet.

The local issuance command and atomic first-admin claim service follow as separate slices.


## Implemented persistence foundation

This slice adds `FirstAdminBootstrapRepository` to the existing Suite security
database and initializes its schema from the daemon runtime.

The repository keeps bootstrap material separate from normal identity
credentials. It stores only a bootstrap identifier, a one-way verifier hash,
expiry and terminal lifecycle timestamps. It creates no normal Session or
Credential row and therefore does not turn bootstrap proof into reusable API
authority.

Server claim state remains derived from the existing persistent identity/account
authority: an active Human Account bound to an active `ActorType::User` actor
with global administrator authority (`role.admin@*` or the existing direct
wildcard grant). There is no second persisted claim-state flag.

Registration is fenced against an already-claimed server, rejects already
expired material and permits at most one effective unconsumed/uninvalidated
bootstrap issuance at a time. Lookup becomes fail-closed once the server is
claimed.

Consumption and invalidation require a caller-owned SQLite transaction. This is
intentional: the later atomic first-admin claim service can consume bootstrap
proof and create the Human Account, normal human credential, grants and
accountability evidence inside one transaction; rollback restores the
pre-claim bootstrap state instead of leaving a partial admin.

The Legacy Basic default remains unchanged. This slice adds no HTTP endpoint,
no local issuance command, no first-admin account mutation and no recovery
workflow.

## Implemented local trusted-operator bootstrap issuer

This slice adds the local trusted-operator bootstrap issuer without opening any
remote or browser-facing authority.

`FirstAdminBootstrapIssuanceService` generates a 128-bit bootstrap identifier,
a 256-bit raw setup secret and an independent verifier salt from Linux
`getrandom(2)`. The raw setup secret is returned only in the successful
in-memory issuance result; the repository receives only the bootstrap ID,
one-way verifier hash and expiry.

The installed `vdr-suite-first-admin-bootstrap` command is root-only and uses
the existing Suite security database by default. It prints the raw setup secret
once to the local operator, then wipes its in-memory result. The command writes
no secret file and adds no HTTP endpoint, browser session, normal credential or
permission grant.

Bootstrap lifetime is deliberately short and bounded to 300..3600 seconds with
a 900-second default. Existing repository fencing remains authoritative for an
already-claimed server and for an already-active bootstrap issuance.

The Legacy Basic default remains unchanged.

## Implemented atomic first-admin claim service

This slice adds the internal atomic first-admin claim service while keeping the
browser and HTTP boundary closed.

`FirstAdminClaimService` accepts the short-lived bootstrap proof plus the
requested first Human Account login, password and display name. It generates
opaque account, actor, credential and accountability identifiers from Linux
`getrandom(2)`. New human passwords are salted and hashed with yescrypt
(`$y$`) before persistence; plaintext passwords and bootstrap secrets are
cleared from the service-owned request copy and are never stored.

The service acquires the existing per-database transaction lease and opens one
`BEGIN IMMEDIATE` SQLite transaction. This is the single SQLite transaction
that owns the complete claim. Inside it the service:

1. proves the server is still unclaimed;
2. loads and verifies the unexpired, unconsumed bootstrap verifier;
3. consumes the bootstrap proof;
4. creates or binds the `ActorType::User` actor and normal
   `human-password` credential through the existing identity repository
   authority;
5. creates or binds the explicit Human Account through
   `HumanAccountRepository`;
6. stores the one-way credential verifier;
7. grants `role.admin@*`;
8. appends the successful claim accountability event; and
9. commits.

Bootstrap consumption intentionally happens before the administrator grant:
once that grant exists the canonical claim-state projection becomes
`claimed`. Because all writes are in the same transaction, any later failure
rolls back the bootstrap consumption together with actor, account, credential,
verifier, grant and accountability writes. Regression coverage forces an
accountability persistence failure after the earlier mutations and proves that
no partial administrator survives.

The service adds no direct identity-table SQL outside the existing repository
owners and creates no parallel identity authority.

The Legacy Basic default remains unchanged.

## Claim-only browser completion boundary

Live inspection of the existing browser-session authority makes the smallest
correct browser-completion slice a claim-only boundary.

`BrowserSessionIssuanceService` requires an existing active Actor, active
Device owned by that Actor, and an active issuing Credential. The atomic
first-admin claim deliberately creates the Human Account, `ActorType::User`,
normal `human-password` Credential, one-way verifier and `role.admin@*`, but
it does not create a Device. The existing browser-session login gate also
authenticates Legacy Basic or Managed Basic; it does not yet authenticate the
new Human Account password.

Creating a browser session in this slice would therefore require Device
provisioning and/or a new human-password authentication bridge in addition to
the claim boundary. That would couple two later authorities into the bootstrap
claim instead of delegating cleanly to the established session machinery.

The implemented boundary is therefore:

```text
POST /api/security/first-admin/claim
```

It is a distinct security-lifecycle route, not Public-v1 Account CRUD. The
route is intercepted before the normal Browser Session and Security HTTP gates
and is the only HTTP path that accepts bootstrap material. It parses a bounded
JSON body, preserves the existing request-ID contract, keeps correlation ID
optional, and delegates identity mutation to `FirstAdminClaimService`.

Successful completion returns only a secret-free claimed result.
It does not create a Browser Session, issue a cookie, emit CSRF material, create a Device
or convert bootstrap proof into any reusable API credential. Wrong, missing,
expired, consumed or invalidated bootstrap proof fails closed. Once claim
state is derived as claimed, replay and any later first-admin claim fail.

The HTTP layer contains no direct identity SQL and creates no second claim,
Account or Identity authority. The existing atomic rollback boundary remains
inside `FirstAdminClaimService`; the browser route cannot expose partial
identity state.

The pre-existing claim service required a non-empty correlation ID even though
the Phase-62/69 HTTP contract defines correlation as optional. This slice
relaxes only that validation to permit an empty correlation ID; request ID
remains mandatory and is generated by the HTTP boundary when absent or invalid.

The Legacy Basic default remains unchanged. No package setting, recovery path,
Profile, pairing, Public-v1 Account mutation or Phase-70 behavior is added.

## Next bounded slice

The next justified slice is normal human-password browser authentication and
the Device/session bridge needed to delegate post-claim login to the existing
`BrowserSessionIssuanceService`, cookie and CSRF authorities. That slice must
remain independent of bootstrap proof: bootstrap material is already consumed
and must never become a session credential.

Local audited recovery and authentication-default migration remain later
ADR-0066 slices.

## Human-password browser session bridge

The browser completion path now continues from a successful claim without
reusing bootstrap proof.

The existing `POST /api/security/browser-sessions` contract remains the
credential-exchange boundary. Human Account passwords use the existing bounded
HTTP Basic transport on that exact session-issuance route; this slice does not
add a reusable password JSON endpoint and does not authorize human passwords on
ordinary application/API routes.

`HumanPasswordBrowserAuthenticator` accepts only a persisted verifier whose
canonical credential has type exactly `human-password` and belongs to an
active explicit Human Account. A known Human Account login owns its password
decision on the session route: a wrong human password fails closed and does not
fall through to Legacy Basic. Managed Basic and Legacy Basic remain available
for logins that are not Human Account credentials, so this slice does not change
the packaged authentication default.

After successful password verification the authenticator idempotently ensures
one logical browser-login Device for the Human Account in the existing
`security_devices` authority. The Device identifier is derived from the
Account identifier as `human-browser-<accountId>`; it is non-secret and carries
no pairing approval, permission grant or independent trust elevation. This
logical Device exists only to satisfy the already accepted canonical
Actor/Device/Session ownership model for browser-session issuance. Future
TV/app pairing remains a separate device lifecycle.

The Device is provisioned lazily rather than added to the bootstrap claim.
Therefore Human Accounts already claimed by the previous claim-only slice can
authenticate after upgrade without rewriting claim history. A revoked login
Device remains revoked because idempotent provisioning never reactivates it.

Once Actor, Human Account, human-password Credential and logical browser Device
are valid, the existing `BrowserSessionHttpGate` delegates unchanged to
`BrowserSessionHttpService` and `BrowserSessionIssuanceService`. The existing
session authority still generates the browser Session, browser Credential,
hardened cookie and one-time CSRF token atomically and records the
human-password Credential as the issuing credential. Existing issuer lifecycle,
expiry, revocation, concurrency, idle and retention contracts therefore remain
authoritative.

Bootstrap identifiers and setup secrets are not accepted by this path and never
become a Session, Device credential, cookie or CSRF value. Human passwords are
verified only against one-way yescrypt/SHA-512-crypt compatible verifiers and
are not persisted or returned.

The Legacy Basic package default remains unchanged. This slice adds no Account
CRUD, grant administration, Profile, pairing, recovery, Public-v1 expansion or
Phase-70 work.

## Next bounded slice after browser completion

The ADR-0066 trusted browser completion flow is now complete at the backend
security boundary: claim and normal post-claim session issuance are separate and
bootstrap proof is consumed before normal authentication begins.

The next justified ADR-0066 slice is local audited Human Account recovery.
Authentication-default migration remains later and still requires explicit
upgrade/rollback behavior.


## Implemented local audited Human Account recovery

Live inspection after the human-password browser-session bridge showed that
local recovery can stay entirely inside the existing Suite identity,
credential-verifier, browser-session and accountability authorities. It needs no
remote recovery endpoint and no second recovery-token store.

The first implementation used an in-place verifier reset on the same active
`human-password` credential. A deeper concurrency audit showed that this was
insufficient. `HumanPasswordBrowserAuthenticator` verifies the password before
`BrowserSessionHttpService` delegates to `BrowserSessionIssuanceService`.
Issuance opens its own transaction later and re-validates the issuing credential
lifecycle, but it does not re-run password verification. An in-flight request
that had authenticated with the old password could therefore issue a new browser
session after an in-place verifier reset because the same issuing credential
would still be active.

The corrected recovery contract is local credential rotation. The root-only
`vdr-suite-human-account-recover` command selects an explicit existing Human
Account and login, reads the replacement password without accepting password
plaintext as a command-line argument, and delegates all identity mutation to
`HumanAccountRecoveryService`.

The service requires the selected account to remain active and bound to
`ActorType::User`. The selected verifier must resolve to the same actor's
active, unexpired, unrevoked credential with type exactly `human-password`.
Inside one `BEGIN IMMEDIATE` transaction it:

1. creates a replacement `human-password` credential for the same Actor through
   the canonical identity repository and records the predecessor in
   `rotated_from_credential_id`;
2. revokes the predecessor credential, fencing any stale old-password
   authentication that has not yet reached session issuance;
3. moves the selected login verifier to the replacement credential while
   replacing its hash with a new salted yescrypt verifier;
4. revokes every still-active browser-session row whose
   `issued_from_credential_id` is the predecessor, plus the corresponding
   canonical Session and browser Credential; and
5. appends secret-free successful recovery accountability evidence.

The Human Account ID, bound Actor ID and permission grants remain unchanged. The
old credential is retained only as revoked history; the replacement credential
is the new normal login authority. A stale password-authenticated request that
carries the predecessor credential ID is rejected by
`BrowserSessionIssuanceService`, while a fresh login resolves the replacement
credential.

Expected unknown, inactive or mismatched target failures append denied,
secret-free evidence. The accountability actor is
`system:human-account-recovery` with `local-root` authentication state rather
than the target user, matching the existing local administration pattern. If
credential rotation, verifier movement, session revocation or successful
accountability persistence fails, the whole transaction rolls back, including
creation of the replacement credential.

There is no remote recovery endpoint, no Public-v1 recovery mutation, no
Bootstrap-secret reuse, no new Human Account, no new administrator grant and no
authentication-default change. Legacy Basic remains unchanged.

Authentication-default migration is the next ADR-0066 slice and still requires
a separate fresh-install, existing-install upgrade and rollback contract.


## Authentication-default fresh-install migration

The recovery slice closes the final prerequisite named by ADR-0066 for changing
fresh-install deployment defaults. The migration is intentionally narrower than
removing Legacy Basic from the codebase.

For a fresh install, the packaged
`/etc/default/vdr-suite-daemon` now contains:

`VDR_SUITE_SECURITY_MODE=enforced`

This is a deployment default, not a second security-mode authority.
`SecurityConfiguration` remains the runtime parser and still maps
`enforced` to the existing fail-closed identity model.

Upgrade behavior is deliberately different from fresh install behavior.
`install-systemd` uses create-only semantics for the daemon defaults file:
an existing `/etc/default/vdr-suite-daemon` is preserved byte-for-byte.
Therefore an existing installation that does not yet carry an explicit mode is
not silently migrated by package installation and continues to use the existing
code fallback.

That code fallback intentionally remains `legacy-basic`. Changing it to
`enforced` would make a missing or older defaults file an implicit upgrade
migration and could lock out an existing operator. The compatibility fallback
is therefore retained until the later full Legacy Basic retirement milestone.

Rollback is explicit and bounded: set
`VDR_SUITE_SECURITY_MODE=legacy-basic` in the existing deployment defaults and
perform the normal daemon restart. No identity database rollback is required;
Human Accounts, credentials, grants, sessions and accountability history are
not deleted or rewritten by the mode change.

Packaging regression proves both sides of the boundary: an empty staging root
receives the enforced fresh-install default, while a second install-systemd run
over a pre-existing defaults file preserves an explicit legacy-basic value and
sentinel instead of replacing it.

The claim route remains isolated before the normal authentication gates, and the
human-password browser-session bridge remains the post-claim normal login path.
This slice adds no new endpoint, credential store, Account authority, Profile,
pairing or Phase-70 behavior.

Full deletion of Legacy Basic compatibility is not claimed here. The roadmap's
deployment-retirement milestone still requires explicit migration of remaining
existing deployments and real runtime rollback evidence.


## Enforced-mode Legacy Basic runtime fence

The fresh-install migration exposed one remaining compatibility ambiguity before
real deployment acceptance could be meaningful. Historically,
`SecurityConfiguration` cleared the built-in compatibility header when
`VDR_SUITE_SECURITY_MODE=enforced`, but then read `VDR_SUITE_BASIC_AUTH` and
the remaining `VDR_SUITE_LEGACY_BASIC_*` values afterward. The
`LegacyBasicAuthenticator` also authenticated solely from the configured
header without checking the selected security mode. An older deployment carrying
an explicit Legacy Basic header could therefore select `enforced` while still
retaining Legacy Basic authentication.

This slice closes that ambiguity without deleting compatibility rollback:

- explicit `enforced` ignores all Legacy Basic credential/identity/grant
  environment inputs;
- `LegacyBasicAuthenticator` authenticates only when the selected mode is
  `LegacyBasicCompatibility`;
- the daemon's existing compatibility-identity provisioning condition remains
  tied to a non-empty compatibility header, so enforced startup no longer
  provisions a Legacy Basic identity from stale environment values;
- browser sessions, Human Account password login and optional Managed Basic keep
  their existing independent authentication paths;
- explicit `legacy-basic` mode still consumes the compatibility variables, so
  rollback remains available until the retirement milestone is accepted.

Regression coverage proves stale Legacy Basic configuration cannot authenticate
ordinary protected requests or the browser-session issuance route in enforced
mode, while browser-session and Managed Basic authentication continue to work.

The code fallback when `VDR_SUITE_SECURITY_MODE` is absent remains
`legacy-basic` for existing installations that have not yet been deliberately
migrated. The next retirement gate is therefore real yaVDR acceptance of an
existing claimed installation across `legacy-basic -> enforced -> legacy-basic`
rollback, followed by the deliberate final migration to `enforced`. Only after
that evidence is recorded is deletion of the compatibility implementation
justified.


## Legacy Basic retirement real-runtime acceptance tooling

The remaining deployment gate now has a dedicated guarded runner rather than a
manual sequence of configuration edits and daemon restarts.

`tools/p2_legacy_basic_retirement_acceptance.py` is root-only for real
execution and requires an exact clean repository head, exact remote ref, expected
installed and candidate daemon fingerprints, the current configuration
fingerprint, current service PID and the exact hosted-CI run identifiers before
it can mutate the system.

The acceptance sequence is deliberately
`legacy-basic -> enforced -> legacy-basic -> enforced`:

1. verify the existing deployment is still effectively in the compatibility
   mode and classify the production security state as pre-P2, unclaimed or
   claimed instead of assuming `security_human_accounts` already exists;
2. for a claimed deployment, resolve exactly one selected active Human Account
   administrator and verify its interactively entered password against the
   persisted verifier before mutation; for a pre-P2/unclaimed deployment,
   require explicit `--bootstrap-first-admin`, fingerprint the exact candidate
   bootstrap issuer and collect the new First Admin login/display/password
   interactively;
3. install only the exact candidate `vdr-suite-daemon`, leaving VDR and the
   Backend Agent untouched; this candidate startup initializes the P2 schema on
   an older database;
4. when the server was pre-P2/unclaimed, issue root-only short-lived bootstrap
   material and atomically claim the First Admin through the existing claim
   endpoint, then establish the persistent identity fingerprint;
5. under the legacy baseline, prove both Legacy Basic protected access and a
   complete Human Account browser-session issue/read/logout round trip;
6. set only `VDR_SUITE_SECURITY_MODE=enforced`, restart only
   `vdr-suite-daemon.service`, prove the legacy credential now receives HTTP
   401, and prove Human Account login/read/logout still succeeds;
7. set the mode explicitly to `legacy-basic`, restart only the daemon and
   prove the compatibility credential is restored while Human Account login
   still works;
8. set the mode back to `enforced`, prove the final legacy denial and Human
   Account success again, and leave the successful deployment in enforced mode.

The runner preserves every non-mode line in the existing defaults file,
including stale compatibility inputs, so the enforced check proves that the
runtime fence from the previous slice is effective rather than merely proving
that the old secret was deleted from configuration.

It fingerprints the selected persistent Human Account, Actor,
`human-password` credential, verifier and grants before and after the
transitions. Browser-session rows and accountability evidence are intentionally
excluded because the acceptance itself creates and revokes sessions. Database
integrity validation is scoped to the existing `security_*` tables plus
`accountability_events`; the runner uses partial SQLite quick-check and
foreign-key checks instead of scanning the complete multi-gigabyte production
database.

No production database snapshot is restored. That avoids overwriting legitimate
concurrent state. On any acceptance failure, the runner instead restores the
exact pre-run daemon binary and exact pre-run defaults-file state, restarts only
the daemon and records whether that failure restoration succeeded. On success it
keeps the exact candidate daemon installed and the deployment explicitly
`enforced`.

The runtime evidence report contains identifiers, fingerprints and status codes,
not the Human Account password, Legacy Basic authorization header, browser
cookie, CSRF secret or password verifier.

The supported real yaVDR deployment completed the guarded sequence successfully
on 2026-10-01. Accepted runtime evidence was produced on acceptance head
`716dbbdceb95aa9c6ea93e169df2ac7364a65be7` with candidate source head
`b811633c71695fc770feb35270b55969b46711b7` and candidate daemon SHA-256
`6ebbcf9385f50f041490ae26bde590e16237119addffff8d2de168f8bca1cc14`.
The compatibility probe returned 404 at baseline, 401 in enforced mode, 404
after explicit rollback and 401 again after the final enforced transition.
Human Account login passed throughout, the persistent identity fingerprint was
unchanged, and the deployment was left explicitly in `enforced` mode.

The retained root-only evidence directory is
`/var/backups/vdr-suite-legacy-basic-retirement-20261001T060257Z-716dbbdceb95`;
the runtime report SHA-256 is
`9ada9b32ecfffe1c714010c23c4fc84586775f23834fb3abbdfc5d2e56f53ea1`.
This satisfies the real deployment migration/rollback gate. Transitional Legacy
Basic implementation deletion was then executed as the next bounded slice.

## Legacy Basic runtime implementation removal

After the accepted real-yaVDR evidence above, the transitional runtime authority
is removed rather than merely fenced by `enforced` mode.

The removal deletes `LegacyBasicAuthenticator`, removes Legacy Basic/enforced
mode parsing and all `VDR_SUITE_BASIC_AUTH` /
`VDR_SUITE_LEGACY_BASIC_*` runtime inputs, stops startup compatibility identity
provisioning, removes browser/general HTTP fallback and removes HbbTV
compatibility-grant injection. Optional Managed Basic and Human Account/browser
session authentication remain separate unchanged authorities.

Fresh package defaults no longer emit `VDR_SUITE_SECURITY_MODE`. Existing
`/etc/default/vdr-suite-daemon` files remain package-preserved, so historical
Legacy Basic lines can survive an upgrade, but they are inert. The old
migration/rollback runner and retained evidence document the gate that justified
deletion; they do not describe a currently available runtime rollback after
removal.

Regression coverage retains the former Legacy Basic Authorization header as a
negative credential probe so the removed secret cannot silently regain
authentication authority.



The first real yaVDR execution exposed an upgrade-shape assumption before any
runtime mutation: the installed production database did not yet contain
`security_human_accounts`, so the original runner raised a SQLite
`no such table` exception during read-only preflight. The corrected contract
treats that as a supported pre-P2 state. It never infers "unclaimed" merely from
a missing eligible credential; when the Human Account schema exists it uses the
same Account/Actor/admin-grant authority shape as
`FirstAdminBootstrapRepository::claimState()` to distinguish claimed from
unclaimed, then separately resolves the eligible Human Account credential.
