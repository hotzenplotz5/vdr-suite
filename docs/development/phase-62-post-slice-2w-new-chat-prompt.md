# Neuer Chat-Prompt — VDR-Suite Phase 62 nach Slice 2W

Kopiere den vollständigen Inhalt des folgenden Blocks in einen neuen Chat.

```text
Wir setzen die Arbeit an VDR-Suite Phase 62 nach der vollständig akzeptierten
Slice-2W-Runtime-Abnahme fort.

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

============================================================
ZUERST VERBINDLICH LESEN
============================================================

1. AGENTS.md
2. docs/NEW-CHAT-HANDOFF.md
3. docs/CURRENT.md
4. docs/development/current-status.md
5. docs/development/phase-62-slice-2w-runtime-closeout.md
6. docs/development/phase-62-slice-2w-browser-session-retention-cleanup.md
7. docs/development/phase-62-runtime-evidence.md
8. docs/planning/phase-62-security-identity-gap-matrix.md
9. docs/architecture/security-identity-foundation.md
10. docs/planning/roadmap.md

Neuere Angaben in Handoff, Current State, Current Status und Slice-2W-Closeout
überschreiben ältere Slice- oder Chat-Zusammenfassungen.

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

Finale PID beim Lauf:
89965

Die PID ist volatil. Die Hashes, Evidence und der akzeptierte Head sind die
relevanten Wiederholungs-Gates.

Verifiziert wurden:
- frische Security-Schemaerstellung;
- deaktivierte Retention als vollständiger No-op;
- Fail-closed HTTP 503 und kompletter Transaktions-Rollback bei erzwungenem
  Accountability-Fehler;
- Erhalt aktiver und innerhalb der Retention liegender Lifecycles;
- Löschung alter widerrufener, absolut abgelaufener und idle-abgelaufener
  Lifecycles;
- keine Löschung allein wegen widerrufenem Issuer;
- Erhalt fremder Credential-Typen;
- Erhalt erneut referenzierter kanonischer Session/Credential-Zeilen;
- exakt secret-free Cleanup-Accountability;
- feste Grenze: 258 Kandidaten, exakt 256 deterministische Löschungen;
- SQLite quick_check ok und foreign_key_check leer;
- unveränderte Produktionsdatenbank, Konfiguration und Loader;
- entferntes Runtime-Drop-in;
- final aktiver neuer Daemon;
- null VDR-Domain-Mutationen.

Diese Abnahme nicht wiederholen, solange kein direkt relevanter Daemon-,
Cleanup-, Schema-, Konfigurations-, systemd-Ausführungs- oder Harness-Fingerprint
geändert wurde.

============================================================
AKZEPTIERTER SLICE-2W-VERTRAG
============================================================

VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 deaktivierter Kompatibilitätsstandard
86400..31536000   aktivierte Retention in Sekunden
feste Batch-Grenze 256

Genau ein begrenzter Cleanup-Lauf beim Security-Runtime-Startup nach
Schema-/Konfigurationsvalidierung und vor securityReady.

Terminale Quellen:
- explizite Browser-Revocation;
- absolute Expiry;
- Idle-Expiry bei aktivierter Idle-Policy.

Atomare Grenze:
- BEGIN IMMEDIATE;
- Eligibility erneut prüfen;
- secret-free browser.session.cleanup Event schreiben;
- Browser-Verifier löschen;
- kanonische Session nur unreferenziert löschen;
- Credential nur bei Typ browser-session und unreferenziert löschen;
- Actor, Device, Issuer, Grants, Rollen und Accountability erhalten;
- vollständiger Batch-Rollback bei Fehlern.

Kein Scheduler, Background-Thread, Request-Path-Cleanup, Issuer-Cascade,
Concurrency-Eviction, Session-Admin, generischer Identity-Cleanup oder
Phase-63-67-Scope.

============================================================
AKTUELLER AUFTRAG
============================================================

Slice 2W ist geschlossen. Es ist noch kein nächster Implementierungsslice
verbindlich ausgewählt.

Führe genau eine frische, begrenzte Post-Slice-2W-Gap-Analyse durch:

1. Verifiziere den tatsächlichen aktuellen Remote-Head und dessen jüngste
   vollständig abgeschlossene CI.
2. Lies die verbleibenden Phase-62-Gaps aus Gap Matrix, Architektur und Roadmap.
3. Prüfe gezielt die konkreten Code-Eigentümer der wenigen realistischen
   Kandidaten; keine repository-weite Wiederholungsanalyse.
4. Vergleiche Nutzen, Abhängigkeiten, Risiko, Testbarkeit und Runtime-Abnahme.
5. Wähle genau einen kleinsten kohärenten nächsten Phase-62-Slice.
6. Dokumentiere Vertrag, Grenzen, Tests, Architektur-Guard, Runtime-Grenze und
   ausdrückliche Ausschlüsse.
7. Aktualisiere Current State, Current Status, Handoff und Gap Matrix.
8. Fordere vollständige CI auf dem finalen Auswahl-/Handoff-Head.
9. Beginne die neue Implementierung erst nach vollständig grüner Auswahl-CI.

Realistische offene Themen sind unter anderem:
- breitere Operation-Outcomes und stärkere Transaktionskopplung;
- gemeinsame Revisionen, Idempotency und durable Operation Lifecycle;
- geschützte Actor-/Identity-/Credential-/Grant-/Role-Administration;
- Native-/Service-Credential-Lifecycle;
- geschützte Audit-Reads, Export, Redaction und Retention;
- Compatibility-Retirement und finaler Phase-62-Closeout.

Nicht mehrere Themen kombinieren. Keine Phase 63-67 Runtime und keine
Android-/Android-TV-Arbeit in Phase 62 ziehen.

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
- erste Zeile exakt `cd /home/yavdr/vdr-suite-phase62`;
- kein set -e, set -u oder pipefail;
- kein sudo, su oder runuser;
- Root-Kommandos als bereits geöffnete Root-Shell beschreiben;
- Fehler in einer Subshell kapseln;
- keine Secrets, Cookies, Authorization-Header, CSRF-Tokens, Verifier-Hashes
  oder Prozessumgebungen ausgeben.

Keine Runtime-Abnahme anfordern, bevor Implementierung committed/gepusht, alle
fünf Source-CI-Jobs grün, der neue Binärfingerprint bekannt und ein begrenzter
rollback-sicherer Runner vorhanden ist.

Der erste Schritt ist die Post-Slice-2W-Gap-Analyse und Auswahl genau eines
nächsten bounded Slice — nicht die Wiederholung von Slice 2W.
```