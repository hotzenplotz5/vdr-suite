# Public GitHub presentation and licensing — remaining owner decisions

## Scope and verified facts (2026-10-12)

This is a **release/publication checklist**, not legal clearance or evidence of installation. It identifies which parts of the public presentation can be improved in source and which require maintainer action or real assets. The audited backend `main` contains an **empty 0-byte `LICENSE`**, no verified UI/TV screenshots, and its GitHub About fields currently have no short description, homepage or topics.

## 1. License — BLOCKED pending copyright and maintainer clearance

A [source-backed GPLv3 compatibility matrix](gplv3-compatibility-audit.md) now records identified boundaries, upstream evidence and unresolved legal questions. The maintainer has explicitly selected **`GPL-3.0-or-later`** as the target for original VDR-Suite source they are authorized to license. The matrix remains conditionally compatible; it does not itself clear upstream rights, other contributors or artwork. No project-wide legal clearance is claimed.

The empty `LICENSE` file does **not** grant a conventional open-source license. Do not call VDR-Suite legally reusable under MIT, GPL or another SPDX license merely because the repository is public.

Before publishing the selected license:

1. Identify ownership and contributor permissions for all original C++/JavaScript source; review external code, vendored assets, logos, fonts and build files separately.
2. Audit integration, linking and distribution relationships with VDR and upstream plugins/libraries; do **not** assume the licensing obligation of every source dependency automatically applies to the entire server, or that a permissive license is safe without provenance.
3. Confirm that every relevant repository copyright holder authorizes the selected `GPL-3.0-or-later` identifier and any contribution terms. If multiple licenses apply to independent components, document exact component boundaries.
4. Only **after** approval, commit the complete unmodified approved license text, provenance/third-party notices as needed, and source-level license statements where warranted. Validate GitHub's detected license afterwards.

The selected identifier is **`GPL-3.0-or-later`**; its application to all affected original files remains conditional on rights clearance. **This PR deliberately leaves `LICENSE` untouched until legal/provenance gates are cleared.**

## 2. Authentic product screenshots — BLOCKED pending capture

[The screenshot capture checklist](screenshots.md) specifies a privacy-reviewed Web Home, Recording browser, optional playback screenshot and real VIDAA image. Source assets and TV launcher icons are not screenshots. The README may use the already-existing project SVG logo, but must not claim a screenshot gallery exists until real images are committed.

## 3. GitHub About metadata — owner action

The available GitHub repository connector supports code/PR operations, but not repository-About editing. These are **suggested values**, not settings silently claimed as applied:

- **Description:** `VDR companion platform for Web and TV: EPG, recordings, playback, timers, secure API and backend control.`
- **Website:** `https://github.com/hotzenplotz5/vdr-suite/blob/main/docs/getting-started.md` (temporary documentation landing page, not a hosted product site).
- **Topics:** `vdr`, `video-disk-recorder`, `pvr`, `live-tv`, `epg`, `media-server`, `openapi`, `cpp`, `vidaa`.

Repository owner: open the repository's **About** section on GitHub, select the gear/edit icon, enter and save the description, website and topics. Check how they render after saving. Do not treat suggested metadata as proof a website or released VIDAA product exists.

## 4. Source-backed beginner/developer entrypoints — included in this documentation change

- [Getting Started](getting-started.md): what can be explored on an existing installation, plus explicit absence of a general packaged installer.
- [Developer Quickstart](development/quickstart.md): isolated checkout, CI-derived dependencies, local tests and source ownership.
- [Contributor guide](../CONTRIBUTING.md): an accessible first contribution path, with binding agent rules still in [AGENTS](../AGENTS.md).

## Publication acceptance checklist

- [ ] Maintainer-approved license and component notices merged and checked.
- [ ] At least two **actual, redacted** product screenshots merged and linked from README.
- [ ] GitHub About description/website/topics saved by a repository owner.
- [ ] Fresh visitor can find first steps and an honest installation status in at most two clicks.
- [ ] New developer can identify a reproducible local-build path without production deployment.
- [ ] Documentation checks green on exact review head; no installed-runtime or Hisense acceptance implied.

## Back

- [README](../README.md)
- [Documentation home](index.md)
- [Screenshots](screenshots.md)
