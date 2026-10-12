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

## Scope of verified facts vs missing evidence

The upstream licenses noted here concern identifiable upstream projects, **not a full source similarity or historical authorship scan** of all repository files. Confirm actual versions, the build-time/linkage dependency inventory, all copyright holders, and each externally sourced file before representing publication as legally cleared. The 100 most recent GitHub commits showed the author name `hotzenplotz5`, which is not proof of ownership of all files.

**Required before final release acceptance:** provenance and permission records for in-tree original/contributed code and graphics, specific SuiteBridge interactions with other in-process plugins, and a distribution-specific dependency/SBOM audit. Independently produced code under GPL-3.0-or-later can be published when its rightsholders have authorized it, without pretending these downstream checks are passed.

See [GPLv3 source audit](gplv3-compatibility-audit.md) and [publication checklist](publication-readiness.md).
