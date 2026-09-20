# ADR-0060: Backend Catalog and Operator Onboarding

## Navigation

- [ADR Index](index.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Platform Productization Roadmap](../planning/platform-productization-roadmap.md)
- [Phase 57 Local Server Permission Model](../planning/phase-57-local-server-permission-model.md)
- [ADR-0039 Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040 Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041 Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)

---

## Status

Accepted architecture / implementation pending.

Date: 2026-09-20

---

## Context

VDR-Suite already has `BackendNode`, `BackendRegistry`, backend-scoped caches and policy, secure Backend Agent lifecycle and Phase-64 multi-backend Timer orchestration.

Those foundations allow multiple backends to exist technically, but they do not yet define a complete operator-owned product workflow for creating, editing, disabling, retiring and selecting backends.

A hidden assumption that "the first runtime context is the backend" is incompatible with a durable MultiBackend product.

---

## Decision

VDR-Suite will maintain a **durable server-owned backend catalog** as the product authority for configured logical backends.

A logical backend has stable identity independent of host name/IP address, current Agent instance, current provider, current backend generation and list/registry iteration order.

The catalog owns operator configuration and lifecycle. Backend Agent state owns technical execution/observation lifecycle. Actor/device/service grants own authorization. These concerns are related but not interchangeable.

### Operator lifecycle

Supported administration must include create, validate, persist, optional Agent binding/enrollment, health/capability verification, enablement, explicit default/preferred selection where needed, and separate actor/device/service grants.

Later administration includes update, enable/disable, Agent rotation, and retire/remove under safety checks.

### Explicit selection

Where a product flow needs a default backend, it uses explicit durable default/preferred-backend state.

Forbidden substitutes include `backendRuntimeContexts_.front()`, first registry entry, lexical backend ID order or initialization order.

### Removal and disable semantics

Disable prevents new work according to policy but does not erase historical provenance.

Removal must fail closed while unresolved backend-owned resources or operations require the backend identity for reconciliation, audit or durable references.

### No automatic grants

Creating/enabling a backend grants no actor, device, application or service permission by itself.

Agent enrollment grants only the technical execution/observation authority defined by ADR-0039/0040/0041.

---

## API/admin direction

The administration contract must support protected operations equivalent to list/read, create, update, enable/disable, set/clear default/preferred role, inspect technical state and retire/remove when safety preconditions permit.

Mutation endpoints require authenticated administrator authority, CSRF/idempotency/revision semantics where applicable, accountability and server-side validation.

Public API shape is finalized under Phase 69; this ADR fixes ownership and safety now.

---

## Acceptance

Implementation is accepted only with a real two-backend proof:

- persistence across restart;
- independent capabilities/health/generation;
- different read-only/read-write policy;
- different actor/device grants;
- deterministic explicit default/preferred selection;
- no `front()` or registry-order product behavior;
- blocked unauthorized/read-only mutations before native dispatch;
- no silent provider/backend fallback for already-owned work;
- safe disable/retire behavior with unresolved operations;
- provenance preserved in cross-backend results.

---

## Consequences

MultiBackend becomes an operator feature rather than a constructor/config accident. Logical backend identity survives host/Agent replacement, and explicit default behavior replaces positional assumptions.

The trade-off is durable catalog schema/migration/admin responsibility and routine multi-backend testing.

---

## Non-Goals

This ADR does not define cloud discovery, automatic LAN scanning, implicit trust of discovered VDRs, public API version details or end-user permission profiles. Client permission profiles are owned by ADR-0061.
