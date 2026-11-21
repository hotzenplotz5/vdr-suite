# Phase 62 Slice 2K — Runtime Acceptance Harness

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Slice 2J](phase-62-slice-2j-searchtimer-create-security-migration.md)

---

## Status

Slice 2K is complete in repository, CI and on the real yaVDR runtime.

The accepted tooling commit is:

```text
dab9a4f7849258debf7a926ec549b34a67a7be92
```

GitHub Actions run `30700027075` completed successfully with all five jobs
green.

The committed harness then completed one real-runtime self-validation without a
daemon replacement, service restart, SearchTimer creation or persistent grant
change.

---

## Purpose

Slice 2K converts the repeated Phase 62 runtime-acceptance shell procedures into
one reusable, repository-owned and manifest-driven tool.

The tool is intentionally specialized for protected browser-session mutation
routes. It complements the existing general Real-VDR acceptance runner rather
than replacing it.

---

## Files

```text
tools/phase62-runtime-acceptance/runner.py
tools/phase62-runtime-acceptance/slice-2j.json
mk/phase62-runtime-acceptance.mk
```

The top-level Makefile includes the new module.

The offline harness target is part of the fast CI test group.

---

## Offline Contract

The following target performs no runtime HTTP request and no runtime database
change:

```text
make test-phase62-runtime-acceptance-harness
```

It verifies:

- Python syntax;
- manifest schema and route presence;
- safe-body and validation contracts;
- grant snapshot, isolation and exact restoration;
- positive accountability summarization;
- fail-closed accountability count mismatches;
- fail-closed accountability backend and action mismatches;
- backup checksum verification.

---

## Runtime Contract

The explicit runtime target is:

```text
make phase62-runtime-acceptance
```

It requires explicit values for:

- the verified backup directory;
- expected branch and commit;
- installed daemon SHA-256;
- installed deferred-loader SHA-256.

The runner then verifies:

- clean repository state;
- installed and running daemon fingerprints;
- loader fingerprint;
- service state and stable process ID;
- backup checksums;
- SQLite integrity;
- unauthenticated denial;
- browser-session CSRF;
- direct permission and backend scope;
- exact administrator scope;
- read-only precedence;
- wildcard-role non-effectiveness;
- query-string route normalization;
- trailing-slash fail-closed behavior;
- mutation-safe authorized validation;
- resource state before and after acceptance;
- accountability outcomes and secret exclusion;
- exact grant restoration;
- browser-session revocation and replay denial.

A JSON report can be written into the build directory.

---

## Safety Boundary

Runtime mode is never implicit.

The checked-in Slice 2J profile permits only the empty JSON object:

```json
{}
```

Its expected application response is the pre-executor validation failure:

```text
HTTP 200
success = false
message = searchtimer name is required
errors = ["name is required"]
```

A profile may not use a body capable of successful backend mutation without a
separately reviewed disposable and reversible mutation contract.

The runner does not install, restart or reconfigure the daemon.

---

## Real-Runtime Self-Validation

**VERIFIED on 2026-08-01 at commit
`dab9a4f7849258debf7a926ec549b34a67a7be92`:**

```text
manifestId                    = slice-2j-searchtimer-create
testsPassed                   = 29
httpRequests                  = 27
accountability.authorized     = 8
accountability.csrf           = 2
accountability.permission     = 2
accountability.read_only      = 2
accountability.scope          = 4
real_searchtimer_creates      = 0
```

The generated report was:

```text
.build/phase62-runtime-acceptance-slice2k-self-validation.json
```

Its SHA-256 was:

```text
a099bd16a1ac7e49f7608c1bd79d69b27eb0f3cea8222f93295650c25c174eb4
```

The self-validation additionally proved:

- resource state unchanged;
- all targeted grants restored exactly;
- unrelated grants untouched;
- browser session revoked;
- revoked-cookie replay denied;
- accountability free of Authorization, cookie and CSRF secrets;
- SQLite quick and foreign-key checks successful;
- service PID unchanged at `57877` during the pass;
- installed and running daemon unchanged;
- installed deferred loader unchanged;
- verified backup checksums unchanged;
- worktree clean after the pass.

The accepted installed fingerprints remained:

```text
daemon:
ccfc7c3c81300562da07b29a42b71e778439e805995abe7718dc702363a91a4c

loader:
a43f04673bb85a4dac21b2918744ae0bca554367c4942a125886c301e3ff51e7
```

The verified backup remained:

```text
/var/backups/vdr-suite-phase62-slice2j-20260801T105140Z-7a3c8a1a3e0e
```

---

## Completion

Slice 2K completed:

1. focused local review and offline tests;
2. commit and push;
3. successful CI;
4. one real-runtime self-validation of the committed harness;
5. verified cleanup and JSON report;
6. documentation closeout.

The harness is now the default acceptance mechanism for compatible future
Phase 62 protected-mutation slices. It should be extended through manifests and
small reusable capabilities instead of recreating large ad-hoc shell matrices.

PR #117 remains open, Draft and unmerged.
