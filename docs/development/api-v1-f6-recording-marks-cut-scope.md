# API.V1.F6 — Recording Marks/Cut: Public-v1-Integration und einfache Rechte

**Status: Public-v1-Marks-/Cut-Adapter, bestehendes Sicherheitsgate,
Referenz-JS, OpenAPI und kanonische Public-ID-Projektion für native
Cut-Ergebnisse committed. Gezielte Native-/Security-Fixtures und
die bestehende Router-Verdrahtungsprüfung liegen im Quellkandidaten
vor. Vollständige CI-Verifikation auf dem aktuellen PR-Head
sowie reale Browser-/Device-/VDR-Abnahme offen.**

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
Für ein im aktuellen Aufnahme-Cache eindeutig nachgewiesenes
natives Cut-Ergebnis projiziert Public v1 nun dessen eigene,
persistente `rec_`-ID aus dem vorhandenen
`PublicRecordingIdentityRepository`. Weder die interne
Ergebnis-ID noch native Pfade oder Cut-Keys werden in Public-v1-GET-
und verifizierten POST-Antworten zurückgegeben; bei noch fehlender
eindeutiger Ergebnisauflösung bleibt die verschachtelte
Ergebnisprojektion aus, statt Legacy-Identitäten preiszugeben.
Die Legacy-Webantwort bleibt unverändert. Die bereits
bestehende Router-Verdrahtungsprüfung bestätigt beide nativen
Cut-Eintrittspunkte; `test_recording_cut_api_runtime.cpp`
prüft die GET-/POST-Ergebnisprojektion in Legacy und Public v1.
Ein isolierter Client-Router-Linktest wurde nach belegtem
Linker-Abhängigkeitsproblem zurückgenommen, statt fremde
Runtimes in den F6-Testumfang zu ziehen.
MLD-/TV-Mutationen sind weiterhin nicht hardwarebestätigt.

**ADR-0059 bleibt gültig:** VDR ist einzige Autorität für
Marks, Index und Schneiden; Kontrollplan, Agent und SuiteBridge
bleiben einzige Ausführungskette. Die Benutzerebene darf die
neue Sammelberechtigung verwenden, ohne diese technischen
Kontrollen zu umgehen.
[ADR-0059](../adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)

## F6.3 — Recordings-2-Webfrontend auf Public API v1

Die bestehende Marks-/Cut-Oberfläche wurde ohne UI-Neubau von
`/api/vdr/recordings/marks` und `/api/vdr/recordings/cut`
auf die vier bereits implementierten Public-v1-Routen
umgestellt. Der Marks-Readback liefert die kanonische `rec_`-
ID; der Editor verwendet exakt diese ID für Cut-/Marks-GET
und -POST, einschließlich übereinstimmendem `backendId`
in Query und Body sowie unverändertem Browser-CSRF.

Da Recordings 2 für vorhandene Legacy-Aufnahmeeinträge
weiterhin eine native Listen-ID liefern kann, verwendet der
Browser die **bereits vorhandene** Recordings-Hero-Auflösung
über die Public-v1-Aufnahmenliste (eindeutiger Aufnahmezeitpunkt
und Titel, gegebenenfalls Cursor-Pagination). Kein Mapping
über Dateipfad oder erratene `rec_`-ID und kein Rückfall
auf Legacy-Schreibendpunkte. Beide Hero-Wrapper reichen
den bestehenden Resolver explizit durch.

Die vorhandenen Editor-/Detail- und Hero-Regressionen prüfen
die URL-/Body-/Backend-/CSRF-Verträge, Polling-/Native-
Bestätigungsabläufe, Schnittfassung-Anzeige und die eindeutige
Aufnahmezuordnung inklusive Ambiguitätsfall.
Die Produktions-Owner von VDR, Agent und SuiteBridge bleiben
unverändert. **Vor echter Freigabe bleibt die gemeinsame
Browser-/yaVDR-Abnahme nötig**, insbesondere der reale
Marks-Readback, ein nur ausdrücklich freigegebener
Schnittstart und das Öffnen der erzeugten Schnittfassung.
Bis dahin ist die Browser-/Legacy-MediaSession-Kompatibilität
der öffentlichen Ergebnisaufnahme nicht hardwarebestätigt.

## Abnahme und Folgeschritt

F6.1 und F6.2 sind **als Quellcode einschließlich
kanonischer Public-Cut-Ergebnis-ID committed, aber noch nicht
vollständig abgenommen**. Die bestehenden Native-/Security-
Tests und Reference-JS-Mocks wurden um Public-v1-Pfad-,
Backend-, CSRF- und Grant-Fälle ergänzt. Die native
`test_recording_cut_api_runtime.cpp`-Fixture deckt
Public/Legacy-GET, überprüfte Cut-Replays und die
öffentliche Ergebnis-ID ohne native Pfad-/Key-Leaks ab.
Die existierende `check_recording_cut_runtime_wiring.py`-
Prüfung schützt die Router-Verdrahtung. Ein zusätzlicher
Gesamt-Router-Linktest führte zunächst zu einer fehlerhaften
Router-Fixture, danach zum Linker-Fehler wegen im
isolierten Testziel nicht vorhandener fremder Runtimes.
Dieser Zusatztest ist ohne Änderung am Produktionscode
entfernt worden, statt unnötige Runtime-Linkabhängigkeiten
einzuführen. Die vollständige CI-Verifikation des aktuellen
PR-Quellkandidaten bleibt offen; ein echter
Browser-vs-Public-v1- und yaVDR-/Device-Test ist
nicht ausgeführt. Keine produktive Aufnahme wurde
mutiert, geschnitten oder gelöscht.
Die frühere pauschale Scope-Ausnahme ist aufgehoben.
Weder MLD noch VIDAA oder Tizen müssen eine eigene
Schneideoberfläche implementieren — sie dürfen die Public-v1-
Funktion nach Abschluss nutzen, sofern gewünscht und berechtigt.
Die gemeinsame Release-/Hardware-Abnahme bleibt
[an einer zentralen Stelle](public-v1-final-hardware-acceptance.md).
