# HEVC HLS keyframe segmentation trial — 2026-10-13

## Branch registry (do not lose branch provenance)

| Scope | Repository | Branch | Status |
| --- | --- | --- | --- |
| HEVC VIDAA capabilities, installed b20261013.2 | hotzenplotz5/vdr-suite-vidaa | `work/fix-hevc-uhd-capability-20261013` | TV accepted for Solo 4K Main10 smooth video-copy; seek and audio track switching still unsupported |
| HEVC HLS keyframe boundary experiment | hotzenplotz5/vdr-suite | `work/hevc-hls-keyframe-segmentation-trial-20261013` | Backend-only candidate, NOT installed or hardware tested |
| Backend baseline | hotzenplotz5/vdr-suite | `main` at `ed10efa72ba018330d884d86e99640ace347f873` | Source parent of experiment; NOT claimed to equal installed daemon |

## Observed evidence

- Medicus II source HEVC Main, 1920x800, AAC 6 channels, average 24 fps; reported severe stuttering on the Hisense despite video copy.
- Runtime FFmpeg uses `-re -c:v copy -c:a aac -ac 2 -hls_time 4 -hls_flags delete_segments+split_by_time+temp_file`.
- The observed fMP4 segments 223 and 225 had no keyframe, segment 224 had a keyframe 0.86s after its start. Source has approximately one keyframe per second.
- These findings establish non-keyframe-aligned segmentation, not that it is the unique cause of stuttering. The decoder may retain reference pictures across fragments.
- Solo 3840x2160 HEVC Main10 has smooth playback on installed VIDAA b20261013.2, but startup is approximately five seconds and seek/audio selection are not yet working.

## Experimental scope and acceptance

The experiment changes **only** the H265 fMP4 video-copy command to keyframe-oriented FFmpeg HLS segmentation; it does not remove `-re`, force encode, loosen resume sync safety, change MPEG-TS or H264 copying, or install a daemon. Unit coverage checks all relevant argv and denial of copy resume.

Next evidence is an isolated media packaging test that compares actual fMP4 fragment boundaries and playlist/segment retention for both Medicus II and Solo, followed by an explicitly approved hardware acceptance of any backend candidate. `independent_segments` expresses an FFmpeg packaging assertion and is NOT independent validation that the resulting HEVC fragments start with usable random-access pictures. Do not deploy based on argv tests alone. Keep live services and recordings untouched.

## Repository workflow

Only GitHub branch commits are implemented. No root builds, chmod/chown, recording mutation, daemon restart, or live deployment has been authorized. CI and exact candidate identity must be checked before any manual build or deployment.
