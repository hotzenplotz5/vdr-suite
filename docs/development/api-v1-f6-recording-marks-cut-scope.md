# API.V1.F6 — Native Recording Marks/Cut: Public-v1-Scopeentscheidung

**Stand:** 21.11.2026 · Repository `hotzenplotz5/vdr-suite` · Branch
`work/api-v1-finalisierung-f2-f8` · Ausgangsstand
`20f5da4ee053b35a3213cc274c223d451a38ef08`.

**Entscheidung für den aktuellen API-v1-Integrationsabschluss:**
Die bereits funktionsfähigen VDR-nativen Marks-/Cut-Routen bleiben
**browser-/kontrollplanintern**. Ein eigenständiger Public-v1-Marks-
oder -Cut-Endpunkt ist **nicht** Teil der F1–F8-Releaseabnahme.
Das ist eine bewusste **Scope-Ausnahme**, nicht die Behauptung
vollständiger Public-v1-Funktionsparität. Vorhandene Webfunktionen
dürfen nicht entfernt, über ein erfundenes v1-Alias umgeleitet
oder als neue Device-Funktion beworben werden.

## Nachgeprüfte Implementierung und Quellbelege

| Funktion | Implementierung / aktueller Pfad | Public-v1-Abdeckung | F6-Entscheidung |
| --- | --- | --- | --- |
| Native Marken lesen | `RecordingMarksApiRuntime::tryHandleGet`, `GET /api/vdr/recordings/marks`, Web `recordings2-marks-detail.js` | Kein `/api/v1/recordings/{id}/marks` | Bestehenden Webread erhalten |
| Marken hinzufügen/löschen/verschieben/zurücksetzen/ersetzen | `RecordingMarksApiRuntime::tryHandlePost`, `POST /api/vdr/recordings/marks`, `recordings2-marks-editor.js` | Keine Public-v1-Markenmutation | Native Revisions-/Operationsgrenzen unangetastet |
| Cut-Vorbedingungen/Ergebnis lesen | `RecordingCutApiRuntime::tryHandleGet`, `GET /api/vdr/recordings/cut` | Kein v1-Cut-Statusvertrag | Bestehende Cut-Vorschau erhalten |
| Nativen Schnitt bestätigen/starten | `RecordingCutApiRuntime::tryHandlePost`, `POST /api/vdr/recordings/cut` | Keine Public-v1-Cut-Startmutation | Keine zusätzliche Schreibroute |
| Recording-Aktionen | `POST /api/v1/recordings/actions/validate` und `.../execute` | RENAME/MOVE/DELETE, **nicht** gleichbedeutend mit native Marks/Cut | Bestehenden R2/R3-Vertrag nicht als Cut-Alias missbrauchen |

**Source-Abgleich am Branch:** Die Routenkonstanten liegen in
`api/rest/src/RecordingMarksApiRuntime.cpp` und
`api/rest/src/RecordingCutApiRuntime.cpp`. Die
`web/frontend/recordings2-marks-*.js`-Module nutzen
`/api/vdr/recordings/marks` und `/api/vdr/recordings/cut`.
`core/security/include/SecurityHttpGate.h` enthält
für POST die separaten Rechte `recordings.marks.modify`
und `recordings.cut`. Die aktuelle partielle
`api/openapi.json` enthält die beiden Public-v1-Recording-
Action-Routen, aber **keine** Public-v1-Marks-/Cut-Routen.
Das beweist die dokumentierte Grenze; es ist **keine**
inventarisierte Garantie, dass jeder mögliche undokumentierte
Runtime-Pfad der gesamten Suite erfasst wurde.

## Verbindliche fachliche und Sicherheitsgrenzen

[ADR-0059](../adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)
bleibt die einzige Marks-/Cut-Autorität:

- VDR besitzt die Marken und den nativen Schnitt; keine Schatten-
  Marks-Datenbank, keine zweite Schnittengine, kein HTTP-Dateizugriff.
- Der vorhandene Kontrollplan löst die Recording-Identität
  backendgebunden auf; native Pfade werden nicht für Clients geöffnet.
- Marks-Mutationen benötigen erwartete `marksRevision`,
  eindeutige Operation, zulässigen Backend-/Agent-/Provider-Stand
  und echten Native-Readback. In-use/Revision-Konflikte
  dürfen nicht mit automatischen Zweitversuchen übergangen werden.
- Ein Cut wird explizit bestätigt, prüft Vorbedingungen vor
  Dispatch und läuft über den nativen VDR-Recording-Handler.
  Das Original wird nicht automatisch gelöscht; ein angenommenes
  Cut-Kommando bedeutet noch nicht, dass eine Ausgabe fertig ist.
- Ein künftiger öffentlicher Vertrag muss Authentifizierung,
  Backend-Grants, native Revision/Idempotenz, Abbruch-/Unbekannt-
  Zustände, Fehler-/Statuscodes und kontrollierte Resultate
  **separat** gegenüber Web und realem VDR nachweisen.
  Er benötigt eine neue ausdrückliche Scopefreigabe.

## Web-/Device-/TV-Paritäts- und Abnahmegrenze

| Vergleichsdimension | Bestätigter Browserumfang | Public-v1-F6-Umfang |
| --- | --- | --- |
| Lesedaten, Reihenfolge, FPS/Frames | Native Marks inkl. Reihenfolge, Timecode und markRevision über bestehende Webroute | **Ausgenommen** |
| Titel und Recording-ID | Bestehende Recording-Detail-/Playback-Identität | Kein zusätzlicher Cut-/Marks-DTO |
| Pagination/Updates | Einzelaufnahme und SSE-/Readback-Refresh (keine neue Collection) | Kein neuer Cursor oder SSE-Kanal |
| Fehler/Grants | Bestehende Native-Read/POST-Sicherheitsgrenzen | Keine automatische Browser-Session- oder Device-Berechtigung |
| MLD, VIDAA, Tizen | Keine gleichwertige Marks/Cut-Nutzung aus F6 belegt | F6 beansprucht keine Unterstützung |
| Live Integration | Bereits eingebundener Recordings-2-Editor und native Cut-Ergebnisbeobachtung | Nicht in F7-Playback- oder F8-Releaseabnahme hineininterpretieren |

**Bestehende Real-VDR-Evidenz wird übernommen, nicht wiederholt:**
[Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
dokumentiert den früher gemergten Web-Editor, real geprüfte
Marks-Änderungen, einen früheren Native-Cut-Pfad und späteres
externes Marks-Live-Refresh. Die Test-/Hardwaregrenzen dort
gelten fort. F6 hat keinerlei Aufnahme mutiert, geschnitten,
getestet oder erneut installiert.

**Konkurrenzvergleich:** Kodi/Plex sind hier lediglich
Bedienbarkeitsreferenzen für klar erkennbare Schnittmarken,
Cut-Vorschau und einen unterscheidbaren laufenden Schnitt.
Aktuelle konkrete Kodi-/Plex-Funktionen und deren native
Cut-/Marks-Schnittstellen wurden **nicht** verifiziert.
Daraus folgt weder Gleichwertigkeit noch ein Entwicklungsauftrag.

## F6-Abnahme, Folgen für F7 und F8

**F6 = abgeschlossen als dokumentierte, quellenbelegte
Scopeentscheidung.** Produktcode, VDR-/Agent-Backend, OpenAPI,
Reference-JS-Client, Security-Grants, installiertes Webfrontend
und PWA.15-Performance-Freeze bleiben unverändert.
Keine neuen CI-/Installations-/Runtime-Gates.

**F7:** Playback, Live/Recording, Progress/Resume und Device-Clients
innerhalb ihrer bereits vorgesehenen Public-v1-Scope
prüfen – **keine** implizite Marks-/Cut-Mutation und
keine Voraussetzung, dass TV-/MLD-Clients schneiden können.

**F5:** Timer-/SearchTimer-Interop bleibt auf Wunsch des Betreibers
zurückgestellt, mit späteren echten MLD-Rückmeldungen.

**F8:** Ein einheitlicher Release-/Hardware-Abschluss.
Marks/Cut werden darin als **explizit außerhalb Public v1**
klassifiziert, nicht als fehlgeschlagener Pflicht-Test.
Die einzige laufende Hardware-Abnahmematrix bleibt
[Public-v1 Final Hardware Acceptance](public-v1-final-hardware-acceptance.md).
