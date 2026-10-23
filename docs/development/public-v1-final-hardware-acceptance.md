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