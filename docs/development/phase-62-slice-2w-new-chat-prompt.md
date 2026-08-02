# Neuer Chat-Prompt — VDR-Suite Phase 62 Slice 2W

Kopiere den vollständigen Inhalt des folgenden Blocks in einen neuen Chat.

```text
Wir setzen die Arbeit an VDR-Suite Phase 62 fort.

Arbeite selbstständig und GitHub-first weiter. Lies zuerst die unten genannten
Projektregeln und den Handoff vollständig. Wiederhole keine abgeschlossenen
Analysen, Tests oder Runtime-Abnahmen ohne einen konkret geänderten relevanten
Fingerprint.

============================================================
PROJEKT UND AKTUELLER STAND
============================================================

Repository:
hotzenplotz5/vdr-suite

Lokaler Checkout:
/home/yavdr/vdr-suite-phase62

Lokaler Branch:
phase62-pr117

Remote-Branch:
phase-62-security-identity-foundation

Pull Request:
#117
Titel:
feat(security): establish Phase 62 identity and authorization foundation

Base:
main @ cb77ff66e11dca7db2eafa36525762dcde35102d

Letzter vollständig grüner Slice-2W-Auswahl-Head:
08b691630c30e1f2a7a74c42f7566ddc96d66b48

Der Branch enthält danach ausschließlich den Dokumentations-Handoff für diesen
neuen Chat. Verifiziere beim Start den tatsächlichen aktuellen Remote-Head und
dessen CI, statt einen selbstreferenzierten Head aus diesem Prompt zu erwarten.

PR-Zustand beim Handoff:
- offen
- Draft
- ungemergt
- mergebar

PR #117 darf ohne ausdrückliche Zustimmung nicht:
- Ready for Review gesetzt werden;
- gemergt oder geschlossen werden;
- Auto-Merge erhalten;
- rebased oder force-gepusht werden;
- in Base, Titel, Reviewern oder sonstigen Review-/Merge-Metadaten verändert
  werden.

Separate Arbeitslinie:
PR #118 enthält den pausierten TVScraper-Bugfix. Keine Commits, Binärartefakte,
Fingerprints oder Planung aus PR #118 in Phase 62 mischen.

============================================================
ZUERST VERBINDLICH LESEN
============================================================

1. AGENTS.md
2. docs/NEW-CHAT-HANDOFF.md
3. docs/CURRENT.md
4. docs/development/current-status.md
5. docs/development/phase-62-slice-2w-browser-session-retention-cleanup.md
6. docs/development/phase-62-slice-2v-browser-session-idle-expiry.md
7. docs/development/phase-62-runtime-evidence.md
8. docs/planning/phase-62-security-identity-gap-matrix.md
9. docs/architecture/security-identity-foundation.md

Diese Dateien sind die kanonische Projektwahrheit. Alte Chat-Zusammenfassungen
oder ältere Slice-Dokumente dürfen neuere Angaben daraus nicht überschreiben.

============================================================
VERIFIZIERTER CI-STAND
============================================================

Slice 2V Implementierung/Runtime:
Head:
e84415fadb2587ff744ff8927f1f0113920ece2f

VDR-Suite CI #6779
Run-ID: 30741293079
Status: vollständig grün, alle fünf Jobs erfolgreich
Direkter Link:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Slice 2V Closeout:
Head:
cf31b2b67f73f12718601ced5468a59a1183adcb

VDR-Suite CI #6799
Run-ID: 30742295881
Status: vollständig grün, alle fünf Jobs erfolgreich
Direkter Link:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742295881

Slice 2W Auswahl:
Head:
08b691630c30e1f2a7a74c42f7566ddc96d66b48

VDR-Suite CI #6807
Run-ID: 30742936735
Status: vollständig grün, alle fünf Jobs erfolgreich
Direkter Link:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742936735

Erfolgreiche Jobs bei #6807:
- docs-check
- make-test-audit
- frontend-regression-test
- fast-regression-test
- packaging-regression-test

Der reine Dokumentations-Handoff nach diesem grünen Auswahl-Head muss beim Start
des neuen Chats als aktueller Remote-Head samt CI verifiziert werden.

Bei jeder späteren CI-Meldung immer angeben:
- direkten Link;
- Run-Nummer;
- Run-ID;
- getesteten Head;
- Gesamtstatus;
- Status aller fünf Jobs.

============================================================
SLICE 2V IST VOLLSTÄNDIG ABGESCHLOSSEN
============================================================

Slice 2V:
Browser-Session Idle Expiry and throttled last_seen

Runtime-Abnahme:
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS

Verifizierte Runtime-Fingerprints:

Installierter/laufender Phase-62-Daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60

Installierter deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Wiederhergestellte Daemon-Konfiguration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Runtime-Report SHA-256:
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86

Durable Evidence:
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25

Verifiziert wurden unter anderem:
- HTTP 200 vor Idle-Ablauf;
- HTTP 401 session_expired für normalen GET nach Idle-Ablauf;
- HTTP 401 session_expired für geschützte Mutation vor Dispatch;
- genau ein fälliges last_seen_at-Update;
- keine erneute Activity-Schreibung im 60-Sekunden-Intervall;
- unverändertes absolutes expires_at;
- Logout HTTP 204 für Ersatzsession;
- Replay der widerrufenen Session HTTP 401 credential_revoked;
- geheime Werte nicht in Accountability/Evidence;
- null aktive Test-Lifecycle-Zeilen nach Cleanup;
- SQLite quick_check ok und foreign_key_check leer;
- null VDR-Domain-Mutationen;
- final aktiver Dienst;
- temporäres systemd-Drop-in entfernt;
- temporäre Idle-Variable final nicht gesetzt.

Diese Abnahme nicht wiederholen, solange kein direkt relevanter Daemon-, Schema-,
Konfigurations- oder Lifecycle-Fingerprint geändert wurde.

============================================================
AKTUELLER ARBEITSAUFTRAG: SLICE 2W
============================================================

Genau ein nächster Slice wurde ausgewählt:

Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup

WICHTIG:
- Slice 2W ist bisher nur ausgewählt und dokumentiert.
- Es wurde noch kein Slice-2W-Code implementiert.
- Es wurde keine Slice-2W-Runtime-Mutation durchgeführt.
- Beginne nicht erneut mit einer breiten Gap-Analyse.
- Ändere nicht erneut die Slice-Auswahl.

Verbindlicher Konfigurationsvertrag:

VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 deaktivierter Kompatibilitätsstandard
86400..31536000   aktivierte Retention in Sekunden

Feste Batch-Grenze:
256 terminale Browser-Lifecycles pro Startup-Lauf

Verbindlicher Trigger:
- genau ein begrenzter Cleanup-Lauf beim Aufbau des Security-Runtime;
- nach Schemaerstellung und vollständiger Konfigurationsvalidierung;
- vor securityReady;
- kein Scheduler, kein Hintergrundthread und kein request-path Cleanup.

Verbindliche terminale Kandidaten:
- explizit widerrufene Browser-Verifier nach Ablauf der Retention;
- absolut abgelaufene Browser-Verifier nach Ablauf der Retention;
- idle-abgelaufene Browser-Verifier nach Ablauf der Retention, aber nur wenn die
  akzeptierte Idle-Policy aktiviert ist.

Verbindliche Löschgrenze:
- Browser-Verifier löschen;
- seine kanonische Browser-Session nur löschen, wenn sie weiterhin zum
  Kandidaten gehört und nicht mehr referenziert wird;
- sein Credential nur löschen, wenn der Typ exakt browser-session ist, es zum
  Kandidaten gehört und nicht mehr referenziert wird;
- Actor, Device, Issuer-Credential, Grants, Rollen und Accountability niemals
  durch Slice 2W löschen.

Verbindliche Transaktion:
- BEGIN IMMEDIATE;
- terminale Eignung im Schreib-Transaction erneut prüfen;
- secret-free Cleanup-Accountability anhängen;
- Browser-Verifier, gegebenenfalls Session und Credential explizit löschen;
- gesamtes Batch bei SQL-, Foreign-Key- oder Accountability-Fehler zurückrollen;
- kein partieller Cleanup.

Verbindliche Accountability:

event_type=operation.succeeded
classes=security,lifecycle,maintenance
actor_type=system
authentication_state=system-maintenance
action=browser.session.cleanup
decision=completed
reason_code=browser_session_retention_elapsed
outcome=deleted

Keine Session-/CSRF-Secrets, Verifier-Hashes, Cookie-Werte, Authorization-Header,
Prozessumgebungen oder rohe Konfigurationsinhalte speichern oder ausgeben.

============================================================
EXAKT ERLAUBTER IMPLEMENTIERUNGSUMFANG
============================================================

Implementiere ausschließlich:

1. strikte Parsing-/Validierungsfunktion für
   VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS;
2. Repository-/Service-Vertrag für den begrenzten terminalen Cleanup;
3. atomare Auswahl, Auditierung und Löschung;
4. Startup-Integration vor securityReady;
5. fokussierte Unit-/Integrationstests;
6. Architektur-Guard;
7. erforderliche Make-Test-Registrierung;
8. bounded Runtime-Acceptance-Harness erst nach vollständig grüner Source-CI.

Vorhandene Eigentümer zuerst prüfen und wiederverwenden:
- core/security/include/SecurityConfiguration.h
- core/security/include/BrowserSessionCredentialRepository.h
- core/security/src/BrowserSessionCredentialRepository.cpp
- core/security/include/SecurityIdentityRepository.h
- core/security/src/SecurityIdentityRepository.cpp
- core/security/include/AccountabilityEvent.h
- core/http/src/TestHttpServer.cpp
- vorhandene Slice-2R/2S/2T/2U/2V Tests und Architektur-Guards
- mk/local-test-groups.mk
- mk/maintenance-tests.mk
- tools/check_architecture.py

Keine breite Architektur-Neuanalyse. Nur fehlende Details gezielt nachlesen.

============================================================
AUSDRÜCKLICH VERBOTENER SCOPE
============================================================

Slice 2W darf nicht enthalten:
- periodischen Scheduler;
- Background-Thread;
- Cleanup in HTTP-Request-Handlern;
- Session-Liste;
- Logout-all;
- Admin-API oder Admin-UI;
- Cleanup allein aufgrund einer widerrufenen Issuer-Credential;
- automatische Eviction zur Freigabe eines Concurrency-Slots;
- Löschung von Actor, Device, Issuer, Grants, Rollen oder Accountability;
- generische Identity-/Credential-Administration;
- generische Outcome-Infrastruktur;
- Outbox;
- Revisionen oder Idempotency-Plattform;
- Android oder Android TV;
- Phase 63-67 Runtime;
- TVScraper-/PR-118-Arbeit.

============================================================
ARBEITSREGELN
============================================================

GitHub-first:
- Bestehende Dateien bevorzugt direkt über den GitHub-Connector lesen und
  ändern, wenn der vollständige Inhalt verfügbar ist.
- Kleine, kohärente Commits erstellen.
- Remote-Head vor jedem Write erneut prüfen.
- Keine parallelen Writes auf dieselbe Datei.
- Nicht nach jedem Zwischencommit auf CI warten; CI auf dem finalen
  Stabilisierung-Head auswerten.
- PR-Metadaten nicht verändern.

Selbstständiges Weiterarbeiten:
- Nicht nach jedem klaren Zwischenschritt um Erlaubnis fragen.
- Bei einem klar abgegrenzten und bereits ausgewählten Scope direkt
  weiterarbeiten.
- Keine künstlichen Stopps nach grüner CI.
- Keine Zusage von Hintergrundarbeit oder späterer Lieferung.
- Bei Problemen Ursache analysieren und korrigieren, statt denselben Lauf
  unverändert zu wiederholen.

Keine Wiederholung:
- abgeschlossene Analysen und Abnahmen nicht nur wegen eines neuen Chats
  wiederholen;
- nur bei konkret geändertem relevanten Fingerprint erneut prüfen;
- keine erneute Post-2V-Gap-Analyse;
- keine erneute Slice-2W-Auswahlrunde.

Benutzer-Updates:
- bei längeren Arbeiten kurze Fortschrittsmeldungen geben;
- Ergebnisse und konkrete Fehler früh mitteilen;
- keine unnötigen Low-Level-Toolmeldungen;
- nach grüner CI ohne künstlichen Halt mit dem genehmigten nächsten Schritt
  fortfahren.

============================================================
SHELL- UND RUNTIME-REGELN
============================================================

Der Benutzer führt lokale Runtime-Befehle aus.

Für jeden sichtbaren Shell-Block gelten zwingend:
- maximal ein sichtbarer Shell-Block pro Antwort;
- erste Zeile exakt:
  cd /home/yavdr/vdr-suite-phase62
- niemals set -e;
- niemals set -u;
- niemals set -o pipefail;
- kein sudo;
- kein su;
- kein runuser;
- Root-Befehle als „in der bereits geöffneten Root-Shell“ beschreiben;
- Fehler nur in einer Subshell kapseln, damit die interaktive Root-Shell offen
  bleibt;
- keine riesigen undurchsichtigen Heredoc-Skripte;
- Credentials, Cookies, Authorization-Header, CSRF-Tokens, Session-Secrets,
  Verifier-Hashes und Prozessumgebungen niemals ausgeben.

Keinen Runtime-Lauf anfordern, bevor:
- Source-Änderungen committed und gepusht sind;
- alle fünf CI-Jobs auf dem exakten Source-Head vollständig grün sind;
- der erwartete neue Daemon-Fingerprint bekannt ist;
- ein eng begrenzter, rollback-sicherer Runtime-Runner vorhanden ist.

============================================================
ERSTER SCHRITT IM NEUEN CHAT
============================================================

1. Lies die verbindlichen Dateien vollständig.
2. Verifiziere PR #117 weiterhin als offen, Draft und ungemergt.
3. Verifiziere den tatsächlichen aktuellen Remote-Head und dessen vollständig
   abgeschlossenen CI-Lauf; `08b691...` ist der letzte grüne Auswahl-Baseline,
   nicht zwangsläufig der aktuelle Dokumentations-Handoff-Head.
4. Prüfe gezielt die genannten Code-Eigentümer und bestehenden Slice-Testmuster.
5. Implementiere danach ausschließlich Slice 2W.
6. Erstelle fokussierte Tests, Architektur-Guard und Make-Registrierung.
7. Prüfe die vollständige CI auf dem finalen Source-Head und liefere den direkten
   Link mit Run-Nummer, Run-ID, Head und allen fünf Jobstatus.
8. Erst danach einen sicheren realen yaVDR-Runtime-Abnahmelauf vorbereiten.

Der aktuelle Chat wurde bewusst vor der Slice-2W-Implementierung beendet. Setze
im neuen Chat direkt an dieser Grenze fort und vermeide weitere reine
Dokumentationsschleifen.
```
