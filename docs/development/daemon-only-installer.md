# Persistent daemon-only installation

Use **tools/install_daemon.py** for a reviewed **daemon-only** installation. This deliberately does not replace the canonical full install-runtime staging procedure for coordinated daemon/frontend changes.

## Real yaVDR root-shell installation: verified GitHub identity split (14 October 2026)

**Observed failure / confirmed cause:** The operator is logged in as root and
intends to stay root. A past block ran
`sudo -u yavdr -H git clone git@github.com:hotzenplotz5/vdr-suite.git`,
and GitHub replied `Permission denied (publickey)`. SSH identities are
account-specific; root having a configured key does **not** grant it to
`yavdr`. A previous interactive `exit` also inadvertently closed the
root shell. Both are prohibited in future acceptance blocks.

**Solution:** This repository is **public** on GitHub. Clone it over public
HTTPS **as `yavdr`** into an isolated `yavdr`-owned directory, while the
terminal remains `root@yavdr`. The clone and the daemon compilation need no
SSH credential. The only privileged step is `apply`, which the installer
intentionally runs as root. In contrast, `vdr-suite-vidaa` is **private**;
its clone and the VIDAA static updater need the account whose private-repo
SSH access has been verified; see that repository's `AGENTS.md`.

### Preflight in the existing root shell — does not install anything

Use a **current, externally verified CI-green, 40-character SHA** for `PIN`;
do not substitute an unverified moving HEAD. This block does not change the
calling shell's user or its fail-fast options:

```bash
PIN="REPLACE_WITH_VERIFIED_40_CHARACTER_SHA"
BRANCH="work/device-media-profile-binding-v1-20261012"
REPO="https://github.com/hotzenplotz5/vdr-suite.git"
OK=1
[ "$(id -u)" = 0 ] || { echo "STOP=ROOT_SHELL_REQUIRED"; OK=0; }
[ "${#PIN}" -eq 40 ] || { echo "STOP=REVIEWED_SHA_REQUIRED"; OK=0; }
command -v sudo >/dev/null || { echo "STOP=SUDO_MISSING"; OK=0; }
command -v git >/dev/null || { echo "STOP=GIT_MISSING"; OK=0; }
[ "$OK" -eq 0 ] || sudo -u yavdr -H git ls-remote "$REPO" "refs/heads/$BRANCH"
```

The last read-only Git result must equal the independently reviewed `PIN`;
if not, stop, recheck PR head and CI and do not create a checkout. Verify
storage before cloning: after all planned clone, retained output and candidate
build bytes, free space on `/home/yavdr` must remain at least
`max(4 GiB, 15% filesystem total)`. Use an explicit, conservatively bounded
budget for that filesystem; `install_daemon.py build` repeats its own 2 GiB
preflight. An unbounded clone/build must not begin.

### Checkout, test, build: commands entered as root, subprocesses as yavdr

These commands are a **sequenced operator procedure**, not permission for a
blind install. Execute the next only after the previous reports success.
The root prompt does not change and none of these commands runs a root build:

```bash
WORK="$(sudo -u yavdr -H mktemp -d /home/yavdr/vdr-suite-daemon-install-XXXXXXXX)"
sudo -u yavdr -H git clone --single-branch --branch "$BRANCH" "$REPO" "$WORK/repo"
cd "$WORK/repo"
git rev-parse --show-toplevel
git rev-parse HEAD
git status --porcelain --untracked-files=no
sudo -u yavdr -H python3 tools/test_install_daemon.py -v
sudo -u yavdr -H python3 tools/install_daemon.py check --sha "$PIN"
sudo -u yavdr -H python3 tools/install_daemon.py build --sha "$PIN"
sudo -u yavdr -H python3 tools/install_daemon.py check --sha "$PIN"
```

Do **not** proceed automatically if clone, exact SHA/branch, clean source,
tests or any `check` failed. Read the complete output, especially
`INSTALLED_SOURCE_SHA`, `DB_BACKUP_DECISION` and `DB_REVIEW_PATH`.
`DB_BACKUP_DECISION=REVIEW_REQUIRED` requires a separate review of every
flagged file. Never auto-approve `no-backup` or invent a database path.
The existing binary's embedded source must resolve within the candidate's
full Git history; do not delete the prior version or fabricate provenance.

### Separate, explicit root installation (only after checks and consent)

```bash
# Root shell, inside the verified checkout of the same exact PIN:
python3 tools/install_daemon.py apply --sha "$PIN" --confirm-apply
systemctl is-active vdr-suite-daemon.service
```

`apply` never compiles. It checks the sealed binary, current service
identity, source ancestry, DB risk and backup reserve, retains a rollback
binary/receipt, and restarts only `vdr-suite-daemon.service`. If SQLite
review requires a real offline backup, use **only** the explicitly verified
`--database` path and reviewed `--db-review backup --db-review-reason`
option described below. A refusal is not an invitation to bypass gates.

**Security boundary:** No global `chown`/`chmod`, private-key copying,
root-owned build checkout, VDR/plugin/recording changes, arbitrary service
restarts, blind repository cleanup or automatic "permissions repair". No
`exit`/fail-fast toggle in an interactive root prompt. Print and retain
the exact candidate checkout path if a step fails. GitHub CI green alone
does not establish real yaVDR deployment or Hisense playback.

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

