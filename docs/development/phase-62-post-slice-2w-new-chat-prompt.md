# Neuer Chat-Prompt — VDR-Suite Phase 62 nach Slice-2X-Auswahl

Kopiere den vollständigen Inhalt des folgenden Blocks in einen neuen Chat.

```text
Wir setzen die Arbeit an VDR-Suite Phase 62 nach der vollständig akzeptierten
Slice-2W-Runtime-Abnahme und der dokumentierten Auswahl von Slice 2X fort.

Arbeite selbstständig und GitHub-first weiter. Lies zuerst die verbindlichen
Projektregeln und den kanonischen Handoff vollständig. Wiederhole keine
abgeschlossenen Analysen, Tests oder Runtime-Abnahmen ohne einen konkret
geänderten relevanten Fingerprint.

============================================================
PROJEKT UND GRENZEN
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

PR #117 muss offen, Draft und ungemergt bleiben. Ohne ausdrückliche Zustimmung
nicht Ready setzen, mergen, schließen, Auto-Merge aktivieren, rebasen,
force-pushen oder PR-/Review-Metadaten verändern.

PR #118 ist die getrennte pausierte TVScraper-Arbeitslinie. Keine Dateien,
Commits, Binärartefakte oder Fingerprints daraus mit Phase 62 vermischen.

Android, Android TV und Phase 63-67 dürfen nicht vorgezogen werden.

============================================================
ZUERST VERBINDLICH LESEN
============================================================

1. AGENTS.md
2. docs/NEW-CHAT-HANDOFF.md
3. docs/CURRENT.md
4. docs/development/current-status.md
5. docs/development/phase-62-slice-2x-protected-accountability-event-read.md
6. docs/development/phase-62-slice-2w-runtime-closeout.md
7. docs/development/phase-62-slice-2w-browser-session-retention-cleanup.md
8. docs/development/phase-62-runtime-evidence.md
9. docs/planning/phase-62-security-identity-gap-matrix.md
10. docs/architecture/security-identity-foundation.md
11. docs/planning/roadmap.md

Neuere Angaben in Handoff, Current State, Current Status, Slice-2W-Closeout und
dem Slice-2X-Auswahlvertrag überschreiben ältere Slice- oder Chat-Zusammenfassungen.

============================================================
SLICE 2W IST VOLLSTÄNDIG AKZEPTIERT
============================================================

Slice:
Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup

Akzeptierter Source-/Runtime-Head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Source-CI:
VDR-Suite CI #6834
Run-ID: 30745952119
Direkter Link:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119
Status: vollständig erfolgreich

Alle fünf Jobs erfolgreich:
- docs-check
- make-test-audit
- frontend-regression-test
- fast-regression-test
- packaging-regression-test

Runtime-Marke:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Installierter/laufender Daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Installierter deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Daemon-Konfiguration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Runtime-Report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea

Durable Evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Diese Abnahme nicht wiederholen, solange kein direkt relevanter Daemon-,
Cleanup-, Schema-, Konfigurations-, systemd-Ausführungs- oder Harness-Fingerprint
geändert wurde.

============================================================
POST-SLICE-2W-GAP-ANALYSE IST ABGESCHLOSSEN
============================================================

Es wurde exakt ein nächster bounded Slice ausgewählt:

Phase 62 Slice 2X
Protected Accountability Event Read

Verbindlicher Vertrag:
docs/development/phase-62-slice-2x-protected-accountability-event-read.md

Die Auswahl ist bewusst klein und unabhängig:

- das append-only Accountability-Modell existiert bereits;
- ein geschützter Produktions-Leseweg fehlt;
- der Read ändert keinen VDR-Domainzustand;
- Authorization und Audit-of-Audit können fail-closed sein, bevor Daten
  ausgeliefert werden;
- der Owner-Satz bleibt auf Repository, Read-Service, Security-Gate und festen
  HTTP-Serializer begrenzt.

Nicht ausgewählt wurden:

- generische Mutation-Outcomes ohne vorherige Coupling-/Outbox-Lösung;
- gemeinsame Revisionen, Idempotenz und durable Operation Lifecycle;
- Actor-/Identity-/Credential-/Grant-/Role-Administration;
- Native-/Service-Credential-Lifecycle;
- Audit-Export, konfigurierbare Redaction und Retention;
- Compatibility-Retirement.

Keine neue Gap-Auswahl durchführen, solange Slice 2X der aktive bounded
Workstream ist.

============================================================
VERBINDLICHER SLICE-2X-VERTRAG
============================================================

Exakte HTTP-Oberfläche:

GET /api/security/accountability/events
GET /api/security/accountability/events?limit=<1..100>

Grenzen:

- Default-Limit 50;
- Minimum 1;
- Maximum 100;
- Reihenfolge recorded_at DESC, event_id DESC;
- doppelte, nicht-dezimale, null, zu große oder unbekannte Query-Parameter
  liefern HTTP 400 vor Repository-Zugriff;
- kein Cursor, Offset, Filter, Export oder beliebiger History-Scan;
- Cache-Control: no-store;
- Route vor dem allgemeinen ApiRouter;
- Browser-GET benötigt kein CSRF, aber vollständige Authorization.

Exakte Berechtigung:

security.audit.read@*

- direkter exakter Grant erlaubt;
- exaktes globales role.admin@* erlaubt;
- role.admin auf anderem Scope erlaubt nicht;
- role.read-only erlaubt nicht;
- Legacy-Basic-Kompatibilität darf die sensible Read-Policy nicht umgehen;
- Grant-Store-Ausfall bleibt fail-closed.

Feste Response-Projektion:

- nur bestehende secret-free Accountability-Felder plus recordedAt;
- keine Header, Bodies, Cookies, Authorization-Werte, Passwörter, Hashes,
  Session-/CSRF-Secrets, Rohkonfiguration oder Prozessumgebung.

Accountability und Transaktion:

- vorhandenes Pre-Dispatch-Authorization-Event bleibt verpflichtend;
- bounded SELECT und exaktes operation.succeeded Read-Outcome laufen in einer
  lokalen BEGIN IMMEDIATE-Transaktion;
- SELECT-, Outcome-Append- oder Commit-Fehler rollen zurück und liefern keine
  Event-Zeilen;
- die lokale Read-Kopplung ist keine generische Mutation-Outbox.

Konfiguration:

- keine neue Environment-Variable;
- Default und Maximum sind compile-time Vertragskonstanten.

============================================================
SLICE-2X-EXCLUSIONS
============================================================

Nicht in Slice 2X:

- Export, Download, Streaming;
- Pagination, Offset, Filter, Zeitbereiche oder Suche;
- konfigurierbare Redaction;
- Audit-Retention, Löschung, Kompaktierung oder Archivierung;
- Frontend-Audit-Viewer;
- Security-Administration;
- generische Mutation-Outcomes oder Outbox;
- gemeinsame Revisionen, If-Match, Idempotenz oder Operation Replay;
- Native-/Service-Credential Enrollment, Rotation oder Revocation;
- Compatibility-Retirement;
- Android, Android TV oder Phase 63-67 Runtime.

============================================================
TEST-, GUARD- UND PACKAGING-GRENZE
============================================================

Focused Tests müssen mindestens beweisen:

- idempotente Schema-/Index-Erstellung;
- deterministische newest-first-Reihenfolge inklusive Tie-Break;
- Limits 1, 50 und 100;
- erfolgreich leer versus Repository nicht verfügbar;
- Anonymous-/Lifecycle-/Grant-/Scope-Deny-Pfade;
- direkter exakter Grant und role.admin@*;
- kein Legacy-Basic-Bypass;
- atomarer Read plus verpflichtendes Success-Outcome;
- erzwungener SELECT-/Outcome-/Commit-Fehler ohne Datenausgabe;
- exakte Query-Grammatik und feste JSON-Allowlist;
- no-store und Request-/Correlation-Header;
- kein allgemeiner ApiRouter- oder Frontend-Owner.

Ein eigener Architektur-Guard muss Route, Maximum 100, Repository-only SQL,
exakte globale Rollenabbildung, Secret-Allowlist und Exclusions festschreiben.

Packaging darf nur bestehende Daemon-/Security-Source- und Testlisten erweitern.
Keine neue Debian-Komponente, systemd-Unit, Nginx-Location, Konfiguration oder
Frontend-Assets.

============================================================
SELECTION-GATE UND JETZIGER AUFTRAG
============================================================

Die Slice-2X-Produktionsimplementierung darf erst beginnen, wenn die Auswahl- und
Handoff-Dokumentation auf ihrem finalen Head alle fünf GitHub-Actions-Jobs grün
hat:

- docs-check
- make-test-audit
- frontend-regression-test
- fast-regression-test
- packaging-regression-test

Falls die Auswahl-CI bereits vollständig grün und im Handoff mit Link,
Run-Nummer, Run-ID und Head festgehalten ist, implementiere anschließend genau
Slice 2X.

Falls die Auswahl-CI noch nicht vollständig grün ist, vervollständige nur die
Auswahl-/Closeout-Dokumentation und CI. Beginne noch keine Produktionsänderung.

Keine Runtime-Abnahme für reine Auswahl-/Dokumentationsänderungen. Die spätere
Slice-2X-Runtime-Abnahme ist erst nach Implementierung, vollständig grüner
Source-CI, bekanntem Binärfingerprint und einem rollback-sicheren isolierten
Runner zulässig.

============================================================
ARBEITSREGELN
============================================================

GitHub-first:
- vorhandene Dateien direkt über den GitHub-Connector lesen und ändern, wenn der
  vollständige Inhalt verfügbar ist;
- Remote-Head und PR-Zustand vor Writes prüfen;
- kleine kohärente Fast-Forward-Commits;
- keine parallelen Writes auf dieselbe Datei;
- CI auf dem finalen Stabilisierung-Head auswerten;
- PR-Metadaten nicht verändern.

CI-Bericht immer mit:
- direktem Link;
- Run-Nummer;
- Run-ID;
- exaktem Head;
- Gesamtstatus;
- allen fünf Jobstatus.

Shell-Regeln für sichtbare Blöcke:
- maximal ein Shell-Block pro Antwort;
- erste Zeile exakt cd /home/yavdr/vdr-suite-phase62;
- kein set -e, set -u oder pipefail;
- kein sudo, su oder runuser;
- Root-Kommandos als bereits geöffnete Root-Shell beschreiben;
- Fehler in einer Subshell kapseln;
- keine Secrets, Cookies, Authorization-Header, CSRF-Tokens, Verifier-Hashes
  oder Prozessumgebungen ausgeben.

Der nächste zulässige Produktionsschritt ist ausschließlich die bounded
Slice-2X-Implementierung nach vollständig grüner Auswahl-CI.
```