# GitHub Actions Status Handoff

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)

---

## Purpose

This handoff records the preferred command-line tool for GitHub Actions status checks.

New chats should use this check when GitHub Actions status matters.

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

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
