# ADR-0041: Authentication, Agent Trust and Multi-Site Transport

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current Architecture State](../development/current-architecture-state.md)

---

## Status

Accepted

Date: 2026-07-15

---

## Context

VDR-Suite must support local clients, remote clients, service integrations and Backend Agents in more than one house or site.

Authentication and transport trust are different from authorization:

```text
Authentication
  Who or what is connecting?

Authorization
  Which operation may that actor perform?
```

ADR-0013 defines the permission direction. This ADR defines the identity and trust boundary required before those permissions can be enforced safely.

Existing VDR-side components are not suitable as the public security boundary:

- RESTfulAPI does not provide the required VDR-Suite user and backend policy model
- SVDRP is a local VDR control transport
- Streamdev is a media transport
- osd2web is a legacy OSD transport
- epgd and epg2vdr rely on direct shared database trust

A multi-site installation must not expose these transports directly to the public network as a substitute for VDR-Suite authentication.

---

## Decision

VDR-Suite will use separate authenticated trust paths for clients and Backend Agents.

```text
User or API Client
  -> Client Authentication
  -> VDR-Suite Client API

Backend Agent
  -> Agent Enrollment and Device Identity
  -> authenticated Agent Protocol
  -> Control Plane
```

Client credentials cannot be used as Agent credentials.

Agent credentials cannot be used as unrestricted user or administrator credentials.

---

## Client Identity

The client-facing authentication layer must support identities such as:

- local user
- remote user
- administrator
- service account
- API client
- future remote VDR-Suite federation actor

The final login provider may be local or external, but the resulting VDR-Suite actor identity must be stable and auditable.

Password-based local accounts, when supported, must use a modern password hashing scheme and must never treat a fast reusable digest as a password equivalent.

Sessions and access tokens must be:

- transmitted only over protected transport
- bounded in lifetime
- revocable
- scoped to the authenticated actor
- protected against disclosure through URLs and logs

Session identifiers must not be placed in URL paths or query strings.

Browser sessions must support appropriate cookie, origin and CSRF protections.

---

## Agent Enrollment

A Backend Agent must be explicitly enrolled before it becomes trusted.

Enrollment establishes:

```text
backendId
agent identity
site identity
credential or certificate binding
allowed protocol versions
initial policy
```

Enrollment may use a one-time pairing code, an administrator-approved request or another controlled bootstrap mechanism.

Enrollment must not automatically grant write permission to the backend for every user.

---

## Agent Transport

The Agent Protocol must provide mutual machine authentication and transport confidentiality.

The preferred production direction is mutual TLS or a security model with equivalent properties:

- Control Plane identity verification by the Agent
- Agent identity verification by the Control Plane
- encryption in transit
- integrity protection
- credential rotation
- revocation
- resistance to replay

The exact transport library is not fixed by this ADR.

Agents normally create an outbound connection from the remote site to the Control Plane.

This avoids exposing inbound VDR plugin ports at the remote site.

---

## Site Trust

A site is not implicitly trusted merely because it is reachable over a private network or VPN.

Each site and Agent must have an explicit trust record.

A compromised Agent may report false state for its own backend, but it must not automatically gain:

- database credentials
- permissions for other backends
- user credentials
- unrestricted administrative API access
- the ability to submit results for another backend identity or generation

Backend and generation checks remain mandatory even on an authenticated connection.

---

## Internal VDR Transports

The following transports stay behind the Backend Agent boundary by default:

- RESTfulAPI
- SVDRP
- Streamdev
- osd2web
- VDR plugin service interfaces
- plugin-owned databases and files

The Agent may use them locally according to capabilities and policy.

The Control Plane and clients do not receive permanent direct credentials for these transports.

A media stream is exposed later through an authenticated VDR-Suite streaming session, not by publishing a permanent Streamdev URL.

---

## Authorization Interaction

Successful authentication does not imply permission.

Every protected action still requires:

```text
actor authenticated
AND
backend capability available
AND
actor permission granted for backend and action
AND
current policy allows execution
```

Read-only backend policy is an additional denial boundary and remains effective regardless of frontend presentation.

Agent authentication proves which Agent is connected. It does not decide which user may mutate that Agent's backend.

---

## Credential Lifecycle

Credentials and certificates require lifecycle handling:

- issuance
- activation
- rotation
- expiry
- revocation
- replacement after compromise
- audit history

A revoked Agent credential invalidates the active trust relationship and lease.

Credential material must not be written to normal logs, diagnostics payloads or frontend APIs.

---

## Protocol Protection

Agent requests and results carry at least:

- backend ID
- backend generation
- agent identity
- protocol version
- message or command ID
- timestamp or deadline where applicable
- integrity-protected payload

Mutating commands additionally use the safe mutation and idempotency contract defined by a later ADR.

The Control Plane must reject:

- unknown Agents
- revoked credentials
- unsupported protocol versions
- backend identity mismatches
- obsolete backend generations
- expired commands
- replayed commands that violate idempotency rules

---

## Rules

- All production client and Agent traffic uses protected transport.
- Authentication and authorization remain separate concerns.
- Session identifiers are not embedded in URLs.
- Fast reusable password digests are not accepted as password storage or bearer credentials.
- Backend Agents are explicitly enrolled and revocable.
- Agents normally connect outbound from remote sites.
- VDR-internal ports are not public platform APIs.
- Private network reachability is not sufficient proof of trust.
- Credentials are never exposed through diagnostics or normal logs.
- A trusted Agent has authority only for its own enrolled identity and current generation.

---

## Consequences

Positive:

- enables secure access across multiple houses
- avoids public exposure of legacy VDR transports
- separates user sessions from device trust
- supports credential rotation and revocation
- limits lateral movement between backends
- gives authorization a reliable actor identity

Trade-offs:

- enrollment and certificate or token management add operational work
- browser and native clients need session lifecycle handling
- remote sites need clock and connectivity considerations
- migration from directly exposed plugin ports requires deployment guidance

---

## Non-Goals

This ADR does not choose:

- one mandatory identity provider
- one mandatory TLS library
- final password policy values
- the complete user and role database schema
- final permission grant storage
- streaming session details
- federation between independent Control Planes

---

## Related Decisions

- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
