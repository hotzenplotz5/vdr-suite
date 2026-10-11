# Documentation Pruning Review — Evidence and Decisions

## Navigation

- [Documentation Guide](../documentation-guide.md)
- [Historical Documentation](../history/index.md)
- [Development Index](index.md)
- [Documentation Standards](documentation-standards.md)

## Scope and method — 2026-10-11

Baseline: [PR #453](https://github.com/hotzenplotz5/vdr-suite/pull/453)
merged as `23ea528d651eb8422cb5ae997fb25f2620465439`.
Its complete recursive Git tree has **3,782 entries and 739 Markdown
files**, zero empty Markdown blobs, and no pair with an identical blob SHA.
Blob-identity checks are **not semantic equivalence** tests. A source/checker
audit cannot establish whether external users have bookmarked an old URL.

Deletion screening used the complete path/tree inventory, selected file
contents, canonical index entries, actual documentation-check scripts
(`tools/check_docs.py`, `tools/check_doc_indexes.py` and
`tools/check_doc_reachability.py`), GitHub main-branch code search for
candidate filenames and source-of-truth overlap. Search yielded **zero indexed
matches** for the two removed filename stems; this is not guaranteed to
include every historical/external reference. Validate CI for the final commit
before calling deletions accepted.

## Decision A — remove two redundant unreferenced leaf pages

| File | Decision | Specific evidence and replacement |
| --- | --- | --- |
| `docs/development/contributor-guide.md` (608 bytes) | **DELETE in this cleanup** | Generic instructions: read onboarding, follow coding standards, run matching tests. No unique phase/implementation/acceptance data. Replaced by [CONTRIBUTING](../../CONTRIBUTING.md), [AGENTS](../../AGENTS.md), [Developer Onboarding](developer-onboarding.md) and [Documentation Standards](documentation-standards.md). No references found in canonical docs/development indexes or indexed code search. |
| `docs/development/release-process.md` (661 bytes) | **DELETE in this cleanup** | Generic 'run relevant tests, review history, tag when appropriate' checklist; no release-build, package, versioning or rollout contract. Superseded by [CONTRIBUTING](../../CONTRIBUTING.md), [AGENTS](../../AGENTS.md), the [Make infrastructure](../../mk/README.md) and [Packaging Status](../../packaging/README.md). No references found in canonical indexes or indexed code search. |

Deleted content remains recoverable from the parent Git commit. Neither
deletion authorizes a release, running tests on yaVDR, changing Git tags or
editing the installed daemon.

## Decision B — do not delete historical material based on brevity

| Scope | Decision | Reason |
| --- | --- | --- |
| `docs/roadmap/` (64 Markdown files, Phase 11–18 sketches) | **RETAIN as historical archive** | The sketches are old, but describe discrete original plans; phase `index.md` files provide navigation and satisfy directory index expectations. A coordinated rewrite/migration needs stronger provenance and backlink proof. |
| `docs/architecture/adr/` (old OSD/federation strategy index and 2 documents) | **RETAIN with supersession links** | Original decisions have historical value distinct from later [ADR-0047](../adr/ADR-0047-legacy-osd-compatibility-bridge.md) and [ADR-0060](../adr/ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md). |
| `docs/development/completed-phases/` and `docs/architecture/history/` | **PROTECTED / RETAIN** | Historical evidence and strict-archive reachability/index requirements; removing pages could invalidate the traceable accepted record. |
| `docs/development/status/overview.md` | **RETAIN as timestamped observation** | Contains unique early RESTfulAPI endpoint, version, count and host-scope proof not reproduced in the current status file. |
| `docs/phase64-suitebridge-native-timer-modify-command-service.md` | **RETAIN pending separate audit** | Small but has distinctive private `NTMOD` fencing/readback contract. Need source and downstream pointer check before deciding it is expendable. |
| `docs/project-overview.de.md` and `docs/project-overview.en.md` | **RETAIN** | Deliberate language variants rather than duplicated mutable current-status authorities. |

## Follow-up deletion investigation — not yet approved

1. **Former roadmap phase sub-indexes:** before any flattening, establish
   complete incoming Markdown links, whether index navigation can preserve all
   64 historical leaves, and CI/checker guarantees. Deleting just the indexes
   while keeping directory leaves may violate archive discoverability.
2. **Old post-phase/current-status snapshots:** retain distinct acceptance data;
   compare with their closeouts and test artifacts before deciding on a move or
   archival index; don't replace specific run evidence with a current summary.
3. **Orphan or very small implementation notes:** find every incoming link and
   concrete C++/Make/CI reference, read full contents and verify uniqueness.
   A low byte count is not deletion proof.
4. **Historical phase outlines not accepted as implemented:** preserve them as
   dated proposals or selectively consolidate only after one-for-one semantic
   replacement and reference updates.

### Per-file deletion gate

Before the next removal, write down: exact old path, canonical replacement,
unique preserved facts, internal and code/test references, required reachability,
archive/index impact, link-check outcome, PR CI on exact head, and rollback path.
**Do not** remove files automatically because they contain 'Phase 12',
'planned', 'future', or resemble another title. Historic closeouts, ADRs and
real-device acceptance have no blanket deletion authorization.

## Acceptance limits

This review provides **two concrete reviewed removals** and an explicit
keep/defer list. It is not a sentence-by-sentence semantic proof for all
739 Markdown files. Future PRs should remain narrowly scoped and source-backed.
No live yaVDR, service, credentials, VDR recordings or database was touched.

## Back

- [Documentation Guide](../documentation-guide.md)
- [Development Index](index.md)
- [Documentation Index](../index.md)
