# Public API v1 – ein zentraler Acceptance-Runner

Die **einzige verbindliche Hardware-Abnahmecheckliste** ist [public-v1-final-hardware-acceptance.md](../../docs/development/public-v1-final-hardware-acceptance.md). Dieser Runner ergänzt die Liste als ausführbarer Prüfadapter. Offline-CI oder HTTP 200 ersetzen keine echte Real-VDR-Abnahme.

## yaVDR: Standardlauf ohne Schreibzugriffe

    # From an isolated PR #526 checkout, not the production main checkout:
    python3 tools/run_api_v1_acceptance.py --profile readonly --expected-daemon-commit 68f6d05f0ed6dbd6691776910e17b29e0b779554
    cat ~/vdr-suite-api-v1-results/last-summary.txt

Fehler lesen:

    cat ~/vdr-suite-api-v1-results/last-errors.log

Offline (kein Daemon erforderlich):

    python3 tools/run_api_v1_acceptance.py --profile offline
    python3 -m unittest discover -s tests -p test_api_v1_acceptance_runner.py -v

Als vorhandener Administrator: `--profile readonly --admin-user vdr-suite --expected-daemon-commit <vollständiger-SHA>`. Der Runner liest das Passwort verdeckt am Root-Terminal ein; es wird weder gespeichert noch protokolliert. Er verwendet eine kurzlebige Browser-Session. **Nur** der fest verdrahtete Session-Login und Session-Logout verwenden POST (Security-Session-Persistenz); sämtliche VDR-/Public-v1-Acceptance-Requests bleiben GET. Keine neuen Konten, Geräte, Grants, Timer oder Aufnahmen. Ein Admin ist kein Device: Device-eigene Playback/Resume-Gates bleiben BLOCKED. Negative Authentifizierungstests senden bewusst keinen Session-Cookie.

Der Standard-Origin ist http://127.0.0.1:18080; der Backend ist default. Über CLI --origin und --backend überschreibbar. HTTP nur auf Loopback, HTTPS für entfernte Origins. Außer ausdrücklich gewähltem Session-Login und -Logout ausschließlich serielle GETs, Timeout 8 Sekunden, JSON maximal 1 MiB, Artwork maximal 16 MiB (Serverlimit VdrRecordingArtworkService); keine Redirects. Es gibt **keine produktive VDR-Mutationsfunktion**, keine POST-dry-runs, keine MediaSessions, keine automatischen Retries oder Dienstneustarts.

## Authentifizierung und private Konfiguration

Externe Datei: ~/.config/vdr-suite/api-v1-acceptance.json. JSON-Objekt, Dateirechte 0600, kein Symlink. Unterstützte Schlüssel:

| Feld | Zweck |
|---|---|
| device.credentialId und device.credentialSecret | Vorhandenes gepaartes Device-Credential |
| origin, backend | Optionaler Origin / genehmigter Backend |
| expectedDaemonCommit | Vollständiger 40-stelliger Git-Commit des installierten Daemons, unabhängig vom Runner-Branch |
| deniedBackend | Nachweislich nicht berechtigter Backend für Negative-Test |
| existingOperationId | Bereits vorhandene eigene öffentliche `op_`-ID für autorisierten Operation-GET; niemals neu erzeugen |
| mediaProfile.appVersion, mediaProfile.firmwareVersion, mediaProfile.playerVersion, mediaProfile.engine | Tatsächliches Device/Player-Profil |

Alternativ nimmt `--config /pfad/zur/bestehenden-credential-datei.json` eine ausdrücklich ausgewählte, private **flache** Device-Receipt mit `credentialId` und `credentialSecret` entgegen. Dieses Altformat kam bei früheren Hardware-Tests zum Einsatz (historischer Pfad: `/run/vdr-suite-api-v1-hqoedm30.json`). Die Datei wird **nur gelesen**, niemals kopiert oder geändert. Eine fehlende oder ungültige Receipt wird nicht automatisch durch Pairing ersetzt. Keine ungefragte Credentialsuche.

Das bestehende Public-v1-Auth-Schema ist VDR-Suite-Device (opaque ID und Secret). Credentials gehören nie in Git, Shell-Befehle oder Logs. Fehlende Device-Grants, andere Benutzer/Accounts oder Administrator-Identitäten werden nicht automatisch eingerichtet; Tests dafür bleiben BLOCKED. Nur ein echter gekoppelter Device mit Backend-Grants kann devicegebundene Leseverträge erfolgreich prüfen.

## Bestehende Identitäten ohne neue Kopplung prüfen

Der vorhandene Human-Admin-Account `vdr-suite` kann bereits `GET /api/v1/accounts` und `GET /api/v1/devices` benutzen. Ein bestehendes Device ist dabei nicht automatisch vom Runner authentifizierbar: dessen einmalig ausgegebenes Credential-Secret gehört weiterhin ausschließlich dem Device. Statt auf Verdacht weitere Testgeräte anzulegen, kann der Root-Operator über `python3 tools/api-v1-acceptance/inspect_existing_access.py --admin-user vdr-suite` die **bestehenden** Account-/Device-Grants lesen. Der Bericht zeigt nur bereinigte Namen, Status und Permission@Scope; IDs und Secrets werden nicht ausgegeben. Nur temporärer Browser-Session-Login/Logout nutzen POST. Alle weiteren HTTP-Anfragen sind GET. Es werden keine Paare/Grants/Accounts/Timer erzeugt.

Beim echten yaVDR-Read-only-Lauf vom 2026-10-23 wurden 4 Devices festgestellt (2 aktiv: `yaVDR Timer Hardwaretest`, `Hisense VIDAA TV`; 2 widerrufen) und 4 Human Accounts. Der Anzeigename eines Human Accounts entspricht nicht zwingend dem Login-Namen `vdr-suite`; die Authentifizierungszuordnung darf nicht aus dem DisplayName geraten werden. Nächster Schritt ist ausschließlich die bestehenden Grant-Sätze festzustellen und zu klären, ob ein gültiges, vom Testoperator ausdrücklich ausgewähltes Device-Credential existiert. Die serverseitigen Einwegverifier können das Secret nicht rekonstruieren.

## Bereits gekoppeltes VIDAA Device authentifiziert prüfen, ohne Secret-Export

Die vorhandene Hisense-VIDAA-App speichert ihr gepaartes Secret ausschließlich in ihrem origin-eigenen clientseitigen Speicher. Ihr TV-Menü **Einstellungen → Zugang prüfen** kann selbst einen autorisierten Public-v1-GET mit diesem Secret auslösen (TV-Version/Installation separat prüfen). Das Secret wird **nicht** an den Root-Host übertragen, nicht aus localStorage exportiert und nicht im Backend zurückgelesen. Zum unabhängigen Nachweis der Auth-Gate-Verarbeitung kann der Root-Operator anschließend `python3 tools/api-v1-acceptance/inspect_device_auth_audit.py --device-name 'Hisense VIDAA TV' --minutes 30` ausführen. Wichtig: Der TV-Button **Zugang prüfen** führt lediglich `GET /api/v1/backends` durch. Ein **erfolgreicher** Backend-Discovery-GET passiert laut `SecurityHttpGate` ohne `appendDecisionEvent` und kann darum **nicht** allein durch den Audit-Collector als PASS nachgewiesen werden. Danach am bereits gekoppelten TV ausdrücklich eine vorhandene, geschützte **Aufnahmen- oder EPG-Seite** öffnen, die z. B. `GET /api/v1/recordings?backendId=default&limit=...` bzw. einen `epg.view@default`-GET ausführt. Nur dieser echte backendgebundene Autorisierungspfad ist für `authorization.allowed` im Audit geeignet. Ein UI-Fehler oder eine nicht ausgeführte Anfrage bleibt NOT_VERIFIED; keine TV-Version oder erfolgreiche Wiedergabe unterstellen. Das Werkzeug liest ausschließlich `security_devices` und `accountability_events` der **tatsächlich laufenden Daemon-Datenbank** mit SQLite `mode=ro` plus `PRAGMA query_only`; den DB-Pfad liest es nur aus dem Daemon-Prozess-Environment, ohne dieses Environment auszugeben. Bei fehlender Pfadangabe oder DB-Schema wird fail-closed BLOCKED statt auf eine angenommene Produktionsdatei auszuweichen. Keine HTTPS-Tokens, Credential IDs, Actor IDs, Request IDs oder DB-Inhalte außer bereinigten, aggregierten Entscheidungssummen gelangen in die Ausgabe.

**Beweisgrenze:** Ein echtes `authorization.allowed` mit authentifiziertem Service-Actor und exaktem Device, aktiver Identität und Backend `default` beweist nur, dass der serverseitige Security-Gate-Check für diesen Device erfolgreich war. Er beweist weder den HTTP-Endstatus noch den Erfolgsweg der Domain, Playback/Transcoding oder des echten TV-Bildes. Ohne Ereignis innerhalb der gewählten Zeitspanne ist der Befund NOT_VERIFIED, nicht FAIL. Der volle Runner braucht weiterhin ein autorisiert bereitgestelltes Device-Credential oder real ausgeführte und dokumentierte Device-Clientprüfungen, die dessen Secret nicht exportieren.

## Output und Exitcodes

Das Ausgabeverzeichnis ist ~/vdr-suite-api-v1-results/ mit 0700. Dateien: last-summary.txt, last-errors.log, last-results.json, last-full.log sowie runs/UTC-TIMESTAMP/{summary.txt,errors.log,results.json,full.log} (0600). Nach jedem einzelnen Testfall wird atomar ein Snapshot persistiert; auch ein Abbruch bewahrt frühere Ergebnisse. Query-Werte, öffentliche Objekt-IDs, Media-URLs und Zugangsdaten werden nicht in Protokolle übernommen. Fehlerdaten enthalten sichere Routenstruktur, Kategorie, erwarteten/aktuellen HTTP-Status, erwartete Struktur und gegebenenfalls bereinigte Correlation-/Operation-ID.

PASS = nachgewiesen, FAIL = tatsächliche Abweichung, SKIP = bewusst nicht ausgeführt, BLOCKED = Voraussetzung fehlt. Gesamt PASS nur wenn alle Pflichtprüfungen des gewählten Profils PASS; FAIL bei Fehlern, INCOMPLETE bei Blockern. Exitcodes: 0 PASS; 1 FAIL; 2 INCOMPLETE; 3 CONFIG_ERROR.

Der installierte Daemon wird ausschließlich mit dem expliziten vollständigen Zielcommit `--expected-daemon-commit` bzw. `expectedDaemonCommit` aus der privaten Konfiguration verglichen, nicht mit dem Runner-HEAD. Ein echter Mismatch ergibt FAIL und sperrt alle abhängigen Hardware-GETs (BLOCKED); fehlender oder nicht beweisbarer Build-Nachweis ergibt BLOCKED. VDR-/SuiteBridge-/Agent-Version und Binäridentität bleiben getrennt zu prüfen.

**Real-yaVDR Admin-Lauf 2026-10-23 nach der 16-MiB-Korrektur (`runner=08a651e63e4ba8672793c673c81f5597a416197a`):** 30 PASS, 0 FAIL, 7 SKIP, 17 BLOCKED; INCOMPLETE. Artwork-Binary, EPG und Recording-Leseprüfungen sind auf echter Hardware bestanden. `RECORDINGS SKIP` im Bereichsstatus war ausschließlich die falsche Runner-Priorisierung von SKIP vor PASS, korrigiert ohne Änderung der Testfall-Zählung. `GET /api/v1/timer-assignments` lieferte 403, weil `role.admin@*` laut `AuthorizationService` nur für ausdrücklich benannte native Timer-Aktionen ein Ersatz für `timers.view` ist. Normale TimerAssignments brauchen `timers.view@backend`. SearchTimer-GETs erzwingen im `SecurityHttpGate` bereits vor Grant-Prüfung einen verifizierten Device-Credential/Service-Actor; ein Human-Admin-Cookie ist hierfür kein Ersatz. Der Runner behandelt solche SearchTimer-Aufrufe bei Admin-Anmeldung daher mit explizitem BLOCKED, statt garantiert verweigerte GETs zu wiederholen. Kein Device/Grant/Pairing wird automatisch angelegt.

**Real-yaVDR Admin-Lauf 2026-10-23 (vor der 16-MiB-Korrektur):** 25 PASS, 1 FAIL, 7 SKIP, 21 BLOCKED. Ein vorhandenes Recording-Artwork überschritt das willkürliche frühere 256-KiB-Testlimit, während der kanonische Artwork-Dienst Bilder bis 16 MiB akzeptiert; der vermeintliche Artwork-Defekt ist durch den Runner-Bug noch **nicht** als Serverfehler belegt. Browser Admin kann Channels, EPG und Recording GET prüfen; einzelne `timers.view`- und `searchtimers.view`-Routen lieferten HTTP 403 unter dem bestehenden Account und bleiben als Berechtigungsgrenzen BLOCKED, nicht künstlich PASS. Negative Invalid-ID-Prüfungen laufen nun gegen die vorhandene authentifizierte Identität statt fälschlich ein Device zu verlangen. Device-only Routen bleiben Device-only.

## Abdeckung und noch offene Real-Hardware-Gates

Safe GET-Prüfketten umfassen Discovery, Capabilities, negative Authentifizierung, Backend-Grant-Isolation (mit geeignetem deniedBackend), Channels → EPG Now/Next/Window + Cursor, native Timer/Detail und Assignments samt optionalem Existing-Operation-Read, SearchTimer Liste/Detail/Revision/Preview/Draft-Preview, Recording Liste/Ordner/Detail/Metadaten/Genre/Artwork **einschließlich echter Bildbytes**, sowie P1 Media-Profil, Continue Watching und Resume Progress. Maximal kleine begrenzte Seiten werden abgefragt. Fehlende Testobjekte blockieren abhängige Prüfungen.

Die zentrale Hardwarematrix erfordert zusätzlich Human-/Admin-/Browser-CSRF- und Grant-Fälle, Geräte-Eigentum/Fremdbenutzer, Provider-Fehler, Agent-/Plugin-Versionen, produktives Readback, Retry/Recovery, native Gen-/Revision-Races, TimerIntent- und SearchTimer-Mutationen, Recording RENAME/MOVE/DELETE, Playback-Session und Resume-Write, Neustartpersistenz und reale Clientkette. OpenAPI bezeichnet sich selbst als partielle Referenz; auch ein vollständiger Offline-Lauf kann diese Lücken nicht schließen. Legacy-/api-Tests werden niemals als Public-v1-PASS importiert.

## Kontrolliertes Profil: bewusst verriegelter Mutationspfad

    python3 tools/run_api_v1_acceptance.py --profile controlled --approval-file /path/to/private-approval.json

Das Approval-Manifest muss restriktive Rechte 0600 haben und den exakten **Daemon-Zielcommit** als sourceCommit (nicht Runner-HEAD), backend, targetHost, approvedFixtureIds, backupEvidence sowie rollbackEvidence enthalten. **Diese Datei entsperrt derzeit keinen produktiven HTTP-Write.** Kontrollierte Tests bleiben explizit BLOCKED, bis ein separat geprüfter Mutationsdispatcher sowie deine konkrete Freigabe mit neuen, eindeutig identifizierten Fixtures vorliegen. Keine historischen Hardware-Testobjekte wiederverwenden und keine pauschale Bereinigung durchführen.

Der CI-Workflow public-api-v1-acceptance.yml führt die Offline-Matrix ohne Hardware und ohne Credentials aus.
