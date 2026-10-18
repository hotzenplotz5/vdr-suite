# Documentation Standards

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Developer Onboarding](developer-onboarding.md)
- [Current State](../CURRENT.md)

## Purpose and authority

Use one primary source per kind of fact:

| Scope | Authority |
| --- | --- |
| Volatile implementation, acceptance, branch/CI state | [CURRENT](../CURRENT.md) (and fresh GitHub state for exact refs) |
| Numbered phase ordering and execution gates | [Strict roadmap](../planning/roadmap.md) |
| Stable current implementation ownership | [Current architecture](current-architecture-state.md) |
| Historical acceptance and phased plans | Their original closeouts, ADRs and timestamped snapshots |
| Actual test/build mechanics | [Makefile](../../Makefile), [mk/maintenance-tests.mk](../../mk/maintenance-tests.mk) and checked source |

Do not duplicate mutable phase statuses across indexes, overview pages and milestone histories. Short navigation summaries should link to the authority. Preserve historical snapshots as **historical**; do not retroactively rewrite completed evidence to match current wording.

## Structure and links

Current-facing entrypoints **should** have one `#` title and clear navigation/back links when useful. This is a human-authored convention, **not** a universal checker requirement: [`tools/check_docs.py`](../../tools/check_docs.py) presently requires a top-level Markdown heading and checks relative links for files under `docs/`, with a specific historical-path exception. It does not require literal `## Navigation` or `## Back` headings on every file.

[Directory index checks](../../tools/check_doc_indexes.py) require a local index/README for directories with non-index Markdown; active directories require local Markdown links and designated archives require complete local listings. [Reachability](../../tools/check_doc_reachability.py) traverses from the root README and requires the configured canonical and archive set to be reachable, **not every single Markdown document in the repository**.

New current-facing material should be discoverable from the appropriate existing index. Prefer updating an existing canonical guide over creating near-identical pages or a parallel current-status dashboard. Retain important redirects/entrypoints when deletion would break incoming links or a navigation/ownership boundary.

## Validation

[`make test-docs`](../../mk/maintenance-tests.mk) runs heading/link/index/reachability/entrypoint checks, the scoped Public-v1 OpenAPI guard and unit tests, ADR and phase marker guards. That checker does not formally validate the complete OpenAPI schema or execute runtime/yaVDR acceptance. GitHub Actions executes the repository docs-check on configured events; successful execution must be verified for the **exact head**.

Before deleting a Markdown file, examine incoming relative links, directory-index/reachability requirements, and any generator, ownership or CI references. Exact-file-content duplication alone does not justify deleting a historical accepted phase record.

## Script snippet safety

- Show shell commands in `bash` blocks and Python update scripts in `python` blocks.
- Python scripts that write Markdown fences should construct them as `FENCE = chr(96) * 3` rather than nesting literal fence lines.
- Never present an unverified runtime/deployment command as a safe documentation test.

## Back

- [Development Index](index.md)
- [Developer Onboarding](developer-onboarding.md)
- [Documentation Index](../index.md)
- [README](../../README.md)
