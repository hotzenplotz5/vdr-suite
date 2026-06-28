# GitHub Actions Status Handoff

## Purpose

This handoff records the preferred command-line tool for GitHub Actions status checks.

New chats must use this check before declaring pushed VDR-Suite work complete when GitHub Actions status matters.

---

## Tool name

The VDR-Suite GitHub Actions polling tool is:

```text
tools/watch_github_ci.py
```

Preferred command:

```bash
tools/watch_github_ci.py --watch --interval 60 --url --chat
```

Use this tool when waiting for GitHub Actions to become green or red.

---

## Completion rule

Do not mark a pushed implementation, guardrail or runtime-sensitive phase complete until the current HEAD shows green GitHub Actions status or the reason for missing GitHub Actions evidence is explicitly documented.
