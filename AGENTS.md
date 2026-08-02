# VDR-Suite Agent and Repository Workflow Rules

These rules apply to all automated assistants and agent-driven repository work.
They supplement the technical, security and phase-specific contracts in `docs/`.

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

## Continuous approved work

Continue through all already-approved steps of a bounded workstream without
artificial confirmation pauses. Do not stop after analysis, after each file, or
after each commit when the next operation is already covered by the user's
instruction and the repository contract.

Stop only when a real decision or safety boundary is reached, including:

- the remote branch moved unexpectedly;
- unrelated or ambiguous changes would be included;
- required source content is incomplete;
- a security, data-loss, runtime or compatibility decision needs user input;
- a required stabilization check failed;
- the next action would change PR state, merge, rewrite history, force-push or
  cross an explicitly gated runtime boundary.

## Commit, push and CI batching

Create small, coherent commits at meaningful checkpoints. Push each completed
and scoped commit immediately with fast-forward-only semantics.

Do not wait for GitHub Actions after every commit. Multiple commits may be
created and pushed consecutively while earlier workflow runs are still queued or
running. Superseded intermediate runs do not need separate analysis unless they
reveal a failure that also affects the current head.

Evaluate CI at the end of the bounded workstream or before a gated runtime,
review, readiness or merge step. The current head must pass every required job
before real-runtime installation, runtime acceptance, Ready-for-review, merge or
other phase-completion gates.

A failed required check on the final stabilization head must be diagnosed and
fixed before crossing that gate. Do not hide failures by adding unrelated
commits or by bypassing required checks.

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
