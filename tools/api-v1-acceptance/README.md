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

Der Standard-Origin ist http://127.0.0.1:18080; der Backend ist default. Über CLI --origin und --backend überschreibbar. HTTP nur auf Loopback, HTTPS für entfernte Origins. Ausschließlich serielle GETs, Timeout 8 Sekunden, JSON maximal 1 MiB, Artwork maximal 256 KiB; keine Redirects. Es gibt **keine HTTP-Mutationsfunktion**, keine POST-dry-runs, keine MediaSessions, keine automatischen Retries oder Dienstneustarts.

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

## Output und Exitcodes

Das Ausgabeverzeichnis ist ~/vdr-suite-api-v1-results/ mit 0700. Dateien: last-summary.txt, last-errors.log, last-results.json, last-full.log sowie runs/UTC-TIMESTAMP/{summary.txt,errors.log,results.json,full.log} (0600). Nach jedem einzelnen Testfall wird atomar ein Snapshot persistiert; auch ein Abbruch bewahrt frühere Ergebnisse. Query-Werte, öffentliche Objekt-IDs, Media-URLs und Zugangsdaten werden nicht in Protokolle übernommen. Fehlerdaten enthalten sichere Routenstruktur, Kategorie, erwarteten/aktuellen HTTP-Status, erwartete Struktur und gegebenenfalls bereinigte Correlation-/Operation-ID.

PASS = nachgewiesen, FAIL = tatsächliche Abweichung, SKIP = bewusst nicht ausgeführt, BLOCKED = Voraussetzung fehlt. Gesamt PASS nur wenn alle Pflichtprüfungen des gewählten Profils PASS; FAIL bei Fehlern, INCOMPLETE bei Blockern. Exitcodes: 0 PASS; 1 FAIL; 2 INCOMPLETE; 3 CONFIG_ERROR.

Der installierte Daemon wird ausschließlich mit dem expliziten vollständigen Zielcommit `--expected-daemon-commit` bzw. `expectedDaemonCommit` aus der privaten Konfiguration verglichen, nicht mit dem Runner-HEAD. Ein echter Mismatch ergibt FAIL und sperrt alle abhängigen Hardware-GETs (BLOCKED); fehlender oder nicht beweisbarer Build-Nachweis ergibt BLOCKED. VDR-/SuiteBridge-/Agent-Version und Binäridentität bleiben getrennt zu prüfen.

## Abdeckung und noch offene Real-Hardware-Gates

Safe GET-Prüfketten umfassen Discovery, Capabilities, negative Authentifizierung, Backend-Grant-Isolation (mit geeignetem deniedBackend), Channels → EPG Now/Next/Window + Cursor, native Timer/Detail und Assignments samt optionalem Existing-Operation-Read, SearchTimer Liste/Detail/Revision/Preview/Draft-Preview, Recording Liste/Ordner/Detail/Metadaten/Genre/Artwork **einschließlich echter Bildbytes**, sowie P1 Media-Profil, Continue Watching und Resume Progress. Maximal kleine begrenzte Seiten werden abgefragt. Fehlende Testobjekte blockieren abhängige Prüfungen.

Die zentrale Hardwarematrix erfordert zusätzlich Human-/Admin-/Browser-CSRF- und Grant-Fälle, Geräte-Eigentum/Fremdbenutzer, Provider-Fehler, Agent-/Plugin-Versionen, produktives Readback, Retry/Recovery, native Gen-/Revision-Races, TimerIntent- und SearchTimer-Mutationen, Recording RENAME/MOVE/DELETE, Playback-Session und Resume-Write, Neustartpersistenz und reale Clientkette. OpenAPI bezeichnet sich selbst als partielle Referenz; auch ein vollständiger Offline-Lauf kann diese Lücken nicht schließen. Legacy-/api-Tests werden niemals als Public-v1-PASS importiert.

## Kontrolliertes Profil: bewusst verriegelter Mutationspfad

    python3 tools/run_api_v1_acceptance.py --profile controlled --approval-file /path/to/private-approval.json

Das Approval-Manifest muss restriktive Rechte 0600 haben und den exakten **Daemon-Zielcommit** als sourceCommit (nicht Runner-HEAD), backend, targetHost, approvedFixtureIds, backupEvidence sowie rollbackEvidence enthalten. **Diese Datei entsperrt derzeit keinen produktiven HTTP-Write.** Kontrollierte Tests bleiben explizit BLOCKED, bis ein separat geprüfter Mutationsdispatcher sowie deine konkrete Freigabe mit neuen, eindeutig identifizierten Fixtures vorliegen. Keine historischen Hardware-Testobjekte wiederverwenden und keine pauschale Bereinigung durchführen.

Der CI-Workflow public-api-v1-acceptance.yml führt die Offline-Matrix ohne Hardware und ohne Credentials aus.
