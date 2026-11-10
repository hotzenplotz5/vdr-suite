# ST.V1 – Einheitlicher epgsearch-Suchtimer-CREATE über Public API v1

Arbeitsbranch: `MLD-Suchtimer` (kein Merge nach `main`). Start-HEAD: `037e0545a89e5c86670563b6ce046f24947704e1`.
Diese Roadmap ist ein Arbeitsplan, keine Behauptung bestandener Tests oder einer erfolgten Installation.

## Ziel und verbindliche Architektur

Ein Benutzer erstellt aktiv oder inaktiv dieselbe vollständige epgsearch-Regel über
`POST /api/v1/search-timers`, unabhängig davon, ob Browser, MLD, yaVDR oder
ein autorisierter Device-Client aufruft. Browser- und Device-Authentifizierung
bleiben verschieden; Berechtigungsprüfung, fachliche Validierung, Operation,
Idempotenz, ein einziger RESTfulAPI-Writer und vollständiger Provider-Readback
bleiben gemeinsame Produktionsautorität. Unsichere/unklare Ergebnisse führen
nie zum automatischen zweiten CREATE. Kein weiterer produktiver Legacy-CREATE.

## ST.V1.0 – Inventur und Feldmatrix

**Aus der Quellprüfung zu Beginn:** `SearchTimerCreateRequest` enthält bereits
umfangreiche epgsearch-Optionen. `RestfulApiSearchTimerMapper` liest viele
dieser Optionen aus `/searchtimers.json`. Die Public-v1-Dauerpayload Version 2
enthält dagegen nur Name, Suchbegriff, Aktivität, Match-Modus/Toleranz,
drei Suchbereiche sowie VDR-Margins. `SearchTimerCreateDispatchService`
überträgt/verifiziert nur diese Teilmenge. Die vorhandene Public-v1-Runtime
lehnt `active:true` explizit ab. Im Branch ist im Gegensatz dazu der
Legacy-Browser-CREATE wieder freigeschaltet. Das ist keine Zielarchitektur.

| UI / Benutzerfunktion | Public-v1 / dauerhafte Payload | Core / RESTfulAPI | Provider-Readback | Befund |
|---|---|---|---|---|
| Suchbegriff, Modus, Toleranz, Titel/Untertitel/Beschreibung | Teilmenge vorhanden | Vorhanden | Vorhanden | Teilweise verbunden |
| Aktiv/inaktiv | Nur `false` zugelassen | `active` vorhanden | Status vorhanden | Aktiv gesperrt |
| Groß-/Kleinschreibung, Summary-Match | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |
| Sender-Modus/Gruppe/Bereich/FTA | Fehlt | `useChannel`, `channels`, `channelMin/Max` | Teilweise vorhanden | Exakte Provider-Modi prüfen |
| Start/Ende, Wochentage, Dauer | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |
| Verzeichnis, Priorität, Lebensdauer, VPS, Margins | Nur VDR-Margins | Vorhanden | Vorhanden | Verbindung fehlt |
| Serienaufnahme, Aktionen, Aufbewahrung | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |
| Wiederholungsgrenzen/-vergleiche | Fehlt | Vorhanden | Teilweise vorhanden | Readback-Details prüfen |
| Blacklists, EPG-Kategorien/-Zusatzinformationen | Fehlt | Vorhanden | Teilweise vorhanden | Verlustfreie Abbildung prüfen |
| Gültigkeit, Favoriten, Folgeaktionen | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |

**Zu verifizieren, bevor produktive Felder geschrieben werden:** exakte
`SearchTimerRestfulApiBodyBuilder.inc`-Felder, Readback-Katalog, Parser,
Browser-Editor, historischer MLD-Stand, Public-v1-HTTP-Router, Operation-Version,
Sicherheitsgrenzen und die epgsearch-Plugin-Version. Bei einer nicht
nachweisbar unterstützten Option muss ein sichtbarer Validierungsfehler
statt stiller Feldverluste erscheinen.

## Umsetzungsslices mit Abnahme

- **ST.V1.0 Inventur:** Ist-Pfad `HTTP → Public-v1-Handler → DaemonRuntime →
  SearchTimerCreateAdmissionService → MutationOperationRepository →
  SearchTimerCreateDispatchService → RestfulApiSearchTimerCommandExecutor →
  /searchtimers.json Readback` und Browser-Legacy vergleichen; Feldmatrix
  nach tatsächlichem Provider ergänzen.
- **ST.V1.1 Public-v1-Vertrag:** Volle strukturierte Optionen in denselben
  Request aufnehmen, strikt validieren, Default-/Altrequests erhalten,
  Payload versionieren und Fingerprint/Idempotenz über alle Optionen binden.
  OpenAPI aktualisieren.
- **ST.V1.2 Core/Backend:** Felder ohne Informationsverlust bis zum vorhandenen
  RESTfulAPI-BodyBuilder übertragen. Keine zweite Service- oder Writer-Instanz.
  Agent-Generationsgrenze und einmalige Dispatch-CAS erhalten.
- **ST.V1.3 Native Readback:** Nach CREATE native neue ID, Backend, Aktivität
  und jede angeforderte wirksame Provider-Option gegen eine vollständige
  neue epgsearch-Inventur prüfen. Abweichung/Backendfehler bleibt
  `outcome_unknown`; keine Wiederholung. Provider-Ausnahmen separat notieren.
- **ST.V1.4 Webfrontend:** Vorhandenen vollständigen Editor erhalten und
  ausschließlich auf Public API v1 umstellen. Aktive/inaktive Auswahl;
  Operation/Readback und SSE für asynchrone Zustände statt zusätzlichem Polling.
  Browser- und Device-Credentials nicht vermischen.
- **ST.V1.5 Clients:** Einheitliches OpenAPI, Device-/Browser-Erstellungsweg,
  Referenz-JS-Client und dokumentierte Consumer-Felder. Keine Samsung-/VIDAA-
  Repository-Änderungen ohne Auftrag.
- **ST.V1.6 Tests:** Kontrakt-/Parser-/Payload-/Idempotenztests, aktive und
  inaktive Fälle, vollständige Feldkombinationen, gezielte Auth-Regressions,
  API-v1-Produktroute, Backendfehler, unbekanntes Ergebnis, Readback-Mismatch
  und doppelte Anlage; exakten C++-Kandidaten kompilieren.
- **ST.V1.7 Installation/Hardware:** Getrennte Paket-/Runtime-/VDR-Tests
  für yaVDR und MLD 6.6. Keine produktive Installation ohne Freigabe.
- **ST.V1.8 Abschluss:** Dokumentation und Status nach belegten Ergebnissen
  berichtigen, Legacy-CREATE produktiv schließen, Feldmatrix und
  Abschluss-SHA mit Ebenen UNIT/CONTRACT, PRODUCTION-INTEGRATION,
  CI und REAL-VDR getrennt festhalten.

## Abnahmekriterien

`active:true` und `active:false` ohne Feldverlust; genau ein Writer und
einmalige Mutation pro Idempotency-Key; native ID und alle schreibbaren Optionen
bestätigt; bestehende Grants unverändert wirksam; Editor nutzt ausschließlich
den gemeinsamen API-v1-Vertrag; alter Legacy-CREATE ist nicht wieder aktiv.
Quellcode-Implementierung und echter Build sind von Real-VDR-Abnahme zu trennen.

## Verifikationsstand

- AGENTS.md (1112 Zeilen) und die SuiteBridge-Unteranweisungen gelesen.
- Remote-Start-HEAD nachgewiesen; `main` nicht verändert.
- **Noch offen:** genaue Feldmatrix gegen Provider, Umsetzung aller Slices,
  produktiver C++-Build und Real-VDR-Tests.
