# yaVDR GitHub/SSH: belegter Vorfall und verbindlicher Operator-Ablauf

Dieses Dokument bewahrt die ausführliche Fehlervorgeschichte und konkrete
SSH-/Git-Anweisungen, die zuvor direkt in `AGENTS.md` standen.
Die neuere [Root-Shell-Vorgabe](../../AGENTS.md) vom 03.11.2026 gilt auch hier:
**Bei Installationen keinen Unix-Benutzerwechsel veranlassen.**
Die hier beschriebene SSH-Prüfung ist für GitHub-Remote-Operationen bestimmt;
sie ist keine neue Installations- oder Build-Sperre.

## GitHub SSH identity verification before console commands (28.10.2026)

**Explicit maintainer order after a repeated live failure:** Never issue GitHub
SSH commands under a guessed Unix user or switch users blindly. The existing
yaVDR checkout is `/home/yavdr/vdr-suite`; the operator currently uses an
interactive `root` shell. `root` and `yavdr` have distinct SSH identities.
The actual incident was `runuser -u yavdr -- git push origin --delete ...`
failing with `git@github.com: Permission denied (publickey)`. No branch was
deleted. **Do not repeat that command or assume either account has working
GitHub credentials.**

For each proposed console GitHub operation:

1. Respect the user's current shell and checkout; do not tell them to enter
   `cd vdr-suite` from inside `/home/yavdr/vdr-suite`, switch shell users,
   run an interactive `exit`, move keys, or change SSH configuration or file
   ownership speculatively.
2. Before suggesting any SSH-based remote mutation (`git push`, push-delete,
   tag push), verify connectivity **under the same effective Unix identity**
   and with the same remote as the subsequent write. For the existing root
   console, a bounded read-only check is:
   `GIT_SSH_COMMAND='ssh -o BatchMode=yes' git -c safe.directory=/home/yavdr/vdr-suite -C /home/yavdr/vdr-suite ls-remote --heads origin`.
   Proceed with a root-shell Git mutation only if that check succeeds.
   `-c safe.directory=...` is process-local, not global Git configuration.
3. If SSH authentication fails, **do not retry the same SSH transport**,
   claim anything was deleted, or switch to `runuser -u yavdr` without a
   separately verified credential. Identify an actually available alternative
   (for example a verified `gh` authentication or another authorized GitHub
   API workflow); never assume `gh` is installed or logged in, and never
   ask users to disclose tokens or private keys.
4. For branch deletion, inspect remote branch names, preserve `main` and every still-needed development or release branch, and ensure any unique
   work has been incorporated or explicitly preserved. Verify deletion with
   a fresh remote listing; describe failed or unattempted deletion accurately.
   A read-only `ls-remote` success proves remote reachability, not push
   permission. Do not run deletions until the user authorized them.
5. Shell examples must be safe when pasted into the current **interactive**
   root shell: no unguarded `exit` and no `return` as an error handler at
   the top level. Prefer `git -C /home/yavdr/vdr-suite` or guarded
   `if cd ...; then ...; fi` for repository commands.

**Abgrenzung:** Der oben historisch dokumentierte Fehler ist keine Erlaubnis,
neue Branches anzulegen oder zu löschen, globale Git-Konfigurationen zu ändern,
vorhandene Schutzrechte aufzuweichen oder eine Installation zu blockieren, die
über den vorhandenen autorisierten GitHub-Connector abgewickelt wird.
