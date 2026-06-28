# GitHub Actions Status Handoff

## Purpose

This handoff records the preferred command-line check for GitHub Actions status.

New chats must use this check before declaring pushed VDR-Suite work complete when GitHub Actions status matters.

---

## Check current HEAD with status icons

Run from the repository root:

```bash
cd /home/yavdr/vdr-suite

SHA=$(git rev-parse HEAD)

gh run list --workflow ci.yml --branch main --commit "$SHA" --limit 1 \
  --json status,conclusion,headSha,displayTitle,url \
  --jq '.[] | (if .status=="completed" and .conclusion=="success" then "✅ GRÜN" elif .status=="completed" then "❌ ROT" else "⏳ LÄUFT" end) + "  " + .headSha[0:7] + "  " + .displayTitle + "  " + .url'
```

Expected meanings:

```text
✅ GRÜN  -> GitHub Actions completed successfully.
❌ ROT   -> GitHub Actions completed with failure or non-success conclusion.
⏳ LÄUFT -> GitHub Actions is still queued or running.
```

---

## Check recent workflow runs

```bash
cd /home/yavdr/vdr-suite

gh run list --workflow ci.yml --branch main --limit 5 \
  --json status,conclusion,headSha,displayTitle,url \
  --jq '.[] | (if .status=="completed" and .conclusion=="success" then "✅ GRÜN" elif .status=="completed" then "❌ ROT" else "⏳ LÄUFT" end) + "  " + .headSha[0:7] + "  " + .displayTitle + "  " + .url'
```

---

## Completion rule

Do not mark a pushed implementation, guardrail or runtime-sensitive phase complete until the current HEAD shows green GitHub Actions status or the reason for missing GitHub Actions evidence is explicitly documented.
