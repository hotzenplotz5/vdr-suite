# Actual emitted HLS-fMP4 codec evidence — bounded init-header proof

Status: isolated internal code and test slice, not connected to the running MediaSession route, public API or TV playback. Based on the verified current Suite main after the #465/#466 merges.

## Exact question answered

The existing source `FfprobeRecordingSource` describes **what is in the VDR Recording**. The existing `MediaPresentationSelector` and `MediaPlannedOutput` describe **what the server intends to emit**. A third, separate fact is needed: **which stream codec identities are actually signaled in the emitted fMP4 initialization header**.

`MediaEmittedFmp4Probe` inspects an already produced `init.mp4` in a trusted MediaSession workspace and compares its declared stream codecs, dimensions and audio channels against the internal selected output plan.

Example:

| Evidence | Video | Audio | Meaning |
| --- | --- | --- | --- |
| Recording source | HEVC Main10 | AC-3 5.1 | Trusted bounded source probe |
| Presentation plan | H.264 (transcode) | AAC stereo (transcode) | Desired worker output |
| Emitted `init.mp4` | H.264 | AAC, 2 channels | What the generated init header **signals** |
| Hisense hardware outcome | Separate user/hardware test | Separate user/hardware test | Not inferred from this probe |

This is **not a decoded frame probe**, nor a verified HDR output or authenticated device report. An fMP4 init header can be valid even if later media segments fail. Thus this result is called `inspected` and `matchesPlannedOutput`, **never** `VerifiedPlayback`.

## Security and resource fences

1. The caller must be the already authorized internal MediaSession owner. No public endpoint or path parameter is exposed. Paths from a client, provider URL or live-TV socket are categorically not accepted.
2. `inspectOwnedWorkspace` requires an absolute, private (0700), current-UID directory. It uses `openat` with `O_NOFOLLOW|O_NONBLOCK` for `init.mp4`, requires a regular single-link file owned by the current process, checks stable inode/size/mtime, and limits input to 256 KiB.
3. The header bytes are copied to a new temporary 0700 snapshot; `ffprobe` runs only on its fixed relative `init.mp4`, with `file` as the sole allowed FFmpeg URL protocol. It does not read any VDR `00001.ts`, MediaSession secret, manifest, growing recording, or remote URL.
4. `MediaProcessRunner` applies a 3-second wall timeout, 2-second CPU limit, 512 MiB address-space limit and 8 KiB output limit; argv is fixed and never shell-composed from client input.
5. The parser rejects unknown codecs, codec/tag disagreement, malformed or duplicate fields/streams, stream-count mismatch, unknown dimensions/channels, oversized output and unexpectedly absent tracks. It only compares positive evidence against a trusted `MediaPlannedOutput`, and cannot enable a new selected codec.
6. No filesystem changes in the original Session workspace; private snapshots are removed on all normal paths. No playback start, production initialization probe or retry is wired by this slice.

## Regression proof

The existing `test-phase65-media-capability-negotiation` Make target compiles the inspector, exercises positive/negative parser tests, and creates a **synthetic** 320x180 H.264/AAC HLS-fMP4 recording with FFmpeg inside a temporary directory. It then reads the emitted real `init.mp4` with the exact fixed `ffprobe` process, confirms codec/dimensions/channels, refuses symlink substitution and rejects corrupted initialization data. This test has no credentials or production VDR inputs.

## Next integration gate

Before wiring to a live MediaSession, explicitly review: which protected sessions may schedule this operation, when the worker has safely published the immutable init header, rate/budget accounting, cleanup on timeout, timeout behavior and how plan mismatches are surfaced only to authorized administrators. The current protected output delivery and Device grants must stay unchanged. Never use this result alone to enable HEVC Direct Play, transcode cancellation, codec-profile fallback or HDR certification. For complete output verification, a later bounded test must also validate a media segment and correlate the authorized decoder's actual picture/sound result.
