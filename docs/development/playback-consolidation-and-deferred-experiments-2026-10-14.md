# Playback integration: consolidated history and deferred experiment archive (2026-10-14)

Status: **source-only integration, CI evidence available, no runtime/TV acceptance**.
This document is a durable review index; it authorizes no VDR, daemon, TV, permission, package or production change.

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

## Follow-on release gates

1. Verify backend #471 and VIDAA #17 exact current HEADs and their current CI together; re-run changed-scope tests if either advanced.
2. Check their Device session, codec capability, HLS-fMP4 init, JPEG trickplay and revocation contracts against each other; failing conditions must fail closed rather than silently claim playback.
3. On a real paired VIDAA device, test one H.264, MPEG-2 and HEVC source via **one normal Play action** and measure first picture, correct picture/sound, seeking, thumbnail timing, and failure presentation. Separately measure whether the server can transcode each incompatible source in real time; a green CI run does not establish CPU/GPU throughput.
4. Only after authenticated integration and actual playback acceptance discuss merging either PR into its repository's `main`. Never infer that a successful isolated build authorizes a host install.
