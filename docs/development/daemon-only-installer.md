# Persistent daemon-only installation

Use **tools/install_daemon.py** for a reviewed **daemon-only** installation. This deliberately does not replace the canonical full install-runtime staging procedure for coordinated daemon/frontend changes.

## Three independent commands

From the exact, clean source checkout (normally /home/yavdr/vdr-suite), select an already-reviewed full 40-character source commit:

    cd /home/yavdr/vdr-suite
    SHA="$(git rev-parse HEAD)"
    python3 tools/install_daemon.py check --sha "$SHA"
    python3 tools/install_daemon.py build --sha "$SHA"
    python3 tools/install_daemon.py apply --sha "$SHA" --confirm-apply

**check** is read-only. It requires an exact clean source HEAD, determines the old commit from the running binary's embedded git-version marker, checks ancestry and evaluates database migration risk. If source identity or history is unknown, it refuses installation instead of creating an unrequested backup.

**build** invokes only the verified Makefile daemon target. It creates a persistent, commit-keyed binary in .build/daemon-installer/<SHA>/ and an integrity seal. Completed builds are reused. A successful binary left behind by an interrupted sealing step is recovered without recompilation. The build and log are never automatically removed.

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

The optional database snapshot is made only **after stopping the daemon**, under the candidate directory, with a storage reserve check. It is never launched as an unbounded live SQLite backup. A source-pattern review is conservative and may flag unchanged SQL; it is not a proof that the database or a migration is harmless.

Cache data might be regenerated, but human accounts, device credentials, grants and other VDR-Suite-authored state cannot simply be recreated from VDR. Therefore a schema-changing deployment requires an explicit human review, not an arbitrary global rule to skip or perform backups.

## Guarantees and limits

- The tool does not clone, fetch, checkout, rebase or merge Git history.
- It never changes recordings, recording indexes, the VDR daemon, Nginx, TV apps, frontend assets or SQLite on a no-backup path.
- Its disk preflight reserves the larger of 4 GiB and 15 percent of total filesystem capacity; the build uses a conservative 2 GiB estimate.
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

Die Befehle sind absichtlich getrennt; alle Arbeiten im Checkout als
normaler Benutzer. Niemals den Source-Build mit sudo starten:

```bash
# Nur im explizit geprüften Checkout dieses Branches:
PIN="$(git rev-parse HEAD)"
python3 tools/test_install_daemon.py -v
python3 tools/install_daemon.py check --sha "$PIN"
python3 tools/install_daemon.py build --sha "$PIN"
python3 tools/install_daemon.py check --sha "$PIN"
```

Erst nach grünem `check`, vollständig versiegeltem Build, überprüftem
`DB_BACKUP_DECISION` und ausdrücklicher Installationsfreigabe:

```bash
sudo python3 tools/install_daemon.py apply --sha "$PIN" --confirm-apply
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
sudo python3 tools/install_daemon.py rollback --sha "$PIN" --confirm-rollback
```

Bei historisch `REVIEW_REQUIRED` ist zusätzlich die Bewertung und
explizite `--db-review no-backup --db-review-reason "…"` erforderlich;
bei einem tatsächlichen SQLite-Schemawechsel reicht ein Binary-Rollback
nicht aus. Datenbanken werden **niemals** automatisch zurückgeschrieben.

`check` ist ohne Produktionsschreibzugriffe. `build` schreibt nur ins
persistente private SHA-Artefaktverzeichnis; root-Build ist verboten.
`apply` stoppt/startet ausschließlich `vdr-suite-daemon.service`,
kopiert die vorherige Binärdatei samt SHA-Receipt und prüft danach den
laufenden Prozess. Fehler führen zum automatischen Wiederanlauf der
Altversion, sofern möglich. Bei fehlerhafter Recovery muss der Operator
den tatsächlichen Service-/Rollback-Status überprüfen; niemals Erfolg
behaupten. Der Daemon wird durch GitHub-CI **nicht** auf yaVDR installiert.

**Status:** isolierte CI-Tests bestanden; echter yaVDR-Build, Install,
Device-JPEG-Antwort, HEVC und TV-Akzeptanz ausstehend.

