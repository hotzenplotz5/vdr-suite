# Playback integration: consolidated history and deferred experiment archive (2026-10-14)

Status: **backend fast-start candidate installed on real yaVDR and running; real latency/VIDAA hardware acceptance pending**. Integration PRs remain draft and unmerged.
This document is a durable review index; it authorizes no further VDR, daemon, TV, permission, package or production change.

## Integration candidates (still draft, not merged into main)

- Backend: [vdr-suite PR #471](https://github.com/hotzenplotz5/vdr-suite/pull/471), original integration anchor `8c69d1cb99517809e94dfcb772e887e0840d0365`, verified combined baseline `6ce70f390eb8842b9894c75d0daa2b0db0e09c8c`.
- VIDAA: [vdr-suite-vidaa PR #17](https://github.com/hotzenplotz5/vdr-suite-vidaa/pull/17), verified candidate `4c97d68cdf81bf95c3432682b3c566c6378df4ad`. The regular **Play** action prefers the supported H.264/AAC HLS-fMP4 MSE path where the runtime exposes it. This is an adaptive target-format choice, not proof that all source codecs transcode in real time.
- The backend integration HEAD above contained the original #471 history, Device Trickplay backend [#473](https://github.com/hotzenplotz5/vdr-suite/pull/473) at `550da7f7328bfb48c8e1fcbb00666842bd0311ed`, and guarded daemon-only Trickplay installer [#474](https://github.com/hotzenplotz5/vdr-suite/pull/474) at `6ffb824eddb649a7db2dbe16aa4ab14d46634799`, all confirmed as ancestors by GitHub comparison.
- Original installer [#451](https://github.com/hotzenplotz5/vdr-suite/pull/451) (`a103166a00db8614c277db011eb37ef611415283`) is **not** a Git ancestor of #471. Its later replacement was independently compared: original function and test names survive, the original workflow blob is identical and newer root-build-refusal, binary-identity, Trickplay-provenance and rollback coverage is added. Preserve the original branch for traceability; never claim an ancestry merge for #451.
- For backend SHA `6ce70f390eb8842b9894c75d0daa2b0db0e09c8c`, GitHub Actions reported success in [VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38460055899), [daemon installer safety](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38460056123), and [fMP4 codec evidence](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38460056028). These runs **do not** certify subsequent documentation commits or real playback. VIDAA SHA `4c97d68cdf81bf95c3432682b3c566c6378df4ad` passed [client CI](https://github.com/hotzenplotz5/vdr-suite-vidaa/actions/runs/38429630125).

## Deferred only; **not** included in either integration candidate

Keep these as reviewable source branches with immutable SHAs; closing an obsolete experimental PR does **not** delete a branch or its tests.

| Repo/PR | Original branch and SHA | Reason to defer |
| --- | --- | --- |
| Backend [#477](https://github.com/hotzenplotz5/vdr-suite/pull/477) | `work/hevc-hls-keyframe-segmentation-trial-20261013` @ `3080e2523f20ebf7bac6d191c59bdea2bd81e49d` | HEVC-only keyframe/fingerprint HLS pilot; source-dependent and no accepted Hisense validation |
| Backend [#478](https://github.com/hotzenplotz5/vdr-suite/pull/478) | `work/codec-reference-corpus-20261013` @ `ebf300532cc749521c7e6c1c3aef5794c5f9f239` | Offline reference downloads/comparisons, based on #477; opt-in experimental script, not regular server playback |
| Backend [#472](https://github.com/hotzenplotz5/vdr-suite/pull/472) | `work/playback-fast-start-gated-b20261012` @ `663d5c5086ebe995d9dbe11e3f917e1860c59d62` | Fast-start tuning behind an opt-in flag; shorter copied-video opening segment not proven independently decodable |
| VIDAA [#13](https://github.com/hotzenplotz5/vdr-suite-vidaa/pull/13) | `work/vidaa-hevc-mse-explicit-b20261011-25` @ `70fd40e9e24d67be777d90728ba91d1a21515689` | Explicit source-HEVC copy MSE trial, not the default compatible-codec path; no general HEVC acceptance |
| VIDAA [#18](https://github.com/hotzenplotz5/vdr-suite-vidaa/pull/18) | `work/fix-hevc-uhd-capability-20261013` @ `2976df92b45f554907e53a27e46a50e91ca25294` | Frame-drop instrumentation and a guarded TV-deploy experiment; never treat as an authorized deployment |

Experimental branch heads above are archive references and must not be reset, force-pushed or deleted as part of routine integration.

## Preserved HEVC evidence

See [the source-specific runtime findings](hevc-solo-medicus-runtime-findings-20261013.md) and [earlier investigation](hevc-recording-framerate-investigation.md), plus the appendix in VIDAA `docs/client-integration-playbook.md`. The five bounded Medicus II 10-second windows included PTS/DTS irregularities and repeated frames in the final sampled window; neither observation alone establishes that remux, decoder, hardware or source is the root cause of subjective judder. The Solo incorrect-duration observation is separate. No recording `info`, `index` or `.ts` data should be altered on that evidence.

## Bounded H.264 HLS first-segment acceleration (14 October 2026)

**Operator evidence, installed yaVDR backend (SHA-256 `65a91d6c71301c9548f3ed70f35e01389361cdc38284104d62586882c0d5ca93`):**

- The VIDAA app previously measured one `CREATE=4238 ms`. The operator subsequently collected 12 redacted server startup logs (10 HLS-fMP4, 2 progressive-fMP4). HLS `provision_ms` was `4716,6991,4111,4375,3948,4143,3880,4668,4640,4717`; numerous runs had `probe_cache=hit`, `descriptor_ms=0`, `selection_ms=0`, `session_ms=15–63` but ~4 seconds in `provision_ms`. For example, total `4152 ms` with `provision_ms=4111 ms`. Progressive fMP4's unrelated `provision_ms=6` only measures FIFO provisioning, **not** TV decodability (progressive playback previously failed `NotSupportedError` on the Hisense).
- Source-verified causal gate: `core/media/src/FfmpegHlsCommandBuilder.cpp` specifies `-re` and four-second `-hls_time` for Recording input; `RecordingMediaSessionRuntime::defaultReady` requires HLS playlist, init and first complete fMP4 segment before `CREATE` succeeds. Thus the roughly 4-second HLS delay is explained by source-rate input and complete-segment readiness, although the source-specific GOP/worker-start contribution is not separated by the existing logs.
- **Confirmed host prerequisite:** The operator's `ffmpeg version 8.0.1-3ubuntu2` prints `-readrate_initial_burst <seconds>` in `-h full`. This option is not universal on older Debian/Ubuntu FFmpeg versions; **mandatory compatibility check before any installation**, not an assumption from build success.

**Backend candidate on existing PR #471 integration branch** `work/device-media-profile-binding-v1-20261012`:

- Selectively request `-readrate_initial_burst 8` in the HLS command plan **only** for Recording (not Live) HLS-fMP4 at `startPositionSeconds=0` with **H.264 stream-copy video**. Keep `-re`, `-hls_time 4`, `split_by_time`, source codec, protected session/grant, ffconcat workspace, worker management, media format and existing server readyness checks unchanged. Never extrapolate to MPEG-TS, HEVC, transcoded video, non-zero seek/resume, progressive or Live-TV. Input-burst value is a hardcoded bounded proof candidate; not a user/client-controlled API request.
- `core/media/tests/test_ffmpeg_hls_command_builder.cpp` must prove the startup option is before input `-i`, remains paired with `-re`, and is absent for disallowed profiles. Initially a **source/CI candidate**; the bounded daemon-only installation below is now operator-confirmed. No additional rebuild, package change, or installation is authorized without another gate. A working command-line parameter plus successful CI does not prove the first segment is emitted quickly or that the Hisense plays smoothly afterward. Seek/Track/Stop/Revoke must remain valid.
- **Release gate:** Check the installed FFmpeg feature, compare real startup logs before and after for `provision_ms`, `total_server_ms`, `probe_cache`; measure first frame and a 300-second no-pause/no-seek window using the already installed VIDAA `b20261014.3` meter (separate `P0` versus `TV-Warte`). Do not blindly recompile an already verified daemon. The code PIN below is **now deployed** as a daemon binary; a speed improvement is still **not established** without real post-install playback timing.

## Operator-confirmed bounded backend rollout (14 October 2026; hardware pending)

The operator ran the existing `tools/install_daemon.py` in the root shell, with the source checkout and compile under `yavdr`, without any global ownership or permission changes. **Observed final log:** `DAEMON_INSTALLATION=PASS`; `DAEMON_INSTALL=PASS`; `DAEMON_STOP=START`; service restarted by the installer and confirmed active. This is a real host rollout, **not** a VIDAA playback/speed acceptance.

- **Code PIN / running daemon:** `6bda8084bf9ebb6e639f15dd9454e24cd424bf29` (PR #471 existing integration branch; final reviewed documentation HEAD `7bc1085a5af217daee32da996ba3879c8a317631` is a docs/AGENTS-only descendant). `RUNNING_SHA256=575e6b1fddecfbaf659f8769a9bca9194e2acb9f64192ed14562905c142fb5d5`.
- **Provenance:** installed previous commit `981086eadd48806f01dcf8058b1160c6b1d70374`; full checked ancestry; read-only check `CHECK=PASS_NO_CHANGES`; `DB_BACKUP_DECISION=NOT_REQUIRED`; old hash `65a91d6c71301c9548f3ed70f35e01389361cdc38284104d62586882c0d5ca93`.
- **Build:** `BUILD=NOT_SEALED` initially, single unprivileged compiled `BUILD=PASS`, then `BUILD=REUSABLE` on final check and installer apply. **Do not build this code PIN a second time.** Checkout and sealed candidate are preserved at `/home/yavdr/vdr-suite-daemon-install-uyqP3jag/repo/.build/daemon-installer/6bda8084bf9ebb6e639f15dd9454e24cd424bf29/`. Previous daemon binary/rollback receipt are retained by the installer in that candidate directory; do not remove or overwrite.
- **Dependencies:** Host FFmpeg `8.0.1-3ubuntu2` and `-readrate_initial_burst` CLI availability were verified before deployment.
- **Next evidence:** Start a supported H.264 recording (e.g. `Action → Jumper`) from installed VIDAA `b20261014.3`; compare after-deploy `recording media startup` for `profile=hls-fmp4`, `probe_cache`, `provision_ms`, `total_server_ms` with the above baseline. Keep session IDs redacted. Record user-observed time to first picture, any audio/video glitch, and five-minute `P0` and `TV-Warte` event counts/durations. **Pending** until supplied by operator; do not invent performance speedup or transcode/HEVC success.
- No additional daemon rebuild/restart or VIDAA reinstall is required merely to collect these measurements.

## Post-install HLS startup logs: no acceleration demonstrated (14 October 2026)

After the successful daemon-only rollout of code PIN `6bda8084bf9ebb6e639f15dd9454e24cd424bf29`, the operator collected three real `profile=hls-fmp4` startup lines from `vdr-suite-daemon.service` using a read-only, session-ID-redacted journal query:

| Sample | probe_cache | descriptor_ms | selection_ms | session_ms | provision_ms | total_server_ms |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1 | miss | 144 | 20 | 126 | 4422 | 4714 |
| 2 | miss | 148 | 30 | 28 | 4843 | 5051 |
| 3 | miss | 313 | 279 | 16 | 4424 | 5035 |

**Result:** `provision_ms` remains around 4.4–4.8 seconds, consistent with the earlier ~3.9–4.7 second baseline. No real backend speedup can be claimed. Three post-install samples do not isolate the recording, codec or type of video transformation; all were `probe_cache=miss`, unlike several prior cache-hit samples. Do **not** treat `profile=hls-fmp4` as proof of H.264 stream-copy: `MediaPresentationSelector::remuxOrTranscode` can select `videoAction=Transcode` while keeping the same HLS-fMP4 profile ID. The bounded `-readrate_initial_burst 8` applies **only** to `videoAction=Copy`, `targetVideoCodec=H264`, zero-offset Recording HLS-fMP4.

**Next minimal read-only diagnosis:** While the supported test recording is actively playing, inspect the corresponding `/usr/bin/ffmpeg` argv (e.g. `ps -eo pid=,args=` filtered for `/usr/bin/ffmpeg` and `input.ffconcat`); determine whether `-readrate_initial_burst 8` and `-c:v copy` are present, or whether `-c:v libx264` / `h264_vaapi` indicates video transcoding. Do not log source paths/credentials or change media workers. If the burst is absent because of transcoding, measure and design a **separate bounded transcoding-specific** experiment before code changes; do not broaden this gate blindly. If the burst is present, next investigate first-segment readiness/GOP/FFmpeg timeline rather than claiming transcoding. No new checkout, rebuild, service restart, TV reinstall or branch is needed for this test.

## Follow-on release gates

1. Verify backend #471 and VIDAA #17 exact current HEADs and their current CI together; re-run changed-scope tests if either advanced.
2. Check their Device session, codec capability, HLS-fMP4 init, JPEG trickplay and revocation contracts against each other; failing conditions must fail closed rather than silently claim playback.
3. On a real paired VIDAA device, test one H.264, MPEG-2 and HEVC source via **one normal Play action** and measure first picture, correct picture/sound, seeking, thumbnail timing, and failure presentation. Separately measure whether the server can transcode each incompatible source in real time; a green CI run does not establish CPU/GPU throughput.
4. Only after authenticated integration and actual playback acceptance discuss merging either PR into its repository's `main`. Never infer that a successful isolated build authorizes a host install.
