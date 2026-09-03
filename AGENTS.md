# VDR-Suite Agent and Repository Workflow Rules

These rules apply to all automated assistants and agent-driven repository work.
They supplement the technical, security and phase-specific contracts in `docs/`.

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
  invalidate already accepted product/runtime evidence.

Do not block a targeted runtime installation or acceptance test on unrelated CI
jobs merely because they belong to the same workflow. Such jobs may still be
queued, running, or failed for a demonstrably unrelated reason. Diagnose enough
to establish that the failure is unrelated to the candidate being installed,
then continue with the already-approved targeted acceptance path.

The complete repository-required CI graph is a gate for Ready-for-review, merge,
phase closeout, or another explicitly documented full-stabilization boundary. It
is not a mandatory gate for every iterative real-runtime test.

A failed check that is required for the current changed surface or current gate
must be diagnosed and fixed before crossing that gate. Do not hide failures by
adding unrelated commits or by bypassing genuinely relevant checks.

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
content edit plus `git diff --check`. Commit and push immediately, then let the
single final-head GitHub CI run the repository's required test graph.

## Branch and pull-request safety

Keep updates fast-forward-only. Never force-push, rebase or rewrite published
history unless the user explicitly approves that exact operation.

Do not mark a Draft pull request Ready, merge it, close it, enable auto-merge,
change its base, or mutate review state or PR metadata without explicit user
approval.

Keep each implementation slice within its documented boundary. Do not combine
unrelated phases, runtime installation, broad refactors or deferred features
merely to reduce the number of commits.

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
