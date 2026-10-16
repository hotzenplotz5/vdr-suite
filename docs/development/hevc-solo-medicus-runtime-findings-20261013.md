# HEVC runtime investigation: Solo duration and Medicus II playback (2026-10-13)

Status: **diagnostic evidence only; no source fix or runtime deployment authorized by this document**.

This extends [the earlier HEVC frame-rate investigation](hevc-recording-framerate-investigation.md). These are separate samples and symptoms. User-supplied measurements are not proof of any shared root cause.

## Sample A: Solo a Star Wars Story

Recording: `/srv/vdr/video.00/SciFi/Solo_a_Star_Wars_Story/2026-05-06.17.48.1-0.rec/`

- One TS segment (`00001.ts`, reported about 5.5 GiB).
- `ffprobe`: HEVC Main 10, 3840x2160, yuv420p10le, `r_frame_rate=24000/1001`, `avg_frame_rate=48000/1001`.
- `ffprobe` format duration `8085.828700` seconds (~2:14:45.83).
- Four AAC-LC audio streams.
- `info` contains `F 47.95204795 3840 2160 i 16:9`.
- `index` is 1,550,912 bytes = 193,864 8-byte entries.
- Index-entry duration at 47.95204795 fps is ~4042.9 seconds (1:07:22.9); at 24000/1001 fps it is ~8085.7 seconds. This matches the reported Suite duration (1:07:22) versus TS duration.
- User reproduced the approximately halved duration also in the Suite web frontend; playback was reported beyond 1:46:00 in another playback mode.

The currently checked-in `core/vdr/include/VdrRecordingDuration.h` reads the `F` line from `info` and uses `indexBytes / 8 / fps` for its duration. `core/daemon/src/RecordingMediaHttpRuntime.cpp` calls this helper. Thus the supplied index/info data are sufficient to **reproduce** the halved duration calculation. They do *not* identify which importer or VDR component originally wrote the wrong `F` line. Do not change recording `info` or regenerate the index on the live host to speculate.

## Sample B: Der Medicus II

Recording: `/srv/vdr/video.00/Historienfilm/Der_Medicus_II/2026-05-18.19.00.1-0.rec/`.

- User confirms playback **duration is correct**, but picture subjectively stutters, roughly as if ~15 fps; this is not an instrumented FPS measurement.
- `ffprobe` first TS: HEVC Main, 1920x800, yuv420p, `avg_frame_rate=24/1`, `r_frame_rate=1000/9`, video duration ~8596.251 s; AAC-LC audio.
- Observed FFmpeg HLS worker command: `-re -f concat -safe 1 -i input.ffconcat -map 0:v:0? -map 0:a:0? -sn -c:v copy -c:a aac -b:a 192k -ac 2 -f hls -hls_time 4 -hls_list_size 8 -hls_segment_type fmp4` and standard bounded rolling HLS flags.
- FFmpeg stderr is the per-session `ffmpeg.log`, not necessarily systemd warning journal.
- FFmpeg log repeatedly records `Non-monotonic DTS` with equal previous/current ticks and `Packet duration: -3749 / dts: ... in stream 0 is out of range`.
- Playlist contained ~4.005/3.996-second segments; this only proves segment duration presentation, not smooth client decode.
- `-c:v copy` confirms no FFmpeg video re-encode for this exact session. It does not prove HEVC hardware decoding or identify the precise frame-drop stage.

Do not infer that the Solo duration error causes Medicus stutter; Medicus duration is normal.

## Other reported symptoms, independently unproven

- Other HEVC titles reportedly return HTTP 422. No response body or server-side rejection path has been captured; EPG `nextOffset=4224` is unrelated.
- Older playback trick-speed behavior worked, but HEVC trick-speed did not.
- Trickplay thumbnails appeared only after completing seek rather than promptly during scrubbing.
- The experimental fast-start and Device trickplay PRs remain separate; do not equate them to verified performance gains.

## Next bounded evidence gates

1. Identify exact imported-recording `info` `F` writer and VDR index creator; find the actual frame cadence rather than indiscriminately halving rates. Add a regression test for the observed 193,864-entry/47.952-fps contract and a valid 47.952-fps counterexample before a code fix.
2. For Medicus capture bounded original-video PTS/DTS samples and emitted fMP4 samples at matched media positions, plus frame presentation/drop metrics on VIDAA if instrumentable. Determine whether timestamp defects preexist in the TS or arise in concat/fMP4 remux.
3. Capture one HTTP 422's response body and exact route/validation branch, redacting credentials.
4. Instrument Trickplay request-to-render-to-TV-display separately from HLS startup and seek completion. Do not claim speed improvement based on thumbnails merely being generated.
5. Preserve existing source files, index, info, grants and running services. No `sudo`/root writes, no production restart, no general codec policy changes without test proof.

Repository starting point: `main` at `ed10efa72ba018330d884d86e99640ace347f873`; this note is **not** evidence that any candidate code is deployed.
