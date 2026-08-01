# Phase 62 Slice 2I — Recording Execution security migration

Status: focused repository implementation complete; full regression, CI and
installed-runtime acceptance pending

## Scope

Slice 2I migrates ausschließlich diese beiden bestehenden Execute-Aliase:

```text
POST /api/recordings/actions/execute
POST /api/vdr/recordings/actions/execute
```

Aktuell vom Webfrontend verwendete Aktionen erhalten getrennte Berechtigungen:

```text
RENAME -> recordings.rename@<backend-id>
MOVE   -> recordings.move@<backend-id>
DELETE -> recordings.delete@<backend-id>
```

Validation, Preview, SearchTimer, EPG und Administration bleiben außerhalb
dieses Slices.

## Security contract

Bei browser-authentifizierten Requests gilt:

1. Browser-Session authentifizieren;
2. Lifecycle und persistierte Grants auflösen;
3. Recording-Aktion klassifizieren;
4. `X-CSRF-Token` prüfen;
5. `backendId` aus dem JSON-Body lesen;
6. aktionsgenaue Berechtigung für den exakten Backend-Scope prüfen;
7. Pre-Dispatch-Accountability persistieren;
8. erst danach den bestehenden Router-, Controller- und Backendpfad erlauben.

Query-Strings bleiben unterstützt. Varianten mit abschließendem Slash bleiben
als nicht migrierte Browser-Mutation fail-closed.

`dryRun:true` verändert nur die nachgelagerte Ausführung. Authentifizierung,
CSRF, Berechtigung und Backend-Scope gelten identisch für Dry-Run und reale
Ausführung.

Andere Action-Werte, einschließlich `METADATA_REFRESH`, bleiben mit
`invalid_recording_action` fail-closed. Es gibt keine pauschale
`recordings.execute`-Freigabe.

## Fixed roles

`role.admin@<backend-id>` umfasst zusätzlich:

```text
recordings.rename
recordings.move
recordings.delete
```

`role.read-only@<backend-id>` verweigert diese Mutationen für denselben
Backend-Scope und hat Vorrang vor direkten Grants und Admin.

Wildcard-Rollen gelten weiterhin nicht für konkrete Backend-Scopes.

## Frontend contract

Der Deferred-Runtime-Loader ergänzt den aktiven, ausschließlich im Speicher
gehaltenen CSRF-Token nur bei POST-Requests auf die beiden exakten
Execute-Aliase.

Caller-Header bleiben erhalten, können den aktiven CSRF-Token aber nicht
überschreiben. GET, Trailing-Slash-Varianten und Recording Validation erhalten
durch Slice 2I keinen CSRF-Header.

## Independent Recording safety

Slice 2I verändert nicht:

- `ApiRouter`;
- `RecordingActionExecutionController`;
- `RecordingActionExecutionService`;
- `RecordingActionSafetyService`;
- `RecordingActionBackendPolicy`;
- Adapterauflösung oder Backend-Dispatch;
- bestehende Validation-, Readback- oder Cache-Logik.

Backend-Verfügbarkeit, Read-only-Policy, Capabilities und Execution-Policy
bleiben unabhängig von der Actor-Autorisierung verpflichtend.

## Accountability

Erlaubte und verweigerte Entscheidungen verwenden:

```text
permission = recordings.rename | recordings.move | recordings.delete
action     = recordings.rename | recordings.move | recordings.delete
backendId  = angeforderter JSON-Backend-Scope
outcome    = dispatch_authorized | dispatch_denied
```

Authorization-Header, Cookies, Session- und CSRF-Secrets, Passwörter und
Recording-Pfade werden nicht als Authorization-Evidence gespeichert.

## Focused validation

Bereits erfolgreich:

```text
make test-security
make test-recording-action-execution-controller
make test-live-remote-frontend
```

Die fokussierten Tests decken beide Aliase, alle drei Aktionen, CSRF,
Berechtigungs- und Scope-Denials, Admin, Read-only, Query-Strings,
Trailing-Slash-Denial, ungültige Aktionen und Frontend-CSRF ab.

## Pending acceptance

Vor Commit und Installation müssen vollständige Regression, Architektur- und
Dokumentationsprüfungen, Daemon-Build und Packaging erfolgreich sein.

Die spätere Runtime-Abnahme bleibt mutationsfrei. Erfolgreiche Execute-Probes
verwenden ausschließlich `dryRun:true` und nicht existente Recording-IDs.

Keine reale Aufnahme darf ohne ausdrückliche Freigabe und einen dedizierten,
entbehrlichen Testdatensatz umbenannt, verschoben oder gelöscht werden.

## Non-goals

- keine Migration von Validation oder Preview;
- keine Freigabe von `METADATA_REFRESH`;
- keine reale Recording-Mutation in automatisierter Abnahme;
- keine SearchTimer-Migration;
- keine generische Rollen- oder Grant-Administration;
- keine Phase-63+-Runtime;
- kein Merge und kein Ready-for-review.
