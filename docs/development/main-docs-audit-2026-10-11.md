# Main documentation reconciliation — 2026-10-11

## Navigation

- [README](../../README.md)
- [Current State](../CURRENT.md)
- [Development Index](index.md)
- [Documentation Standards](documentation-standards.md)

## Source baseline and scope

- Repository: `hotzenplotz5/vdr-suite`.
- Main source checkpoint: `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11).
- Complete recursive Git tree was enumerated (3,766 entries); 736 files use the `.md`, `.rst` or `.adoc` suffix. Other source, config and binary assets are not counted as prose documentation.
- Documentation changes are confined to `docs/main-documentation-audit-2026-10-11` and must never modify production processes, databases or recording directories.
- The tree-wide filename inventory is **not** a claim that the body of every historical phase note has been reread. Historical ADRs, phase plans and accepted closeouts are preserved as timestamped evidence unless proven factually incorrect for their original checkpoint.

## Corrected source-of-truth drift

1. Root changelog had stopped at a 2026-07-27 Phase-61 documentation reconciliation. It now separates the July history from post-Phase-69 merged source developments.
2. Root README, ROADMAP, development/current-status and the new-chat handoff included stale or contradictory MU.9F current-slice statements despite MU.10B acceptance in `CURRENT.md`. They are aligned to `CURRENT.md` as the volatile status authority.
3. Public Device Recording playback and index/track/SRT documents described code as an unfinished isolated candidate. The R2 source is on `main`; exact installed yaVDR daemon and Hisense hardware acceptance remain **unverified by this GitHub-only audit**.
4. The legacy `docs/planning/project-progress.md` template still advertised Phase 58 as currently active and unimplemented identity/metadata functionality that is now accepted. Its snapshot has been reconciled to current phase foundations while preserving the generator delimiters. The implementation dependency map's present-tense Phase-65.D statements were recast as accepted history.
5. Planning index, architecture gap matrix and project overviews (English/German) also contained current-tense Phase-69 or MU.9F statements and were aligned with the authoritative closeouts. The old `docs/development/status/overview.md` now explicitly identifies itself as a historical early-daemon snapshot.
6. The historical Phase-69.F original eight accepted method/resource contracts remain frozen. Account extensions and the separately gated Recording candidate must not be counted as retroactive Phase-69.F acceptance (current matrix: nine accepted plus one R2 candidate).
7. VDR 2.7.9 index generation is potentially destructive to an existing index; the implemented guard refuses existing `index`, `index.vdr` or symlink paths. Nothing in this documentation change triggers index operations.

## Public API documentation bootstrap

- The previously empty `api/README.md` and `api/rest/README.md` now give developers a source-linked API entry point and REST implementation boundary.
- A **partial** OpenAPI 3.1 JSON contract began with verified `GET /api/v1`, `GET /api/v1/capabilities`, actor-filtered `GET /api/v1/backends` and federated `GET /api/v1/channels`. It now additionally covers the separately source-reviewed Timer, Operation, Account and Device HTTP contracts listed below; other implemented routes are not silently promoted to stable public-v1 contracts.
- The dedicated [Backend discovery developer contract](../../api/backend-discovery.md) links implemented route/security/test evidence and documents cursor expiry, filtering and errors.
- The [Channel collection developer contract](../../api/channel-collection.md) documents explicit source selection, per-backend permissions and partial-federation semantics.
- The [TimerAssignment guide](../../api/timer-assignments.md) documents actor/backend scoped list and item reads, opaque ETags, conditional 304 and safe `If-Match` + `Idempotency-Key` Timer CREATE admission. The [Operation guide](../../api/operation-status.md) documents actor-owned durable states and safe reconciliation without automatic retries. OpenAPI now covers the original eight accepted Phase-69.F method/resource contracts across seven paths; this is not a claim of complete modern Public-v1 coverage.
- The [Account/Grant guide](../../api/accounts-and-grants.md) and [Device pairing guide](../../api/device-pairing.md) now specify selected read/bootstrap flows, distinct administrator/Device authority, one-time credential issuance and explicit omitted administration operations. OpenAPI coverage remains partial: 36 method/resource contracts across 25 paths as of this docs-only checkpoint. The [Account Credential/Session administration contract](../../api/account-security-administration.md) now documents unpaginated secret-free collections, independently ETagged items and canonical desired-state revocation. [Account mutations](../../api/account-mutations.md) and [Device lifecycle](../../api/device-lifecycle.md) now cover protected writes and one-time secret failure/recovery boundaries. The [Device administrator guide](../../api/device-administration.md) now covers Pairing approval and Device grant desired-state management.
- The [Recording/Genre/EPG developer guide](../../api/recording-browse-epg.md) distinguishes Recording list keyset pagination from folder-mode offset pagination, 30-item genre offsets and strict two-event `epg.view`-gated now/next cached reads. Device media playback remains a separate unmodeled OpenAPI surface.
- The [dependency-free scoped Public-v1 OpenAPI guard](../../tools/check_public_v1_openapi.py) and [negative-case unit tests](../../tools/tests/test_public_v1_openapi.py) are now wired into `make test-docs` (existing CI docs-check job). They verify all 36 documented operations against their dedicated C++ GET/POST branches, local schema refs, path parameters, unique operation IDs and required precondition/authentication metadata. This does not amount to full route discovery, formal OpenAPI Schema validation or a successful CI run; no deployed runtime acceptance is claimed.

## Empty documentation placeholders: deletion audit

The initial branch inventory found **22 zero-byte Markdown files**. In this documentation slice **nine** were replaced by source-linked entrypoints, leaving **13** untouched empty placeholders. These counts concern the audited branch snapshot; future additions/removals need a new tree check. This is a file-size finding, **not** evidence that the remaining files can safely be deleted. They are divided by the actual file contents currently present beneath each directory:

| Disposition | Exact empty paths | Source-tree evidence and action |
| --- | --- | --- |
| **Fill rather than delete** | `core/README.md`, `tools/README.md`, `packaging/README.md`, `packaging/debian/README.md`, `database/README.md`, `database/schema/README.md` | Core and tools contain extensive implementations; packaging contains nginx/systemd assets, and database/schema contains SQL definitions. These entry points should eventually state their real owners, tested build/install boundaries, and links to the authoritative guides. The `packaging/debian` leaf currently has no package implementation beyond the README: describe actual package readiness, never claim a working Debian package. |
| **Keep explicitly marked as future-only until addon authority is resolved** | `modules/README.md`, `modules/image/README.md`, `modules/music/README.md`, `modules/rectools/README.md`, `modules/tvscraper/README.md` | All five paths currently contain only empty README stubs, not modules. Do not advertise installability or confuse them with the separately developed add-on foundation branch. Prefer an honest single architecture/onboarding entry before adding concrete package instructions. Do **not** merge the add-on branch as part of the documentation audit. |
| **Review for removal or historical relocation after incoming-link audit** | `.github/PULL_REQUEST_TEMPLATE.md`, `legacy/README.md`, `legacy/adapters/README.md`, `ui/README.md`, `ui/osd/README.md`, `ui/web/README.md`, `database/migrations/README.md`, `services/jobs/README.md` | These are currently empty leaf/structural stubs. The empty PR template provides no reviewer guidance, `legacy` and `ui` contain no implementation beneath these paths, and the listed migrations/jobs leaf has no content besides the README. Deletion must still check links, generator/index ownership, and whether a future reserved path is intentional. |
| **Fill short indexes that point to existing service contracts** | `services/README.md`, `services/metadata/README.md`, `services/recordings/README.md` | `services/metadata/metadata-service.md` and several `services/recordings/*.md` contracts already exist; a small truthful navigational README is more useful than removing their directory entry point. |

**Deletion count in this checkpoint: zero.** No path is removed without checking references and its purpose against current `main`. The next documentation-cleanup slice should first replace the useful empty entry points with concise, source-linked guides, then verify any candidate deletion with the existing docs-link/reachability checks. The historical 736-file content-level review remains open; this list is a targeted inventory of empty files only.

## Repository-area README repair (2026-10-11)

- Replaced nine zero-byte Markdown entrypoints with bounded, source-linked guides: [Core](../../core/README.md), [Tools](../../tools/README.md), [Database](../../database/README.md), [Database schemas](../../database/schema/README.md), [Packaging](../../packaging/README.md), [Debian status](../../packaging/debian/README.md), [Service sketches](../../services/README.md), [Metadata sketches](../../services/metadata/README.md), and [Recording sketches](../../services/recordings/README.md).
- The new entrypoints distinguish actual C++/SQL/Makefile assets from early Phase-1 interface sketches and planned Debian packaging. No empty file is removed and no existing schema, executable, CI workflow, or systemd unit is changed.
- The disposition table above records the *initial* zero-byte inventory and its intended handling. It is preserved as audit evidence, not a claim that those nine are still empty. Remaining 13 files still require reference/owner checks before deletion or replacement.
- This checkpoint does not retroactively rewrite historical planning pages or infer live yaVDR behavior from source presence.

## Phase-1 direct-entrypoint clarification

- Added **non-destructive historical-context banners** to four early [`services/`](../../services/README.md) implementation sketches (MetadataService, RecordingService, RecordingRepository, ActionService). Their original design content remains intact; present-tense implementation claims must instead cite current Core and runtime contracts.
- Marked [`docs/database-design.md`](../database-design.md) explicitly as the legacy Phase-1 schema overview and linked the live source-schema indexes and additive metadata architecture.
- Linked the newly repaired repository-area entrypoints from [Developer Onboarding](developer-onboarding.md). No historical content was silently deleted and no production code was edited.

## Verification and limits

- Verified: relative Markdown link targets of the first 18 distinct edited documents (587 references; no missing target paths), exact GitHub `main` merge SHA, recursive file inventory, Device route implementations in `core/daemon/src/RecordingMediaHttpRuntime.cpp`, authorization classification in `core/security/include/SecurityHttpGate.h`, and matrix resource entries.
- Not verified here: a local `make test-docs` run (this environment has no checked-out repository), the installed daemon binary, host permissions, runtime HTTPS/Device route behavior or real VIDAA audio/subtitle acceptance.
- To finish a **full content-level audit of all 736 prose files**, each historical/archive document must be traversed for link resolution, stale current-tense language and contradictions against its time-specific baseline. This checkpoint covers the current-facing authority chain and the newly merged R2 documents; it does **not** assert that archive-wide content validation has already passed.
- Documentation validation must be performed on the final branch head before merge. No PR is approved or merged by this documentation commit.

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
- [README](../../README.md)

## Remaining placeholder cleanup (2026-10-11)

- Verified the complete recursive branch tree at `76588914d60d488baa58e17bf7b614b8e3afc4c0` (3,781 entries, not truncated): exactly **13 zero-byte Markdown placeholders** remained.
- Replaced all 13 with concise, explicit scope/status documents: five under `modules/`, two under `legacy/`, three under `ui/`, one under `database/migrations/`, one under `services/jobs/`, plus `.github/PULL_REQUEST_TEMPLATE.md`. **No files were deleted.**
- Tree inspection showed these directories contained no implementation beyond their placeholder Markdown files. In particular, the four named module leaves are not implemented/installable add-ons in this branch, and add-on platform work elsewhere was not imported.
- Verified relative targets in the new documents against the original recursive tree. Retention rather than deletion avoids breaking potential incoming links and reserved organizational boundaries; exhaustive incoming-reference and generator scans remain an open validation item.
- No Makefile, source, workflow, production filesystem, database, deployment or service was changed. GitHub-only static review is not a successful local `make test-docs`, formal OpenAPI validation, or hosted CI result.

## Developer/build-guide source reconciliation (2026-10-11)

- Compared `docs/development/developer-onboarding.md` and `build-system-state.md` with `tools/check_docs.py`, the index/reachability checkers, `mk/maintenance-tests.mk`, `mk/common.mk`, `mk/local-test-groups.mk` and `.github/workflows/ci.yml`.
- Corrected the obsolete blanket Navigation/Back heading mandate, documented the **actual** current `test-docs` recipe (including scoped OpenAPI guard and unit tests), and separated source build/test from deployed-runtime acceptance.
- Clarified that the multi-backend topology illustration is not an installation claim; the current Agent, registry and authorization boundaries are source-backed.
- No Makefile, CI, security or runtime changes were made; tests remain unexecuted in this GitHub-only review.

## PR #452 CI compatibility correction

- The initial PR check run `38166892812` exposed a stale dashboard label versus the accepted Phase-69 closeout guards (`tools/check_phase69e_closeout.py` and `tools/check_phase69_closeout.py`); this is a **documentation classification drift**, not a reason to alter historical guard evidence.
- Restored the guarded `Stable public API/SDK | Completed numbered domain` row and separately documented additive Account/Device Recording R2 contracts and their unverified client/hardware acceptance.
- Initial PR architecture/docs jobs failed; passing final CI must be verified independently after this fix. No CI success is inferred from this edit.

## Post-merge active authentication authority correction (2026-10-11)

- After PR #452 merged at `65a2c2a96c1a632e2e0db6cca01584ab4e620432`, the existing documentation branch was fast-forwarded to that exact merged main checkpoint (no new branch).
- Reconciled a stale current-facing Legacy Basic compatibility sentence in [Current Architecture State](current-architecture-state.md) against the accepted [P2 retirement closeout](post-phase69-p2-legacy-basic-retirement-closeout.md) and `core/security/include/SecurityHttpGate.h`.
- The current authority is Human Account browser sessions and optional explicitly configured Managed Basic; the retired Legacy Basic runtime mode must not be described as transitional supported authentication. No phase-history document was rewritten, and no code or runtime behavior was changed.

## Reserved-directory navigation (2026-10-11)

- Linked the previously repaired `modules`, `legacy`, `ui`, `database/migrations`, and `services/jobs` reserved entrypoints from [Documentation Index](../index.md) and [Developer Onboarding](developer-onboarding.md).
- Kept a clear divide between these placeholders and executable source owners; no add-on installability or packaged service status is claimed.
- Changes are navigation and status text only. Relative targets were checked against the recursive Git tree; no local test or GitHub CI result is claimed for this new commit.

## Device Recording API developer-navigation slice (2026-10-11)

- Added [Device Recording playback and media tracks](../../api/device-recording-playback.md) as a human-readable source-linked developer map for Device create/status/stop, track inventory and subtitle selection, matching `RecordingMediaHttpRuntime.cpp`, `SecurityHttpGate.h`, and the existing R2 source/historical notes.
- Linked the guide from `api/README.md`. Kept the machine-readable OpenAPI inventory at **36 methods / 25 paths**; routes are **not** promoted to accepted Phase-69.F scope or falsely marked as hardware-tested. Formal schemas and full route/denial testing remain open.
- No C++/CI/JSON API spec/runtime changes; static link check only until actual branch CI is run.
