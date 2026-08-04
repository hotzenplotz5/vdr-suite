# Manual Recording Metadata Assignment

## Status

This document owns the implementation and real-system acceptance contract for manual movie, series, season and episode assignment in Recordings 2.

The architecture decision is [ADR-0051](../adr/ADR-0051-manual-recording-metadata-assignment.md).

The feature remains incomplete until the repository tests, daemon build, install staging and one real yaVDR acceptance run have all succeeded against the same reviewed source head.

---

## User problem

A Recording may have no usable TVScraper match or may be associated with the wrong movie, series or episode. Existing read-only TVScraper integration can return an already selected match but cannot present several candidates or accept one exact selection through its public service contract.

Recordings 2 therefore needs a Suite-owned correction workflow that:

- searches a managed external catalog from the backend;
- lets the user choose one candidate explicitly;
- persists the relationship and its provenance;
- prevents later automatic evidence from silently replacing the manual choice;
- allows the choice to be withdrawn;
- restores the ordinary TVScraper/native fallback after withdrawal.

---

## Implemented flow

### Movie

```text
Recording detail
  -> Metadaten suchen
  -> Film
  -> title search
  -> bounded TMDB candidates
  -> Diesen Treffer verwenden
  -> selected locked manual assignment
  -> existing Recording detail read model
```

### Series and exact episode

```text
Recording detail
  -> Metadaten suchen
  -> Serie
  -> series candidates
  -> series directly, or Staffeln anzeigen
  -> Folgen anzeigen
  -> exact episode selection
  -> selected locked manual assignment
  -> existing Recording detail read model
```

### Withdrawal

```text
Manuell zugeordnet
  -> Manuelle Zuordnung entfernen
  -> selected assignment becomes withdrawn
  -> automatic TVScraper/native read model becomes visible again
```

---

## Ownership boundaries

- TVScraper remains unchanged and read-only from VDR-Suite.
- VDR-Suite never writes into TVScraper's SQLite database or image cache.
- The browser never calls TMDB or another catalog directly.
- The browser never receives a provider token or provider filesystem path.
- Search does not run during normal Recording GET rendering.
- Only an explicitly selected poster is downloaded into the Suite-owned cache.
- The existing Recording metadata and image routes remain the public read-model owner.

---

## Persistence contract

The selected relationship creates and retains:

- immutable `manual` evidence;
- a normalized Suite metadata entity;
- one selected `manual_assignment`;
- `relationship_locked = true`;
- one `manual-override` evidence link;
- the selected external provider identity;
- presentation values for the Recording detail;
- actor provenance from the authenticated HTTP security context;
- a monotonically advanced assignment revision.

A replacement supersedes the previous selected assignment. A withdrawal changes the assignment state but does not delete history.

The browser sends the existing backend-native Recording identity. The server resolves that identity through `vdr_recording_cache` to the canonical Suite metadata target resource key.

---

## Provider and artwork contract

The initial provider is TMDB.

Runtime configuration uses the existing managed environment variables:

```text
VDR_SUITE_TMDB_READ_ACCESS_TOKEN
VDR_SUITE_TMDB_LANGUAGE
```

Candidate search supports:

- movies;
- television series;
- seasons;
- episodes.

The selected candidate poster is materialized beneath:

```text
/var/cache/vdr-suite/recording-metadata/posters
```

The materializer enforces:

- allow-listed TMDB hosts through the existing HTTP transport;
- bounded connect and total timeouts;
- bounded response size;
- JPEG, PNG or WebP media type;
- safe provider namespace and numeric external ID;
- safe one-segment TMDB image reference;
- no symlink traversal;
- exclusive temporary creation, fsync and atomic rename;
- no TMDB credential on the image-host request.

If TMDB search credentials are not configured, the search API reports provider unavailability. Existing automatic Recording metadata remains unaffected.

---

## API contract

```text
GET  /api/backends/{backendId}/recordings/metadata/manual
POST /api/backends/{backendId}/recordings/metadata/search
POST /api/backends/{backendId}/recordings/metadata/seasons
POST /api/backends/{backendId}/recordings/metadata/episodes
POST /api/backends/{backendId}/recordings/metadata/assign
POST /api/backends/{backendId}/recordings/metadata/withdraw
```

The backend route segment is authoritative. A body field cannot change the authorization or accountability scope.

The assignment and withdrawal contracts use an expected revision. Stale browser state fails with a conflict and must not overwrite a newer decision.

---

## Security contract

Required permission:

```text
metadata.recording.assign
```

Every POST operation requires:

- authenticated actor identity;
- exact backend scope;
- the dedicated permission;
- valid browser-session CSRF evidence when a browser session is used;
- pre-dispatch authorization accountability;
- success or failure outcome accountability.

`role.admin` grants this permission for its exact backend scope. `role.read-only` blocks it.

The client cannot provide or override `created_by_ref`. The actor comes from `SecurityHttpGate` through the HTTP request context.

Search terms, candidate descriptions, tokens and private paths are not copied into accountability events.

---

## Capability contract

```text
metadata.recording.manualSearch
metadata.recording.manualAssignment
metadata.recording.manualAssignment.movie
metadata.recording.manualAssignment.series
metadata.recording.manualAssignment.episode
```

Capability advertisement describes the supported Suite workflow. Each provider request still validates its current runtime configuration.

---

## Deterministic repository validation

The feature is covered through the existing test graph by:

- manual assignment repository persistence, replacement, revision conflict and withdrawal;
- immutable evidence and `manual-override` relation;
- TMDB movie and series candidate parsing;
- season and episode parsing;
- query encoding, rate-limit retry and provider failure behavior;
- selected-poster validation, cache reuse and atomic publication;
- route and payload validation;
- API search, assignment, readback and withdrawal;
- route-authoritative authorization;
- Admin, Read-only, wrong-backend and CSRF behavior;
- authorization decision and protected-operation outcome events;
- manual Recording read-model priority;
- automatic provider fallback after unlock or withdrawal;
- Recordings 2 film, series, season, episode, assignment and withdrawal runtime contracts;
- public-base-path-safe lazy runtime loading;
- frontend and cache install staging;
- full daemon linkage.

Primary aggregate targets:

```text
make test-metadata-foundation
make test-security
make test-recordings2-runtime
make test-fast
make test-install-staging
make vdr-suited
```

Successful GitHub Actions jobs must be recorded in the pull request before real-system acceptance begins.

---

## Final real-system acceptance

Perform this once, after the complete reviewed source head has been built and installed on the yaVDR system.

### Preconditions

- the installed daemon and web assets come from the same source head;
- TMDB credentials used by the existing series-artwork integration are available to the daemon;
- an Admin browser session has the exact backend scope;
- a Read-only browser session is available for the denial check;
- at least one unmatched or deliberately incorrect Recording is available;
- one known movie and one known series episode can be identified unambiguously.

### Acceptance sequence

1. Open the unmatched Recording in Recordings 2.
2. Confirm that the normal detail view still renders before any manual action.
3. Open **Metadaten suchen oder korrigieren**.
4. Search for a known movie.
5. Confirm that several bounded candidates can be displayed when the provider returns them.
6. Select the correct movie.
7. Confirm immediately in the same Recording detail:
   - manual-assignment badge;
   - selected title and description;
   - selected provider identity;
   - selected poster when the candidate has one;
   - no direct TMDB URL in browser-visible JSON or DOM attributes.
8. Reload the browser and confirm persistence.
9. Restart the VDR-Suite daemon and confirm the same manual selection survives.
10. Select a television series for the same or another test Recording.
11. Open the season list.
12. Open one season and select the exact episode.
13. Confirm season and episode number and the selected episode title in the Recording detail.
14. Attempt a stale replacement from an older browser tab and confirm a conflict instead of silent overwrite.
15. Use a Read-only session and confirm search/assignment is denied.
16. Attempt the operation against a backend outside the actor's scope and confirm denial.
17. Confirm an authorization decision and operation outcome exist for successful and denied requests without token, description or filesystem-path leakage.
18. Remove the manual assignment.
19. Confirm the Recording detail returns to the automatic TVScraper/native metadata result or deterministic fallback.
20. Restart the daemon again and confirm the withdrawn relationship does not reappear.

### Acceptance evidence

Record:

- exact source head;
- installed package/build identity;
- backend ID;
- Recording backend-native identity with sensitive path portions redacted when published;
- selected movie provider namespace and external ID;
- selected series, season and episode IDs;
- HTTP status codes for search, assignment, stale conflict, Read-only denial, wrong-backend denial and withdrawal;
- daemon restart result;
- screenshot or browser evidence for the selected and withdrawn states;
- confirmation that provider tokens and local artwork paths were absent from public responses and audit evidence.

Do not mark the pull request ready or merge it until this acceptance has passed or the user explicitly accepts a documented remaining limitation.

---

## Rollback

Rolling back the source and package removes the UI and routes but must not destructively delete metadata history.

A subsequent compatible build may continue to read the schema. An operator who needs to stop the feature without deleting data can remove the permission or disable the provider credentials while ordinary TVScraper/native Recording reads continue to work.

---

## Related documentation

- [ADR-0051](../adr/ADR-0051-manual-recording-metadata-assignment.md)
- [TVScraper Recording Metadata Integration](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [Suite Metadata Platform Schema v1](../architecture/metadata-platform-schema-v1.md)
- [Metadata Identity Foundation](../architecture/metadata-identity-foundation.md)
- [Security and Identity Foundation](../architecture/security-identity-foundation.md)
