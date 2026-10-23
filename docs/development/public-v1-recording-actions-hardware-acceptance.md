# Public API v1 R2/R3 – yaVDR Hardware-Abnahme (2026-10-21)

## Aktuelle Abnahme – verbindlich

Branch: main. Der vorhandene ältere Prüfblock weiter unten ist historisch
und darf NICHT mehr ausgeführt werden; das frühere Report-Mainz-Fixture
existiert nicht mehr. Der neue Helfer ist:
tools/accept_api_v1_recording_actions_hw.py

Freigegeben ist ausschließlich eine NEUE, fertiggestellte, kurze
VDR-Aufnahme namens VDR-SUITE-TEST-R3. Keine produktiven Aufnahmen.
Das Testergebnis ist nur aussagekräftig, wenn der lokal installierte
VDR-Suite-Daemon tatsächlich dem aktuellen, vollständig per CI
geprüften Build entspricht. Ein Git-Checkout auf main allein beweist
keine installierte Version. Build-ID vorher separat festhalten.

Der neue Helfer akzeptiert ausschließlich öffentliche rec_- und
fld1_-Identitäten. Native Aufnahme-/Zielpfade werden nur LESEND
gegen den VDR abgeglichen, nie als öffentliche Aktionsparameter
gesendet. Mutationen gehen ausschließlich an Public API v1 mit
If-Match und Browser-CSRF; die RESTfulAPI dient ausschließlich dem
unabhängigen Readback.

## Vorbereitungen

1. Die VDR-Aufnahme VDR-SUITE-TEST-R3 für etwa 1–3 Minuten anlegen.
   Aufnahme vollständig beenden und sicherstellen, dass sie nicht mehr
   wächst. Keine bereits vorhandene persönliche Aufnahme umbenennen.
2. Für MOVE ist ein bereits im öffentlichen Recording-Folder-Browse
   sichtbarer Root-Zielordner notwendig. Es wird KEIN Ordner angelegt,
   geraten oder aus einem absoluten Pfad konstruiert. Den echten
   fld1_-Wert aus der read-only Liste wählen.
3. Den aktuellen Checkout und die installierte Daemon-Build-ID notieren.
   Kein automatischer Paket-/Binary-Wechsel.

## Schritt A – nur lesen: Testaufnahme und Zielordner

Auf dem yaVDR-Host im VDR-Suite-Checkout ausführen:

    git rev-parse HEAD
    python3 tools/accept_api_v1_recording_actions_hw.py --list-folders

Der Helfer fragt das Human-Account-Passwort interaktiv ab. Bitte niemals
Passwort, Cookie oder CSRF-Token in Chats kopieren. Er verlangt exakt eine
neue VDR-SUITE-TEST-R3-Aufnahme mit passender .rec-Identität, bekannter
Dauer und Größe. Die Ausgabe zeigt die öffentliche rec_-ID und bereits
vorhandene root-fld1_-Ordner.

## Schritt B – schreibfrei Validate / Dry-run

Ohne MOVE-Ziel:

    python3 tools/accept_api_v1_recording_actions_hw.py

Mit echtem Root-Zielordner aus Schritt A:

    python3 tools/accept_api_v1_recording_actions_hw.py --folder-id 'fld1_REALE_32_HEX_ZEICHEN'

Validierung und Ausführung mit dryRun:true müssen die aktuelle rec_-ID
behalten und den serverseitig ausgegebenen starken ETag prüfen.
Es erfolgt keine echte Aufnahmeänderung. Ohne Folder-ID bedeutet
R3_MOVE_PREFLIGHT=NOT_RUN; dies ist KEIN R3-Gesamterfolg.

## Schritt C – erst danach echte Aktionen, jeweils einzeln bestätigen

Nur mit aktueller installierter Revision, zuvor grünem Preflight und
der freigegebenen, abgeschlossenen Testaufnahme:

    python3 tools/accept_api_v1_recording_actions_hw.py --folder-id 'fld1_REALE_32_HEX_ZEICHEN' --execute

Nach einer Gesamtfreigabe muss jede einzelne Aktion RENAME, MOVE und
DELETE nochmals interaktiv mit ACTION + rec_-ID bestätigt werden.
Der Helfer fordert vor jedem Schreibzugriff eine frische Validierung
und einen providergebundenen If-Match-ETag an. Jeder POST mit
dryRun:false wird exakt EINMAL gesendet; es gibt KEINEN automatischen
Retry nach Timeout oder HTTP-Konflikt.

Nach RENAME/MOVE werden alter und neuer nativer Zustand sowie
die weiterhin gleiche öffentliche rec_-ID unabhängig geprüft.
DELETE wird explizit mit confirmDelete:true ausgelöst; die alte
Aufnahme muss danach fehlen und die rec_-ID aus der Public Collection
verschwunden sein. Ein unklarer Status: STOP, nicht erneut ausführen.

Die echten Tests dürfen niemals mit einem alten nativen Aufnahmeindex,
einem erratenen Zielpfad oder einer fremden Recording-ID erfolgen.

## Evidenz und Abschluss

Benötigt werden die installierte Build-ID, source HEAD, die getestete
Backend-ID, die rec_-ID, der fld1_-Zielordner sowie die Statusmeldungen:

    HUMAN_AUTH=PASS
    R2_RENAME_DELETE_PREFLIGHT=PASS
    R3_MOVE_PREFLIGHT=PASS
    API_V1_RECORDING_RENAME=PASS
    API_V1_RECORDING_MOVE=PASS
    API_V1_RECORDING_DELETE=PASS
    API_V1_RECORDING_HW_ACCEPTANCE=PASS
    AUTOMATIC_RETRY=DISABLED

Nur drei verifizierte echte Mutationen erlauben das Hardware-PASS.
CI-Grün und Dry-run sind ausdrücklich nicht ausreichend.
Unklare Mutationsausgänge müssen vor allgemeinem Client-Rollout
auch hinsichtlich Durable Operation / Idempotenz gesondert bewertet
werden. Das allgemeine Router-Refactoring ist erst nach R3
gemäß ADR-0068 erlaubt.

---

## Historischer Abnahmevertrag (VOR R1–R3; NICHT AUSFÜHREN)


## Scope

One **dedicated, disposable** VDR test recording exercises the new public-v1 routes via the unchanged existing Recording Action controllers. This is not Timer API v1 acceptance. Do not use production recordings.

Branch: `work/public-v1-recording-actions`. Before running, record exact branch HEAD and deployed server version; the source branch existing on GitHub is not proof that yaVDR runs it.

## Fixture

Create a short, separate, identifiable VDR recording whose recording title contains `VDR-SUITE-TEST`. Read it from the live recording collection and capture its **actual** `recordingId`, `backendNativeId` (absolute native `.rec` directory) and `backendId`. Confirm that it is no longer recording. Never synthesize or modify recording identity, never choose a real recording by positional index, and never run DELETE without affirming the exact final test-fixture identity.

Use a unique rename title such as `VDR-SUITE-TEST-API-V1-RENAMED`, and a test-only target folder `VDR-SUITE-TEST-API-V1-MOVED`. No filesystem-level moves or deletes.

## HTTP requests (existing DTO)

Each of the three actions uses the same sequence:

1. `POST /api/v1/recordings/actions/validate` with `dryRun: true`; require `valid: true`.
2. `POST /api/v1/recordings/actions/execute` with `dryRun: true`; inspect the established dry-run response, **not** a fabricated `success: true` requirement (backend dispatch is deliberately skipped).
3. Only if explicitly authorized for this dedicated fixture: `POST /api/v1/recordings/actions/execute` with `dryRun: false`. Apply existing backend-scoped permission and Browser CSRF requirements.
4. Re-read the live Recording collection, confirm result and refresh **both** IDs before proceeding. If ambiguous or unsuccessful, STOP and do not issue the next mutation or automatically retry the POST.

### RENAME

```json
{"backendId":"default","recordingId":"<CURRENT_ID>","backendNativeId":"<CURRENT_ABSOLUTE_REC_PATH>","action":"RENAME","newName":"VDR-SUITE-TEST-API-V1-RENAMED","dryRun":true}
```

Require the old display name to be replaced by the new name in the follow-up read. Capture the new recording and native ID.

### MOVE

```json
{"backendId":"default","recordingId":"<REFRESHED_ID>","backendNativeId":"<REFRESHED_ABSOLUTE_REC_PATH>","action":"MOVE","targetPath":"VDR-SUITE-TEST-API-V1-MOVED","dryRun":true}
```

Require source location absent and the same fixture present in the test target folder, with fresh identifiers. `targetPath` is a target **folder**, not a complete filename; the existing backend request builder preserves the recording leaf. Check the actual backend semantics before executing.

### DELETE

```json
{"backendId":"default","recordingId":"<REFRESHED_ID>","backendNativeId":"<REFRESHED_ABSOLUTE_REC_PATH>","action":"DELETE","dryRun":true}
```

Require exact fixture confirmation before real DELETE; after the action, the fixture must no longer be present in live recordings. Do not treat a successful HTTP response without readback as a PASS.

## Evidence and pass criteria

Record exact deployed commit, HTTP status + redacted JSON for each validate/dry-run/execute, initial/current/final recording identities and three readback outcomes. Do not log credentials or cookies.

```text
API_V1_RECORDING_RENAME=PASS|FAIL|NOT_RUN
API_V1_RECORDING_MOVE=PASS|FAIL|NOT_RUN
API_V1_RECORDING_DELETE=PASS|FAIL|NOT_RUN
API_V1_RECORDING_HW_ACCEPTANCE=PASS|FAIL|NOT_RUN
```

An actual PASS requires three live action executions and their independent readbacks. CI green, preview success or historical pre-v1 real-VDR evidence is not an API-v1 hardware pass. On failure, retain the existing fixture state for diagnosis and **do not restart the sequence**.
