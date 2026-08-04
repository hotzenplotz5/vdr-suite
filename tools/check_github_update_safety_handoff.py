#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CURRENT_STATUS = ROOT / "docs/development/current-status.md"
NEW_CHAT_HANDOFF = ROOT / "docs/NEW-CHAT-HANDOFF.md"
AGENT_RULES = ROOT / "AGENTS.md"

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
# - Prefer GitHub-first execution when the connector can perform the complete
#   bounded operation safely.
# - Continue through already-approved steps without artificial confirmation
#   pauses.
# - Create and push coherent commits consecutively with fast-forward-only
#   semantics.
# - Do not wait for GitHub Actions after every commit. Evaluate only the jobs
#   that can validate components changed by the current diff.
# - Do not create a temporary pull request solely to wait for GitHub Actions
#   unless the user explicitly requests that workflow.
# - End every final VDR-Suite repository response with current local build,
#   test and installation guidance tailored to the active branch and change.
# - Never invent or infer local operational context, including checkout paths.
# - Never use set -e, set -o errexit, bash -e or sh -e in user shell blocks.
# - Do not prescribe installation or service restart when installable content
#   did not change and the user did not explicitly request it.
# - The protected operational safety invariants are append-only and must not
#   be removed, shortened, weakened or overridden without explicit user order.

REQUIRED_CURRENT_STATUS_RULES = [
    "### Preferred edit path for new chats",
    "Prefer direct GitHub repository updates for existing files",
    "Use local edits first only when the change requires:",
    "a workaround because the GitHub connector blocks a file operation",
    "### Permanent operational safety invariants",
    "append-only and must not be removed, shortened, weakened or overridden",
    "Never invent, guess or silently infer local paths",
    "Never use `set -e`, `set -o errexit`, `bash -e` or `sh -e`",
    "Wait only for GitHub Actions jobs that can validate components changed by the current diff",
    "Do not prescribe installation or service restart when installable content did not change",
    "### Required final local command block",
    "Every final VDR-Suite repository response must end with a copyable",
]

REQUIRED_NEW_CHAT_HANDOFF_RULES = [
    "## Binding execution rules for every new chat",
    "## Permanent operational safety invariants",
    "These exact operational safety invariants are append-only",
    "must not be removed, shortened, weakened or overridden",
    "never invent, guess or silently infer local paths",
    "canonical checkout path must be explicitly verified before emitting `cd`",
    "Never use `set -e`, `set -o errexit`, `bash -e` or `sh -e`",
    "select GitHub Actions evidence by change impact",
    "do not wait for daemon build, packaging, frontend or other unrelated jobs",
    "Do not prescribe installation, daemon replacement or service restart",
    "## Required final local command block",
    "Every final VDR-Suite repository response must end with a ready-to-copy shell block",
    "Do not use `set -e`, `set -o errexit`, `bash -e` or `sh -e`",
    "Do not require `make daemon`, daemon installation or service restart",
    "Do not require waiting for every GitHub Actions job",
]

REQUIRED_AGENT_RULES = [
    "## GitHub-first execution",
    "Continue through all already-approved steps of a bounded workstream",
    "Push each completed",
    "Do not wait for GitHub Actions after every commit.",
    "Evaluate CI at the end of the bounded workstream",
    "Keep updates fast-forward-only.",
    "Do not mark a Draft pull request Ready",
]

REQUIRED_GUARDRAIL_RULES = [
    "Never replace a complete existing file through GitHub update_file from a",
    "truncated or partial fetch.",
    "For index files and status files, a small link addition must remain a",
    "small diff.",
    "If a GitHub fetch result is truncated, fetch the missing ranges before",
    "After every GitHub file update, inspect the commit diff",
    "Prefer GitHub-first execution when the connector can perform",
    "Continue through already-approved steps without artificial confirmation",
    "Do not wait for GitHub Actions after every commit.",
    "Evaluate only the jobs",
    "Do not create a temporary pull request solely to wait for GitHub Actions",
    "End every final VDR-Suite repository response with current local build,",
    "Never invent or infer local operational context",
    "Never use set -e, set -o errexit, bash -e or sh -e",
    "Do not prescribe installation or service restart when installable content",
    "protected operational safety invariants are append-only",
]


def main() -> int:
    missing = []

    if not CURRENT_STATUS.exists():
        missing.append("docs/development/current-status.md is missing")
    if not NEW_CHAT_HANDOFF.exists():
        missing.append("docs/NEW-CHAT-HANDOFF.md is missing")
    if not AGENT_RULES.exists():
        missing.append("AGENTS.md is missing")

    current_status_text = (
        CURRENT_STATUS.read_text(encoding="utf-8")
        if CURRENT_STATUS.exists()
        else ""
    )
    new_chat_handoff_text = (
        NEW_CHAT_HANDOFF.read_text(encoding="utf-8")
        if NEW_CHAT_HANDOFF.exists()
        else ""
    )
    agent_rules_text = (
        AGENT_RULES.read_text(encoding="utf-8")
        if AGENT_RULES.exists()
        else ""
    )
    own_text = Path(__file__).read_text(encoding="utf-8")

    for item in REQUIRED_CURRENT_STATUS_RULES:
        if item not in current_status_text:
            missing.append("current-status.md missing rule: " + item)

    for item in REQUIRED_NEW_CHAT_HANDOFF_RULES:
        if item not in new_chat_handoff_text:
            missing.append("NEW-CHAT-HANDOFF.md missing rule: " + item)

    for item in REQUIRED_AGENT_RULES:
        if item not in agent_rules_text:
            missing.append("AGENTS.md missing rule: " + item)

    for item in REQUIRED_GUARDRAIL_RULES:
        if item not in own_text:
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
