# Public API v1 – einmalige End-to-End-Abnahme am realen VDR

**Status: VORBEREITET – NICHT AUSGEFÜHRT.**

**Verbindliche Arbeitsregel (22.10.2026):** Erst die gesamte API-v1-Implementierung vollständig integrieren, dann **eine gemeinsame, nachvollziehbare Endabnahme am echten VDR**. Keine separaten, wiederholten Hardware-Slices je API-Funktion. Automatische Source-CI bei Commits ersetzt die Endabnahme nicht. Eine neue Änderung nach Abnahme erfordert nur die betroffenen Regressionen und die zentralen Pflichtprüfungen, nicht einen Neustart der gesamten Entwicklung.

Diese Checkliste ist der **einzige operative Hardware-Abnahmeplan** für API v1. Bestehende fachliche ADRs, OpenAPI und `docs/CURRENT.md` bleiben Vertrags- bzw. Statusquellen; bei Abweichung den offenen Punkt hier notieren, keine zweite parallele Checkliste anlegen.

## 0. Vorbedingung: Der Implementierungsumfang ist geschlossen

Alle Kästchen dieses Abschnitts müssen vor dem ersten realen VDR-Mutationstest abgehakt sein. Ein vorhandener Branch, ein grüner Unit-Test oder eine syntaktisch vorhandene Route gilt **nicht** als integrierte Funktion.

- [ ] Sämtliche für die API-v1-Freigabe vorgesehenen Implementierungen sind im gemeinsamen Integration-HEAD enthalten und durch **eine** API-v1-Versionsmatrix abgedeckt (keine schwebenden konkurrierenden Branch-Versionen).
- [ ] OpenAPI, Referenz-JS-Client, Auth-/Grant-Modell, Daemon-Komposition, Provider-/Agent-Transport und tatsächlich installierte Paketinhalte entsprechen demselben Git-Commit.
- [ ] Source-CI, Build der nativen Plugins und Release-/Packaging-CI sind grün für **genau** diesen HEAD; kein älterer erfolgreicher Lauf als Ersatz.
- [ ] Native Suchregel-Schreiboperationen verfügen über echten epgsearch-lock-atomaren Generations-/Revisionsvergleich, persistierte Metadaten und vollständige readback-verifizierte Operationen; kein SVDRP-/RESTfulAPI-Write-Fallback.
- [ ] Backend-Anbindung funktioniert durchgängig: Public-v1-Route → Device-/Browser-Gate → Backend-Grant → dauerhaft reservierter Operation-Datensatz → Agent-/SuiteBridge-Transport → native epgsearch-`Service()` → authoritative readback → Operation-Abschluss. **Keine** vorbereitete Mock-Provider-Schnittstelle wird als produktiv verdrahtete Implementierung gewertet.
- [ ] Echte Provider-Capability wird nur bei kompatiblem epgsearch/SuiteBridge aktiviert. Ist sie nicht nachweisbar, sind UPDATE, Aktivieren, Deaktivieren und Löschen explizit geschlossen (Fehler statt unsicherem Fallback).
- [ ] Ein einziger Release-Kandidat, ein Change-Log und eine Rückfallstrategie stehen fest. Keine Änderung an produktiven Daten vor dokumentierter Backup-Entscheidung.
- [ ] Die Freigabegrenzen und noch bewusst nicht implementierte Endpunkte sind in OpenAPI und Release-Notizen identisch dokumentiert.

**Stopp:** Solange eines dieser Kästchen offen ist, **kein** Hardware-Mutationstest. Dann weiter implementieren und Source-CI reparieren, aber keine neue Phase/Slice erfinden.

## 1. Vorbereitung – genau eine reale VDR-Testsitzung

- [ ] Git-Commit/SHA des Kandidaten: `______________________________`
- [ ] Datum/Zeitzone, VDR-Version, Plugin-Versionen/ABI, SuiteBridge-Version, Daemon-Version und Paketnamen festhalten.
- [ ] Aktuellen Zustand der Dienste, Backend-Zuordnung, Timer-/Suchregel-Anzahl sowie HTTP-/API-Erreichbarkeit **nur lesend** erfassen.
- [ ] Zielsystem verifizieren: Test findet ausschließlich auf dem ausdrücklich freigegebenen yaVDR statt; kein stillschweigendes Deployment aus einem Chat.
- [ ] Vorab entscheiden: Sicherung von `epgsearch.conf` samt Metadaten, VDR-Timern und Suite-SQLite; Pfade/Rechte/Owner/Checksumme dokumentieren. Datenschutzrelevante Inhalte nicht in öffentliche Logs kopieren.
- [ ] Explizite Freigabe für Testobjekte (eindeutig benannte, neu angelegte inaktive Test-Suchregel und je eine Testaufnahme/Test-Timer ohne reale Serienaufnahme). Bestehende echte Timer und Aufnahmen dürfen nicht gelöscht werden.
- [ ] Definiertes Rollback: Pakete/Services, epgsearch-Konfiguration und SQLite können auf den Ausgangszustand zurückgesetzt werden; Prüfpfade und Verantwortlichkeit stehen fest.
- [ ] Ein zentrales Ergebnisprotokoll führt **pro Prüffall** Request-ID, Zeitpunkt, HTTP-Status, korrelierte Operation-ID, nativen Provider-Status, erwartetes/tatsächliches Ergebnis und Beleg/Log-Ausschnitt.

## 2. Public API v1 – lesende Endpunkte und Vertragsprüfung

- [ ] `/api/v1` Discovery/Capabilities, versionspezifische OpenAPI-Pfade und nicht unterstützte Methoden: konsistente Statuscodes, kein Legacy-Fallback.
- [ ] Device-Pairing, Credential-Ausgabe, -Rotation, -Sperrung und Widerruf; negative Authentifizierung mit 401/403 ohne Nebenwirkungen.
- [ ] Backend-Auswahl/Grenzen: exakt berechtigter Backend-Umfang, fremde Backend-IDs abweisen, Offline-Backend als 503 statt leerem 200.
- [ ] Channels/EPG: korrekte Sender- und Ereignisidentität, Zeitfenster, Cursor/Pagination, Grenzwerte, stabile Reihenfolge und stale-event-Verhalten.
- [ ] EPG-Suche/Genres und globale Suche: EPG-/Aufnahmen-Abgrenzung, Filter/Pagination, eindeutige IDs.
- [ ] Timer-/TimerAssignment-/TimerIntent-GETs: geplante vs. wirklich native Timer korrekt unterscheiden, stabile Revisionen/ETags.
- [ ] SearchTimer-Collection/Detail/Preview/Draft-Preview: native IDs bleiben privat, EPG-Preview ist keine Aufnahmegarantie, korrektes Verhalten bei leerer Liste bzw. Provider-Ausfall.
- [ ] Recordings R1/R2/R3: Sammlung, Ordneransicht, Detail, Metadaten, Artwork, Schnittmarken, ETags, korrekte Backend-Bindung.
- [ ] Playback/Resume/ContinueWatching: Medien-URL-Berechtigung, Geräte-/Backend-Scope, Positionspersistenz und keine Weitergabe privater Dateipfade.
- [ ] JSON-Schema, Content-Type, Cache-Control/no-store bei geschützten Antworten, method-not-allowed, falsche UUID-/ID-Formate und unbekannte Ressourcen.
- [ ] Referenz-JS-Client und der vorgesehene VIDAA-Client verwenden ausschließlich die dokumentierten Public-v1-Verträge; keine versteckten alten Schreibendpunkte.

## 3. Public API v1 – schreibende Endpunkte (nur freigegebene Testobjekte)

- [ ] EPG → TimerIntent: gültiger ETag/EPG-Snapshot, einmalige native Erstellung, belastbares Readback; kein Doppel-Timer bei Replay.
- [ ] Native Timer: genehmigte CREATE/MODIFY/TOGGLE/DELETE-Proben, falls Bestandteil des Freigabekandidaten; aufnahme- bzw. laufende Timer nicht unabsichtlich anfassen.
- [ ] TimerAssignment: CRUD/Zuordnung und Revisionskonflikte nur in den öffentlich implementierten Teilverträgen.
- [ ] SearchTimer CREATE: ausschließlich explizit `active:false`, separate Berechtigung und bestätigte inaktive Anlage; keine implizite Aufnahmeplanung.
- [ ] SearchTimer UPDATE: inaktive Testregel, vollständiger erwarteter ETag und Generation, neue persistierte Revision, unveränderte Generation.
- [ ] SearchTimer ACTIVATE: frisch angezeigte, begrenzte EPG-Preview und separate explizite Aktivierungsfreigabe; Zahl gefundener Sendungen nicht als Zahl aufzunehmender Sendungen behaupten.
- [ ] SearchTimer DEACTIVATE: nur richtige native Generation verändern; Revision rotieren.
- [ ] SearchTimer DELETE: keine numerische-ID-only-Löschung, keine fremden VDR-Timer löschen; endgültige Abwesenheit derselben Generation nachweisen.
- [ ] Aufnahmen: RENAME/MOVE und erlaubte MARKS/CUT-Funktionen nur an freigegebenen Testobjekten; keine echte Aufnahme unbeabsichtigt löschen.
- [ ] Jede Mutation: durable Operation, Idempotency-Key, Scope/Actor/Backend, genau ein mutierender Provider-Aufruf, autorisierter Readback-Abschluss, korrekte Audit-Einträge.

## 4. Negative Sicherheits- und Fehlerfälle (Pflicht)

- [ ] Auth fehlt/falsch/widerrufen; Grant fehlt/falscher Backend-Scope; kein Schreibvorgang.
- [ ] Stale Revision/ETag; geänderte native Generation; gelöschte und gleichnamig/-inhaltlich neu angelegte Regel mit wiederverwendeter numerischer ID; kein falscher Schreibvorgang.
- [ ] Zwei parallele Aufrufe mit gleicher Operation/gleichem Idempotency-Key: höchstens **ein** nativer Write, deterministische Konflikt-/Replay-Semantik.
- [ ] Parallel veränderte Regel und ungespeicherte zweite Regel: keinerlei fremde Änderungen durch katalogweites `Save()` unbemerkt persistieren.
- [ ] Agent/Backend/SuiteBridge/epgsearch offline, versionsinkompatibel oder fehlende Fähigkeit: fail-closed, niemals EDIS/MODS/DELS-/RESTfulAPI-Fallback.
- [ ] Simulierter Transportabbruch vor/nach Write oder Persistenzfehler: `outcome_unknown`, **keine** blinde automatische Wiederholung; anschließende autoritative Ermittlung.
- [ ] Neustart von Daemon/VDR/epgsearch: Generation/Revisionskontinuität, Wiederanlauf verwaister Operationen, keine Phantom-Operationen.
- [ ] Bösartige/zu große Felder, ungültiges UTF-8, numerischer Overflow, doppelter JSON-Schlüssel und unzulässige Methoden: verworfen, ohne Nebenwirkungen.
- [ ] Readback mit unvollständigem/dupliziertem Inventar und Out-of-order-Antwort: niemals als erfolgreicher Abschluss bestätigt.
- [ ] OSD und Backend gleichzeitig: Native-Race mit noch geöffnetem OSD-Menü, keine Use-after-free-Situation, keine Änderungen an neu verwendeter ID.

## 5. Paket, Betrieb und abschließende Entscheidung

- [ ] Pakete installieren denselben geprüften Build; alle Binärdateien/Plugins haben passende VDR-ABI und erforderliche Laufzeitabhängigkeiten.
- [ ] Dienststart, Restart und Logs ohne neue kritische Fehler; Datenbankmigration bleibt idempotent.
- [ ] Keine unbeabsichtigten neuen Listener oder erhöhte Netzwerk-/Dateisystemrechte.
- [ ] Native Konfigurationsdateien und SQLite bleiben konsistent; Backups sind bei Bedarf wiederherstellbar.
- [ ] Referenz-Client und reale TV-App lesen/bedienen freigegebene Funktionen ohne Legacy-Write-Fallback.
- [ ] Negativtests nach Produkt-Neustart wiederholen, insbesondere Rechte-Widerruf und Generation/ETag.
- [ ] Testobjekte nach sicherem Readback entfernen; keine Test-Timer/Regeln/Operationen mit unerwünschter Folgewirkung zurücklassen.
- [ ] Alle Pflichtfälle und CI für den **exakten** End-HEAD sind grün; offene Einschränkungen werden als explizite Release-Blocker oder bewusst ausgeschlossen dokumentiert.
- [ ] Abschlussprotokoll mit Entscheidung: **Freigabe** / **Blockiert**, verantwortlicher Review und Datum. Erst dann PR-Review/Merge/Release gemäß ausdrücklicher Freigabe.

## Vorläufige reale yaVDR-Teilnachweise – keine Endabnahme (23.10.2026)

Diese Tabelle dokumentiert bereits vorhandene **produktive Read-only-Evidenz**, ohne die offene Endabnahme-Matrix unten vorzeitig abzuhaken. **Die Phase 0-/Mutation-/TV-Voraussetzungen bleiben offen.** Eine historische Autorisierungsentscheidung und ein gegenwärtiger fachlicher API-Lese-PASS sind unterschiedliche Ebenen.

| Ebene | Exakte Herkunft | Ergebnis | Was dadurch *nicht* nachgewiesen ist |
| --- | --- | --- | --- |
| REAL-VDR / Human-Browser-GET | Installierter Daemon `git-68f6d05f0ed6`, Admin-Session Login `vdr-suite`, Backend `default`, Runner `08a651e63e4ba8672793c673c81f5597a416197a`, 23.10.2026 16:39–16:40 UTC | **30 PASS / 0 FAIL / 7 SKIP / 17 BLOCKED**, INCOMPLETE; Discovery, EPG, Recording-GET und Artwork-Binary real ausgeführt | Sämtliche 17 BLOCKED- und 7 SKIP-Fälle, kontrollierte Mutationen, Device-MediaSession-/TV-Ende-zu-Ende, finale Candidate-Identität |
| REAL-VDR / historisches Device-SecurityGate | Sieben-Tage-Accountability-Audit auf der echten yaVDR-SQLite; `Hisense VIDAA TV` mit verifiziertem Service-Actor, Backend `default`, letztes `authorization.allowed` **2026-10-21T22:21:09Z** | Historische `allowed`-Entscheidungen: `channels.view` ≥35, `epg.view` ≥87, `recordings.view` ≥236, `timers.view` ≥23, `searchtimers.view` ≥14, `media.live.play` ≥19, `media.recording.play` ≥31. **Untergrenzen:** vorheriger Collector las nur die letzten 500 Device-Events und meldete `AUDIT_WINDOW=PARTIAL`. | HTTP-Rückgabecode, VDR-Readback, realer Stream, Bild/Ton, gespeicherter Resume, derzeit verfügbare TV-Verbindung oder der exakte Daemon-Build zum historischen Zeitpunkt |
| REAL-VDR / historisches Timer-Device-SecurityGate | Gleiches Audit, vorhandenes aktives `yaVDR Timer Hardwaretest`, Backend `default`, letztes `allowed` **2026-10-20T13:38:58Z** | `epg.view=16`, `timers.view=35` erlaubte historische Gate-Ereignisse, kein Partial-Hinweis | Timer-/Operation-Abschluss oder schreibende Pfade |
| CI / Audit-Helfer | PR #526, Commit `eea2cbe7029751b04e669078c9e29d3c48612bfb`, [Runner-CI #39994128018](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39994128018) | 41 Python-Tests PASS; verbesserte Audit-Abfrage aggregiert über das gesamte noch erhaltene UTC-Zeitfenster (SQL `COUNT`, 6-Sekunden-Query-Limit). Kein Gerät muss dafür eingeschaltet sein. | Neuer REAL-VDR-Nachweis des erweiterten sieben-Tage-Zählers; installierte Produktänderung; vollumfängliche API-v1-Endabnahme |

**Abhängigkeit ohne TV-Zugriff:** Für einen weiteren vollständigen Device-Ende-zu-Ende-Lauf fehlen im yaVDR-Runner ein ausdrücklich ausgewähltes vorhandenes Device-Credential bzw. ein aktuell ausführender Device-Client sowie für Playback/Resume autorisierte Test-Sessions/Medien und deren Readback. Der lokale SQLite-Audit kann ausschließlich Gate-Evidenz belegen und darf diese Blocker **nicht** als PASS markieren. Aktuell sind keine neuen Accounts, Geräte, Grants, Credential-Rotationen, Daemon-Installationen oder Produktiv-Mutationen genehmigt bzw. durchgeführt. Die ursprüngliche historische Hisense-Auswertung soll nur einmal mit dem korrigierten Collector vollständig nachgelesen werden; der gesamte Hardware-Runner muss deswegen nicht erneut laufen.

## Eine zentrale Ergebnis-Matrix – erst am Tag der Endabnahme ausfüllen

| Bereich | Implementierung vorhanden | Exakte Source-CI | Realer VDR | Entscheidung / Beleg |
| --- | --- | --- | --- | --- |
| Security / Pairing / Grants | offen | offen | nicht begonnen | |
| Discovery / OpenAPI / SDK | offen | offen | nicht begonnen | |
| EPG / Channels / Genres / Search | offen | offen | nicht begonnen | |
| TimerIntent / TimerAssignment / Native Timer | offen | offen | nicht begonnen | |
| SearchTimer Read / CREATE / UPDATE / TOGGLE / DELETE | offen | offen | nicht begonnen | |
| Recording / Metadata / Artwork / Marks / Cut | offen | offen | nicht begonnen | |
| Playback / Resume / Clients | offen | offen | nicht begonnen | |
| Fehlerfälle / Neustart / Recovery / Rollback | offen | offen | nicht begonnen | |

**Nicht als fertig markieren**, solange nur GitHub-Mocks laufen, ein Provider bloß ein Interface implementiert oder keine reale Autorisierung/Readback-Verkettung existiert. Keine separat erfundenen „S1a/S1b“-Abnahmephasen.