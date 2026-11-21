# Public GitHub presentation and licensing — remaining owner decisions

## Scope and verified facts (2026-10-12)

This is a **release/publication checklist**, not legal clearance or evidence of installation. It identifies which parts of the public presentation can be improved in source and which require maintainer action or real assets. PR #464 was merged to `main` as commit `d11732a17fea8698538526ddcc785e3676fd6771`, which includes the complete GPLv3 text in `LICENSE` and the scoped license inventory. Real Web/TV screenshots have not been supplied; the README now offers **clearly labeled text-only capture slots**, not screenshots. GitHub About settings must be checked live before their status is marked complete.

## 1. License — merged; distribution-specific checks remain

A [source-backed GPLv3 compatibility matrix](gplv3-compatibility-audit.md) and a [third-party licenses/attribution inventory](third-party-license-inventory.md) record identified boundaries, upstream evidence and unresolved legal questions. Direct upstream VDR source confirms GPLv2-or-later, making the core VDR/SuiteBridge GPLv3 option available, while non-VDR third-party provenance still requires verification. The maintainer has explicitly selected **`GPL-3.0-or-later`** as the target for original VDR-Suite source they are authorized to license. The matrix remains conditionally compatible; it does not itself clear upstream rights, other contributors or artwork. No project-wide legal clearance is claimed.

The merged full `LICENSE` text and maintainer-selected `GPL-3.0-or-later` cover original code **only to the extent it can legally be licensed by its rights holders**. They do not silently relicense third-party programs or artwork. Provenance and final scope still require review.

Before publishing the selected license:

1. Identify ownership and contributor permissions for all original C++/JavaScript source; review external code, vendored assets, logos, fonts and build files separately.
2. Audit integration, linking and distribution relationships with VDR and upstream plugins/libraries; do **not** assume the licensing obligation of every source dependency automatically applies to the entire server, or that a permissive license is safe without provenance.
3. Confirm that every relevant repository copyright holder authorizes the selected `GPL-3.0-or-later` identifier and any contribution terms. If multiple licenses apply to independent components, document exact component boundaries.
4. The complete unmodified GPLv3 text is present on `main` as of the verified merge. Before a general release, finish provenance/third-party notices as needed and accurate source-level license statements. Validate GitHub's detected license afterwards.

The selected identifier is **`GPL-3.0-or-later`**; its application to all affected original files remains conditional on rights clearance. **The merged source includes the GPLv3 license text but does not relicense third-party materials or certify every future distributed binary package.**

## 2. Screenshot capture slots — ready for contributor images

[The screenshot capture checklist](screenshots.md) specifies a privacy-reviewed Web Home, Recording browser, optional playback screenshot and real VIDAA image. Source assets and TV launcher icons are not screenshots. The README contains **text-labeled capture slots** for Home, Recordings and VIDAA; none is presented as a real screenshot. Replace each slot only with an authentic, privacy-reviewed capture and record device/build evidence.

## 3. GitHub About metadata — owner action

Live GitHub repository metadata was checked on 2026-10-12: **description = null, homepage = null, topics = []**. The available connector does not provide a repository-About editing action. The following are proposed settings, **not** changes already made:

- **Description:** `VDR companion platform for Web and TV: EPG, recordings, playback, timers, secure API and backend control.`
- **Website:** `https://github.com/hotzenplotz5/vdr-suite/blob/main/docs/getting-started.md` (temporary documentation landing page, not a hosted product site).
- **Topics:** `vdr`, `video-disk-recorder`, `pvr`, `live-tv`, `epg`, `media-server`, `openapi`, `cpp`, `vidaa`.

Repository owner: open the repository's **About** section on GitHub, select the gear/edit icon, enter and save the description, website and topics. This is the only remaining owner-side step in item 3 because the connected GitHub actions do not support changing these repository settings. Check how they render after saving. Do not treat suggested metadata as proof a website or released VIDAA product exists.

## 4. Source-backed beginner/developer entrypoints — included in this documentation change

- [Getting Started](getting-started.md): what can be explored on an existing installation, plus explicit absence of a general packaged installer.
- [Developer Quickstart](development/quickstart.md): isolated checkout, CI-derived dependencies, local tests and source ownership.
- [Contributor guide](../CONTRIBUTING.md): an accessible first contribution path, with binding agent rules still in [AGENTS](../AGENTS.md).

## Publication acceptance checklist

- [x] GPLv3 license text, initial component notices and ABI origin findings merged via PR #464; future redistribution review remains separate.
- [ ] At least two **actual, redacted** product screenshots merged and linked from README.
- [ ] GitHub About description/website/topics saved by a repository owner.
- [x] Fresh visitor can find first steps and an honest installation status from the README.
- [x] New developer can identify a non-production local-build path.
- [ ] Documentation checks green on exact current review head; no installed-runtime or Hisense acceptance implied.

## Back

- [README](../README.md)
- [Documentation home](index.md)
- [Screenshots](screenshots.md)
