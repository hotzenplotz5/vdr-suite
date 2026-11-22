# API.V1.F6 — Recording Marks/Cut: Public-v1-Integration und einfache Rechte

**Status: Public-v1-Marks-/Cut-Adapter, Sicherheitsgate, Referenz-JS und
OpenAPI als Quellkandidat committed. C++-Build, automatisierte
Web-vs-v1-Regression und finale Hardwareabnahme noch offen.**

Branch: `work/api-v1-finalisierung-f2-f8`.
Diese Fassung korrigiert den unautorisierten Ausschluss der bereits vorhandenen
Marks-/Cut-Funktionen aus Public API v1 vom 21.11.2026. Der Betreiber hat
ausdrücklich festgelegt: **Wer Aufnahmen bearbeiten darf, darf auch
Schnittmarken bearbeiten und schneiden.** Keine zusätzlichen, separat
zuzuweisenden Schneiderechte.

## F6.1 — Berechtigungen (Quellimplementierung)

Die neu auswählbare, backend-gebundene Berechtigung
`recordings.edit` ermächtigt zu folgenden bestehenden Einzelaktionen:

| Gewünschter Zugriff | Wirksame bestehende Berechtigung |
| --- | --- |
| Aufnahme ansehen / Metadaten lesen | `recordings.view` |
| Aufnahme umbenennen | `recordings.rename` |
| Aufnahme verschieben | `recordings.move` |
| Aufnahme löschen | `recordings.delete` |
| Schnittmarken ändern | `recordings.marks.modify` |
| Nativen Schnitt starten | `recordings.cut` |

`AuthorizationService` wertet `recordings.edit` für **exakt
diese sechs** Permission-Checks aus, ohne die vom jeweiligen
Endpoint verlangte konkrete Aktion oder den Audit-Permission-Code
zu verändern. `SecurityHttpGate` prüft wie zuvor
CSRF, authentifizierten Akteur und den konkreten Endpoint.
Der bestehende `role.read-only`-Schutz, Backend-Scope,
Native-Revision, Agent-/Provider-Fencing, In-use-Schutz,
Einmal-Dispatch und VDR-Readback gelten unverändert.

**Device-Grenze:** Die Device-Grant-Allowlist erlaubt jetzt ebenfalls
`recordings.edit`, **nur für ein ausdrücklich administrativ
freigegebenes Gerät und genau ein Backend**. Es gibt keinen
automatischen Grant beim Pairing, keine globale Device-Bearbeitung
und keine Vererbung von Human-Account-Rechten.
Der native Recording-Editor prüft weiterhin Actor, CSRF bei Browser-
Sessions, Scope, Revision, Readback und Provider-/Agent-Fencing.

**Nicht** impliziert sind Playback (`media.recording.play`),
`recordings.execute` als unbekannter/fallback Action-Code,
Metadatenzuordnung, Add-on-Aktionen, Timer und andere Schreibrechte.

`recordings.view` allein bleibt ein reines Leserecht. Ein bisher
vergebenes, feingranulares `recordings.delete` oder
`recordings.marks.modify` wird **nicht** automatisch zum
allgemeinen Bearbeitungsrecht aufgewertet.
Vorhandene Einzelrechte und persistierte Grants bleiben gültig.
Eine Migration bestehender Grants findet **nicht** statt.

Die Konto-Verwaltung bietet bei einem Server mit
`recordings.edit` im Berechtigungs-Picker nur noch
„Aufnahmen bearbeiten (inkl. Schnitt)“ statt fünf
neuer Einzelwahlen an; bestehende ältere Einzelrechte
bleiben sichtbar und entziehbar. Auf einem älteren Server
ohne neues Sammelrecht bleibt die bisherige Auswahl erhalten.

## F6.2 — Public-v1-Routen (Quellkandidat vorhanden)

Bereits vorhanden und funktionsfähig:

- `GET/POST /api/vdr/recordings/marks` —
  `RecordingMarksApiRuntime`, native Marken und Revisionen
- `GET/POST /api/vdr/recordings/cut` —
  `RecordingCutApiRuntime`, Native-Cut und Cut-Status
- Webfrontend `recordings2-marks-*.js`, Backend Agent und
  SuiteBridge zur VDR-eigenen Marken-/Schnittinstanz.

Neu im Quellcode und OpenAPI:
- `GET /api/v1/recordings/{recordingId}/marks?backendId=...`
- `POST /api/v1/recordings/{recordingId}/marks?backendId=...`
- `GET /api/v1/recordings/{recordingId}/cut?backendId=...`
- `POST /api/v1/recordings/{recordingId}/cut?backendId=...`

Der Adapter verwendet **die existierenden** `RecordingMarksApiRuntime`/
`RecordingCutApiRuntime`-Handler, die bereits mit Agent und
SuiteBridge verbunden sind; kein zweiter Writer/Schneidepfad.
Die öffentliche `rec_`-ID wird nicht als native Cache-ID missverstanden:
Die Registrierung über `ApiRouter::setPublicRecordingActionResolver`
verwendet denselben persistenten `PublicRecordingPlaybackTargetResolver`
wie Public Recording-Aktionen. Die nachgeschlagene native Identität
wird vor dem SuiteBridge-Aufruf erneut gegen die aktuelle Cache-Bindung
geprüft; öffentliche JSON-Responses behalten die ursprüngliche
`rec_`-ID. Native Pfade und das interne Aufnahmeschlüsselmaterial
werden nicht als Client-Mutationsziel akzeptiert.
Die neuen Pfade werden im Router vor dem generischen v1-404
zugestellt; Legacy-Routen bleiben erhalten. Der SecurityHttpGate
verwendet backendgebundenes `recordings.view` für GET und
die vorhandenen `recordings.marks.modify`-/`recordings.cut`-
Checks für POST. Ein neues Human-`recordings.edit`-Grant
deckt beide ab. Der Native-Handler verlangt für POST
**exakte Übereinstimmung** von `rec_`-Pfad-ID,
`backendId`-Query und beiden Body-Identitäten;
außerdem Operation-ID, Revision, Schutz vor Zweitversuchen
und unveränderte Native-Readbacks.

Die vier SDK-Methoden
`getRecordingMarks`, `getRecordingCutState`,
`submitRecordingMarksMutation`, `submitRecordingCut`
senden nur diese Pfade; der POST-Client wiederholt
niemals automatisch einen möglicherweise gestarteten Schnitt.

**Noch nicht abgenommen:** Native/Legacy-Fehlerkörper sind
teilweise `application/json` mit `error.code`, während
andere Public-v1-Routen `application/problem+json`
verwenden. Die semantische Gleichwertigkeit von
Browser-/Device-Transport, Fehlercodes und Status muss
am gematchten Kandidaten getestet werden. Die neue
Device-Berechtigung `recordings.edit` ist als **explizit
admin-vergebbarer, konkreter Backend-Grant** im Katalog
und in beiden Device-Clients vorhanden; Pairing oder
Playback erteilen sie nicht automatisch.
Die öffentliche Antwort zur bereits fertig geschnittenen
Aufnahme kann noch eine legacy-interne Ergebnis-ID enthalten:
die kanonische Public-Result-ID-Projektion ist vor
F6-Endabnahme gesondert zu vergleichen.
MLD-/TV-Mutationen sind weiterhin nicht hardwarebestätigt.

**ADR-0059 bleibt gültig:** VDR ist einzige Autorität für
Marks, Index und Schneiden; Kontrollplan, Agent und SuiteBridge
bleiben einzige Ausführungskette. Die Benutzerebene darf die
neue Sammelberechtigung verwenden, ohne diese technischen
Kontrollen zu umgehen.
[ADR-0059](../adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)

## Abnahme und Folgeschritt

Bisherige Quellevidenz: Der isolierte
`test_mu9e_grant_mutation_ui.js`-Test bestand;
`test_public_v1_f6_recording_edit_client.js`
bestand im JavaScript-Mock mit exakt diesem SDK-Code.
Der kleine `PublicRecordingEditRoute.h`-Parser wurde
isoliert mit `g++ -std=c++17 -Wall -Wextra -Werror`
gegen eine äquivalente Testkopie überprüft – **kein**
vollständiger C++-Build des Repository-Kandidaten.
C++-Native-/Security-Fixtures sind ergänzt, ihre Ausführung
ist noch nicht belegt. Keine produktive Aufnahme mutiert,
geschnitten oder gelöscht.

F6.1 und F6.2 sind **als Quellcode committed, aber noch nicht
vollständig abgenommen**. Native-API- und Security-Fixtures
wurden um Public-v1-Pfad-/Backend-/CSRF-/Grant-Fälle ergänzt.
Ein isolierter C++-Header-Test und der Reference-JS-F6-
HTTP-Mock-Test bestanden; der vollständige C++-Build und
die echte Browser-vs-v1-/VDR-Abnahme fehlen weiterhin.
Die frühere pauschale Scope-Ausnahme ist aufgehoben.
Weder MLD noch VIDAA oder Tizen müssen eine eigene
Schneideoberfläche implementieren — sie dürfen die Public-v1-
Funktion nach Abschluss nutzen, sofern gewünscht und berechtigt.
Die gemeinsame Release-/Hardware-Abnahme bleibt
[an einer zentralen Stelle](public-v1-final-hardware-acceptance.md).
