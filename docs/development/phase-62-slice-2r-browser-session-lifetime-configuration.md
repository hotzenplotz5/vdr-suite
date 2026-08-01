# Phase 62 Slice 2R — Configurable Absolute Browser-Session Lifetime

## Status

Repository implementation is prepared. Full CI and guarded real yaVDR runtime
acceptance remain pending.

PR #117 remains open, Draft and unmerged.

---

## Why this slice follows Slice 2Q

The fresh post-Slice-2Q HTTP inventory found no remaining unmigrated product
POST family:

- browser-session issue/logout are handled by the dedicated lifecycle gate;
- every POST registered by the central API router is either a protected mutation
  or an explicitly classified Safe POST.

A further route-migration slice would therefore be artificial. The next smallest
real Phase-62 gap is the duplicated fixed browser-session lifetime.

Before Slice 2R, browser-session issuance and the response cookie both used
`28800` seconds through separate call sites. The persistence layer already
supports bounded absolute expiry but no runtime setting selected that value.

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
row is created. Other API and already-issued session behaviour is not changed
by this bounded slice.

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

## Focused Verification

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

---

## Planned Real-Runtime Acceptance

Real-runtime acceptance must run only after all five PR CI jobs are green.

The guarded pass will:

1. verify exact branch/head and a clean worktree;
2. back up the installed runtime, database and current
   `/etc/default/vdr-suite-daemon`;
3. install only the runtime from the CI-accepted head;
4. temporarily set the lifetime to a bounded non-default value such as `900`;
5. restart the daemon and issue one browser session through Legacy Basic;
6. prove `Max-Age=900` and a persisted expiry approximately 900 seconds after
   issuance;
7. prove ordinary authenticated access and CSRF behaviour remain functional;
8. logout, verify revocation and deny revoked-cookie replay;
9. prove no raw cookie or CSRF secret entered accountability;
10. restore the original environment file and restart the service;
11. verify database integrity, installed fingerprints and active service state;
12. automatically restore the complete pre-test state on every failure.

The acceptance must not change grants, business data or any Phase-63+ runtime.

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

Slice 2R is not complete until:

1. the atomic repository diff contains only this bounded contract;
2. all focused and architecture tests pass;
3. all five PR CI jobs pass;
4. the guarded custom-lifetime yaVDR test succeeds;
5. the temporary session is revoked and the original environment is restored;
6. durable evidence and closeout documentation are committed.
