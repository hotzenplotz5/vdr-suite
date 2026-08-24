# GitHub Actions Status Handoff

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)

---

## Purpose

This handoff records the preferred command-line tool and mandatory continuation
behaviour for GitHub Actions status checks.

New chats should use this check when GitHub Actions status matters.

For general project orientation, start with [New Chat Handoff](../NEW-CHAT-HANDOFF.md).

A non-terminal CI snapshot is never a stopping point. If the result of a run is
required for the next already-authorized gate, re-read that run before ending
the working response. Continue independent already-approved work while the run
is queued or in progress. Do not return a stale queued/in-progress status as the
final state when the run can be re-read and its result matters to the next
authorized operation.

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

Use this tool when the terminal GitHub Actions result is required for a gate.
While polling, keep all independent already-approved work moving. A CI watcher
is a validation mechanism, not a handoff mechanism and not a reason to finish a
working response early.

---

## Back

- [Back to New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
