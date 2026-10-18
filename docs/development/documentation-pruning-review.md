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
| `docs/development/contributor-guide.md` (608 bytes) | **DELETE in this cleanup** | Generic instructions: read onboarding, follow coding standards, run matching tests. No unique phase/implementation/acceptance data. Replaced by the updated [CONTRIBUTING](../../CONTRIBUTING.md), [AGENTS](../../AGENTS.md), [Developer Onboarding](developer-onboarding.md) and [Documentation Standards](documentation-standards.md). No references found in canonical docs/development indexes or indexed code search. |
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

## Follow-up content-level decisions — round 3 (2026-10-11)

Compared with verified `main` commit
`03a13193d8a57a47e877ef598ddd5617835c8550`.
The following are **individual retention decisions**, not a declaration that
all development or roadmap prose was semantically audited. No files are
deleted in this round. Index/Markdown-source inspection and GitHub indexed
code search do not prove the absence of external URLs or every unindexed
reference.

| Exact candidate | Distinct content and authoritative successor | Reference and deletion impact | Decision |
| --- | --- | --- | --- |
| [`docs/phase64-suitebridge-native-timer-modify-command-service.md`](../phase64-suitebridge-native-timer-modify-command-service.md) | Unique short description of private `NTMOD` UPDATE/TOGGLE, operation/provider fencing, persisted Agent start time, Timer write lock, `accepted_unverified` and authoritative readback. The later [Phase-64 closeout](phase-64-closeout.md) owns real-yaVDR acceptance and the continuing private SVDRP-help boundary; [CURRENT](../CURRENT.md) owns live status. | GitHub indexed search for the filename stem found a corresponding Make test target in `mk/phase64-suitebridge-native-timer-modify-command-service-tests.mk`, **not proof of a Markdown backlink**. Separate source/tests contain `NTMOD` (`vdr-plugin-suite-bridge/suitebridge_native_timer_modify.cpp` and `tools/check_phase64_suitebridge_native_timer_modify_command_service.py`). The [history hub](../history/index.md) now links the retained page. Full external-link coverage is unknowable. | **RETAIN and label as historical**, preserving its original pre-acceptance wording beneath a dated context notice. Do not collapse its technical constraints into a generic closeout summary. |
| [`docs/roadmap/phase-11/index.md`](../roadmap/phase-11/index.md) | Its unique role is the Phase-11 archive directory entry linking the distinct [runtime-hardening outline](../roadmap/phase-11/overview.md), whose five goals and expected outcome document historical intent, not implementation acceptance. [Strict Roadmap](../planning/roadmap.md) owns future sequencing. | [Archive parent](../roadmap/README.md) links `phase-11/index.md`, and the index itself links `overview.md`. Removing the short index creates a broken inbound link and loses local navigation; the current index validator distinguishes designated strict archives from ordinary directories, so passing a checker alone would not prove a safe removal. Other code/CI/external refs are not exhaustively excluded. | **RETAIN**, not a duplicate phase plan. No flattening without comprehensive inbound/reference and archive-discoverability checks. |
| [`docs/roadmap/README.md`](../roadmap/README.md) | Navigation to Phase-11–18 sketches and their sub-indexes; no authority over current phase gates. | Linked by the [history hub](../history/index.md) and [documentation guide](../documentation-guide.md); removal breaks the archival entrypoint. | **RETAIN**; correct the misleading alternate-current-roadmap pointers in place. |
| [`docs/development/index.md`](index.md) and [`docs/planning/index.md`](../planning/index.md) | Distinct development-contract and planning navigation, unlike the volatile MU PR/CI status register. [CURRENT](../CURRENT.md) and original MU closeouts own current state and historical evidence respectively. | Both are canonical discoverability entrypoints and are referenced from root/current navigation; preserved all original contract/closeout links, updated only duplicate status prose. Historical static guards may inspect index anchors. | **RETAIN**; remove stale MU.9 candidate language and parallel MU.6–10 status ledger rather than delete either index. |

The separate [early VDR status snapshot](status/overview.md) remains
**RETAINED** under Decision B: historical daemon URL, RESTfulAPI version,
endpoint and observed counts are unique time-bound evidence, not current
deployment instructions. No accepted ADR, slice/phase closeout, security
test or real-yaVDR proof was edited, relocated or removed.

### Historical roadmap — complete bounded content pass

Read all **64 Markdown bodies** listed by the `docs/roadmap/` directory
inventory at the pre-banner round-3 checkpoint
`13b7ea87a4a9fc7279338d50c804009049457fe0`:
root README (1), contribution sketches (5), final examples (5),
Phase 11–14 (8), Phase 15 (6), Phase 16 (3), Phase 17 (2),
Phase 18 (32) and the miscellaneous phases group (2).
This is a content pass of this **one archive**, not of all repository docs.

- The many short Phase-18 documents have recurring navigation/Goals/Expected
  Result scaffolding, but address **different original concerns**:
  identity, discovery, versioning, safety, permissions, trust, backend-specific
  channel/recording/Timer views, offline behavior, conflict resolution and
  remote jobs. Repetitive formatting is not evidence of interchangeable
  technical meaning. All 32 remain discoverable from
  [the Phase-18 index](../roadmap/phase-18/index.md); that index now explains
  the difference from accepted ADR-0060/0061 and implemented Agent ownership.
- The [Phase-17 outline](../roadmap/phase-17/overview.md) actually describes
  a **native OSD frontend**, not mobile frontends. Corrected only its label
  in the archive root; the original outline remains untouched.
- [Contribution-guide sketches](../roadmap/contribution-guides/index.md) still
  uniquely show early workflow expectations, including historical tag/Make
  suggestions. Their section index now points to the actual current
  contribution rules and Make recipes; deleting the leaves would remove
  provenance and break their sub-index.
- [Reference-client examples](../roadmap/final-example/README.md) preserve
  early Windows OSD, multi-client and VDR Live reimplementation aspirations,
  including historical `/api/...` route examples. Their index now warns
  that these do not establish a current installed client or Public-v1 API
  inventory; the present [API entrypoint](../../api/README.md) owns coverage.
- The remaining phase directories have distinct overview goals and local
  indexes. Their shortness does not justify removing paths linked by the
  [archive root](../roadmap/README.md). A future consolidation would need
  replacement of every individual topic and an exhaustive inbound-link scan.

**Disposition for the entire 64-file historical roadmap group: RETAIN.**
No semantic line was rewritten in a historical leaf, and no phase or
security acceptance is inferred from roadmap proposals. Cross-repository
external links and references outside the reviewed GitHub inventory remain
unverified; therefore this review does **not** approve a deletion.

### Verification boundary for this round

- Reviewed `AGENTS.md`, the current/history/pruning guides, the main-docs
  audit, index and reachability scripts, and the actual
  `mk/maintenance-tests.mk` `test-docs` recipe.
- The changes affect navigation, historical context and this decision
  register only. They do not change C++, tests, Make, CI, DB, packages,
  running servers, recordings or clients.
- Final-head PR checks remain a **separate requirement**; GitHub search
  results can reflect an older indexed revision. No local test run or live
  yaVDR validation is inferred from these edits.

## Phase-0 empty-outline cleanup — 2026-10-12

At the GitHub `main` source checkpoint
`e616894899e2fb891e5b0e85ddd33cd1b750e67a`, examined the
[Phase-0 index](../phase-0/index.md) and two historical leaves:

| Historic path | Verified original content | Disposition and safe successor |
| --- | --- | --- |
| [Phase-0 Data Model](../phase-0/03-data-model.md) | Title, generic navigation, two separators and back links, **no schema or design body** | **RETAIN original URL**; label as an unfinished original outline, link the current [database schema index](../../database/schema/README.md) and [implemented architecture](current-architecture-state.md); do not fabricate Phase-0 acceptance |
| [Phase-0 UI Concept](../phase-0/06-ui-concept.md) | Title, generic navigation, two separators and back links, **no UI design body** | **RETAIN original URL**; label historical placeholder and link [ADR-0058](../adr/ADR-0058-media-home-responsive-browse-preview.md), [frontend architecture](frontend-architecture.md) and [project overview](../project-overview.md) |

**Why no deletion:** the [Phase-0 index](../phase-0/index.md)
links both filenames; external bookmarks cannot be exhaustively inventoried,
and deleting a path would turn a thin but existing historical archive
entry into a broken link. Replacing the empty separators with a truthful
notice and successor links preserves discoverability, supplies useful
navigation and retains original provenance in Git history. No distinct
technical facts have been removed; no historical design body is rewritten
because neither leaf contained one. No archive directory index is
flattened, no ADR, accepted closeout or original Phase-11–18 roadmap
content is touched.

This is a **two-page, evidence-based non-destructive cleanup**, not approval
to delete other small or historically stale pages. The separate
[deep historical audit](../history/index.md) inventories broader
archive-link limitations; this narrow change does not claim to repair all
relocated snapshot hyperlinks. GitHub PR CI on the exact branch head remains
a necessary independent gate.

## Acceptance limits

This review provides **two concrete reviewed removals** and an explicit
keep/defer list. It is not a sentence-by-sentence semantic proof for all
739 Markdown files. Future PRs should remain narrowly scoped and source-backed.
No live yaVDR, service, credentials, VDR recordings or database was touched.

## Back

- [Documentation Guide](../documentation-guide.md)
- [Development Index](index.md)
- [Documentation Index](../index.md)
