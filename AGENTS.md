# VDR-Suite Agent and Repository Workflow Rules

## AGENTS.md-Änderungen ausschließlich nach persönlicher Erlaubnis

**Nur Änderungen an Agents.md mit meiner persönlichen Erlaubnis, oder nach meiner Anordnung!**

Diese Regel gilt für jede `AGENTS.md`-Datei im Repository. Ohne die ausdrückliche persönliche Genehmigung oder konkrete Anordnung des Maintainers dürfen Assistenten solche Dateien weder ändern noch vermeintliche Fehler darin eigenmächtig korrigieren.

These rules apply to all automated assistants and agent-driven repository work.
They supplement the technical, security and phase-specific contracts in `docs/`.

## Striktes Branch-Verbot und ein Media-Tools-Entwicklungsbranch (28.10.2026)

**Ausdrückliche Maintainer-Anordnung: AB SOFORT KEINE NEUEN GIT-BRANCHES ANLEGEN.**
Assistenten dürfen für diese Entwicklung weder zusätzliche Feature-, Fix-, Test-,
Hotfix- noch Ausweichbranches erstellen. Dieses Verbot gilt auch bei Fehlern,
Konflikten und CI-Problemen; solche Probleme sind im vorhandenen Branch zu lösen.

**Einziger Entwicklungsbranch für das Media-Tools-/Media-Import-Add-on:**
`fix/media-tools-root-scan-visible-results` (PR #556).
Sämtliche Media-Tools-Änderungen an Produktcode, Weboberfläche, Backend/Agent,
Tests, Installationswerkzeugen und Dokumentation erfolgen in diesem Branch.
Vor jeder Änderung den aktuellen Remote-HEAD und vorhandene Implementierungen
prüfen, damit keine bereits umgesetzte Arbeit nochmals oder parallel entsteht.
Andere bereits existierende Media-Tools-Branches dürfen zur Bestandsaufnahme
gelesen werden; notwendige noch fehlende Änderungen sind nach Prüfung im
einzigen Entwicklungsbranch zusammenzuführen, ohne einen neuen Branch
anzulegen. Keine zusätzlichen Media-Tools-PRs eröffnen.

**Abschluss:** Wenn die vereinbarten Funktionen vollständig implementiert,
getestet und auf dem echten yaVDR abgenommen sind, den gesamten
Media-Tools-Entwicklungsstand dieses einen Branches nach `main` mergen.
Nicht vorzeitig als fertig oder bereits gemergt melden. Bestehende
fremde Entwicklungsbranches nicht eigenmächtig löschen oder umschreiben.

## Usability First – No Unapproved Restrictions

Neue Sicherheits-, Berechtigungs-, Aktivierungs- oder Installationsbeschränkungen dürfen nicht eigenständig eingeführt werden. Bestehende Schutzmechanismen dürfen nicht ohne begründeten Bedarf verschärft werden. Änderungen müssen die Benutzerfreundlichkeit erhalten oder verbessern. Zusätzliche Prüfungen, die keine konkret nachweisbare Fehlerklasse verhindern, sind zu vermeiden. Bereits bestätigte Testergebnisse dürfen nicht ohne Anlass erneut angefordert werden.

## No hallucinated facts, user mandates, or blockers

**Hard rule:** Never invent or imply user instructions, approvals, prohibitions,
repository requirements, CI outcomes, tested behavior, repository paths, or
technical blockers. Separate **user instruction**, **actual repository rule**,
**verified source/tool result**, and **assistant recommendation/inference**.
If a rule was written by an assistant, do not attribute it to the user;
quote its actual wording and path before invoking it as a constraint. An
assistant-authored rule is not evidence that the user requested that rule.

Check the actual file or tool result before making factual claims. If a fact
cannot be verified, say **unverified**, not "confirmed"; if a conclusion is
inferred, label it an inference and its supporting observations. Do not
reinterpret a recommendation, pending test, or incidental discovery as a
mandatory stop gate. A queued, running, slow or unavailable CI job alone is
not a reason to stop already authorized independent analysis, implementation
or local tests. Continue bounded independent work without overstating its
validation, and explicitly report what is still untested. Never fabricate
successful builds, validations, deployments, or approvals.

## Non-negotiable scope, simplicity, and verification gates

**Priority:** These gates constrain every workstream, including the top-level
non-stop mandate, GitHub-first execution and commit/CI batching. "Continue
working" means finish the **already authorized, bounded task**, not discover
or implement new tasks. No speed, autonomy or non-stop instruction overrides
these gates.

1. **One task, one verifiable outcome.** Before changing product code, identify
   the concrete bug or authorized feature, its observable acceptance criterion
   and the smallest affected surface. Stay inside that boundary. Do not
   interpret "keep going" as permission to extend the project scope.
2. **Prove the need before coding.** Reproduce or trace the concrete failure
   and inspect existing safeguards before adding another guard. Hypothetical
   risks, possible future features and speculative hardening do not authorize
   implementation. Record unrelated findings without silently fixing them.
3. **Reuse existing architecture.** Prefer a small change in the current
   implementation. New database tables, services, persistent state fields,
   protocols, cross-cutting abstractions and broad refactors require BOTH
   evidence that the existing design cannot meet the agreed acceptance
   criterion AND the user's explicit authorization **before** implementation.
4. **Validate before declaring completion.** For C++, backend or
   runtime-contract changes, compile the exact candidate and run relevant
   functional/regression tests before claiming it works. State precisely
   which SHA was tested and which checks remain pending; never equate a
   pushed commit or queued CI with a passing build. A running or blocked CI
   job does NOT prohibit preparing and implementing the next already
   authorized slice in parallel. Keep work bounded, check compatibility,
   and test each candidate when tooling is available. Do not claim that an
   assistant-authored sequencing preference is a user requirement. Docs-only
   edits need proportionate documentation checks.
5. **No chain reaction of adjacent work.** Discovering an issue during a fix
   does not add it to the authorized task. Note it concisely. Only a proven
   defect blocking the current acceptance or a verified urgent security issue
   warrants interrupting the sequence; any required out-of-scope architectural
   change still needs explicit user authorization.
6. **Finish and stop at the agreed gate.** Once the bounded acceptance
   criterion and its required checks pass, report the result and stop. Do not
   invent follow-up implementation, additional hardening, refactoring,
   documentation cycles or CI-watching tasks. The non-stop mandate prohibits
   premature handoff, **not completion**.

**Mandatory pre-change check:** Before every product-code edit and subsequent
commit, ask: Is this change required for the authorized outcome? What evidence
demonstrates that? Can the existing design solve it? How will the exact result
be compiled and tested? If any answer is missing, perform a bounded diagnostic
instead of writing more code. These scope gates take precedence wherever
another section could be read as encouraging speculative expansion or
unverified commit batching.

## Top-level non-stop execution mandate

Once the user has authorized a bounded workstream, never voluntarily stop,
hand off, or end the working response while any authorized implementation,
diagnosis, validation, CI resolution, runtime preparation, repository operation
or other executable step remains. Continue using the available tools and
repository operations until the requested end state is reached.

Status updates are progress reports, not stopping points. A completed analysis,
intermediate commit, pushed head, running or queued CI job, available next
implementation step, chat length, or ordinary turn boundary is never a reason
to end an already-approved workstream. When a check fails, diagnose and repair
the demonstrated cause and continue; do not turn a fixable failure into a
handoff to the user.

An intermediate finding is never an end state. Never convert a diagnosis,
hypothesis, partial validation, partial implementation, commit, CI result,
runtime observation or progress summary into a final response while authorized
executable work remains. Continue immediately with the next executable step.

Every working response that is permitted to end must end with the heading
`## Testblock` followed by exactly one ordinary fenced `bash` block containing
the concrete branch-/head-specific commands the user can run to validate the
current candidate. The test block is the final content; no prose, status, offer
or summary may follow it. If no truthful test block can yet be produced because
the candidate is not testable, the response is not permitted to end; continue
working instead.

There is no generic repository permission to stop an authorized workstream.
A genuinely external dependency or genuinely new user decision may make the
remaining requested operation temporarily impossible, but that is a blocked
wait state rather than permission to abandon the workstream. Before reporting
such a blocker, exhaust every independent approved operation, re-read all
available authoritative repository and CI state, and identify the exact external
event or input without treating the response boundary as project completion.
Existing authorization counts: do not ask again or stop at a PR-state change,
merge, runtime action or other gate that the user already explicitly approved.
If an unexpected remote change or another safety condition can be resolved by
re-reading authoritative state, resolve it and continue.

If a GitHub Actions run for the relevant head is required for the next already-
authorized gate, do not end the working response while that run is still known
to be queued or in progress. Continue independent approved work while it runs
and re-read the run before any response termination. Never report a stale
non-terminal CI snapshot as though it were the end of the workstream.

## Product boundaries and real-VDR acceptance (2026-10-22)

**Binding rule:** Isolated test success, a passing CI matrix, a compilable
daemon, and an installed package are different evidence levels. None of them
alone proves that a capability works against the operator's real VDR.

**Keep architecture bounded:**

- Use one production owner for each API operation, domain action, persistence
  mutation, backend dispatch and readback. Route through the existing ownership
  chain; never implement a second parallel or test-only productive path just
  to make an isolated test green.
- Extend an existing module when its responsibility already includes the new
  behavior. Split/create a module only for a clear independent responsibility,
  lifecycle, dependency boundary or reuse requirement. Do not split solely to
  manufacture another test target, slice or Make include.
- Respect the documented source-group boundaries in
  `docs/development/phase-56.21-build-boundary-map.md`. Keep mocks, smoke
  helpers and test HTTP servers out of the production composition; keep
  transport parsing out of reusable domain/core code.
- Prefer completing one vertical production flow over adding successive
  wrappers, adapters and new slices around an unfinished flow. Any newly
  introduced layer must name the production dependency/boundary it protects.

**Require meaningful end-to-end evidence:**

- An API capability is implementation-complete only when the *real production
  path* is connected where applicable: public route -> authentication and
  authorization -> controller -> domain service -> durable state -> backend
  adapter/command -> VDR outcome/readback -> public response, including the
  frontend/client path for client-visible capabilities. Mark nonapplicable
  stages explicitly rather than inventing them.
- Focused unit and contract tests catch regressions while coding, but do not
  multiply near-identical unit tests or mock-only test binaries as a
  replacement for the productive composition test. Add at least one test
  covering the actual production route/composition for each changed API flow.
- Track evidence separately as `UNIT/CONTRACT`, `PRODUCTION-INTEGRATION`,
  `CI`, and `REAL-VDR`. Never collapse these into one `GREEN` or
  `DONE` claim. A missing or failed level remains clearly
  `NOT_VERIFIED` or `FAILED`, not implicitly passed.
- Maintain one cumulative API-v1 acceptance checklist (by actual route and
  capability) covering authentication/grants, valid and invalid requests,
  expected state mutation, persistence, backend command and readback,
  externally observable result, and failure paths as applicable.
  Close the implementation scope before requesting the operator's **final
  real-VDR API-v1 acceptance run**. Do not claim real-VDR acceptance from
  simulations, CI mocks, a helper's `--help`, or an HTTP 200 alone.
- Live-VDR acceptance must operate on a verified installed code candidate,
  with explicit preflight, safe read-only probes first, and separate operator
  authorization for any destructive or scheduling mutations. The real-VDR
  checklist is the final acceptance authority, not permission to bypass
  targeted safe tests for changed code beforehand.

**Limit test/build overhead:**

- Reuse accepted unchanged test evidence and verified build artifacts.
  Run focused tests on changed code during implementation and full required CI
  once at the final merge boundary under the existing CI rules.
- Do not run packaging, full regression, entire daemon builds or hosted-CI
  waits per tiny slice unless that exact changed surface requires it. Do not
  keep producing new slices solely because earlier tests were green while the
  user-visible workflow remains unfinished.

## GitHub-first execution

Use GitHub-first execution whenever the connected GitHub tools can perform the
operation safely. Read, edit, commit, push and inspect repository state through
the GitHub connector instead of asking the user to copy shell commands.

Use the local checkout only when the work genuinely requires compilation,
generated artifacts, focused local tests, access to the installed yaVDR runtime,
or an operation that the GitHub connector cannot perform safely.

Never replace an existing file from a truncated or partial fetch. Fetch the
complete content or use a bounded edit strategy. Recheck the remote branch head
before every write and inspect the resulting commit or diff before treating the
change as complete.

## Ubuntu command fidelity: no `.sh` SVDRP shortcut

On the user's Ubuntu/yaVDR host, **never propose or invoke `svdrpsend.sh`** (nor silently substitute a shell-script wrapper for an executable). This is an explicit operator prohibition, not a fallback preference. Do not confuse MLD-specific `svdrpsend.sh` behavior or forum examples with the Ubuntu host.

For all VDR/SVDRP operator test blocks, verify the Ubuntu-installed command name, binary path, supported syntax and service context from actual host/package evidence before issuing commands. If the exact working Ubuntu invocation is unverified, avoid issuing a guessed command; perform a bounded read-only identification instead. Use the verified native Ubuntu command, never a `.sh` workaround. Shell code blocks may use Bash for orchestration; the prohibition concerns invoking `.sh` wrappers as replacement utilities.

## Absolute prohibition on guessing: verified evidence before instructions

**Guessing operational facts is strictly forbidden.** Never invent, assume,
estimate, extrapolate from an earlier phase, or hand the user plausible-looking
commands when correctness is verifiable. Urgency, prior success, or pressure to
finish never waives this rule. The user's time, yaVDR installation, and live
project state must not be used to discover preventable mistakes.

**Before writing code, starting tests, or handing over executable commands,
establish the required facts from the current authoritative source:**

- Verify the intended repository, branch, **exact** remote head SHA, checkout
  root, tested SHA, and whether all necessary fixes are present. Do not
  present an older checkout's result as evidence for the current PR head.
- Read the current Makefile and included makefiles (or inspected target list)
  to prove the actual build/test targets, their prerequisites and flags.
  **Never invent a Make target, executable name or path.**
- Derive HTTP status codes, authentication and authorization behavior from
  the actual gate, router, service and tests. **Never guess a status code,**
  and never weaken a security check just to match an unsupported expectation.
- Establish intended output paths, user privileges, possible side effects and
  whether the action touches the installed service. Host-dependent facts require
  a read-only compatibility check, without unapproved storage-reserve gates.
- Check the expected head **before** handing off a long-running, detached or
  resource-intensive build. Never accidentally start an old checkout after
  fixing a newer branch.

**The test block itself must fail closed on wrong provenance.** It must
verify the checkout root, actual SHA against the selected immutable test
candidate (or an explicitly verified tracking policy), required commands
and Make targets, and applicable non-storage compatibility checks *before* work begins.
If the branch advanced, fetch and establish the corrected candidate first.
Do not give a detached build script that silently tests stale code.
Reuse accepted evidence for unaffected inputs; do not restart tests only
because documentation or unrelated code changed.

**A background process is not proof of a completed build.** `nohup`, a
printed PID or an absent result file alone does not prove the job is still
running. Detached jobs require a recorded exact SHA, log, process identity,
terminal return codes, and a read-only status command distinguishing running,
completed successfully, completed with error, and unexpectedly missing.
Never report success before the result is known. Continue to honor existing
minimal-validation and shell-option rules.

**If evidence is unavailable, stop the affected unsafe action, not the entire
authorized workstream.** Retrieve authoritative information or perform a
bounded read-only diagnostic; continue independent approved work. State
precisely what is unknown rather than manufacturing an answer. Never shift
basic repository fact-checking to the user. Diagnose a failed user test
against the exact tested SHA and demonstrated failure before requesting
another test.

**Mandatory pre-handoff self-check:** Inspect every command, checkout path,
Make target, branch/SHA precondition, expected status, side effect, output,
failure path and success criterion against the current repository and
applicable `AGENTS.md` requirements. Correct known discrepancies before
handing over any test or acceptance block. If some fact is only knowable on
the host, build a harmless read-only check into the block instead of
pretending it has already been established.

## Temporary worktree and test-artifact lifecycle

Temporary Git worktrees, detached test checkouts, build directories and staging
directories created for a bounded test or acceptance step must be treated as
short-lived resources, not retained project state.

As soon as the test or acceptance step no longer needs them:

1. verify that the exact temporary worktree contains no uncommitted or otherwise
   unsaved work that belongs to the active workstream;
2. remove that exact worktree immediately with `git worktree remove` (use
   `--force` only when the worktree is intentionally disposable);
3. remove the test-specific build/stage directories created for that step;
4. run `git worktree prune` to discard stale metadata.

Do not defer this cleanup to a later phase or accumulate obsolete worktrees in
`/tmp`, `/home`, `/root` or tool-managed worktree directories. Test blocks
that create temporary worktrees or substantial temporary build/stage trees must
include their own cleanup path in the same bounded workflow.

Before deleting anything, identify the exact resource created by the current
test. Never delete or prune a still-active worktree, build tree or stage tree
belonging to another concurrent workstream. In particular, shared or parallel
MU/client/runtime work must remain untouched unless its own workstream has
finished and its state has been checked.

Do not create large disposable build trees when verified existing artifacts can
be reused. Report actual filesystem write failures without speculative cleanup.

## yaVDR root shell, public GitHub checkout and least-privileged daemon build (2026-10-14)

**Confirmed incident:** The operator works in a persistent
`root@yavdr:/home/yavdr#` shell. An earlier installer block incorrectly
ran `sudo -u yavdr -H git clone git@github.com:hotzenplotz5/vdr-suite.git`,
which failed with `Permission denied (publickey)`: root's and yavdr's SSH
credentials are separate. An even earlier block contained an interactive
`exit` and unintentionally left the user's root shell. **Do not repeat
either failure.** The failed clone did not run the build or installation.

**Repository fact:** `hotzenplotz5/vdr-suite` is **public** on GitHub.
For a fresh unprivileged checkout, prefer
`https://github.com/hotzenplotz5/vdr-suite.git`, which does not require
root's private SSH key or an SSH identity for `yavdr`. Check real network
access before suggesting execution. Do **not** use an SSH URL under
`sudo -u yavdr` unless that account's SSH access has separately been
proven read-only. The distinct VIDAA repository is private; see its own
`AGENTS.md` for the root-identity SSH procedure.

**Privilege contract, binding for all future installation answers:**

1. Keep the operator in their existing **root shell**; never demand
   `sudo su`, `su - yavdr`, a change of login, or an interactive `exit`.
   A failed check must not terminate the calling shell.
2. Identify the actual checkout/build location before creating a
   substantial new clone. Check exact branch and full 40-character pinned
   SHA against the authoritative GitHub ref (never gate install on CI),
   and inspect existing installed service/
   source/DB risk without touching production state.
3. **Only if no suitable existing verified checkout/candidate is available,**
   create an **isolated, new, yavdr-owned temporary directory** with
   `sudo -u yavdr -H mktemp -d /home/yavdr/vdr-suite-daemon-install-XXXXXXXX`
   and clone the **public HTTPS** repository as `yavdr`. That avoids
   copying any root-owned tree, SSH private keys, or file ownership into
   the build checkout. Verify `git rev-parse --show-toplevel`, branch,
   full SHA, and tracked-clean state after clone.
4. Reuse previously accepted tests for unchanged product code. When required,
   `sudo -u yavdr -H python3 tools/test_install_daemon.py -v` and
   `sudo -u yavdr -H python3 tools/install_daemon.py build --sha ...`
   run without root, but **only after** the no-duplicate-build gate below
   proves the latter action is necessary. `install_daemon.py` explicitly refuses a root
   build. `check` is read-only and may also run under `yavdr`.
   Because the checkout belongs to `yavdr`, perform repository Git
   inspection using `sudo -u yavdr -H git ...` instead of presuming
   root trusts a differently owned working tree.
5. **Only after** a successful sealed build, repeated `check`, and
   explicit deployment authorization, execute
   `python3 tools/install_daemon.py apply --sha ... --confirm-apply`
   **as root** from the same exact checkout. **Root's Git subprocesses
   may refuse a yavdr-owned checkout with `dubious ownership`.** Give
   `apply` a process-local Git `safe.directory` for the **verified
   exact checkout path only** using `env GIT_CONFIG_COUNT=1
   GIT_CONFIG_KEY_0=safe.directory
   GIT_CONFIG_VALUE_0="$WORK/repo" python3 ...`; never modify global
   `git config --global safe.directory` or change ownership. The
   installer alone owns the daemon binary replacement and scoped service
   restart.
6. Do not run recursive `chown`/`chmod`, copy keys, alter global
   permissions, run a whole build as root, modify VDR/recordings, or
   overwrite an existing checkout to resolve an authentication failure.
   Preserve the temporary checkout and print its absolute path for a
   rerun/review; no speculative deletion.
7. For handed-off command blocks follow the repository's interactive-shell
   safety rules below: **no `set -e/-u/-o pipefail`, no naked `exit`**.
   Treat each step's failure as a STOP and guard subsequent steps. Do not
   report installation success before checking the running service.

Authoritative operational examples and gates:
`docs/development/daemon-only-installer.md`.

## KISS: daemon-only replacement is independent of the running Git branch

**Binding rule for `tools/install_daemon.py` and EVERY future install block:**

- The installed daemon is a **replaceable executable**, not the parent of the
  next build. The old and new source commits may be siblings, diverged,
  ancestors, or on unrelated branches. **Never add an ancestor/merge-base,
  branch-equality, “installed source must be X”, or CI-completion gate** as a
  prerequisite for replacing the daemon. Do not merge/cherry-pick feature
  branches solely to satisfy an invented installer ancestry rule.
- `check` determines the requested target SHA, existing verified binary/seal
  and **actual database risk**. Source-to-source `git diff OLD NEW` works
  without ancestor relationships. Existing historical source information is
  needed for assessing schema changes, **not** to dictate upgrade direction.
  Preserve database, pairing, configuration and recordings.
- **KISS sequence: `check` → reuse sealed target if present, otherwise build
  exactly once as `yavdr` → separately approved `apply` as root.** No
  needless rebuild after interrupted installation. No root compilation.
- **Never put GitHub Actions queries, CI polling, `sleep` loops, CI waits or
  external CI status checks in any installation command block or installer.**
  CI is a separate development/merge concern, not an execution step in the
  user's installation. Deliver an accurate existing-installer block promptly.
- Preserve the regression tests
  `test_divergent_daemon_history_is_not_an_install_blocker` and
  `test_divergent_previous_commit_reuses_sealed_target_without_compiling`.
  They protect this invariant. **Do not replace them with stricter old-version checks.**

## Mandatory established-feature parity and competitor comparison

Before extending a working feature to Public-v1, VIDAA, Kodi, another client
or any replacement interface, compare it against the **existing, proven
VDR-Suite Webfrontend** behavior and the authoritative service implementation.
Write down a short acceptance matrix of inputs, output totals, ordering,
metadata/title behavior, pagination, errors, grants and live integration.
Use exactly one shared domain service and index. Public-v1 adapters may
project and authorize results but must not introduce a second search algorithm,
change the effective title/ranking, or turn one stale record into failure for
otherwise valid results. Require one automated Web-vs-Public-v1 parity
regression with realistic production examples before installation acceptance.
If a required parity behavior cannot be preserved, explicitly document why
and keep the migration unaccepted until the user approves the deviation.

Additionally, for every user-facing feature or client migration, include a
concise, evidence-based comparison with the **relevant competing product**
(e.g. Kodi and Plex for TV/media discovery and playback). Mark unavailable or
unverified competitor behavior as such. Use this comparison to identify
usability gaps, not as permission for speculative features, security bypasses,
duplicated infrastructure or unrelated refactoring. The already working
Webfrontend remains the minimum functional baseline, independent of what a
competitor does.

## Public-v1 VIDAA search permission contract (2026-10-15)

Public-v1 Device `/search/recordings` requires `recordings.view`, and
`/search/epg` plus `/epg/now-next` requires `epg.view`, independently
of `channels.view` or `media.live.play`. The product grant vocabulary,
Device supported-grant allowlist and Device-grant HTTP `supportedPermissions`
**must include both read-only permissions**, and tests must prove assignment
through the normal administration service and Device authentication. Never
work around missing grants with a weaker HTTP gate, inherited browser grants,
implicit pairing rights, or unauthenticated Client fallback. Existing devices
are granted rights only by an explicit authorized administrator mutation.

## Mandatory: NEVER compile the same daemon twice / reuse before building

**ABSOLUTE PROHIBITION:** Do not compile the same daemon twice. Re-running a
daemon compilation simply because a chat restarted, a deployment failed, a
new temporary checkout was created, an agent changed, CI ran, or a
**documentation-only commit changed HEAD** is forbidden. Reuse a previously
validated build; build only when absolutely necessary and **provably no
suitable verified artifact exists**.

**Required read-only gate BEFORE any new checkout, test build or installation
block is proposed or executed:**

1. Establish the **last product-code candidate SHA** (the daemon binary's
   exact source/build provenance) separately from the **current repository
   documentation HEAD**. For a docs-only commit, verify the diff has no
   daemon source, generated/version-stamp inputs, build options, dependency,
   toolchain or packaging changes. Do **not** treat a changed docs Git SHA
   by itself as a reason to rebuild. Reuse the last verified code-SHA and
   its already accepted build/test evidence.
2. Inspect the **actually installed and running** daemon and its source
   marker, the target code-SHA, and all *known* previous, authorized
   checkout/build locations. Do not blindly discard or replace old
   checkouts or assume a newly cloned tree has the build cache. Ask for a
   small read-only host preflight if installed-build evidence is not
   available to the agent. Record the exact path and source SHA.
3. Within the correct, clean checkout of that code-SHA, run the installer's
   read-only `check --sha FULL40`; inspect `INSTALLED_SOURCE_SHA`,
   `TARGET_SOURCE_SHA`, `BUILD=REUSABLE` versus
   `BUILD=NOT_SEALED`, the candidate path, and the running service
   identity. `check` validates an existing sealed candidate's SHA-256
   and embedded `git-<source-prefix>` marker. Do not infer valid reuse
   from an artifact filename, timestamp, or identical-looking bytes alone.
4. If the intended code is **already installed and running correctly**,
   report `BUILD=SKIPPED_ALREADY_INSTALLED` and
   `APPLY=SKIPPED_ALREADY_INSTALLED`. Do not build, restart, or redeploy.
5. If `BUILD=REUSABLE` and the sealed candidate is verified, report
   `BUILD=SKIPPED_REUSABLE` and **go directly to the separately gated
   root `apply`**; **do not invoke `build` at all**.
6. If the seal is absent, inspect the known artifact directory:
   `build` may certify an existing, complete, valid binary without a
   compilation (`BUILD=RECOVERED_NO_COMPILE`). Use that supported
   recovery rather than recompiling. **Only if there is no valid binary
   or seal anywhere in the identified trusted candidate checkout(s)**
   and product-code changes actually require it, authorize **one**
   unprivileged build, under the installer's existing provenance gates. An invalid/mismatched artifact is a STOP requiring
   diagnosis; do not overwrite or rebuild over it speculatively.
7. For an existing build in another checkout, first establish exact
   source provenance, seal integrity, build options and trust. Do not
   silently copy, relabel or re-seal a binary as a different commit,
   weaken `verify_binary`, or declare two commits equivalent without
   proving the build inputs match. Prefer using its original verified
   checkout and `apply`. If safe reuse cannot be proven, **STOP for
   diagnosis** instead of automatic recompilation or new clones.
8. An actual, reviewed change to daemon build inputs, a missing valid
   binary after exhaustive bounded inspection, or an explicitly
   demonstrated corrupt/nonreusable artifact can warrant **one** new
   compile, never automatically. Report **why** the old build was
   unsuitable before invoking the build tool. Build still runs only
   under `yavdr`, the operator's shell stays root, and `apply` is
   separately authorized as root.

**Confirmed installer behavior:** `tools/install_daemon.py check` prints
`BUILD=REUSABLE` only after validating the seal and binary. Its `build`
action internally prints `BUILD=REUSED` if sealed or
`BUILD=RECOVERED_NO_COMPILE` if the already-linked binary can be verified.
This safety optimization is **per checkout and source SHA**; it does
**not** discover builds in other checkouts. Therefore the operator/agent
**must check existing artifacts before creating a new checkout**.
Do not confuse "build command is idempotent" with authorization to launch it
repeatedly.

## Canonical runtime/frontend deployment

For real yaVDR installation or runtime acceptance of repository-built VDR-Suite
runtime/frontend changes, the raw source tree is never the deployment payload.
Use the repository's canonical staged path:

1. `make stage-install-runtime RUNTIME_STAGE=...`;
2. `sudo make deploy-install-runtime RUNTIME_STAGE=... LIVE_ROOT=/`;
3. `make check-install-runtime-deployment RUNTIME_STAGE=... LIVE_ROOT=/`.

The sealed `install-runtime` `DESTDIR` tree is the authority for installable
runtime artifacts. Never cherry-pick files from `web/frontend` into
`/usr/share/vdr-suite/web/frontend`, and never treat a direct
`web/frontend` versus live-directory comparison as deployment evidence because
source-only and composed runtime artifacts differ by design.

A real runtime/frontend acceptance is not valid until the post-deployment check
reports `RUNTIME_DEPLOYMENT_MATCH=YES`. Existing `/etc` files are preserved by
the deploy helper. Frontend deployment replaces the complete staged frontend
tree so missing, stale or extra live frontend files cannot be accepted as a
current candidate.

The detailed contract is
`docs/development/install-runtime-deployment-reliability.md`.

## KISS: no arbitrary storage-reserve gates for Media Tools installation

The maintainer explicitly rejected mandatory filesystem-capacity preflights and
fixed free-space reserves (such as 4 GiB or 15%) for Media Tools staging,
installation and updates on 2026-10-28. Do not reintroduce such restrictions
in shell installers, stage composers, test blocks, or equivalent wrappers.
Reuse the already-built binaries and sealed stages whenever available.
Report real filesystem I/O failures plainly; do not perform speculative cleanup
or delete existing user data. This instruction does not authorize changes to VDR
recordings, permissions, configuration or access grants.

## Testblock repository entry

Every local VDR-Suite test or acceptance block handed to the user must enter the
intended VDR-Suite checkout before any Git, build, test, install, service or
runtime command. The first executable repository action in the block must be an
explicit `cd` to the checkout root, or a safe resolver whose successful result
is immediately `cd`'d before any repository command. Never assume the user's
shell is still in the repository merely because a previous command or message
was.

When the checkout path is already known from the current acceptance context,
use that exact absolute path. Do not rely on `~/vdr-suite` in blocks that may be
run after `sudo`, `sudo su`, `su`, or another user change, because `~` may resolve
to a different home directory. If the exact path is not known, resolve the
current checkout and bounded known absolute candidates without discarding a
valid current `vdr-suite` worktree, then enter the resolved root. A failed
hard-coded candidate must never cause `STOP` while the current directory is
already inside the correct checkout.

Immediately after entering, verify the repository root with
`git rev-parse --show-toplevel` and, when useful, the expected repository name
before branch/head validation. Test blocks must be self-contained and safe to
paste from an arbitrary shell working directory.

## Temporary worktree and test-artifact lifecycle

Temporary Git worktrees, detached test checkouts, build directories and staging
directories created for a bounded test or acceptance step must be treated as
short-lived resources, not retained project state.

As soon as the test or acceptance step no longer needs them:

1. verify that the exact temporary worktree contains no uncommitted or otherwise
   unsaved work that belongs to the active workstream;
2. remove that exact worktree immediately with `git worktree remove` (use
   `--force` only when the worktree is intentionally disposable);
3. remove the test-specific build/stage directories created for that step;
4. run `git worktree prune` to discard stale metadata.

Do not defer this cleanup to a later phase or accumulate obsolete worktrees in
`/tmp`, `/home`, `/root` or tool-managed worktree directories. Test blocks
that create temporary worktrees or substantial temporary build/stage trees must
include their own cleanup path in the same bounded workflow.

Before deleting anything, identify the exact resource created by the current
test. Never delete or prune a still-active worktree, build tree or stage tree
belonging to another concurrent workstream. In particular, shared or parallel
MU/client/runtime work must remain untouched unless its own workstream has
finished and its state has been checked.

Do not create large disposable build trees when verified existing artifacts can
be reused. Report actual filesystem write failures without speculative cleanup.

## Frontend lifecycle and composition gates

For cross-cutting frontend behavior, prove the real production composition and
lifecycle before treating an isolated implementation as integrated. Playback
work must follow `docs/development/frontend-playback-integration-contract.md`.

A wrapper around an exported method is not proof that the real browser action
passes through that wrapper. Owners may bind DOM controls directly to internal
closures before decorators are installed. Session-bound extensions must observe
the canonical owner/session lifecycle or an explicit ownership event from owner
creation through replacement, stop/restart and destroy; they must not rely only
on intercepting `start()`, `stop()`, seek or selection methods.

Tests for a new cross-cutting control must exercise the same composition root and
owner topology used by the production view, including transport replacement
when production can perform it. A direct unit call to the decorator method is
useful but insufficient as the integration test. At least one regression must
prove the user-style action -> canonical owner/session transition -> expected
Suite request/state change chain.

Keep owner-level UI outside replaceable transport DOM and extend established
owners instead of adding parallel player, request, restart or cleanup paths.
When persistent browser objects are involved, installed bundle identity and
syntax are deployment evidence only; runtime acceptance must recreate the
relevant owner before claiming the new lifecycle integration is active.

## Continuous approved work

Continue through all already-approved steps of a bounded workstream without
artificial confirmation pauses. Do not stop after analysis, after each file, or
after each commit when the next operation is already covered by the user's
instruction and the repository contract.

Do not hand a candidate to the user for acceptance testing while the current
branch head has a known implementation, runtime-wiring, packaging, deployment or
validation gap that prevents the requested acceptance scenario from being tested
truthfully. Such an intermediate head is not a testable candidate. Continue the
approved implementation and stabilization work until the requested acceptance
scope is actually runnable.

Potential blockers are not stop permissions. They matter only when they make
every remaining authorized operation impossible after all safe evidence and
independent work have been exhausted. Examples include an unrecoverable remote
branch conflict, inseparable unrelated changes, unavailable required source
content, a genuine security/data-loss/runtime compatibility decision requiring
new input, an irreparable required stabilization failure, or an exact gated
operation for which authorization has genuinely never been granted. In each
case, continue every other approved operation first and preserve the workstream
as blocked rather than declaring it complete.

## Commit, push and CI batching

Create small, coherent commits at meaningful checkpoints. Push each completed
and scoped commit immediately with fast-forward-only semantics.

Do not wait for GitHub Actions after every commit. Multiple commits may be
created and pushed consecutively while earlier workflow runs are still queued or
running. Superseded intermediate runs do not need separate analysis unless they
reveal a failure that also affects the current head.

Validation gates are surface-scoped during iterative implementation and runtime
acceptance. Require only the checks that can materially validate the changed
surface and the next action:

- frontend-only JavaScript/CSS/HTML changes require the focused frontend tests
  and `frontend-regression-test` before installing that frontend candidate;
- frontend install or packaging wiring changes additionally require the relevant
  packaging/install staging check;
- backend, daemon, C++ or runtime-contract changes require the corresponding
  build, fast-regression, architecture and other directly affected checks;
- documentation-only changes require documentation validation and do not
  invalidate already accepted product/runtime evidence. Validate relevant
  text/link/format, not a new build or unrelated runtime acceptance.
  **Do not wait for GitHub Actions to finish for docs-only commits.**

Do not block a targeted runtime installation or acceptance test on unrelated CI
jobs merely because they belong to the same workflow. Such jobs may still be
queued, running, or failed for a demonstrably unrelated reason. Diagnose enough
to establish that the failure is unrelated to the candidate being installed,
then continue with the already-approved targeted acceptance path.

The complete repository-required CI graph is a gate for Ready-for-review, merge,
phase closeout or another explicit full-stabilization boundary. Use
surface-scoped checks for draft development and independently gated deployment.

**FULL CI is a mandatory gate immediately before merging the reviewed
candidate into `main`, not a reason to wait on documentation-only commits,
draft updates, interim work or routine source handoffs.** The full final-head
CI must pass before merge, plus any genuinely separate safety/hardware gates.
Relevant targeted tests for code/runtime changes still apply before risky
actions; do not skip them under this rule.

A failed check that is required for the current changed surface or current gate
must be diagnosed and fixed before crossing that gate. Do not hide failures by
adding unrelated commits or by bypassing genuinely relevant checks.

## Product-code-first CI and post-merge stabilization

**Documentation-only changes must NEVER cause waiting for a CI run, a CI
polling loop, a new daemon build or a repeat of already proven runtime
acceptance.** Commit the documentation, inspect its contents and the
branch/commit, report the status and proceed immediately. GitHub Actions may
run automatically; do not await completion or generate another commit to
"make CI green" merely for docs. Require the full relevant CI graph
**once, at the actual final-head pre-merge-to-`main` gate**. If code changes
alter a relevant safety/test surface, validate that surface at the time it
matters; this rule never permits installing untested changed product code.

**Finish the bounded productive code and focused tests before spending time on
full CI stabilization.** Batch coherent implementation commits and validate the
relevant final candidate. Respect required pre-merge security, architecture,
test, runtime and branch-protection gates; this rule does not authorize merging
red or unverified code or skipping required checks.

**A successful local daemon build is sufficient build evidence before merge.**
When the daemon must be built locally, a successful build of the exact candidate
is the build proof. Do not demand, repeat or await an additional hosted CI
build merely to prove that the same daemon compiles. Record the tested commit
and build result. This does not substitute for distinct functional/security
acceptance or checks that GitHub branch protection actually requires.

**After an authorized merge, verify the exact merged `main` head and its hosted
CI run.** If that run reveals a defect, repair it promptly against the current
`main` through the repository's permitted branch/PR workflow, then verify the
resulting merged `main` again. Do not silently rewrite `main`, bypass PR
restrictions or overlap other developers' work. Avoid endless repetitive CI
polling: use checks only when they can change the next action; keep advancing
independent approved product tasks while the run progresses.

**Priority:** shipping safe, tested product functionality over polishing
intermediate documentation status. Status docs and guards should be updated at
a meaningful product closeout, not as repeated standalone blockers.

## Minimal necessary validation

Choose the shortest safe path and treat the user's time as a project resource.
Run only checks that can still change the next decision.

Do not repeat a test merely because another commit was created, documentation
was edited, the chat changed, or an earlier accepted result is inconvenient to
reuse. Accepted CI, runtime evidence and immutable fingerprints remain valid
until a directly relevant input changes.

Do not run broad local Make targets, full regression suites or local CI copies
when GitHub Actions already covers the same unchanged product code. In
particular, a documentation-only closeout must not trigger local product,
architecture, packaging or full documentation suites unless that exact change
cannot be validated by the final-head GitHub CI.

Use the local yaVDR host only for facts GitHub cannot establish, such as the
installed daemon, service state, real configuration, database integrity and
controlled runtime acceptance. Once that runtime acceptance is recorded, do not
repeat it for a documentation-only follow-up.

When a check fails, run the smallest command that reproduces the failure and
fix only the demonstrated cause. Do not add speculative validators, duplicate
safety gates or large recovery scripts without a concrete risk they uniquely
cover.

Before a routine documentation commit, the normal local maximum is the minimal
content edit plus `git diff --check`. Commit and push immediately, **do not
wait for or poll the automatically triggered CI**, and reuse accepted product
evidence. Require complete CI only when the exact candidate is about to be
merged to `main`.

## Branch and pull-request safety

Keep updates fast-forward-only. Never force-push, rebase or rewrite published
history unless the user explicitly approves that exact operation.

Do not mark a Draft pull request Ready, merge it, close it, enable auto-merge,
change its base, or mutate review state or PR metadata without explicit user
approval.

Keep each implementation slice within its documented boundary. Do not combine
unrelated phases, runtime installation, broad refactors or deferred features
merely to reduce the number of commits.

### Main-first branch lifecycle and grandfathered work

Follow [Branch- und Pull-Request-Lebenszyklus](docs/development/branch-pull-request-lifecycle.md).
New work starts on the **current `main`** of the correct repository; do
not create nested successor branches for the same work. **Branches and PR
stacks already active before 2026-10-18 are grandfathered**: never
force-rebase, rename, close, merge, delete or replace them just to satisfy
the new rule. Any new stacked branch needs explicit approval and a documented
dependency.

After an authorized merge confirm `Merged/Closed`, exact `main` head and
relevant CI, then delete the head branch **only after** checking Launchpad
recipes, dependent PRs, workflows and other consumers. Report
`BRANCH=RETAINED` with the reason if needed or if deletion is unavailable.
Branch protection always takes precedence over a request for a direct
`main` write.

## Reporting

Report completed commits, the current remote head, relevant validation and the
next actual gate. Do not present every pushed commit as a reason to pause. When
CI is intentionally batched, state that the final current head is the one whose
CI result matters.

Whenever a GitHub Actions run exists for the relevant head, every CI status
report must include a directly clickable link to that exact run. Also state the
workflow or run number, run ID and head commit so the user never has to search
for the run manually. This applies while the run is queued, running, failed,
retried or completed. Do not provide only a textual CI status. When no run has
been assigned yet, state that explicitly and provide the link as soon as the run
exists.

Before ending any working response, verify that the requested end state has
actually been reached. If a relevant CI run exists for the current head and its
result matters to the next already-authorized gate, re-read that run immediately
before the response ends. Never finish with a statement that the next step is
still executable with the available tools; perform that step instead.

## Testblock shell option safety

VDR-Suite and yaVDR test/acceptance blocks must never enable shell fail-fast or
unset-variable abort semantics. Do not use `set -e`, `set -u`, `set -o pipefail`
or combined forms such as `set -euo pipefail`. Test blocks must keep the
interactive shell alive so later diagnostics remain visible. At the beginning
of every handed-off test block explicitly neutralize inherited settings with:

```bash
set +e
set +u
set +o pipefail
```
