# API.V1.F6 — Recording Marks/Cut: Public-v1-Integration und einfache Rechte

**Status: F6 wieder geöffnet; Rechtevereinfachung im Quellcode implementiert,
Public-v1-Marks-/Cut-Routen noch ausstehend.**

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

## F6.2 — Public-v1-Routen (offen, kein Scope-Ausschluss)

Bereits vorhanden und funktionsfähig:

- `GET/POST /api/vdr/recordings/marks` —
  `RecordingMarksApiRuntime`, native Marken und Revisionen
- `GET/POST /api/vdr/recordings/cut` —
  `RecordingCutApiRuntime`, Native-Cut und Cut-Status
- Webfrontend `recordings2-marks-*.js`, Backend Agent und
  SuiteBridge zur VDR-eigenen Marken-/Schnittinstanz.

Die partielle `api/openapi.json` enthält noch **keine**
Public-v1-Marks-/Cut-Endpunkte.
F6.2 muss vorhandene Domain-Services ohne zweiten Writer oder
zweite native Schnittlogik unter den **öffentlichen** API-v1-Vertrag
bringen. Dafür nötig sind opake, backend-gebundene
`rec_`-Aufnahmeidentitäten, passende Sicherheitsprojektion,
Revision/Idempotenz und gleichwertige Fehler-/Statussemantik.
Die Browserfunktionen bleiben funktionsfähig, bis eine echte
Web-vs-v1-Regressionsprüfung gleichwertige Antworten belegt.

**ADR-0059 bleibt gültig:** VDR ist einzige Autorität für
Marks, Index und Schneiden; Kontrollplan, Agent und SuiteBridge
bleiben einzige Ausführungskette. Die Benutzerebene darf die
neue Sammelberechtigung verwenden, ohne diese technischen
Kontrollen zu umgehen.
[ADR-0059](../adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)

## Abnahme und Folgeschritt

Quellen-/Mock-Test `test_mu9e_grant_mutation_ui.js`:
**PASS** im isolierten JavaScript-Testadapter für die geänderte
Picker-Auswahl. C++-Tests für Grant-Katalog und
AuthorizationService sind ergänzt, ein echter C++-Build,
Hosted CI und eine yaVDR-Installation am exakten Kandidaten
sind **noch nicht nachgewiesen**. Keine produktive Aufnahme
mutiert, geschnitten oder gelöscht.

F6.1 ist **committed, aber noch nicht hardware-/build-abgenommen**.
F6.2 (echte Public-v1-Routen/Parität) bleibt **offen**;
die frühere pauschale Scope-Ausnahme ist aufgehoben.
Weder MLD noch VIDAA oder Tizen müssen eine eigene
Schneideoberfläche implementieren — sie dürfen die Public-v1-
Funktion nach Abschluss nutzen, sofern gewünscht und berechtigt.
Die gemeinsame Release-/Hardware-Abnahme bleibt
[an einer zentralen Stelle](public-v1-final-hardware-acceptance.md).
