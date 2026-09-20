# ADR-0061: Client Identity and Permission Profiles

## Navigation

- [ADR Index](index.md)
- [Platform Productization Roadmap](../planning/platform-productization-roadmap.md)
- [ADR-0013 Permission Model](ADR-0013-permission-model.md)
- [ADR-0041 Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0049 Audit and Security Event Model](ADR-0049-audit-security-event-model.md)
- [ADR-0060 Backend Catalog and Operator Onboarding](ADR-0060-backend-catalog-operator-onboarding.md)

---

## Status

Accepted architecture / implementation pending.

Date: 2026-09-20

---

## Context

VDR-Suite already enforces actor/backend/resource permissions and backend access policy. Product expansion adds Web, administrative, living-room, Kodi/mobile/API, automation, Backend Agent and HbbTV client families.

Treating a frontend name as a permission would create a second authorization model and make "running locally" accidentally equivalent to "administrator".

---

## Decision

A **frontend is not a security principal**.

Every protected operation is authorized from an authenticated actor/device/service/application context plus backend/resource policy.

UI/client profiles are product defaults and pairing/provisioning workflows over the existing authorization system; they are not alternate permission engines.

Effective access is:

```text
authenticated identity grants
  INTERSECT backend access policy
  INTERSECT resource/operation policy
  INTERSECT current lifecycle/fencing preconditions
```

### Client families

- Web browser: authenticated user/browser session; only that actor's effective grants.
- Administrative Web/CLI: explicit administrator grants; localhost/LAN does not imply admin.
- Living-room/output client: paired device and optional associated user/household actor; no implicit administration.
- Independent API/Kodi/mobile client: user/device/application identity through Phase-69 stable public contracts.
- Automation/integration: dedicated narrow service identity.
- Backend Agent: technical backend execution/observation identity only.
- HbbTV application session: bounded HbbTV session capabilities only.

UI visibility may mirror effective access but never replaces server authorization.

---

## Effective-access reporting

First-party clients should receive normalized effective-access information sufficient to present truthful controls: backend readable/writable state, granted operation families, safe denial/restriction reason categories and explicit administrative capability when granted.

This is explanatory read state, not secret/policy-graph exposure.

---

## Pairing/provisioning

Device-oriented clients use explicit pairing/provisioning rather than shared static administrator credentials.

Pairing produces revocable scoped credentials. Device revocation must not revoke unrelated user/browser/service credentials.

---

## Acceptance

Implementation is accepted when:

- the same frontend with different identities gets different effective access;
- different frontend families cannot bypass server policy;
- read-only backend policy blocks writes even for an otherwise mutation-capable actor;
- living-room device has no implicit admin access;
- Backend Agent cannot call user/admin surfaces using Agent trust;
- HbbTV cannot escape its bounded capability set;
- service identity revokes independently;
- accountability attributes protected actions to the effective identity.

---

## Consequences

New clients need identity provisioning, capability mapping and acceptance tests, not a new permission subsystem.
