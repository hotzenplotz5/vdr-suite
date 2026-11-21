# Phase 62 Slice 2R — Configurable Absolute Browser-Session Lifetime

## Status

Repository implementation, all five CI jobs and guarded real yaVDR runtime
acceptance are complete.

Accepted source/runtime head:

```text
d65af5a24688fe4dbf090030226fd45825260060
```

Accepted implementation CI:

```text
VDR-Suite CI #6661
Run ID 30715365583
All five jobs successful
```

PR #117 remains open, Draft and unmerged.

---

## Why this slice follows Slice 2Q

The fresh post-Slice-2Q HTTP inventory found no remaining unmigrated product
POST family:

- browser-session issue/logout are handled by the dedicated lifecycle gate;
- every POST registered by the central API router is either a protected mutation
  or an explicitly classified Safe POST.

A further route-migration slice would therefore be artificial. The next smallest
real Phase-62 gap was the duplicated fixed browser-session lifetime.

Before Slice 2R, browser-session issuance and the response cookie both used
`28800` seconds through separate call sites. The persistence layer already
supported bounded absolute expiry but no runtime setting selected that value.

---

## Exact Scope

Slice 2R adds exactly one optional runtime setting:

```text
VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS
```

The sole environment-variable name is
`VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS`.

Contract:

```text
default: 28800 seconds
minimum:   300 seconds
maximum: 86400 seconds
format: strict unsigned decimal digits only
```

The same canonical value controls:

1. `BrowserSessionIssuanceRequest::lifetimeSeconds` and therefore persisted
   `expires_at` values;
2. the `vdr_suite_session` cookie `Max-Age` attribute.

No query parameter, request body, actor grant or frontend value can override the
server configuration.

---

## Fail-Closed Behaviour

When the environment variable is absent, behaviour remains exactly compatible:

```text
lifetimeSeconds = 28800
cookie Max-Age = 28800
```

When the variable is present, parsing accepts only decimal digits and then
requires the inclusive range `300..86400`.

The following are invalid examples:

```text
empty value
299
86401
+3600
 3600
3600 
3600x
extremely long decimal input
```

Invalid configuration does not fall back silently and does not issue a browser
session. An already authenticated Basic caller receives a secret-free,
no-store response:

```text
HTTP 503
browser_session_lifetime_configuration_invalid
```

No `Set-Cookie` header is emitted and no session, credential or browser-token
row is created. The failure path therefore has no `Set-Cookie` header. Other
API and already-issued session behaviour is not changed by this bounded slice.

---

## Security and Persistence Boundary

The existing issuance service remains the authority for the hard lifetime
bounds:

```text
MinimumLifetimeSeconds = 300
DefaultLifetimeSeconds = 28800
MaximumLifetimeSeconds = 86400
```

`SecurityConfiguration` references those constants instead of creating a
second range. Its parser checks each digit before multiplication, so arbitrarily
long input cannot overflow an integer.

The HTTP service consumes one immutable
`BrowserSessionLifetimeConfiguration` value. It validates that value before
calling the issuance service and passes the same number to both persistence and
cookie construction.

Existing protections remain unchanged:

- CSPRNG-generated token and independent session/CSRF secrets;
- one-way secret hashes only;
- transactional session plus credential insertion;
- absolute expiry verification on every browser request;
- HttpOnly, Secure and SameSite=Strict cookie attributes;
- no cookie secret in the JSON response;
- cookie precedence, grant resolution, CSRF and logout revocation;
- secret-free accountability.

---

## Packaging Contract

The systemd environment template documents the compatible default:

```text
VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS=28800
```

Existing installations are not overwritten by `make install-runtime`, and an
absent setting retains the same default. A future full package/systemd install
makes the option visible to new installations.

---

## Repository Verification

Repository tests cover:

- default `28800`;
- custom `900` seconds;
- inclusive minimum `300`;
- inclusive maximum `86400`;
- below-minimum and above-maximum rejection;
- empty, signed, whitespace-padded and mixed-text rejection;
- overflow-safe rejection of very long decimal input;
- persisted expiry exactly 900 seconds after the fixed test clock;
- cookie `Max-Age=900` from the same configuration;
- absence of the old `Max-Age=28800` in the custom test;
- invalid configuration returning 503 without `Set-Cookie`;
- values outside the service bounds rejected even when constructed directly;
- unchanged logout and revoked-cookie behaviour.

The static architecture checker enforces:

- one environment-variable owner in `SecurityConfiguration`;
- reuse of issuance-service minimum/default/maximum constants;
- pre-multiplication overflow protection;
- one canonical HTTP-service lifetime value;
- no direct `DefaultLifetimeSeconds` use in the HTTP service;
- matching issuance and cookie values;
- strict Packaging documentation;
- continued cookie secrecy and lifecycle contracts.

GitHub Actions **VDR-Suite CI #6661**, run `30715365583`, completed with all
five jobs successful:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`;
- `packaging-regression-test`.

---

## Completed Real-Runtime Acceptance

The accepted guarded yaVDR pass temporarily set the lifetime to `900`, restarted
the daemon, issued one browser session through Legacy Basic, exercised ordinary
browser authentication and CSRF enforcement, revoked the session and restored
the original environment file.

Accepted runtime evidence:

```text
service_pid_custom_lifetime=68813
service_pid_after_restore=68893
custom_lifetime_seconds=900
runtime_http_requests=5
persisted_remaining_seconds=900
cookie_max_age=900
cookie_http_only=yes
cookie_secure=yes
cookie_same_site=strict
ordinary_browser_get=yes
missing_csrf_denied=yes
logout_succeeded=yes
session_revoked=yes
credential_revoked=yes
revoked_cookie_replay_denied=yes
accountability_issue_allowed=yes
accountability_csrf_denied=yes
accountability_logout_allowed=yes
accountability_secret_free=yes
original_runtime_config_restored=yes
original_runtime_environment_restored=yes
database_integrity=yes
service_state=active
```

Installed fingerprints:

```text
daemon: 12953babb3a2ce3aebeb99a377f66a94375bf55cf1e839cf8163bf574f4d7660
loader: 3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Evidence fingerprints:

```text
runtime report:
5fc0540f68d377c2dbce8351758fdf187527c3cb8e8538820041b224e3d9b478

database before:
35e84aa1e0b181dd425262ceeea6a65b297bfe68fd5ffe717a63d39a911de861

database after:
f6d5a57271658bca45aa0a9b30a39ee904dfa12f31c26d651206216ecdbab52f
```

Durable evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2r-20260801T202314Z-d65af5a24688/runtime-acceptance-slice2r
```

The database snapshots intentionally differ because the acceptance session and
its canonical credential rows remain as revoked lifecycle evidence. The pass
proved all three rows inactive with revocation timestamps; replay was denied.
Database quick and foreign-key checks passed before and after the test.

### Earlier guarded attempt

The earlier attempt at
`/var/backups/vdr-suite-phase62-slice2r-20260801T201619Z-d65af5a24688`
completed the product operations but used a wrapper assertion that compared the
accountability `action` column with the permission name. The runtime correctly
records:

```text
permission=session.issue.self
action=browser.session.issue

permission=session.revoke.self
action=browser.session.revoke
```

That attempt failed only in the wrapper-side assertion. Automatic rollback
passed, restored the previous daemon and original environment file, and left no
accepted evidence. It is rollback evidence, not the Slice-2R acceptance pass.

---

## Explicitly Deferred

Slice 2R does not implement:

- idle timeout or `last_seen` persistence;
- sliding expiry;
- refresh-token or session-refresh routes;
- expired-session cleanup;
- concurrent-session limits;
- user-selectable lifetime values;
- security administration APIs;
- generic roles, audit query or outcome accountability;
- Android clients;
- Phase 63-67 runtime;
- PR Ready transition or merge.

## Acceptance Gate

Slice 2R is complete because:

1. the repository diff contains only this bounded contract;
2. focused and architecture tests pass;
3. all five PR CI jobs pass;
4. the guarded custom-lifetime yaVDR test succeeded;
5. the temporary session was revoked and the original environment restored;
6. durable non-secret evidence is recorded here.

No next Phase-62 implementation slice is selected by this closeout.
