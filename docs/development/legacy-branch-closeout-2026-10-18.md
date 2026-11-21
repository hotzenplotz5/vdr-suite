# Altbranch-Inventar vor Konsolidierungsabschluss (18.10.2026)

**Repository:** `hotzenplotz5/vdr-suite`. **Ziel-PR:** [#500](https://github.com/hotzenplotz5/vdr-suite/pull/500). Dieses Dokument zeichnet die exakten Remote-Heads vor der zweiten Löschwelle auf und stellt **keinen** Merge-/Funktionsnachweis dar.

Der erste Löschlauf hat 17 nach Git-Ancestry vollständig integrierte Altbranches entfernt. Bei erneuter GitHub-Prüfung verblieben 41 Branches: `main`, `work/public-v1-vidaa-unified-20261018`, vier Add-on-Branches, der PPA/Packaging-Branch und die folgenden 34 alten Branches.

**Schutzgrenzen:** `main`, `work/public-v1-vidaa-unified-20261018`, die vier `work/addon-*`-Branches und `work/packaging-debian-ubuntu-install` dürfen durch die Altbereinigung nicht gelöscht werden. Das PPA-Paket-Rezept und `.github/workflows/packaging-plugin-experimental.yml` erfordern weiterhin den Packaging-Branch.

**Zwei weitere Workflow-Abhängigkeiten:** Die Push-Trigger in `main` referenzieren weiterhin `work/r2-recording-folder-browse` und `work/vidaa-device-tracks-index-srt-webvtt-integrated-20261011`. Im Sammelbranch wurden diese Trigger bereits auf den Sammelbranch retargetet, doch eine Änderung im Sammelbranch ist noch keine Änderung in `main`. Diese beiden Branches erst nach kontrollierter Umstellung in `main` entfernen.

**Bewertung:** Alte Branches mit eigenständigen Commits sind **nicht** allein wegen ihres Alters funktionsgleich mit dem Sammelbranch. Bereits auf `main` gemergte Multiuser-Arbeiten und spätere API-Integrationen können ältere Implementierungen ersetzt haben. Sonderfälle mit nur in den Altbranches vorhandenen Dateien betreffen HEVC-HLS-Probe/Codec-Referenz, Recording-Resume-Diagnostik, PublicSearchRecordingInventory, R2-Recording-Identität und ein einmaliges Hisense-Grant-Skript. Nichts davon wird ungeprüft in Public API v1 übernommen.

**Zurückgestellte Experimente:** `work/hevc-hls-keyframe-segmentation-trial-20261013`, `work/codec-reference-corpus-20261013` und `work/playback-fast-start-gated-b20261012` wurden zuvor ausdrücklich als nicht akzeptierte Experimente gekennzeichnet; siehe `docs/development/playback-consolidation-and-deferred-experiments-2026-10-14.md`. Deren HEADs dürfen nicht als produktiv integriert bezeichnet werden. Vor einer Löschung ist eine wiederherstellbare lokale Archivierung samt SHA-/Bundle-Prüfung nötig.

## Exakte historische Remote-Heads

| Alter Branch | Git-HEAD | Einordnung |
| --- | --- | --- |
| `docs/temporary-worktree-cleanup-rule` | `a09c8ceba1e21c89ebdc66cc48449f390a974097` | Abweichende Historie; Archiv/Review erforderlich |
| `fix/public-timer-assignment-unused-actor-ref` | `1cf7826b9938d89e9ec747c69ea594d5f2cb40d7` | Abweichende Historie; Archiv/Review erforderlich |
| `work/codec-reference-corpus-20261013` | `ebf300532cc749521c7e6c1c3aef5794c5f9f239` | Zurückgestelltes Experiment |
| `work/daemon-installer-db-risk-build-reuse-20261011` | `a103166a00db8614c277db011eb37ef611415283` | Abweichende Historie; Archiv/Review erforderlich |
| `work/device-media-profile-binding-v1-20261012` | `1f43fd251e8567787e9eab10eb23f90cbbd92459` | Abweichende Historie; Archiv/Review erforderlich |
| `work/device-recording-resume-status-parity-20261012` | `3ff0112414545af7d550b9a77bfd08ddc56859f0` | Eigenständige Dateien; Archiv/Review erforderlich |
| `work/hevc-hls-keyframe-segmentation-trial-20261013` | `3080e2523f20ebf7bac6d191c59bdea2bd81e49d` | Zurückgestelltes Experiment |
| `work/integrated-vidaa-codecs-live-search-genres-recordings-20261015` | `d3bae6bccf6a2a0002c146c68c2c6e641e829612` | Eigenständige Dateien; Archiv/Review erforderlich |
| `work/integration-current-daemon-public-v1-live-20261015` | `685b0b44f316dd73ddf1da7459290e8a08d94ff0` | Abweichende Historie; Archiv/Review erforderlich |
| `work/media-probe-hdr-metadata-slice1` | `0f91221215c2a4ae350d7ff4900727f1e2c280f0` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu6d-public-account-create` | `b1444a962991e83fa124b3f737822d08bcda41d1` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu7-backend-access-grant-administration` | `dfd85d386091dc3b14c2037a43edea2b0c190593` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu8b-post-merge-closeout` | `c2ddcc6aed06defe57884115fea9882d5414dfc3` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu8c-human-password-credential-revoke` | `d06872d48f02019afcc83f5dfba001387bee5b6b` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu9-documentation-sync` | `b51caa534e437819d169e2ebb45b349c34e10d2e` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu9b-post-merge-closeout` | `648d5ea988bfb7b54c0976d7fda46947d3c539de` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu9c-closeout-mu9d-selection` | `c0617befe136c7f721b90d8d2b7be647cab1f1f4` | Abweichende Historie; Archiv/Review erforderlich |
| `work/mu10b-post-merge-doc-closeout` | `7e901b197a076809f589ad77442b720838b9e644` | Abweichende Historie; Archiv/Review erforderlich |
| `work/playback-fast-start-gated-b20261012` | `663d5c5086ebe995d9dbe11e3f917e1860c59d62` | Zurückgestelltes Experiment |
| `work/public-v1-device-live-tv-epg-20261015` | `fc06350e2291c5ff13c106c706f35d3ecda161fa` | Abweichende Historie; Archiv/Review erforderlich |
| `work/public-v1-epg-window-vidaa-20261017` | `c2f7f299196f0d7f6ceb951015bbb23451a16ae1` | Abweichende Historie; Archiv/Review erforderlich |
| `work/public-v1-unified-search-epg-vidaa-20261014` | `8f44916be431014b5b3032d4caac286e05e64893` | Abweichende Historie; Archiv/Review erforderlich |
| `work/r2-recording-folder-browse` | `ad9e8781a40a2f296e5a4a04a9677bc88d09f814` | Workflow-Referenz in main |
| `work/recordings-r1-public-contract-audit` | `595ba07094c00dcffc356c840b968bc2ea64bc9d` | Abweichende Historie; Archiv/Review erforderlich |
| `work/vidaa-device-track-api-for-b20261011-9` | `6cf5e5e488d76346e993a121ee261cef0966a69f` | Abweichende Historie; Archiv/Review erforderlich |
| `work/vidaa-device-tracks-index-srt-webvtt-integrated-20261011` | `bd6009018a0465d1ae5830a1b823437d2d89bf45` | Workflow-Referenz in main |
| `work/vidaa-hisense-device-grants-20261017` | `0c4d14fb27936735f2b17ddba9fa2d1bee927dac` | Eigenständige Dateien; Archiv/Review erforderlich |
| `work/vidaa-index-video-root-discovery-20261012` | `f25fa9b184404bc72ae408f40dd3752d6f67b298` | Eigenständige Dateien; Archiv/Review erforderlich |
| `work/vidaa-public-v1-track-inventory-20261011` | `89b1ca466ef33bfd8e2839a5676db641010b9d57` | Abweichende Historie; Archiv/Review erforderlich |
| `work/vidaa-public-v1-track-selection-20261011` | `362ec850792c9d7489c7f18a1c6d8a4db1505a9e` | Abweichende Historie; Archiv/Review erforderlich |
| `work/vidaa-recording-r2-public-identity` | `2421e9419cbb95c1d9ca53539b2c2ab2beebd065` | Abweichende Historie; Archiv/Review erforderlich |
| `work/vidaa-recordings-r2` | `f16cb1c8e9fab4f1b8188ccf41f0dec24157efa7` | Eigenständige Dateien; Archiv/Review erforderlich |
| `work/vidaa-srt-sidecar-discovery-20261011` | `bc6866e752b9bf1f51cf6cdab7a7d8b69bd1fe09` | Abweichende Historie; Archiv/Review erforderlich |
| `work/vidaa-vdr279-index-safety-20261011` | `12cb97ac6337eff7c9791ab1c75cb88c4f3a0353` | Abweichende Historie; Archiv/Review erforderlich |

## Abschlussbedingungen

1. Keine Branch-Löschung allein aus dieser Inventarliste ableiten. Vor einem Löschlauf frische `ls-remote`-HEADs und offene PRs prüfen; jeden SHA-Wechsel als Stopp werten.
2. Für die 32 divergierenden Branches vor Löschung eine verifizierte, außerhalb des Repository-Checkouts gespeicherte Git-Bundle-Sicherung der exakten Heads erstellen (inkl. erfolgreichem `git bundle verify` und `git bundle list-heads`). Dieses Inventar allein konserviert die Git-Objekte **nicht**.
3. Unterschiede, die eigenständige Produktfunktionalität enthalten können, fachlich prüfen und gegebenenfalls gezielt übernehmen; Archivieren ist kein Funktionsmerge.
4. Die zwei an `main` gebundenen Workflow-Branches bis zum tatsächlich wirksamen Retargeting erhalten.
5. Nach der Aktion GitHub-Refs, offenen PR #500 und die geschützten SHAs erneut kontrollieren. Kein Force-Push, kein Reset, kein zweiter Integrationsbranch, keine ungefragte Änderung an `main`.
