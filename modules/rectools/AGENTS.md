# Agent rules: Media Tools / Rectools

Inherit all requirements from [root AGENTS.md](../../AGENTS.md). These stricter rules apply to every change under `modules/rectools/` and to shared helpers changed specifically for this module.

1. Owner boundaries: Suite owns authentication, grants, asynchronous job identity/lifecycle, API and VDR recording reconciliation. Rectools is a restricted external worker, not a second control plane.
2. **Permitted only:** import, read-only check, and separately gated optional shrink. Exclude repair, smart_repair, PES-to-TS, cut, marks, rename and move, including indirect calls from the import path.
3. Before adding execution, audit exact Rectools version and sources. Its legacy `import` can delete source files and invoke smart_repair; `check_single` writes markers/log/mail; `shrink_single` rewrites recordings. Never enable those legacy paths unchanged.
4. No direct shell interpolation or arbitrary actor-supplied paths. Use allowlisted source roots resolved on the executing backend; verify symlink and traversal confinement at use time. Avoid the legacy world-writable `/tmp/vdr-rectools-jobs` worker.
5. Prefer stream-copy/remux where supported. Any encoding needs preflight capacity including source, staging and destination, recording concurrency protection, bounded CPU/GPU use and restart-safe verified outcomes. For shrink, preserve originals unless the user explicitly approves replacement after verification.
6. Import success requires a valid VDR recording and authoritative recording-cache reconciliation; an exit code alone is insufficient. Report duplicates, partial results and uncertain dispatch as distinct outcomes.
7. Package metadata must remain disabled/scaffold until focused integration/security/runtime tests demonstrate actual capability. Keep README, manifest, AGENTS and packaging tests synchronized.
8. Never install, restart, modify live VDR media, or run destructive acceptance automatically. Real TV/recording effects require explicit authorization and read-only host preflight.
