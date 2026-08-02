# Neuer Chat-Prompt — VDR-Suite Phase 62 am Slice-2X-Runtime-Gate

Kopiere den vollständigen Inhalt des folgenden Blocks in einen neuen Chat.

```text
Wir setzen die Arbeit an VDR-Suite Phase 62 fort.

Arbeite selbstständig und GitHub-first weiter. Wiederhole keine abgeschlossenen
Analysen, Tests oder Runtime-Abnahmen ohne einen konkret geänderten relevanten
Fingerprint.

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

Keine Android-, Android-TV- oder Phase-63-67-Runtime-Arbeit vorziehen.

============================================================
ZUERST VERBINDLICH LESEN
============================================================

1. AGENTS.md
2. docs/NEW-CHAT-HANDOFF.md
3. docs/CURRENT.md
4. docs/development/current-status.md
5. docs/development/phase-62-slice-2x-protected-mutation-response-outcomes.md
6. docs/development/phase-62-slice-2x-runtime-acceptance-runbook.md
7. docs/development/phase-62-slice-2w-runtime-closeout.md
8. docs/planning/phase-62-security-identity-gap-matrix.md
9. docs/architecture/security-identity-foundation.md
10. docs/planning/roadmap.md

Neuere Angaben in Handoff, Current State, Current Status, Gap Matrix, Roadmap und
dem Slice-2X-Vertrag überschreiben ältere Chat-Zusammenfassungen.

============================================================
AKTUELLER PHASE-62-STAND
============================================================

Vollständig real-runtime-akzeptiert durch:

Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup

Akzeptierter Slice-2W-Head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Runtime-Marke:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Durable Evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Slice 2W nicht wiederholen, solange kein direkt relevanter Daemon-, Cleanup-,
Schema-, Konfigurations-, systemd-Ausführungs- oder Harness-Fingerprint geändert
wurde.

Aktueller begrenzter Slice:

Phase 62 Slice 2X
Protected Mutation Response Outcomes

Slice-2X-Status:
- Production-Implementierung abgeschlossen;
- Focused Tests abgeschlossen;
- Architektur-Guard abgeschlossen;
- isolierter Installations-/Runtime-Harness abgeschlossen;
- reale yaVDR-Abnahme noch offen.

============================================================
IMPLEMENTIERTER SLICE-2X-VERTRAG
============================================================

Verbindliches Exit-Kriterium:

every privileged mutation has actor, decision and outcome evidence

Für jeden bereits geschützten, autorisierten POST, der
ApiRouter::handleClientPost() erreicht, wird nach dem Router-Return exakt ein
Outcome-Event geschrieben:

HTTP 200..299  -> event_type=operation.succeeded, outcome=succeeded
alle anderen   -> event_type=operation.failed,    outcome=failed
reason_code    -> http_status_<dezimaler Status>

Wiederverwendeter Kontext:
- Actor und Actor-Type;
- Device und Session;
- Authentication State;
- Permission und Backend Scope;
- Action und vorhandene Operation-ID;
- Request-ID und Correlation-ID;
- decision=allowed.

Keine Request-/Response-Bodies, Header, Cookies, Credentials, CSRF-Tokens,
Verifier-Hashes, Konfiguration oder Prozessumgebungen werden persistiert.

Owner:
- SecurityGateDecision hält AuthorizationDecision und operationId;
- SecurityHttpGate baut und schreibt das Outcome;
- TestHttpServer ruft den Pfad nach handleClientPost() und vor finaler Response;
- AccountabilityEventRepository bleibt append-only.

Fehlergrenze:
- Pre-Dispatch-Append-Fehler verhindert Dispatch;
- Post-Dispatch-Outcome-Append-Fehler liefert HTTP 503
  accountability_unavailable;
- keine Behauptung eines Domain-Rollbacks;
- keine Behauptung, dass automatischer Retry sicher ist.

Keine neue Route, Permission, Rolle, Tabelle, Index, Repository,
Environment-Variable, Frontend- oder Packaging-Komponente.

============================================================
SOURCE-/HARNESS-VALIDIERUNG
============================================================

Der frühere Implementierungs-/Harness-Head
4b61583b604626cd49e213356241759c81e60d04
bestand:

VDR-Suite CI #6871
Run-ID 30750871845
alle fünf Jobs grün

Danach wurde der Runtime-Weg weiter gehärtet:

- protected-mutation-outcome-runtime-entry.py;
- Backup des alten Daemons, Loaders, der Konfiguration und einer konsistenten
  Produktions-DB-Kopie;
- atomare Kandidateninstallation;
- temporärer systemd-Drop-in für beide DB-Pfade;
- isolierte Scenario-Datenbank;
- automatischer Rollback auf den alten Daemon bei fehlgeschlagener Abnahme oder
  fehlgeschlagenem Kandidaten-Restart;
- Wiederherstellung des normalen Produktionsdienstes;
- eigenes yaVDR-Runbook.

Darum muss der exakte aktuelle Final-Head vor Runtime-Installation erneut alle
fünf Jobs bestehen:

- docs-check
- make-test-audit
- frontend-regression-test
- fast-regression-test
- packaging-regression-test

CI-Bericht immer mit direktem Link, Run-Nummer, Run-ID, exaktem Head,
Gesamtstatus und allen fünf Jobstatus.

============================================================
REALE YAVDR-ABNAHME
============================================================

Nur das folgende Runbook verwenden:

docs/development/phase-62-slice-2x-runtime-acceptance-runbook.md

Der Wrapper muss:

1. exakten sauberen Branch/Head und Kandidatenhash prüfen;
2. alten Runtime-Stand sichern;
3. Kandidaten atomar installieren;
4. beide DB-Pfade auf eine isolierte SQLite-Kopie setzen;
5. denselben echten geschützten Owner für zwei Fälle verwenden:
   - kein stale Probe -> HTTP 200 -> operation.succeeded;
   - test-eigener stale Probe + DELETE-Guard -> HTTP 500 -> operation.failed;
6. pro Request exakt Authorization- und Outcome-Event mit identischem Kontext
   beweisen;
7. Secrets ausschließen;
8. Testzeile, Trigger und Grants wiederherstellen bzw. Testsession widerrufen;
9. Produktionsdatenbank während des Scenarios unverändert lassen;
10. systemd-Drop-in entfernen;
11. Kandidaten nur nach vollständigem Pass behalten;
12. bei Fehler den alten Daemon wiederherstellen;
13. normalen Produktionsdienst aktiv hinterlassen.

Eine fehlgeschlagene Abnahme ist kein Teilpass. Nur den nachgewiesenen Fehler
beheben. Bei geändertem relevantem Fingerprint zuerst wieder vollständige CI.

============================================================
AUSDRÜCKLICH NICHT IMPLEMENTIEREN
============================================================

Ohne neue vollständige Notwendigkeitskette nicht implementieren:

- GET /api/security/accountability/events;
- security.audit.read oder Audit-Rollenlogik;
- Audit-Frontend, Export, Filter, Pagination, Redaction oder Retention;
- generische Security-Administration;
- Native-/Service-Credential-Lifecycle vor einem realen Consumer;
- universelle Revision-/Idempotency-/Operation-Infrastruktur;
- transactional Outbox oder generische Cross-System-Coupling;
- Android, Android TV oder Phase 63-67 Runtime.

Nach erfolgreicher Slice-2X-Abnahme nur:

1. Slice-2X-Runtime-Closeout dokumentieren;
2. Dokumentations-CI auf dem Closeout-Head;
3. Compatibility-Retirement-Readiness prüfen;
4. finalen Phase-62-Closeout analysieren.

Nicht automatisch einen weiteren Implementierungsslice erfinden.

============================================================
AKTUELLER AUFTRAG
============================================================

1. Verifiziere Remote-Head und PR-Zustand.
2. Prüfe, ob der aktuelle Head alle fünf CI-Jobs grün hat.
3. Falls nicht, behebe nur die nachgewiesene Ursache.
4. Nach grüner Final-CI führe die reale yaVDR-Abnahme exakt nach dem Runbook aus,
   sofern der lokale Checkout und die installierte Runtime erreichbar sind.
5. Bei PASS erfasse:
   - accepted_head;
   - source_ci_run_number;
   - source_ci_run_id;
   - daemon_sha256;
   - loader_sha256;
   - configuration_sha256;
   - runtime_report_sha256;
   - evidence_directory;
   - final_service_pid.
6. Erstelle danach den Slice-2X-Runtime-Closeout.
7. PR #117 offen, Draft und ungemergt lassen.

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

Keine Secrets, Cookies, Authorization-Header, CSRF-Tokens, Verifier-Hashes oder
Prozessumgebungen ausgeben.
```
