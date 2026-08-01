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

The harness foundation is implemented locally.

Offline manifest validation, self-tests, Make inventory and documentation checks
must pass before commit.

The real-runtime harness run remains deferred until the tooling commit is
reviewed, committed, pushed and green in CI.

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

## Completion Boundary

Slice 2K is complete only after:

1. focused local review and offline tests;
2. commit and push;
3. successful CI;
4. one real-runtime self-validation of the committed harness;
5. verified cleanup and JSON report;
6. documentation closeout.

PR #117 remains open, Draft and unmerged.
