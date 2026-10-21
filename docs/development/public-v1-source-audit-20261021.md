# Public API v1 – Source-Inventur und Umsetzungsstart (2026-10-21)

**Basis:** `main` bei `5addcc916a39a48bd59f099c076871c1ae1a8d2b` (PR #504 / #505 gemergt). Dieser Prüfpunkt ergänzt die [historische Coverage-Roadmap](../planning/public-v1-coverage-roadmap.md), ersetzt aber keine Hardware-/Release-Abnahme. Ältere Aussagen der Roadmap (z. B. zu fehlender TimerIntent-Submit-Registrierung oder fehlendem Device-SSE) sind nicht mit diesem aktuellen Stand gleichzusetzen.

## Evidenzdisziplin

- **Implementiert:** tatsächlich geroutete `/api/v1`-Funktion und Runtime-Wiring.
- **Teilweise:** v1-Route vorhanden, jedoch Identität/Fehler/Response/Security/Client-Vertrag unvollständig.
- **Legacy:** Funktion liegt nur auf `/api` oder `/api/vdr`; **intern:** vorhandener Dienst ohne stabile v1-Route.
- **Getrennt berichten:** Code, Tests, CI am exakten Commit, echte yaVDR- und VIDAA-Abnahmen sowie OpenAPI-Abdeckung. Merge oder Unit-Test ist keine Live-Abnahme.
- `api/openapi.json` ist OpenAPI 3.1 **partial**: 45 Pfadvorlagen / 58 Operationen am Baseline-Commit. `tools/check_public_v1_openapi.py` überprüft ausgewählte verifizierte Routen, keine vollständige Live-/Schema-Conformance.

## Nachgewiesene Coverage und Lücken

| Bereich | Stand Public API v1 | Lücke und Priorität |
| --- | --- | --- |
| Root/Discovery/Backend/Capabilities | `GET /api/v1`, `/capabilities`, `/backends` | Verfeinerte Adapter-/Capability-Verträge P1 |
| Channels | `GET /api/v1/channels`, Provider/Gruppen/partielle Föderation | Logo-Delivery und langfristige Channel-ID-/Gruppen-Semantik P2 |
| EPG | Now/Next, Window, Event-Detail, Suche, Genre-Events | Window-Fortsetzung, Meta/Artwork, ETag/OpenAPI-Parität P1 |
| Recordings | `/recordings` inkl. Browse (`fld1_`), Suche, Genres und persistente `rec_`-IDs | v1 Action-Identität P0; Detail/Metadaten P1 |
| Recording Mutations | `POST /api/v1/recordings/actions/validate` und `execute` als gemeinsame Legacy-Controller | Public-ID Resolver, native Pfad-Parameter, Upstream-Leaks, Problem Details, Move-Rebind P0 |
| Recording Marks/Cut | Nur `/api/vdr/recordings/marks` und `/cut` | separate v1-Acceptance P2 |
| Metadata | vorhandene VDR-/TVScraper-, TMDB-/Manual-Resolver, Genres/Personen/Artwork | sichere versionierte Recording-/EPG-Projektion P1 |
| Timer | native READ/detail, MODIFY/DELETE, Assignment, Operation, EPG TimerIntent/Submit-Code | breite Produktiv-/Unbound-/Conflict-Abnahme und Agent/Reconciliation P1 |
| SearchTimer | `GET/POST /api/v1/search-timers`, Draft/Rule Preview | MODIFY/DELETE/EXECUTE noch Legacy P2 |
| Playback | Recording- und Live-Device-MediaSessions, Tracks, Untertitel, Media-Gateway | persistenter Resume und vollständige TV-Abnahme P1 |
| Accounts/Devices | Human Accounts, Pairing, Grants, CSRF, Revoke, Credential Rotation | gesamte OpenAPI-/Security-Conformance P1 |
| Operations/Events | persistente Operation `GET`; Device-SSE `GET /api/v1/events` im Source mit HTTP-Hub | endgültige Hisense-Abnahme offen P1 |
| Add-ons | kein Registry-Code auf diesem `main`-Baum verifiziert | separat bestehende Add-on-Arbeit, spätere Rechte-Anbindung; TVScraper **kein** Add-on P2 |
| Contract | OpenAPI 3.1, Problem Details in nativer Public Runtime | Actions-Aliase und fehlende Specs/Methoden/Gates P0 |

**Wichtige Nachweise:** `api/rest/src/PublicApiRuntime.cpp`, `api/rest/src/ApiRouter.cpp`, `core/security/include/SecurityHttpGate.h`, `core/daemon/src/DaemonRuntimeInitialization.cpp`, `api/openapi.json`, `core/recordings/src/PublicRecordingIdentityRepository.cpp`, `core/recordings/src/PublicRecordingPlaybackTargetResolver.cpp`, `core/vdr/src/EpgScraperMetadataPublicJsonSerializer.cpp`, `core/vdr/include/DeviceLiveEventHub.h` und zugehörige Tests. `registerEpgTimerIntentSubmit` und `GET /api/v1/events` existieren im gegenwärtigen Source. Der ältere Entwurfsstand in `docs/development/public-v1-device-event-stream-web-parity-gap-20261017.md` ist nicht mehr die aktuelle Implementierungswahrheit.

## Aufnahmeidentität: konkreter Architekturbruch

- `PublicRecordingIdentityRepository` persistiert `(backend_id, native_id) -> rec_<32hex>`. `PublicRecordingPlaybackTargetResolver` nutzt dieselbe Zuordnung mit backend-spezifischem aktuellen Cache-Read und nachfolgendem Binding-Recheck. Kein zweites Identity Repository anlegen.
- `RecordingActionValidationRequestParser` akzeptiert weiterhin `backendNativeId`/`recordingPath`; `RecordingActionExecutionController::resolveBackendNativeId` nutzt bislang Legacy `VdrRecording::id` und Snapshot, **nicht** konsequent das persistente `rec_`-Mapping.
- `RecordingActionExecutionResultJsonSerializer` exponiert Backend- und Upstream-Details. Validierung/Execution liefern das Legacy-DTO und sind keine vollständig versionierte Problem-Details-Grenze.
- `PublicRecordingIdentityRepository::rebindAfterVerifiedMove` existiert, wird aber im untersuchten `DaemonRuntimeInitialization.cpp`-Callback nach RENAME/MOVE nicht ausdrücklich aufgerufen; dieser aktualisiert Snapshots. Eine garantiert erhaltene `rec_`-ID nach MOVE/RENAME ist daher **nicht belegt**. DELETE entfernt die alte Zuordnung über `removeAfterVerifiedDeletion`.
- **2026-10-21 yaVDR-Acceptance** für RENAME/MOVE/DELETE der ausdrücklich freigegebenen einmaligen Fixture: PASS. Fixture bereits gelöscht. Ein einzelner upstream MOVE-HTTP-409 wurde nicht reproduziert; keine Ursache nachgewiesen, **kein** spekulativer Fix und **kein** weiterer destruktiver Test ohne Freigabe.

## Zwölf-Stunden-Fenster: Entscheidung für E1

Die Grenze steht in `PublicApiRuntime.cpp::parsePublicEpgWindowQuery` als `until > from && until-from <= 43200LL` und `limit <= 192`. **Es ist eine Request-Grenze, keine Speichergrenze des EPG-Caches.** Der Public-Handler ruft `EpgCacheService::findWindowForBackend` auf; auch `/api/epg/cache/window` verwendet diese bestehende Datenquelle. Die Anzahl tatsächlich verfügbarer Tage hängt von Cache und EPG-Quelle ab.

**Ziel:** Jede im bestehenden Cache tatsächlich verfügbare Zeitspanne soll für Clients durch begrenzte, eindeutig fortsetzbare Abfragen erreichbar sein. Kein unbegrenztes JSON, kein neuer Cache, kein stiller externer Provider-Fallback. Zuvor Query-Ordering, `hasMore`, Fortsetzungscursor oder disjunkte Time-Chunks, Backend-Scope, Event-Provenienz und Regressionen untersuchen. Bestehende 12h-Consumer unverändert unterstützen. `GET /api/v1/epg/event` als vorhandene Source-ID-/ETag-Basis wiederverwenden.

## Priorisierte kleine Implementierungsslices

| Slice | Umfang | Abnahme/Gates |
| --- | --- | --- |
| **R1 – zuerst** | v1 Recording `rec_` intern mit bestehendem Public Resolver auf native Identität abbilden; fremde/alte/konfliktäre Client-Felder fail-closed; nur validate/dry-run | gleiche Security-/CSRF-Checks; Unit/Router/negative Tests; **keine Live-Mutation**; Legacy unverändert |
| R2 | RENAME und DELETE sauber versionieren; öffentliche DTOs/Problem Details, Revisionsschutz und Lifecycle | R1; Tests für Stale/Mismatch/Revoke; reale Mutation erst mit neuer freigegebener Fixture |
| R3 | MOVE mit logischer Zielordner-ID und bestätigtem Rebind nach Readback | R1/R2; Zielkonflikte, keine automatische Mutation-Retry |
| E1 | gesamte vorhandene Cache-Zeitspanne paginieren; Event-ID Now/Next; Event-Detail/OpenAPI | Provenienz, Grenzen, Berechtigungen, deterministische Fortsetzung |
| E2 | EPG↔TimerIntent, Konflikt-/Statusdarstellung | vorhandene Native Timer/Operation/Agent-Autorität |
| M1 | Read-only EPG-/Recording-Metadaten | vorhandene TVScraper/Manual-/Cache-Dienste, keine Parallelverwaltung |
| M2 | Berechtigte Poster/Fanart/Bilder | bestehender Artwork-Service; Traversal/Content-Type/Size-/Auth-Tests |
| S1 | SearchTimer MODIFY/DELETE + Operation/ETag | Legacy- und Grant-Parität |
| P1 | Resume/Device-Profile-Integration | MediaSession/Ownership, Hisense-Abnahme |
| A1 | Add-on-Registry-Grants | erst nach Integration der bestehenden separaten Add-on-Arbeit; TVScraper ausgeschlossen |

OpenAPI-Schema, Security-Gates, Rückwärtskompatibilität, CI und Client-Beispiele gelten in jedem Slice. **Keine neuen Branches ohne Freigabe; kein Force-Push/Rebase; keine Eingriffe in produktive Aufnahmen oder yaVDR.**

## R1 Startpunkt (noch nicht als implementiert markieren)

Berührpunkte: `ApiRouter::handlePost` (Legacy/v1 sauber unterscheiden), `PublicRecordingPlaybackTargetResolver`, `PublicRecordingIdentityRepository`, `RecordingActionValidationController`, `RecordingActionExecutionController`, `SecurityHttpGate`; `DaemonRuntimeInitialization.cpp` für bereits vorhandene Resolver-/Service-Instanzen. Kein zweiter Executor. Zuerst eine **v1-spezifische, fail-closed Read/Validate/Dry-run-Grenze** implementieren und alle alten `/api/recordings/actions/...`- und `/api/vdr/recordings/actions/...`-Routen unberührt lassen.

**Akzeptanz:** `rec_` aus Collection wird für genau denselben autorisierten Backend-Scope auf eine aktuell vorhandene native Aufnahme aufgelöst; falsches Backend, ungültiges Format, fehlender Datensatz, stale Snapshot, manipulierte native Identitätsfelder und revoked Actor scheitern vor Dispatch. Test nur mit Mock-/Repository-Fixtures. Öffentliche Aktion darf keine Backend-Pfade/Upstream-Bodies an Clients weitergeben. RENAME/MOVE-Identitätskontinuität und produktive Mutation **nicht** in R1 freischalten.
