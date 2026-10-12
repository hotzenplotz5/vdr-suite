# Third-party license and attribution inventory

This is a distribution-review inventory, **not** a declaration that the listed upstream software or media is owned by VDR-Suite. The root `LICENSE` contains GNU GPLv3 for the VDR-Suite code that its relevant rightsholders authorize under `GPL-3.0-or-later`. External dependencies and plugins retain their own licenses. The VDR-Suite repo does not claim that calling external tools or APIs incorporates all their source into this repository.

| External component | Upstream evidence | Relationship / release action |
| --- | --- | --- |
| VDR (Klaus Schmidinger) | [vdr.c](https://github.com/vdr-projects/vdr/blob/master/vdr.c) explicitly GPLv2 or any later version; [plugin.h](https://github.com/vdr-projects/vdr/blob/master/plugin.h) refers to main copyright | Native SuiteBridge plugin interface; GPLv3 option available. Verify exact installed/build version and credit upstream; keep notices. |
| Streamdev (Sascha Volkenandt; Frank Schmirler) | [README](https://github.com/vdr-projects/vdr-plugin-streamdev/blob/master/README), [COPYING](https://github.com/vdr-projects/vdr-plugin-streamdev/blob/master/COPYING) (GPLv2 license text) | Separate HTTP/VTP streaming service. Do not assume all files are `-or-later` solely from COPYING; preserve original rights and notices. |
| TVScraper (Stefan Braun; Markus Ehrnsperger) | [README.md](https://github.com/MarkusEh/vdr-plugin-tvscraper/blob/master/README.md) explicitly GPLv2 or later | Native plugin metadata provider. Its downloaded TMDB/TVDB artwork and datasets are **not** automatically GPL. |
| SQLite | [SQLite copyright](https://www.sqlite.org/copyright.html) (public domain library) | Locally linked library. Separate SQLite tools/extensions may have separate terms. |
| libcurl | [curl copyright](https://curl.se/docs/copyright.html) | Linked HTTP library; preserve notices when redistributing binaries. TLS backend has its own license. |
| OpenSSL | [OpenSSL license information](https://www.openssl.org/source/license.html) | Exact major version/configuration and redistribution notices required for packaging review. |
| FFmpeg | [FFmpeg legal page](https://ffmpeg.org/legal.html) | Used as external executable for media operations; packaged build configuration (LGPL/GPL/nonfree) must be inspected before redistribution. |
| Browser/TV graphics | `web/frontend/assets/`, `web/frontend/*logo*.svg`, `web/frontend/channel-logos/`, separate VIDAA `app/*.png` | Creator, source file and redistribution grant not independently evidenced. Do **not** republish as cleared third-party media; request provenance, then classify file-by-file. |
| TMDB / TVDB / TV channel art | External content sources | Separate content terms and rights. Do not infer a GPL grant from VDR/TVScraper code. |
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
| Logos synchronized at runtime | `tools/vdr_suite_logo_sync.py` defaults to https://github.com/3PO/Senderlogos/archive/refs/heads/master.zip | **External source identified:** upstream repository API currently shows no detected license. Channel logos may include third-party trademarks/artwork. Downloading to a private cache is not a license grant to bundle/relicense them; packaging and redistribution remain gated by upstream and rightsholder permissions. |

### Practical clearance record needed

For each group supply: *original creator or generator*, *source project/artifact*, *date and context of creation*, *underlying reference/input if adapted*, *ownership or express permission for redistribution under the intended terms*, and *whether any trademarks, programme stills or artwork from third parties appear*. Keep evidence in a maintainer-held location if it contains personal or confidential data; put only a non-sensitive conclusion and accurate license/attribution in the repository.

**Current status:** origin of the ten genre posters and sprite is documented by the maintainer as ChatGPT-created. They are not presumed foreign stock images on the basis of their JPEG encoding. Rights/provenance of other assets and external code remain under review. No historical files were removed or relicensed as part of this inventory.

## Creator clarification (2026-10-12)

The maintainer confirms the ten recording genre posters and genre sprite were created using ChatGPT for VDR-Suite. Their SVG-embedded JPEG encoding is not itself evidence of third-party imagery. The earlier unknown-origin designation for these eleven assets is superseded by this declaration. No independent review of potential reference material or substantial similarity was completed; other assets remain separate questions.

## Additional creator declaration (2026-10-12)

The maintainer confirms: `web/frontend/assets/vdr-remote-photorealistic.png` and the VIDAA graphics (`app/Code_Generated_Image.png` and `app/vdr-suite-icon*.png`) were created using ChatGPT for this project; VIDAA `app/startvideo.mp4` was created using Gemini Pro. These files should no longer be listed as unknown-origin stock media. This is a project-owner statement about their production tools, not a claim of exclusive copyright over AI output, nor a finding that all possible reference/input rights are cleared. Separately obtained broadcast channel logos and programme artwork remain subject to their own rights.
