# Phase 65 copied-recording HLS segmentation

This Phase-65 maintenance follow-up keeps browser playback of directly copied H.264 recordings on the same four-second delivery cadence as transcoded recordings.

## Root cause

The recording `/Krieg/1917/2026-05-12.10.59.1-0.rec` is progressive H.264 and therefore correctly selected the video-copy path. The existing HLS command still advertised a four-second target while also requiring independent segments. Because copied video cannot create new keyframes, FFmpeg had to cut at source GOP boundaries instead of the four-second cadence.

A 120-second reproduction on the real recording produced copy-path segment durations ranging from 1.043 to 11.762 seconds, including a 27 MB segment. With paced input (`-re`) and `temp_file`, a long segment is not published until it is complete; this can drain the browser's twelve-second forward buffer even though the producer itself is healthy.

The same source with `split_by_time` produced segments consistently around four seconds while completing successfully.

## Policy

- Video-copy HLS uses `delete_segments+split_by_time+temp_file`.
- Video-transcode HLS keeps `delete_segments+independent_segments+temp_file` because the encoder already forces keyframes every four seconds.
- Copy segments are not advertised as independent because their boundaries may not start on source keyframes.
- Playback errors remain terminal in the UI and are not overwritten by a later `waiting` event.

## Acceptance

Real yaVDR validation must confirm that `/Krieg/1917/2026-05-12.10.59.1-0.rec` continues playing over the browser transport without the previous rebuffer stall, while the live worker command uses `split_by_time` and the sliding playlist remains close to four-second `EXTINF` durations.
