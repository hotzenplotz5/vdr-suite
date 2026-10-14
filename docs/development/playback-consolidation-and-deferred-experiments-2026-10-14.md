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

## Live FFmpeg argv evidence: startup burst actually selected (14 October 2026)

During a real ongoing recording session, the operator checked the running `/usr/bin/ffmpeg` process (PID 29894) and returned this **read-only, argument-filtered evidence**: `-readrate_initial_burst 8`, `-re`, `-c:v copy`, `-c:a copy`, `-hls_time 4`. **The H.264 stream-copy startup optimization is definitely selected for that worker**, so the earlier suspicion of silently falling into video transcoding is ruled out for this observed session. The previous three post-rollout `provision_ms=4422,4843,4424` samples are uncorrelated to the worker PID, so do not assert they represent this very same session. The still-slow measured startup and selected option can coexist; CLI option presence does not prove its input-timestamp or muxer-readiness effects. Official FFmpeg docs specify an initial burst **in input media seconds**, after which `-re` takes effect. They do not guarantee a completed HLS segment by a wallclock deadline.

**Next diagnosis** (read-only, no new daemon binary): compare one clearly correlated startup session's `provision_ms` and first segment characteristics, including first `#EXTINF` duration / timestamps and any `ffmpeg.log` warnings from the existing worker workspace. Use `/proc/<verified PID>/cwd` solely as a read-only handle; do not publish recording/credential/source paths or dump the unredacted ffconcat list. To isolate the 4-second delay, inspect media input PTS/DTS and muxer first-segment completion rather than changing output segment boundaries or unconditionally removing `-re`. Preserve exact source PIN and prior built artifact; **no speculative second build, restart, permissions operation, or new branch**.

## Readiness isolation: completed first-segment timeline versus FFmpeg probing (14 October 2026)

Independent, **isolated container reproduction** (not yaVDR hardware), using a generated 14-second H.264/AAC MPEG-TS input, an `input.ffconcat` with the same relative source-name layout, and the server's HLS-fMP4 command options (`-hls_time 4`, `delete_segments+split_by_time+temp_file`, stream-copy and video/audio bitstream filters):

| Input pacing | First nonempty `init.mp4`, `segment-000000.m4s`, `master.m3u8` observed | First `#EXTINF` |
| --- | ---: | --- |
| `-re` only | 3648 ms | 4.000000 s |
| `-readrate_initial_burst 8 -re` | 129 ms | 4.000000 s |

Both generated HLS segments were four seconds in *media time*, proving that a four-second target segment does **not** inherently require four seconds of elapsed wall time when FFmpeg can read its input rapidly. These numbers are **not** a measurement of any real VDR recording or its CPU/I/O/GOP/PTS behavior, and may differ with the installed FFmpeg 8.0.1.

**Candidate underlying causes (not yet distinguished):** A real VDR recording's TS probe/concat initialization, packet DTS/PTS irregularities, keyframe cadence or HLS first-segment cutover can all affect first-segment readiness; without more evidence, none may be labeled the confirmed bottleneck. The operator's live worker argv already confirmed initial-burst plus copied video/audio, but did not time the appearance of `segment-000000.m4s.tmp` or its final rename. In particular, the concat workspace uses a list of source symlinks and the HLS path does not supply explicit ffconcat per-TS durations; any expensive probing would occur **inside FFmpeg**, not the server's already measured descriptor-cache phase.

**Next minimal host measurement, purely read-only:** Start a bounded 50-second observer in the existing root shell *before* opening `Action → Jumper`. It must ignore pre-existing workers, select only a newly started Recording worker with `-readrate_initial_burst 8` and `-c:v copy`, use that process's `/proc/<pid>/cwd` to watch only static workspace filenames, and report elapsed time after discovering the worker for `ffmpeg.log`, `init.mp4`, `segment-000000.m4s.tmp`, `segment-000000.m4s`, `master.m3u8`. Print only the first `#EXTINF` duration, not the playlist URLs, source path, ffconcat contents or sensitive log lines. A later post-test `journalctl` query must redact the session ID. If the worker appears too late/does not appear, label the window inconclusive. Do not run synthetic FFmpeg transcodes on real recordings or change file ownership, permissions, daemon, playback or any live service. **A read-only monitor cannot by itself measure exact FFmpeg spawn time; 50–150ms polling error applies.**

## Real recording readiness observation: 3738 ms after new worker detected (14 October 2026)

The operator ran the bounded read-only PID/workspace observer for a **new** ffmpeg `/usr/bin/ffmpeg` process after initiating `Action/Jumper` on the real VIDAA device. Observed results:

- `MONITOR=READY`, `NEW_WORKER=YES`, `RESULT=DIAGNOSIS_COMPLETE`.
- `INIT_MS=3737`; `SEGMENT_READY_MS=3738`; `PLAYLIST_MS=3738`. Values are measured from first discovery of the new worker, **not** fork time. The files became visible within 1 ms of one another at the observer's ~50-ms sampling resolution; do not infer when FFmpeg first read or wrote packets.
- `SEGMENT_TMP_MS=NOT_OBSERVED` (can be short-lived between two polls, not evidence of missing intermediate output).
- First HLS segment `#EXTINF=4.004000 s` in media time. `TIMESTAMP_WARNING_COUNT=0` from a keyword scan of the most recent bounded 16 KiB of worker warnings. **No printed warnings does not prove valid/monotonic packet timestamps.**
- The subsequent five-minute-window journal query produced **no matching server startup line**, so this exact session's `provision_ms` was not independently correlated with the above PID. Earlier post-rollout samples `4422,4843,4424 ms` remain valid but not tied to this worker.

Interpretation: All three readiness resources appeared together ≈ 3.7 s after new-worker detection, rather than the backend delaying several additional seconds *after* the first HLS segment became available. The four-second media segment length **alone** is insufficient to explain this wall-clock wait: the synthetic ffconcat/TS counterexample completed the same four-second segment in `129 ms` with the same bounded burst. Potential hidden causes include FFmpeg TS/concat probe initialization, packet timebase/PTS/DTS, GOP/keyframe scheduling, and media I/O. No individual cause is yet proven.

**Next narrow, read-only field observation:** While the specific recording session is alive, use the already verified process's `/proc/<pid>/cwd/input.ffconcat` in a bounded, timeout-protected `ffprobe` reading only the **first eight media seconds of video packets** (no decoding/transcoding/writes). Report first video PTS/DTS, first keyframe offset relative to first packet, number of packets, negative or non-monotonic DTS deltas and probe wall-clock latency. Never reveal VDR source paths or dump the ffconcat manifest. If the worker/session does not exist, stop without touching production. This test separates early packet/keyframe anomalies from a slow initial probe. **No code change or daemon rebuild is justified yet**.

## Real Source Probe: fast TS and ffconcat packet access, slow HLS first segment (14 October 2026)

The operator ran the previously supplied bounded read-only `ffprobe` command **during** `Action/Jumper` (after new FFmpeg worker discovery). Real yaVDR results:

| Probe | Wall clock | Video packets in first 8 input-media seconds | First video PTS | First video DTS | First keyframe after first video packet | Backward DTS | DTS gap > 0.5 s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| direct `source-000001.ts` | 147 ms | 189 | 1.483 s | 1.400 s | 0 ms | 0 | 0 |
| `input.ffconcat` | 150 ms | 189 | 0.026 s | -0.057 s | 0 ms | 0 | 0 |

**Interpretation/limit:** Direct input and concat video packet probes differ by only 3 ms; first video keyframe is present in both and no measured video DTS discontinuities were detected. Relative timestamp rebasing through concat is expected; the values do **not** prove an error. `ffprobe` neither creates nor waits for a 4.004-second HLS segment, so a 150-ms source probe completing far before TV first picture is entirely expected. **Do not infer that the production FFmpeg input/output pipeline must therefore take 150 ms.** These probes selected only `v:0`, and did not measure audio/subtitle packet timing, muxer interleaving, per-input readrate sleeps, or FFmpeg CPU and I/O.

The prior independent new-worker observer found `init.mp4`, `master.m3u8` and `segment-000000.m4s` together after ~3738 ms. FFmpeg's official CLI documentation confirms `-re` sets readrate=1 and `-readrate_initial_burst 8` postpones throttling for 8 seconds of **input media time**, but the presence of the option in a process argv alone cannot establish its runtime pacing.

**Next safe isolation, no extra decoder/transcoder or media writes:** During one new HLS worker, sample `/proc/<pid>/io` `rchar` alongside the presence of readiness files in the worker's existing cwd at 100-ms to 1-s intervals for up to 6 seconds. If input bytes advance rapidly in the first second but readiness arrives at 3.7 seconds, investigate FFmpeg output scheduling/muxer; if bytes accumulate gradually, investigate actual readrate/throttle or source I/O. `rchar` is a process-wide byte counter, not a per-stream byte counter; CPU and asynchronous I/O can confound correlation. Do not dump source paths, ffconcat content, log lines, credentials, record filenames or record source bytes. No new branch, binary rebuild, service restart, permissions changes or modifications to VDR recordings.

## Real FFmpeg worker read-byte timeline: first HLS files after 3.775 s (14 October 2026)

New operator read-only monitor, **on the installed backend daemon code PIN `6bda8084bf9ebb6e639f15dd9454e24cd424bf29`**, waited for a **new** Recording FFmpeg worker with `-readrate_initial_burst 8` and `-c:v copy`, then sampled process-wide `/proc/<pid>/io` `rchar`. It reported:

| Elapsed since process discovery | Delta `rchar` |
| --- | ---: |
| 0 ms | 0 KiB |
| 252 ms | 696 KiB |
| 504 ms | 856 KiB |
| 1,007 ms | 1,176 KiB |
| 2,014 ms | 1,880 KiB |
| 3,020 ms | 2,552 KiB |
| 4,026 ms | 3,544 KiB |
| 5,029 ms | 4,664 KiB |
| 6,032 ms | 5,848 KiB |

The first nonempty `init.mp4`, `segment-000000.m4s`, and `master.m3u8` all appeared by `3,775 ms` after new-worker detection. `RESULT=PASS_READ_ONLY`.

**Evidence and interpretation:** Input-side read activity is gradual, not a multi-megabyte burst in the first 250 ms. `rchar` is **process-wide** and counts all successful read-system-call characters, including format probing and logs; it does not distinguish a demuxer rate sleep, output backpressure, media I/O, audio/video interleaving or buffering. It does not time precisely when the first TS packet entered the muxer. Do **not** label `-re` a proven root cause merely from read rate or suggest disabling it universally.

**FFmpeg 8.0 source audit:** `fftools/ffmpeg_demux.c::readrate_sleep` computes `max_pts = stream_ts_offset + initial_burst + wc_elapsed * readrate` for each non-discarded stream and sleeps only if stream `pts > limit_pts`. With an 8-second burst, normal video timestamps in the first four seconds should not cause a sleep; anomalous audio or other selected stream timestamps **could** still participate. See FFmpeg 8.0 source `https://ffmpeg.org/doxygen/8.0/ffmpeg__demux_8c_source.html` around `readrate_sleep`. The earlier 8-s video-only `ffprobe` does **not** reveal selected audio-stream DTS behavior. This is now the highest-value untested branch.

**Next read-only gate:** From the live FFmpeg worker's actual `-map 0:a:N?` selection, inspect only first 8 media seconds of audio packet PTS/DTS using a bounded `ffprobe` (direct TS and ffconcat side-by-side, 12-s timeout each) and report aggregate packet count, first/min/max DTS, maximum DTS step, backwards jumps, and packet probe duration, without exposing source paths or recording bytes. A normal audio timestamp sequence would rule against an initial-8-second audio DTS anomaly; any remaining blocked path then warrants targeted real FFmpeg demux/mux scheduling analysis rather than a speculative daemon rebuild. No edits or restarts.

## Real audio timestamp acceptance and upstream FFmpeg 8.0 input burst bug (14 October 2026)

**Host audio source evidence, matched to new HLS FFmpeg worker:**

| Probe on first 8 media seconds | Direct MPEG-TS | FFconcat |
| --- | ---: | ---: |
| Selected audio stream | 0 | 0 |
| Probe wall time | 140 ms | 151 ms |
| Packets | 376 | 376 |
| First PTS/DTS | 1.457 / 1.457 s | 0.000 / 0.000 s |
| Minimum DTS | 1.457 s | 0.000 s |
| Maximum DTS | 9.456833 s | 7.999833 s |
| Backwards DTS | 0 | 0 |
| Maximum consecutive DTS step | 0.021334 s | 0.021334 s |

Together with the previously normal video PTS/keyframe and `150ms` concat probe, **no timestamp discontinuity in the inspected initial audio/video window** explains the four-second startup. This cannot exclude internal pacing, uninspected later packets or FFmpeg scheduling.

**New high-value upstream causal hypothesis:** FFmpeg 8.0 `fftools/ffmpeg_demux.c::readrate_sleep` used `max_pts = stream_ts_offset + initial_burst + wc_elapsed * readrate`, then treated the initial positive difference as input "lag" and switched to the **catchup** limiter. Without a guard to exempt the startup window, the normal `-readrate_initial_burst 8` may thus read at near-real-time anyway if `-readrate_catchup` defaults near `readrate * 1.05`. FFmpeg release/8.0 upstream commit `96cfa53add31bf5d7a49559b2df44e07afd9ff2c` dated **2026-06-29** explicitly fixes "readrate to respect initial burst" by skipping the loop while `pts <= stream_ts_offset + initial_burst` (issue #21510). Primary source: https://www.mail-archive.com/ffmpeg-cvslog@ffmpeg.org/msg74697.html . Refer to FFmpeg 8.0 `ffmpeg_demux.c` source https://ffmpeg.org/doxygen/8.0/ffmpeg__demux_8c_source.html . **Do not confuse presence of CLI option with correct implementation.**

**Ubuntu provenance:** Operator installed `ffmpeg 8.0.1-3ubuntu2`; Ubuntu's published source package and binary date are 2026-02-17, four months *before* the fix was backported to FFmpeg's 8.0 release branch. Official Ubuntu mirror index: https://ftp.ubuntu.com/ubuntu/pool/universe/f/ffmpeg/ . This strongly suggests the affected default code was shipped, **but Ubuntu/Debian packaging might independently cherry-pick the fix**; neither the binary's exact behavior nor source patches have yet been established on host. Do not assert patched/unpatched based on the release date alone. Likewise do not install FFmpeg 8.1 packages from another Ubuntu series, switch OS release, change global `/usr/bin/ffmpeg`, or rebuild daemon without controlled evidence.

**Minimal isolated decision gate:** On the installed ffmpeg binary, create **only a short, synthetic 10-second low-resolution MPEG-TS test inside an unprivileged temporary directory owned by `yavdr`**, then compare two bounded output-to-null runs (`-readrate_initial_burst 8 -re` versus `-readrate_initial_burst 8 -readrate_catchup 8 -re`), reading 4 media seconds via `-t 4`. No reading/decoding of real recordings, no new service media worker, no stored output outside temp, and no package changes. If first run takes roughly four wall seconds and second under one, the old limiter defect and a potential bounded `-readrate_catchup` compatibility workaround are reproducibly shown *on this host*. If both are fast, do **not** claim the bug on this host; revisit worker timeline/muxer instead. If either fails, diagnose exact error before any source change. Benchmark is a production decision gate, not permission to silently change the playback worker's flags.

## FFmpeg 8.0.1 hosted readrate-catchup reproduction and narrow source fix (14 October 2026)

The operator ran a **non-destructive, unprivileged synthetic MPEG-TS comparison on their installed FFmpeg 8.0.1-3ubuntu2**. A 10-s low-resolution TS fixture was generated in a temporary `yavdr`-owned directory; two bounded four-second output-to-null tests succeeded:

| Identical synthetic TS input and `-re` | elapsed wall clock | Exit |
| --- | ---: | ---: |
| `-readrate_initial_burst 8` (previous H.264 recording startup options) | **3992 ms** | 0 |
| `-readrate_initial_burst 8 -readrate_catchup 8` | **641 ms** | 0 |

Both return codes are 0; `RESULT=CATCHUP_ACCELERATION_CONFIRMED`. This directly demonstrates a *real speed difference caused by the catch-up setting on the installed FFmpeg binary*, in agreement with the 2026-06-29 FFmpeg 8.0 upstream readrate initial-burst correction; it does not require speculative claims about Ubuntu's vendor patch history. The synthetic test was never run on the operator's actual recordings and did **not** measure a full real fMP4 segment, first TV picture, or 300-s no-stall playback. Those acceptance steps remain open.

**Source implementation on the same existing backend PR #471 integration branch:** `core/media/src/FfmpegHlsCommandBuilder.cpp` now places `-readrate_initial_burst 8 -readrate_catchup 8` directly before the existing `-re -f concat -safe 1 -i input.ffconcat` **only** for Recording HLS-fMP4 `startPositionSeconds==0`, video `MediaTrackAction::Copy`, output H.264. Retains `-re`, `-hls_time 4`, fMP4 format, A/V copy/transcode policy, grant/session lifecycle, existing playlist/first-segment readiness gate and existing 8-segment buffer/deletion rules. HEVC, MPEG-TS HLS, video transcode, non-zero seek/resume, Live-TV and progressive playback **must not** inherit this extra rate setting. The paired catch-up limit allows bounded recovery to 8x when the FFmpeg 8.0 bug otherwise throttles the initial burst; it does **not** remove all readrate pacing. `core/media/tests/test_ffmpeg_hls_command_builder.cpp` checks the positive option pair, its input placement and exclusions (including H.264 copy/AAC-transcode and both stream-copy).

**Host version gate:** The exact yaVDR `ffmpeg -h full` output already established support for both options. Other Debian/Ubuntu releases may not have this combination, so package runtime compatibility must be checked before any future deployment; do not globally upgrade or replace FFmpeg to force this candidate. **Historical status before installation (superseded by the verified deployment below)**: The previously running daemon was SHA `6bda8084bf9ebb6e639f15dd9454e24cd424bf29` with ELF SHA-256 `575e6b1fddecfbaf659f8769a9bca9194e2acb9f64192ed14562905c142fb5d5`. At that time, this new source code had not yet been deployed. Do not restart, rebuild twice, alter permissions, touch recordings or create another branch. The previously installed protected rollback copy must be retained, and any future new-code build/apply requires the documented exact-code-PIN, DB-risk, resource, unprivileged build/reuse and root-only apply gates.

**Post-install acceptance when separately authorized:** On the same real Jumper recording measure `provision_ms` / `total_server_ms`, fMP4 first segment readiness and first picture; run one 300-s no-seek/no-pause session and record `P0`, `TV-Warte`, dropped frames, buffer and seeking/trickplay regression. Compare with documented pre-change `provision_ms` ~3.9–4.8 s; never extrapolate the synthetic 641ms number to real hardware.

## yaVDR deployment and actual Hisense HLS-fMP4 fast-start acceptance (14 October 2026)

**Exact installed and running code:** Source PIN `ebc52d4385bd06d3008653ba4163c79280356d88` (product-code commit, preceding documentation-only HEAD `666b1aba464c24a1940bcf56524d00b93c5aaf50`). The existing guarded `tools/install_daemon.py` workflow completed once: `BUILD=PASS`, then `BUILD=REUSABLE`, `DB_BACKUP_DECISION=NOT_REQUIRED`, `DAEMON_INSTALL=PASS`, `DAEMON_INSTALLATION=PASS`. Installed daemon reported binary SHA-256 `b2e4b6b1cf744fe9258d3c108a0b9ed1e9d2608c6adf672feb0000909b7d0a77`. Independent `systemctl`/`/proc` operator check showed `active` / `running`, PID `33415`, `/usr/sbin/vdr-suite-daemon`, and the same running ELF SHA-256. DB backup was not required by the installer's reviewed risk gate; the presence of a specific rollback file has **not** been independently inventoried. No second build, apply or restart is justified for these unchanged bytes.

**Actual recording media start, pre-/post-change:** Earlier real-session `provision_ms` samples: **4422, 4843, 4424 ms** (mean **4563 ms**), with `total_server_ms` **4714, 5051, 5035 ms**. New `Action → Jumper` HLS-fMP4, `start_position=0` sessions: `provision_ms=740`, `total_server_ms=1106`, `probe_cache=miss`; and `provision_ms=659`, `total_server_ms=695`, `probe_cache=hit`. New mean provision time **699.5 ms**, about **84.7% lower**, or about **6.5× faster** across these small, non-paired samples. Both miss and hit cases were sub-second; do not attribute the entire gain to cache warming or claim guaranteed future latency. The installed `ffmpeg 8.0.1-3ubuntu2` worker PID `33500` showed `-readrate_initial_burst 8 -readrate_catchup 8 -re -f concat`, `-c:v copy -c:a copy`, `-hls_time 4`, and `-hls_segment_type fmp4` in its actual command line; this confirms the intended runtime flags for one observed worker.

**Actual Hisense VIDAA b20261014.3 display, 300-second C3 observation:** The operator photographed the on-screen diagnostic overlay during `Action → Jumper`: `5min 300/300s`, **7296 frames**, **0 dropped (0.0%)**, **P0 0 events / total 0 ms / max 0 ms**, **TV-Warte 0 events / total 0 ms / max 0 ms**, **buffer 7.1 s** at observation. The overlay also reported `CREATE 784 ms`, `sourceopen 23 ms`, `Manifest 59 ms`, `init 116 ms`, `1. Segment 432 ms`, `Play→playing 233 ms`, `HLS Sequenz 77`, `bisher kein Nummernsprung`, and `Prewarm übernommen`. Thus the observed five-minute **no-underrun/no-wait/no-drop C3 counters PASS**. The operator described first picture as *subjectively very fast*, but **actual wall-clock time to first visible picture was not measured**: `CREATE 784 ms` and `Play→playing 233 ms` cannot be substituted for it. The screenshot alone also does not independently prove the absence of all user inputs during the interval.

**Acceptance boundary:** This is one demonstrated H.264/AAC recording via the real TV, not an all-codec or release-wide production sign-off. The original 300-s baseline used 8646 frames / approximately 10.8-s ending buffer and also zero stalls/drops; the frame/buffer differences are not diagnosed and should not be interpreted as regressions without controlled like-for-like evidence. Non-zero seeking/resume, trickplay, HEVC, MPEG-2, transcoding, and Live-TV remain separate acceptance gates. Preserve the established C3 working path; no VIDAA client edit, recording/database mutation or additional daemon installation was necessary.

**GitHub provenance at acceptance:** Backend [Draft PR #471](https://github.com/hotzenplotz5/vdr-suite/pull/471) remained open on `work/device-media-profile-binding-v1-20261012` at documentation HEAD `666b1aba464c24a1940bcf56524d00b93c5aaf50`. On that exact HEAD, GitHub Actions [VDR-Suite CI run 38620200100](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38620200100) completed **success**, including packaging, fast-regression, make-test audit, docs, frontend and architecture jobs; the separate installer-safety and fMP4 evidence runs also completed **success**. This is historical CI evidence for that SHA, not an assertion about a later documentation-only HEAD. No branch merge or Draft PR state change is authorized.

## Follow-on release gates

1. Verify backend #471 and VIDAA #17 exact current HEADs and their current CI together; re-run changed-scope tests if either advanced.
2. Check their Device session, codec capability, HLS-fMP4 init, JPEG trickplay and revocation contracts against each other; failing conditions must fail closed rather than silently claim playback.
3. On a real paired VIDAA device, test one H.264, MPEG-2 and HEVC source via **one normal Play action** and measure first picture, correct picture/sound, seeking, thumbnail timing, and failure presentation. Separately measure whether the server can transcode each incompatible source in real time; a green CI run does not establish CPU/GPU throughput.
4. Only after authenticated integration and actual playback acceptance discuss merging either PR into its repository's `main`. Never infer that a successful isolated build authorizes a host install.
