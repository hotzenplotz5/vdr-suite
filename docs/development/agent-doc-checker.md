# Agent-Dokumentationsprüfer (freiwillige Diagnose)

Der vorhandene Agent-Regelkatalog soll übersichtlich bleiben. Der kleine
[Prüfer](../../tools/check_agent_docs.py) meldet bekannte redaktionelle
Regressionen, ohne Dateien zu verändern oder daraus eine zusätzliche
Freigabe-, Build-, Installations- oder CI-Sperre zu erzeugen.

## Aufruf im vorhandenen Checkout

Nur lesend, mit Python-Standardbibliothek und ohne Build, Root-Wechsel,
GitHub-Abfrage, Netzwerkzugriff oder Servicekontakt:

```bash
set +e
set +u
set +o pipefail
if cd /home/yavdr/vdr-suite; then
  git -c safe.directory=/home/yavdr/vdr-suite rev-parse --show-toplevel
  python3 tools/check_agent_docs.py
fi
```

Optional kann `--root /pfad/zum/checkout` einen anderen bereits
vorhandenen Quellbaum prüfen. Keine Kopie wird angelegt.
Die [isolierten Fixture-Tests](../../tools/test_check_agent_docs.py)
lassen sich ohne produktive Dateien ausführen:

```bash
set +e
set +u
set +o pipefail
if cd /home/yavdr/vdr-suite; then
  python3 -m unittest discover -s tools -p 'test_check_agent_docs.py'
fi
```

## Was geprüft wird

| Kennung | Prüfung |
| --- | --- |
| `DUPLICATE_ID` | Die kanonische AG-000…AG-999-**Deklarationstabelle** in [agent-rule-register.md](agent-rule-register.md) enthält dieselbe Kennung in mehreren Zeilen. Referenzen auf eine Regel in anderen Dokumenten sind ausdrücklich keine Duplikate. |
| `BROKEN_LINK` | Ein lokaler Inline-Markdown-Link aus einer `AGENTS.md` oder `docs/development/agent-*.md` zeigt auf keinen existierenden relativen Pfad oder verlässt den geprüften Repo-Baum. Externe URLs und reine `#fragment`-Links werden ignoriert. |
| `OBSOLETE_COMMAND` | Ein als `bash`, `sh`, `shell` oder `console` markierter ausführbarer Markdown-Block beginnt eine Zeile mit einem überholten yaVDR-Benutzerwechsel (`sudo -u yavdr`, `runuser -u yavdr`, `su - yavdr`) oder einem für Operator-Blöcke unzulässigen Shell-Fail-Fast (`set -e`, `set -u`, `set -o pipefail`). |
| `OBSOLETE_RULE` | Eine eindeutig direktive alte Textzeile verlangt beispielsweise pauschal `No root compilation.` oder `Build still runs only under yavdr`. |
| `REGISTER_MISSING` | Die zentrale Regelregister-Datei ist im angegebenen Checkout nicht vorhanden. |

Der Prüfer betrachtet **die Root- und alle Unterverzeichnis-`AGENTS.md`**
sowie die `agent-*.md`-Dateien unter `docs/development`.
Versteckte/generierte Verzeichnisse wie `.build` werden ausgelassen.
Historische Erwähnungen alter Befehle **im Fließtext** sind kein
Befund. Unterschiedliche Build-Scopes (isolierter unprivilegierter
Debian-Paketbau versus genehmigte yaVDR-Installation in Root-Shell)
werden dadurch nicht willkürlich gleichgesetzt.

**Bewusste Grenzen:** Kein vollständiger Markdown-Parser; prüft nur
Inline-Links auf die Existenz des Ziels, nicht dessen Anker-Text,
HTTP-Verfügbarkeit oder referenzbasierte Markdown-Linkdefinitionen.
Kein vollautomatischer semantischer Widerspruchstest aller Agent-Regeln.
Dafür gilt weiterhin die [inhaltliche Audit-Dokumentation](agent-instruction-audit.md).

## Ergebnis und Verhalten

Beispiel ohne Befund:

```text
AGENT_DOC_AUDIT=PASS FILES=16 FINDINGS=0 ADVISORY_ONLY=YES
```

Beispiel mit Befund (nur zur Illustration):

```text
BROKEN_LINK docs/development/agent-guide.md:12: missing.md
AGENT_DOC_AUDIT=WARNINGS FILES=16 FINDINGS=1 ADVISORY_ONLY=YES
```

Die `FILES`-Zahl ist vom tatsächlichen Checkout abhängig.
**Absichtliche Rückgabecodes:** `0` auch bei Warnungen; nur
Aufruf-/I/O-Fehler führen zu einem Fehlercode. Das Werkzeug ist
nicht in `Makefile`, `make test-docs`, GitHub Actions oder
Package-Installationspfade eingebunden. Es gibt ausdrücklich kein
automatisches Blockieren eines Fixes, Commits oder Updates.

Aufgefundene Warnungen begründen **keine eigenmächtige Änderung**
einer `AGENTS.md`; dafür bleibt die ausdrückliche
Maintainer-Genehmigung nach [AGENTS.md](../../AGENTS.md) erforderlich.
