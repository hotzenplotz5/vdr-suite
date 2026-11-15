# Persistent daemon-only installation

Use **tools/install_daemon.py** for a reviewed **daemon-only** installation. This deliberately does not replace the canonical full install-runtime staging procedure for coordinated daemon/frontend changes.

## KISS invariant: a daemon binary may replace a different Git history

The running daemon's source SHA and branch **do not have to be ancestors**
of the chosen target. Static binary replacement is valid across diverged
branches. `tools/install_daemon.py` has no `merge-base --is-ancestor`
gate; `git diff OLD NEW` is used only to assess **actual SQLite/schema
risk**. Protect the database and other mutable state separately.

Reuse an existing verified target build and seal instead of compiling again.
Only build once when there is no reusable target; `apply` never builds.
The `build` action passes `VDR_SUITE_SERVER_VERSION=git-<source SHA first 12>`
to `make daemon` explicitly: the repository `VERSION` file contains the
product release (`0.14.1`), which is not an immutable source identity.
An unsealed binary with the wrong provenance is rejected without silently
sealing, relabeling or deleting it. When retrying at a new code SHA, the
candidate path is distinct, preserving any unsuccessful earlier build.
Keep the installation block short and use the **existing installer**.
**Never add a GitHub CI query, poll, `sleep` or completion prerequisite to
the install block.** CI happens independently during development/review.

Regression contract: `tools/test_install_daemon.py` covers diverged
old/new commits, read-only `check`, sealed build reuse without invoking
`make`, and schema changes still requiring review.

## Aktueller yaVDR-Ablauf in der bestehenden Root-Shell (03.11.2026)

**Verbindlicher Vorrang:** Die neuere [Root-Shell-Vorgabe](../../AGENTS.md)
ersetzt die älteren Beispiele vom 14.10.2026, die `sudo -u yavdr`,
separate Benutzer-Checkouts und unprivilegierte Daemon-Builds verlangten.
Der Operator bleibt für die gesamte beauftragte yaVDR-Installation
einschließlich eines nötigen C++-Builds in seiner **bestehenden Root-Shell**.
Kein `runuser`, `su`, `sudo -u`, interaktives `exit` oder spekulatives
Umschreiben von SSH-Schlüsseln, Berechtigungen oder Dateieigentümern.
Die nachgewiesenen SSH-Fehler und die passende Identitätsprüfung sind in
[agent-yavdr-github-ssh.md](agent-yavdr-github-ssh.md) dokumentiert.

Die aktuellen ausführbaren Voraussetzungen stammen aus
`tools/install_daemon.py`: `check` ist lesend, `build` prüft die
versiegelten Artefakte und enthält keine eigene UID-0-Sperre, `apply`
und `rollback` verlangen Root-Rechte sowie ihre jeweilige
ausdrückliche Bestätigung. Vor einer Ausführung den tatsächlichen
Quellstand und die aktuelle Implementierung prüfen.

### Quelle, Kandidat und Wiederverwendung

1. Zuerst den **vorhandenen** Checkout `/home/yavdr/vdr-suite`, seinen
   tatsächlichen Branch, Git-Status und vollständigen HEAD prüfen.
   Vorhandene gültige Kandidaten, Objekt-Caches und Builds in bekannten
   genehmigten Checkouts zuerst identifizieren; kein unnötiger neuer Clone.
   Ein anderer existierender Branch darf gemäß `AGENTS.md` regulär
   gewechselt werden; keine neuen Branches oder Worktrees als Umweg.
2. `CODE_PIN` ist die tatsächlich zu installierende, zuvor geprüfte
   **40-stellige Produktcode-SHA**, nicht automatisch der neueste
   Dokumentations-HEAD (`DOC_HEAD`). Bei reinem Dokumentations-Commit
   bleiben belegte Build-/Testnachweise des passenden Code-Kandidaten
   gültig; Änderungen an Build-Eingaben müssen separat bewertet werden.
   Ein vorhandenes Binary niemals mit einer fremden SHA neu etikettieren.
3. Im zur `CODE_PIN` passenden, sauberen Checkout
   `tools/install_daemon.py check --sha CODE_PIN` **lesend** ausführen.
   `INSTALLED_SOURCE_SHA`, `TARGET_SOURCE_SHA`,
   `DB_BACKUP_DECISION`, `DB_REVIEW_PATH` und `BUILD` prüfen.
   Die Identität von Binary und Seal wird über SHA-256 und eingebettete
   Source-Markierung geprüft; Dateiname oder Zeitstempel genügen nicht.
4. Falls der Ziel-Daemon bereits korrekt installiert und aktiv ist,
   weder bauen noch anwenden. Bei `BUILD=REUSABLE` den Build
   überspringen. Wenn nur das Seal fehlt, kann `build` ein
   vollständiges, verifizierbares Binary ohne Kompilierung nachversiegeln
   (`BUILD=RECOVERED_NO_COMPILE`).
5. Nur falls nach gezielter Prüfung kein gültiger Build existiert und
   eine echte Änderung der Build-Eingaben es erfordert, **einmal**
   `tools/install_daemon.py build --sha CODE_PIN` im bestehenden
   Root-Kontext ausführen. Fehlende oder beschädigte Artefakte nicht
   spekulativ überschreiben, verschieben, neu etikettieren oder durch
   frische Checkouts verstecken.
6. Vor `apply` die tatsächlichen Datenbank-/Schema-Risiken prüfen.
   `apply` ist eine getrennt zu genehmigende mutierende Aktion,
   die selbst niemals kompiliert. Bestehende Daten, Zugangsdaten,
   Konfigurationen und Aufnahmen bleiben geschützt. Keine CI-Wartezeit,
   kein pauschales Speicherkontingent und keine erfundenen
   Vorbedingungen in Installationskommandos einführen.

### Kompakte, lesende Identifizierung im vorhandenen Checkout

Das folgende Beispiel **installiert nichts**. Der spätere
`CODE_PIN` ist aus dem geprüften Produktcode-Stand zu bestimmen und
muss mit dem tatsächlich verwendeten Checkout übereinstimmen.
Nicht ohne Prüfung direkt `HEAD` als installierbaren Code-Stand ausgeben.

```bash
set +e
set +u
set +o pipefail
cd /home/yavdr/vdr-suite
git -c safe.directory=/home/yavdr/vdr-suite status --short
git -c safe.directory=/home/yavdr/vdr-suite branch --show-current
git -c safe.directory=/home/yavdr/vdr-suite rev-parse HEAD
```

Bei einem von `root` nicht als sicher eingestuften bestehenden Checkout
nur die **konkrete geprüfte** Git-`safe.directory`-Ausnahme pro Prozess
verwenden; keine globale Git-Konfiguration oder rekursiven
`chown`/`chmod`-Anpassungen. Ein nicht zur geprüften SHA
passender Checkout ist vor `check`/`build` gezielt und verlustfrei
zu korrigieren. In der vorhandenen Root-Shell bleiben alle Installations-
und Verifikationsbefehle unter derselben Identität; der getrennte
`apply`-Schritt benötigt die ausdrückliche Installationsfreigabe.

### Historische Einordnung

Der früher dokumentierte Ablauf vom 14.10.2026 enthielt einen
als `yavdr` gestarteten SSH-Clone, einen Wechsel in einen temporären
Benutzer-Checkout und den pauschalen Ausschluss von Root-Builds.
**Diese damaligen Beispiele sind keine aktuell gültige Anleitung.**
Die Belege zu den damaligen Fehlern stehen in
[agent-yavdr-github-ssh.md](agent-yavdr-github-ssh.md).
Der Installer unterscheidet weiterhin `check`, `build`,
`apply` und `rollback`; daraus entsteht keine Verpflichtung,
den Unix-Benutzer zu wechseln.

## Three independent commands

From the exact, clean source checkout (normally /home/yavdr/vdr-suite), select an already-reviewed full 40-character source commit:

    cd /home/yavdr/vdr-suite
    SHA="$(git rev-parse HEAD)"
    python3 tools/install_daemon.py check --sha "$SHA"
    # If already installed: do nothing.
    # If BUILD=REUSABLE: do NOT invoke build.
    # Only if BUILD=NOT_SEALED, no valid binary/reusable prior build exists,
    # and actual product changes demand it: build once in the existing root shell.
    # After rechecking the sealed result and DB gate, root applies explicitly.

**check** is read-only. It requires an exact clean source HEAD, determines the old commit from the running binary's embedded git-version marker, evaluates database migration risk across the verified old and target sources. If source identity or history is unknown, it refuses installation instead of creating an unrequested backup.

**build** is **not** a mandatory step in every installation. Use it once
only when the no-duplicate-build gate proves there is no reusable artifact.
It invokes only the verified Makefile daemon target. It creates a persistent, commit-keyed binary in .build/daemon-installer/<SHA>/ and an integrity seal. Completed builds are reused. A successful binary left behind by an interrupted sealing step is recovered without recompilation. The build and log are never automatically removed.

**apply** never builds. It requires a sealed matching binary, root permissions, explicit confirmation, a running old service and sufficient disk space. It preserves the previous daemon binary as a bounded rollback copy, replaces only /usr/sbin/vdr-suite-daemon, restarts only vdr-suite-daemon.service and verifies that the running executable matches the expected hash. On failure or interruption during service modification, it attempts to restore the previous executable and restart it.

## Database backup policy

There is **no automatic database backup** for an ordinary daemon update. Database decisions use a conservative source comparison between the verified installed commit and the candidate:

- **DB_BACKUP_DECISION=NOT_REQUIRED**: No detected schema/migration-risk source changes. Apply makes no database backup and does not access SQLite.
- **DB_BACKUP_DECISION=REVIEW_REQUIRED**: Source changes touch schema/migration-related paths or SQL-bearing C++ sources. Installation refuses to proceed until the listed DB_REVIEW_PATH files have been evaluated.
- **Unknown installed provenance or missing Git history**: Stop. Neither guess nor perform a speculative backup.

After reviewing the relevant files, explicitly select one decision with a documented reason:

    python3 tools/install_daemon.py apply --sha "$SHA" --confirm-apply --db-review no-backup --db-review-reason "Reviewed flagged sources; no schema mutation during startup"

Or only if the reviewed migration really requires a backup, give the **verified actual** database path:

    python3 tools/install_daemon.py apply --sha "$SHA" --confirm-apply --db-review backup --database /path/to/verified/vdr-suite.db --db-review-reason "Reviewed startup migration; offline snapshot required"

The optional database snapshot is made only **after stopping the daemon**, under the candidate directory, without an arbitrary free-space reserve gate. It is never launched as an unbounded live SQLite backup. A source-pattern review is conservative and may flag unchanged SQL; it is not a proof that the database or a migration is harmless.

Cache data might be regenerated, but human accounts, device credentials, grants and other VDR-Suite-authored state cannot simply be recreated from VDR. Therefore a schema-changing deployment requires an explicit human review, not an arbitrary global rule to skip or perform backups.

## Guarantees and limits

- The tool does not clone, fetch, checkout, rebase or merge Git history.
- It never changes recordings, recording indexes, the VDR daemon, Nginx, TV apps, frontend assets or SQLite on a no-backup path.
- It does not impose fixed or percentage-based free-space reserves on builds, binary rollback copies or explicitly requested database backups.
- A failure cannot silently erase the successfully compiled candidate; no automatic removal of old tree directories or backups occurs.
- This is a bounded **daemon-only** workflow, not a replacement for full staged runtime/frontend deployments or an application-package upgrade.
- CI acceptance is not proof of operation on yaVDR, authenticated Device API behavior or actual Hisense playback.

Run the isolated, non-destructive regression suite:

    python3 tools/test_install_daemon.py -v

## Device-Trickplay-Daemon – geprüfter Kandidat (13.10.2026)

Dieser Branch basiert auf Backend-Trickplay-PR #473 und ergänzt den bestehenden
Daemon-only-Installer aus PR #451. **Nur Daemon**, keine Plugin-, VIDAA-,
Nginx-, Frontend-, Paket- oder VDR-Änderungen.

Der Installer prüft, dass das gebaute Binary sowohl die existierenden
Device-Track-Funktionen als auch die neue Device-JPEG-Trickplay-Route enthält.
Das ist eine statische Binärprüfung und kein Ersatz für einen authentifizierten
HTTP-Hardwaretest.

Die Befehle sind absichtlich getrennt; der folgende historische Testfall
unterliegt heute der Root-Shell-Vorgabe und wird im bereits geprüften Checkout ausgeführt:

```bash
# Nur im explizit geprüften Checkout dieses Branches:
PIN="$(git rev-parse HEAD)"
python3 tools/test_install_daemon.py -v
python3 tools/install_daemon.py check --sha "$PIN"
# Only if the preceding read-only check found no valid existing artifact:
# python3 tools/install_daemon.py build --sha "$PIN"
# Re-check if a fresh build/recovery was actually necessary:
python3 tools/install_daemon.py check --sha "$PIN"
```

Erst nach grünem `check`, vollständig versiegeltem Build, überprüftem
`DB_BACKUP_DECISION` und ausdrücklicher Installationsfreigabe:

```bash
python3 tools/install_daemon.py apply --sha "$PIN" --confirm-apply
```

Bei `DB_BACKUP_DECISION=REVIEW_REQUIRED` erfolgt **keine automatische
Installation**: Die angezeigten `DB_REVIEW_PATH` müssen gegen den exakten
Altstand bewertet werden. Anschließend ist eine dokumentierte Entscheidung
mit `--db-review` und `--db-review-reason` erforderlich, gegebenenfalls
mit einer explizit geprüften Datenbankdatei. Geräte-Credentials, Benutzer
und Grants dürfen niemals aufgrund einer vermuteten Schema-Verträglichkeit
gefährdet werden.

Für spätere Probleme gibt es den eigenständigen, ausschließlich manuellen
Binary-Rollback. Er ist nur bei nachgewiesener Binärherkunft, versiegeltem
Vorversion-Backup und geprüfter Datenbank-Risikostufe erlaubt:

```bash
python3 tools/install_daemon.py rollback --sha "$PIN" --confirm-rollback
```

Bei historisch `REVIEW_REQUIRED` ist zusätzlich die Bewertung und
explizite `--db-review no-backup --db-review-reason "…"` erforderlich;
bei einem tatsächlichen SQLite-Schemawechsel reicht ein Binary-Rollback
nicht aus. Datenbanken werden **niemals** automatisch zurückgeschrieben.

`check` ist ohne Produktionsschreibzugriffe. `build` schreibt nur ins
persistente private SHA-Artefaktverzeichnis; ein nötiger Build erfolgt im bestehenden autorisierten Root-Kontext.
`apply` stoppt/startet ausschließlich `vdr-suite-daemon.service`,
kopiert die vorherige Binärdatei samt SHA-Receipt und prüft danach den
laufenden Prozess. Fehler führen zum automatischen Wiederanlauf der
Altversion, sofern möglich. Bei fehlerhafter Recovery muss der Operator
den tatsächlichen Service-/Rollback-Status überprüfen; niemals Erfolg
behaupten. Der Daemon wird durch GitHub-CI **nicht** auf yaVDR installiert.

**Status:** isolierte CI-Tests bestanden; echter yaVDR-Build, Install,
Device-JPEG-Antwort, HEVC und TV-Akzeptanz ausstehend.

