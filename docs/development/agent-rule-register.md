# VDR-Suite – verbindliches Agent-Regelregister

**Zweck:** Eindeutige Nachschlagestelle für bestehende Regeln, ihren Geltungsbereich
und prüfbare Nachweise. Dieses Register ist **kein neues Freigabe-, CI-,
Installations- oder Runtime-Gate**. Die bindende Norm bleibt die jeweils
aktuelle `AGENTS.md` (inklusive einschlägiger Unterverzeichnisse) und
ihre ausdrücklich referenzierten technischen Verträge. Die Kennungen
werden dauerhaft vergeben; eine bestehende Kennung nicht für eine andere
Regel wiederverwenden.

## Lesereihenfolge

1. Vor substantieller Repository-Arbeit die **vollständige aktuelle**
   [Root-AGENTS.md](../../AGENTS.md) lesen, nicht nur eine Suche/Teilansicht.
2. Für den tatsächlichen Änderungsbereich alle zutreffenden
   `AGENTS.md`-Dateien entlang des Verzeichnispfads und ausdrücklich
   referenzierte fachliche Verträge lesen. Bei mehreren Eigentümern
   (z. B. Media Tools unter `addons/` und `core/addons/`) alle
   tatsächlich betroffenen Regelquellen lesen.
3. Repository/Branch/SHA und gelesene Pfade samt Git-Blob-SHA oder
   Commit-Ref knapp nachweisen. Lesefakten nur melden, wenn im
   aktuellen Arbeitsablauf ein tatsächlicher Abruf stattgefunden hat.
4. Im weiteren Verlauf bereits bekannte unveränderte Belege verwenden,
   nicht dieselbe Datei vor jedem Commit erneut abrufen. Bei relevantem
   Branch-/HEAD-Wechsel Regeln erneut mit dem aktuellen Stand abgleichen.
5. Fehlende Zugriffs-/Hostinformationen als **nicht verifiziert** melden,
   sichere unabhängige Arbeiten fortsetzen. Daraus keine künstliche
   Zustimmungspflicht, CI-Wartezeit oder erneute Kompilierung machen.

## Referenzierte Regeln

| ID | Verbindlicher Gegenstand | Autoritative Stelle | Typischer Nachweis |
| --- | --- | --- | --- |
| AG-000 | Vor der Arbeit geltende Regeln tatsächlich lesen und Datei-/SHA-Beleg liefern | Root: „Arbeitsbeginn: verbindliche Lesepflicht und Regel-Nachweis“ | nachvollziehbare Dateiabrufe und geprüfter Ref; keine Behauptung ohne Abruf |
| AG-001 | `AGENTS.md` nur mit persönlicher Maintainer-Genehmigung ändern | Root `AGENTS.md`: „AGENTS.md-Änderungen ...“ | expliziter Auftrag; Git-Diff |
| AG-002 | Keine neuen Sperren, Gates oder Berechtigungsregeln ohne vorherige Genehmigung | Root: „KEINE neuen Sperren ...“ | Diff/Vertragsprüfung, Maintainer-Freigabe bei neuer Sperre |
| AG-003 | Keine erfundenen Fakten/Befehle, Quellen-/SHA-Nachweis | Root: „No hallucinated facts — verified evidence ...“ | tatsächliche Datei-/Werkzeugabrufe und geprüftes HEAD |
| AG-004 | Auftragsumfang klein halten; keine spekulativen Refactorings | Root: „Non-negotiable scope ...“ | konkretes Akzeptanzkriterium, überprüfbarer Diff |
| AG-005 | Genehmigte Arbeit ohne künstliche Unterbrechung zu Ende führen | Root: „Top-level non-stop execution mandate“ | fertiggestellter Slice oder belegter externer Blocker |
| AG-006 | Zu Fix, Diagnose oder Doku passende **direkt benutzbare** Testblöcke | Root: „Top-level non-stop ...“, „Testblock repository entry“, „Testblock shell option safety“; [Operator-Leitfaden](agent-operator-workflows.md) | überprüfbarer Bash-Block mit echtem Checkout/Commit und passender Aktion |
| AG-007 | Auf yaVDR im vorhandenen Root-Shell-Kontext installieren | Root: „yaVDR-Installationsbefehle ...“; [Installer](daemon-only-installer.md) | kein unbegründeter Benutzerwechsel im Installationsblock |
| AG-008 | C++-Objektcache erhalten; identische Daemon-Builds nicht wiederholen | Root: „Schnellen inkrementellen C++-Daemon-Build ...“ und „NEVER compile ...“ | geprüfte Artefakte/Build-SHA; vorhandene Tests `tools/test_build_cpp_cached.py`, `tools/test_install_daemon.py` |
| AG-009 | SSE für asynchronen Frontend-Status priorisieren | Root: „SSE geht vor Polling“ | produktiver SSE-/Reconnect-Nachweis und gezielter Regressionstest |
| AG-010 | Git-/Branch-/PR-Genehmigungen, keine History-Manipulation | Root: „Media Tools nach PR #556“, „Branch and pull-request safety“; [Branch-Lebenszyklus](branch-pull-request-lifecycle.md) | aktueller Git-HEAD, Branchstatus, dokumentierte Freigabe |
| AG-011 | Nur relevante Zwischenprüfungen; vollständige erforderliche CI am finalen Merge-Gate | Root: „Commit, push, CI and minimal necessary validation“ | Testergebnisse exakt für Kandidaten-SHA, Status des finalen CI-Gates |
| AG-012 | CI/Mock/Installation nicht mit realer VDR-Abnahme verwechseln | Root: „Product boundaries and real-VDR acceptance“ | getrennte UNIT/CONTRACT, PRODUCTION-INTEGRATION, CI und REAL-VDR-Evidenz |
| AG-013 | Modul-/Media-Tools-Rechte aus bestehenden kanonischen Eigentümern ableiten | [Core-Addons](../../core/addons/AGENTS.md), [Media Tools](../../addons/media-tools/AGENTS.md), Root AG-002 | gezielte produktive Grants-/Autorisierungstests |
| AG-014 | Frontend-Lifecycle und produktive Composition prüfen | Root: „Frontend lifecycle and composition gates“ | gezielte Composition-/User-Action-Regression statt nur Mock |
| AG-015 | Paketbau isolieren, keine Installationsbelege aus Paket-CI erfinden | [Debian](../../debian/AGENTS.md), Root „Schnellen inkrementellen C++-Daemon-Build ...“ | saubere Paketbau- und gezielte Profiltests |

## Grenzen des Nachweises

Werkzeugabrufe und Blob-SHAs belegen den **Zugriff** auf die geltenden Regeln,
nicht automatisch ihr Verständnis oder ihre Einhaltung. Produkt- und
Regressionstests sichern konkrete technische Eigenschaften ab; es gibt
keinen universellen Automatismus, der jeden Agentenfehler verhindert.
Das Register erzeugt insbesondere **keinen neuen blockierenden CI-Check**.

## Konflikte und Änderungen

- Die Root-`AGENTS.md` ist bei gemeinsamen Maintainer-Vorgaben maßgeblich.
  Unterdateien ergänzen **bereichsgebunden** bestehende technische Regeln,
  schaffen aber keine eigenmächtigen neuen Sperren.
- Bei einem Widerspruch Quelle, Wortlaut, Scope und Regel-ID benennen,
  statt willkürlich eine der Fassungen auszuwählen. Die betroffene
  riskante Aktion nicht auf Vermutung ausführen; unabhängige beauftragte
  Arbeit fortsetzen.
- Bestehende tatsächlich erforderliche Autorisierungen und Schutzmaßnahmen
  werden nicht durch eine pauschale „Keine Sperren“-Interpretation aufgehoben.
- Ein Verstoß erfordert zuerst Fehlerursachenanalyse und gegebenenfalls
  einen Regressionstest. **Nicht reflexartig eine weitere Textregel
  hinzufügen.** Änderungen an irgendeiner `AGENTS.md` brauchen die
  ausdrückliche Genehmigung des Maintainers.
- [Widerspruchsaudit](agent-instruction-audit.md) enthält den geprüften
  Snapshot und konkrete Klärungsfälle.
