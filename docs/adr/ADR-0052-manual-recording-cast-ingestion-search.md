# ADR-0052: Manual Recording Cast Ingestion and Search Integration

- Status: Accepted for implementation in Draft PR #136
- Date: 2026-08-04
- Extends: [ADR-0051](ADR-0051-manual-recording-metadata-assignment.md)

## Context

PR #135 established backend-only candidate search and Suite-owned manual recording metadata assignments. A selected movie could override the presentation read model, but its actors were not acquired or indexed. The existing person and global-search paths therefore continued to know only automatic TVScraper people, and global recording-title search primarily used the recording cache and automatic metadata.

The feature must preserve the existing ownership boundaries:

- TVScraper remains unchanged and read-only;
- the browser never communicates with TMDB;
- ordinary recording, folder and search GETs perform no provider access;
- active manual assignments remain relationship-locked and revision protected;
- superseded and withdrawn evidence remains historical;
- backend isolation, CSRF, permission and accountability contracts remain unchanged.

## Decision

### Credits acquisition point

TMDB movie credits are requested only after the user selects one concrete movie candidate. Candidate search, season discovery and episode discovery do not fetch credits. The production provider caps a selected movie cast at 128 entries, matching the existing recording-person contract.

A provider response is classified as one of:

- successful cast, including a legitimate empty cast;
- malformed or oversized payload;
- authentication, HTTP, timeout, rate-limit or transport failure.

Technical failure aborts the assignment. A successful empty cast is persisted with `cast_complete = true` and zero relations. This prevents a superficially successful assignment from silently losing actors.

### Atomic persistence

Movie assignment and cast use one `BEGIN IMMEDIATE` transaction. The transaction creates or reuses:

- immutable manual evidence;
- the selected movie entity and assignment;
- provider-qualified movie identity;
- canonical Suite-owned person entities;
- provider-qualified `tmdb/person/<id>` identities;
- normalized person values;
- assignment-scoped recording-person relations containing role, character and order.

Any failure rolls back the complete assignment. No retry-only half-state is created.

### Person identity and history

The same TMDB person is deduplicated through provider, external namespace and external ID. Several recordings may reference the same Suite-owned person entity.

Recording-person relations belong to the concrete manual assignment revision. Reassignment supersedes the former selected assignment but does not delete its people, evidence or relations. Withdrawal marks the assignment withdrawn. Only relations belonging to the currently selected, manual, relationship-locked assignment participate in active reads.

### Effective read model

The existing person-search and global-search repositories are extended; no manual-only search service or JSON shadow index is introduced.

For each recording:

- an active manual assignment supplies the effective title, original title and people;
- automatic TVScraper people are suppressed while that manual assignment is active;
- after withdrawal, automatic title and person data become effective again.

The recording detail serializer emits manual people through the existing public person shape: role, name, character name and a safe image availability object. Internal person entity IDs and provider identities are not added to that detail contract.

### Performance

All reads remain set based and backend scoped:

- folder readback loads active assignments and all people in one backend query;
- dedicated person search uses one count and one page query over an effective-person CTE;
- global search uses one count and one page query over active manual title and person CTEs;
- no query runs per recording or per person;
- no credits fetch occurs during navigation, detail GET or search;
- schema creation remains an initialization operation, not a row-loop operation;
- deterministic ordering uses cast order, folded name and provider identity tie breakers.

### Security

The existing `metadata.recording.assign` permission owns the selected-movie enrichment. The route backend remains authoritative for authorization and credential resolution. Browser CSRF and protected-operation accountability remain mandatory. Read-only and wrong-backend requests are denied before provider access.

The managed backend TMDB credential resolver is reused. Tokens, provider URLs, local paths and actor provenance are not returned by the public recording-detail contract or written into search results.

## Consequences

- Manual movies become searchable by selected title, original title and actor.
- The existing person-search result can return manual actors and their recordings.
- Restarted daemons use local Suite-owned data without repeating TMDB requests.
- Historical assignments retain full evidence and cast relations.
- Movie assignment availability now depends on successful selected-movie credit acquisition, even when the resulting cast is legitimately empty.
- Series and episode assignments remain unchanged by this movie-cast slice.

## Rejected alternatives

### Fetch credits for every candidate

Rejected because it multiplies provider requests, increases latency and rate-limit exposure, and acquires data the user may never select.

### Separate manual person index

Rejected because it would fork identity, deduplication, search and withdrawal semantics from the existing Suite metadata model.

### Persist the film first and ignore credit failures

Rejected because it creates a silent permanent partial success that ordinary GETs cannot distinguish from a genuinely empty cast.

### Fetch credits during detail or search GETs

Rejected because it violates local-read, security, restart and performance contracts.

## Validation contract

The implementation must prove:

- credit parsing, empty cast and all technical failure classes;
- atomic persistence, deduplication, restart, reassignment, withdrawal and history;
- manual title, original-title, person and connected-recording search;
- backend isolation and automatic fallback;
- Admin, Read-only, backend, CSRF and accountability behavior;
- no token, provider URL or private path leakage;
- constant set-based reads and no repeated schema DDL;
- complete repository CI on the exact final head;
- real yaVDR assignment, search, restart and withdrawal acceptance before the Draft PR becomes ready.
