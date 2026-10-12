# Third-party license and attribution inventory

This is a distribution-review inventory, **not** a declaration that the listed upstream software or media is owned by VDR-Suite. The root `LICENSE` contains GNU GPLv3 for the VDR-Suite code that its relevant rightsholders authorize under `GPL-3.0-or-later`. External dependencies and plugins retain their own licenses. The VDR-Suite repo does not claim that calling external tools or APIs incorporates all their source into this repository.

| External component | Upstream evidence | Relationship / release action |
| --- | --- | --- |
| VDR (Klaus Schmidinger) | [vdr.c](https://github.com/vdr-projects/vdr/blob/master/vdr.c) explicitly GPLv2 or any later version; [plugin.h](https://github.com/vdr-projects/vdr/blob/master/plugin.h) refers to main copyright | Native SuiteBridge plugin interface; GPLv3 option available. Verify exact installed/build version and credit upstream; keep notices. |
| Streamdev (Sascha Volkenandt; Frank Schmirler) | [README](https://github.com/vdr-projects/vdr-plugin-streamdev/blob/master/README), [COPYING](https://github.com/vdr-projects/vdr-plugin-streamdev/blob/master/COPYING) (GPLv2 license text) | Separate HTTP/VTP streaming service. Do not assume all files are `-or-later` solely from COPYING; preserve original rights and notices. |
| TVScraper (Stefan Braun; Markus Ehrnsperger) | [README.md](https://github.com/MarkusEh/vdr-plugin-tvscraper/blob/master/README.md) explicitly GPLv2 or later; [upstream services.h](https://github.com/MarkusEh/vdr-plugin-tvscraper/blob/master/services.h) | In-tree `vdr-plugin-suite-bridge/services.h` is a locally adapted service contract retaining upstream attribution and `GPL-2.0-or-later` SPDX; native provider, compatible through GPLv3 option. TMDB/TVDB data is not tracked as repository assets. |
| SQLite | [SQLite copyright](https://www.sqlite.org/copyright.html) (public domain library) | Locally linked library. Separate SQLite tools/extensions may have separate terms. |
| libcurl | [curl copyright](https://curl.se/docs/copyright.html) | Linked HTTP library; preserve notices when redistributing binaries. TLS backend has its own license. |
| OpenSSL | [OpenSSL license information](https://www.openssl.org/source/license.html) | Exact major version/configuration and redistribution notices required for packaging review. |
| FFmpeg | [FFmpeg legal page](https://ffmpeg.org/legal.html) | Used as external executable for media operations; packaged build configuration (LGPL/GPL/nonfree) must be inspected before redistribution. |
| Browser/TV graphics | `web/frontend/assets/`, `web/frontend/*logo*.svg`, `web/frontend/channel-logos/`, separate VIDAA `app/*.png` | Creator, source file and redistribution grant not independently evidenced. Do **not** republish as cleared third-party media; request provenance, then classify file-by-file. |
| TMDB / TVDB / TV channel art (runtime only; **not tracked in the repository**) | Fetched or supplied by external services | **Outside the repository source license inventory.** Their separate terms are relevant only to caching, display or a future package that actually redistributes them; they do not block licensing the tracked source tree. |
| VIDAA `app/startvideo.mp4` | Maintainer states generated using Gemini Pro | Creator/tool provenance recorded; actual source materials, similarity and any third-party media inputs not independently checked. |

## Scope of verified facts vs missing evidence

The upstream licenses noted here concern identifiable upstream projects, **not a full source similarity or historical authorship scan** of all repository files. Confirm actual versions, the build-time/linkage dependency inventory, all copyright holders, and each externally sourced file before representing publication as legally cleared. The 100 most recent GitHub commits showed the author name `hotzenplotz5`, which is not proof of ownership of all files.

**Required before final release acceptance:** provenance and permission records for in-tree original/contributed code and graphics, specific SuiteBridge interactions with other in-process plugins, and a distribution-specific dependency/SBOM audit. Independently produced code under GPL-3.0-or-later can be published when its rightsholders have authorized it, without pretending these downstream checks are passed.

See [GPLv3 source audit](gplv3-compatibility-audit.md) and [publication checklist](publication-readiness.md).

## File-level media provenance audit (12 October 2026)

This assessment distinguishes *repository introduction* from independent evidence of **the creator and the right to redistribute**; the latter cannot be proven merely by a Git commit.

| Asset / group | Specific repository evidence | Audit finding |
| --- | --- | --- |
| `web/frontend/logo-vdr-suite.svg` | Initially added by `hotzenplotz5` in [63623bbb](https://github.com/hotzenplotz5/vdr-suite/commit/63623bbb193683f3c28acdec6f4cc0f0bd8ad06f), revised in [2354d9b1](https://github.com/hotzenplotz5/vdr-suite/commit/2354d9b1e120edad2bf10452f093b4e5e11192e8). SVG source contains paths, text and gradients. | **Introducer identified; original author/license grant not independently proven.** Confirm whether the image was commissioned, AI-generated, drawn in-house or adapted from an existing work; collect source/consent before treating as fully cleared. |
| Ten `web/frontend/assets/recording-genre-*.svg` files | All added in [410f104a](https://github.com/hotzenplotz5/vdr-suite/commit/410f104a45890dc86f3a83813552ffe3dd141202), message “approved local poster assets”. Actual XML embeds `<image ... href="data:image/jpeg;base64,...">` with JPEG pictures. | **Provenance clarified by maintainer:** created using ChatGPT for VDR-Suite, as recorded below. JPEG embedding is a technical format, not evidence of copied third-party artwork. Independent similarity/source-input review was not performed. |
| `web/frontend/assets/recording-genre-sprite.svg` | Embedded JPEG data URI confirmed in its XML | **Provenance clarified by maintainer:** created using ChatGPT for VDR-Suite, as recorded below. No independent similarity/source-input review was performed. |
| `web/frontend/assets/vdr-remote-photorealistic.png` | Binary raster asset present in repository | **Creator provenance confirmed by maintainer (2026-10-12):** created using ChatGPT for VDR-Suite. No independently established source/reference-image or third-party similarity review. |
| `web/frontend/icon-vdr-suite.svg`, `favicon.svg`, `logo-vdr-suite-dark.svg`, `channel-logos/vdr-suite-brand/logo-vdr-suite-dark.svg` | In-tree branded SVGs | **Unresolved:** identify sources and relation to primary logo; confirm consistent rightsholder grant. |
| VIDAA `app/Code_Generated_Image.png` | Introduced by `hotzenplotz5` in [38901625](https://github.com/hotzenplotz5/vdr-suite-vidaa/commit/38901625f247d9e753504a28657d609fe7d37) with message “Add files via upload” | **Creator provenance confirmed by maintainer (2026-10-12):** generated using ChatGPT for VDR-Suite; filename alone was not evidence, but maintainer statement now documents origin. Confirm any reference-material restrictions separately. |
| VIDAA `app/vdr-suite-icon*.png` | Binary assets in private VIDAA repository | **Creator provenance confirmed by maintainer (2026-10-12):** VIDAA graphics created using ChatGPT for VDR-Suite. Individual derivation and input history not independently established. |
| Logos synchronized at runtime | `tools/vdr_suite_logo_sync.py` defaults to https://github.com/3PO/Senderlogos/archive/refs/heads/master.zip | **External source identified:** upstream repository API currently shows no detected license. Channel logos may include third-party trademarks/artwork. **These downloaded logos are not repository contents.** Their separate rights matter only if distributed or reused outside the allowed runtime usage; they do not block licensing the tracked source tree. |

### Practical clearance record needed

For each group supply: *original creator or generator*, *source project/artifact*, *date and context of creation*, *underlying reference/input if adapted*, *ownership or express permission for redistribution under the intended terms*, and *whether any trademarks, programme stills or artwork from third parties appear*. Keep evidence in a maintainer-held location if it contains personal or confidential data; put only a non-sensitive conclusion and accurate license/attribution in the repository.

**Current status:** origin of the ten genre posters and sprite is documented by the maintainer as ChatGPT-created. They are not presumed foreign stock images on the basis of their JPEG encoding. Rights/provenance of other assets and external code remain under review. No historical files were removed or relicensed as part of this inventory.

## Creator clarification (2026-10-12)

The maintainer confirms the ten recording genre posters and genre sprite were created using ChatGPT for VDR-Suite. Their SVG-embedded JPEG encoding is not itself evidence of third-party imagery. The earlier unknown-origin designation for these eleven assets is superseded by this declaration. No independent review of potential reference material or substantial similarity was completed; other assets remain separate questions.

## Additional creator declaration (2026-10-12)

The maintainer confirms: `web/frontend/assets/vdr-remote-photorealistic.png` and the VIDAA graphics (`app/Code_Generated_Image.png` and `app/vdr-suite-icon*.png`) were created using ChatGPT for this project; VIDAA `app/startvideo.mp4` was created using Gemini Pro. These files should no longer be listed as unknown-origin stock media. This is a project-owner statement about their production tools, not a claim of exclusive copyright over AI output, nor a finding that all possible reference/input rights are cleared. Externally fetched broadcast channel logos and programme artwork are **not part of the Git repository**; any separate use or redistribution is outside this source-tree license audit.

## Repository boundary clarification (2026-10-12)

The maintainer confirms that externally downloaded station logos and film/series artwork are **not committed to this repository**. The Git tree contains the synchronization *script* and project-owned UI asset files, not the third-party downloaded logo/archive or TMDB/TVDB artwork collections. These external runtime inputs are **not blockers for selecting a license for the GitHub source repository**. A separately distributed binary/package or bundled media archive must be checked against what it actually includes. Do not confuse the in-repository brand directory `web/frontend/channel-logos/vdr-suite-brand/` with fetched broadcast logos.

## Native HbbTV / Teletext ABI boundaries (2026-10-12)

| Repo file | Source evidence | Rights status |
| --- | --- | --- |
| `vdr-plugin-suite-bridge/suitebridge_hbbtv_provider_contract.h` | Explicitly references `hotzenplotz5/vdr-plugin-web` and upstream `Zabrimus/vdr-plugin-web@34ded5090fbad021338c491355566dbdb4d98f9d`; fork has a GPLv2 COPYING | **Version permission unresolved.** Check exact provider-side interface provenance and upstream author notices before assigning SPDX. |
| `vdr-plugin-suite-bridge/suitebridge_teletext_provider_contract.h` | Explicitly mirrors `vdr-plugin-osdteletext@70496310d1fa5200ff808d1552f0f5d252893870`; fork has GPLv2 COPYING; upstream README credits Marcel Wiesweg | **Version permission unresolved.** Check source of service ABI, legal status of copied expressiveness, and any GPLv3 permission; don't stamp GPLv3 without proof. |

These are **in-process plugin service APIs**. The existence of GPLv2 license text is not by itself proof of a GPL-2.0-only prohibition, nor permission to choose GPLv3. The GPL version and source-copying questions must be answered per actual files and contributors before full legal sign-off. External runtime media are not part of this issue.

### Verified ABI source comparison (2026-10-12)

Direct source inspection confirms that the SuiteBridge Teletext ABI mirrors `hotzenplotz5/vdr-plugin-osdteletext@70496310d1fa5200ff808d1552f0f5d252893870:services.h`, including service identifiers, values and struct layout. The HbbTV media service ABI matches the fork's `service/vdrsuite_hbbtv_media_service.h` at `hotzenplotz5/vdr-plugin-web@1b3d2e785a1caf2d9f991fafa9752a433c5a86ae`. The code origin has been located, but ownership of new fork contributions and exact GPL version grants remain unverified. The upstream GPLv2 COPYING is not proof that novel contracts are GPL-2.0-only nor a GPLv3 relicensing grant. File-scoped SPDX changes must await this confirmation.

## Build and packaging boundary verification (12 October 2026)

- `mk/common.mk` explicitly uses `pkg-config --libs sqlite3` for the daemon/test binaries and provides local C++ include directories; this is a **direct SQLite link** in the build graph.
- `vdr-plugin-suite-bridge/Makefile` builds a separate VDR loadable shared plugin with `vdr.pc` flags. Such a plugin is not just an independent HTTP client, and its in-process interfaces have been separately inventoried.
- The installation targets in `mk/install.mk` stage locally built daemon/agent binaries, frontend static resources, service configuration and the project’s own UI resources. The inspected install rules do not establish bundled copies of FFmpeg, OpenSSL, libcurl, VDR, TVScraper, channel-logo packs or broadcast artwork. **No conclusion about all future binary packages is inferred.**
- The `ffmpeg` CLI used by media command builders is a separate runtime executable as opposed to shown libav linkage; the precise installed FFmpeg configuration remains a **binary-distribution** issue if VDR-Suite ships a bundled FFmpeg binary. Libraries brought in transitively by a linked component must be assessed against the particular built package, not assumed to be redistributed in the source tree.
- This is a *repository/source-build audit*, not a package-by-package ELF/SBOM signature. It is inappropriate to list system-packaged components or runtime-downloaded media as vendored third-party source without evidence. For release packages, inspect staging manifests, `ldd`/ELF dependencies, `ffmpeg -buildconf`, package scripts and any redistribution notices from the actual artifact.

## Specific remaining rights authorizations

1. `suitebridge_teletext_provider_contract.h` mirrors the OSD Teletext fork service declarations. The source is pinned, but the upstream file-specific permission for redistribution under GPLv3 has not been demonstrated; the version string of COPYING is insufficient.
2. `suitebridge_hbbtv_provider_contract.h` mirrors service ABI declarations in the maintainer's web-plugin fork, referencing its Zabrimus base. Identify which declarations were newly authored in the fork, whether any expressive upstream source was carried over, and the rightsholder's authority to license the relevant declarations under GPLv3.
3. Contributions named in Git history must be checked against the actual provenance of included source; names and API-compatible numeric layouts alone cannot serve as a blanket copyright assignment.

These are narrowly scoped outstanding authorizations; **no incompatibility is yet established**, and no blanket SPDX license should be applied to the mirrored interfaces before those source rights are validated.

## Verified original introductions of HbbTV / Teletext service contracts (2026-10-12)

- Teletext: `services.h` was **added** in commit [`76c62292835c5138a41548d2b35eb305974d736c`](https://github.com/vdr-projects/vdr-plugin-osdteletext/commit/76c62292835c5138a41548d2b35eb305974d736c), attributed to `hotzenplotz5`. The same commit is visible in the project's upstream Git history. The SuiteBridge header mirrors this newly introduced service ABI, not a contract observed in the preceding plugin baseline.
- HbbTV: The pinned Zabrimus base at `34ded5090fbad021338c491355566dbdb4d98f9d` has **no** `vdrsuite_hbbtv_*` service headers. The fork history attributes the introduction of discovery to `25e52132`, runtime to `220711b5`, presentation to `b8c52779`, and media to `1d5af33c`, all authored as `hotzenplotz5` commits. SuiteBridge mirrors these particular service ABI declarations, not all pre-existing Zabrimus plugin source.
- **License treatment:** The maintainer has selected `GPL-3.0-or-later` for their own VDR-Suite contributions. Both local SuiteBridge ABI mirrors now have file-scoped `SPDX-License-Identifier: GPL-3.0-or-later` notices with provenance comments. This decision applies to these newly authored definitions, **not** to the full separate original VDR plugins. Commit metadata and absence in an upstream baseline are strong origin evidence, though not an independent legal ownership assignment or a whole-repository source similarity audit.
- **Resolution:** The previous unidentified-origin/GPLv2-only hypothesis for these two ABI mirrors is narrowed and no longer treated as a demonstrated GPL-version incompatibility. All other upstream source retains its own terms. For distribution of independently packaged plugin forks, audit those complete repositories separately.
