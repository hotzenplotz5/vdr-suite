#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CURRENT_STATUS = ROOT / "docs/development/current-status.md"
AGENT_RULES = ROOT / "AGENTS.md"
NEW_CHAT_HANDOFF = ROOT / "docs/NEW-CHAT-HANDOFF.md"
CI_STATUS_HANDOFF = ROOT / "docs/development/github-actions-status-handoff.md"

# GitHub update safety rules for future assistants:
#
# - Never replace a complete existing file through GitHub update_file from a
#   truncated or partial fetch.
# - For index files and status files, a small link addition must remain a
#   small diff.
# - If a GitHub fetch result is truncated, fetch the missing ranges before
#   updating the file or use a smaller safe edit strategy.
# - After every GitHub file update, inspect the commit diff before treating
#   the change as correct.
# - Once a bounded workstream is authorized, never voluntarily end the working
#   response while authorized executable work remains. Status updates and
#   ordinary turn boundaries are not handoff points.
# - An intermediate finding is never an end state.
# - Every working response that is permitted to end must end with ## Testblock.
# - The Testblock must be exactly one ordinary fenced bash block and final content.
# - If the candidate is not truthfully testable, continue working instead of ending.
# - There is no generic repository permission to stop an authorized workstream.
#   External dependencies are blocked wait states, not project completion.
# - A fixable failed check is diagnosis/fix work, not a reason to stop.
# - Existing authorization counts; do not ask again or stop at an already
#   approved PR-state, merge, runtime or other gate.
# - If relevant CI is required for the next already-approved gate, continue
#   independent work and re-read that run before any response termination.
# - Never finish with a stale queued/in-progress CI status when the run can be
#   re-read and its result matters to the next authorized operation.
# - Prefer GitHub-first execution when the connector can perform the complete
#   bounded operation safely.
# - Continue through already-approved steps without artificial confirmation
#   pauses.
# - Create and push coherent commits consecutively with fast-forward-only
#   semantics.
# - Do not wait for GitHub Actions after every commit. Keep approved
#   documentation and frontend work moving while unrelated CI jobs run.
# - Use surface-scoped checks for iterative implementation and runtime
#   acceptance. Require the complete repository CI graph only at
#   Ready-for-review, merge, phase closeout or another explicit
#   full-stabilization boundary.
# - Do not create a temporary pull request solely to wait for GitHub Actions
#   unless the user explicitly requests that workflow.
# - Every executable command supplied to the user must remain inside an
#   ordinary fenced Markdown code block, preferably tagged bash.
# - A daemon build-and-install request must produce one concise ordinary bash
#   block containing only the verified checkout, build, install and service
#   commands unless the user explicitly requests additional scope.
# - Established-host instructions must use git pull --ff-only and must not add
#   package-management commands unless the user requests them or a real build
#   failure proves a missing dependency.

REQUIRED_CURRENT_STATUS_RULES = [
    "### Preferred edit path for new chats",
    "Prefer direct GitHub repository updates for existing files",
    "Use local edits first only when the change requires:",
    "a workaround because the GitHub connector blocks a file operation",
]

REQUIRED_AGENT_RULES = [
    "## Top-level non-stop execution mandate",
    "never voluntarily stop,",
    "Status updates are progress reports, not stopping points.",
    "An intermediate finding is never an end state.",
    "Every working response that is permitted to end must end with the heading",
    "`## Testblock` followed by exactly one ordinary fenced `bash` block",
    "The test block is the final content; no prose, status, offer",
    "the candidate is not testable, the response is not permitted to end",
    "There is no generic repository permission to stop an authorized workstream.",
    "wait state rather than permission to abandon the workstream",
    "do not end the working response while that run is still known",
    "re-read the run before any response termination",
    "## GitHub-first execution",
    "Continue through all already-approved steps of a bounded workstream",
    "Potential blockers are not stop permissions.",
    "Push each completed",
    "Do not wait for GitHub Actions after every commit.",
    "Validation gates are surface-scoped during iterative implementation and runtime",
    "frontend-only JavaScript/CSS/HTML changes require the focused frontend tests",
    "documentation-only changes require documentation validation and do not",
    "Do not block a targeted runtime installation or acceptance test on unrelated CI",
    "The complete repository-required CI graph is a gate for Ready-for-review, merge,",
    "Keep updates fast-forward-only.",
    "Do not mark a Draft pull request Ready",
    "Before ending any working response, verify that the requested end state has",
    "Never finish with a statement that the next step is",
]

FORBIDDEN_AGENT_RULES = [
    "A hard stop is allowed",
    "Stop only when a real decision or safety boundary",
]

REQUIRED_NEW_CHAT_HANDOFF_RULES = [
    "Root-level `AGENTS.md` is binding.",
    "Status reports are progress updates, not stopping points.",
    "unrelated queued/running CI does not block already-approved progress.",
    "A genuinely external dependency is a blocked wait state, not project completion",
    "re-read relevant repository/CI state before reporting it",
    "re-read that run before ending the working response",
    "Continue the authorized workstream to its requested end state.",
    "Never end with an executable next step still available through the current tools",
    "## Command presentation contract",
    "Every shell command intended for the user to copy or execute must be presented inside a normal fenced Markdown code block",
    "Never place executable commands in prose, inline-code fragments, writing blocks, generated UI controls or custom code-block formats",
    "the final answer must contain those commands in ordinary copyable Markdown code blocks",
    "## Binding daemon build and installation manifest",
    "The answer must use the heading `## Lokaler Bau, Test und Installation`",
    "exactly one ordinary fenced Markdown `bash` block without IDs, attributes or metadata",
    "For the established yaVDR checkout, the mandatory daemon flow has this shape:",
    "`git pull --ff-only origin <exact-branch>` is mandatory for the established checkout.",
    "Do not add package-manager commands, dependency bootstrapping, a second clone",
    "Do not add package-manager commands, dependency bootstrapping, a second clone, backups, rollback scripts, HTTP checks, browser checks or unrelated diagnostics unless explicitly requested or proven necessary by an observed failure.",
    "Keep the answer branch-/PR-specific and as short as the complete safe flow permits.",
    "## Binding branch- and PR-specific installed-result acceptance manifest",
    "## Prüfung des installierten Ergebnisses",
    "A successful build, file installation and `active (running)` service state prove only that deployment completed.",
    "Never describe an acceptance item as passed merely because the daemon started or automated CI is green.",
]

FORBIDDEN_NEW_CHAT_HANDOFF_RULES = [
    "unless a genuine unresolved safety/technical boundary requires new user input",
]

REQUIRED_CI_STATUS_HANDOFF_RULES = [
    "A non-terminal CI snapshot is never a stopping point.",
    "re-read that run before ending the working response",
    "Continue independent already-approved work while the run is queued or in progress.",
    "Do not return a stale queued/in-progress status as the final state",
]

REQUIRED_GUARDRAIL_RULES = [
    "Never replace a complete existing file through GitHub update_file from a",
    "truncated or partial fetch.",
    "For index files and status files, a small link addition must remain a",
    "small diff.",
    "If a GitHub fetch result is truncated, fetch the missing ranges before",
    "After every GitHub file update, inspect the commit diff",
    "Once a bounded workstream is authorized, never voluntarily end the working",
    "An intermediate finding is never an end state.",
    "Every working response that is permitted to end must end with ## Testblock.",
    "The Testblock must be exactly one ordinary fenced bash block and final content.",
    "If the candidate is not truthfully testable, continue working instead of ending.",
    "There is no generic repository permission to stop an authorized workstream.",
    "External dependencies are blocked wait states, not project completion.",
    "A fixable failed check is diagnosis/fix work, not a reason to stop.",
    "Existing authorization counts; do not ask again or stop at an already",
    "re-read that run before any response termination.",
    "Never finish with a stale queued/in-progress CI status",
    "Prefer GitHub-first execution when the connector can perform",
    "Continue through already-approved steps without artificial confirmation",
    "Do not wait for GitHub Actions after every commit. Keep approved",
    "documentation and frontend work moving while unrelated CI jobs run.",
    "Use surface-scoped checks for iterative implementation and runtime",
    "acceptance. Require the complete repository CI graph only at",
    "Ready-for-review, merge, phase closeout or another explicit",
    "Do not create a temporary pull request solely to wait for GitHub Actions",
    "Every executable command supplied to the user must remain inside an",
    "ordinary fenced Markdown code block, preferably tagged bash.",
    "A daemon build-and-install request must produce one concise ordinary bash",
    "block containing only the verified checkout, build, install and service",
    "Established-host instructions must use git pull --ff-only",
    "package-management commands unless the user requests them",
]


def normalize_whitespace(text: str) -> str:
    return " ".join(text.split())


def main() -> int:
    missing = []

    if not CURRENT_STATUS.exists():
        missing.append("docs/development/current-status.md is missing")
    if not AGENT_RULES.exists():
        missing.append("AGENTS.md is missing")
    if not NEW_CHAT_HANDOFF.exists():
        missing.append("docs/NEW-CHAT-HANDOFF.md is missing")
    if not CI_STATUS_HANDOFF.exists():
        missing.append("docs/development/github-actions-status-handoff.md is missing")

    current_status_text = (
        CURRENT_STATUS.read_text(encoding="utf-8")
        if CURRENT_STATUS.exists()
        else ""
    )
    agent_rules_text = (
        AGENT_RULES.read_text(encoding="utf-8")
        if AGENT_RULES.exists()
        else ""
    )
    new_chat_handoff_text = (
        NEW_CHAT_HANDOFF.read_text(encoding="utf-8")
        if NEW_CHAT_HANDOFF.exists()
        else ""
    )
    ci_status_handoff_text = (
        CI_STATUS_HANDOFF.read_text(encoding="utf-8")
        if CI_STATUS_HANDOFF.exists()
        else ""
    )
    own_text = Path(__file__).read_text(encoding="utf-8")

    current_status_text = normalize_whitespace(current_status_text)
    agent_rules_text = normalize_whitespace(agent_rules_text)
    new_chat_handoff_text = normalize_whitespace(new_chat_handoff_text)
    ci_status_handoff_text = normalize_whitespace(ci_status_handoff_text)
    own_text = normalize_whitespace(own_text)

    for item in REQUIRED_CURRENT_STATUS_RULES:
        if normalize_whitespace(item) not in current_status_text:
            missing.append("current-status.md missing rule: " + item)

    for item in REQUIRED_AGENT_RULES:
        if normalize_whitespace(item) not in agent_rules_text:
            missing.append("AGENTS.md missing rule: " + item)

    for item in FORBIDDEN_AGENT_RULES:
        if normalize_whitespace(item) in agent_rules_text:
            missing.append("AGENTS.md contains forbidden stop permission: " + item)

    for item in REQUIRED_NEW_CHAT_HANDOFF_RULES:
        if normalize_whitespace(item) not in new_chat_handoff_text:
            missing.append("NEW-CHAT-HANDOFF.md missing rule: " + item)

    for item in FORBIDDEN_NEW_CHAT_HANDOFF_RULES:
        if normalize_whitespace(item) in new_chat_handoff_text:
            missing.append("NEW-CHAT-HANDOFF.md contains forbidden stop exception: " + item)

    for item in REQUIRED_CI_STATUS_HANDOFF_RULES:
        if normalize_whitespace(item) not in ci_status_handoff_text:
            missing.append("github-actions-status-handoff.md missing rule: " + item)

    for item in REQUIRED_GUARDRAIL_RULES:
        if normalize_whitespace(item) not in own_text:
            missing.append("guardrail missing workflow rule: " + item)

    if missing:
        print("GitHub update safety handoff check failed:")
        for item in missing:
            print("- " + item)
        return 1

    print("GitHub update safety handoff check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
