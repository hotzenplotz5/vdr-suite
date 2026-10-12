# Hisense 43A6K — provisional external capability evidence

Status: research hint only, NOT a certified VDR-Suite VIDAA/MSE playback profile. Date: 2026-10-12.

## Grounded manufacturer specifications

Hisense's own 43A6K product information lists 3840x2160 display output, 50–60 Hz, H.264, HEVC/H.265, MPEG-2, MPEG-4, VP9 and AV1 video decoding; container/file format support including MP4, MKV, TS and WebM; AAC, AC-3, MP3, FLAC, PCM and Opus audio formats; HDR10+ decoding, HLG and Dolby Vision.

Source: https://qrcode.hisense.com/appliance/0000000000200118530000000000000000000?lang=en

These are device/media-player specifications, **not** proof of support in the VDR-Suite VIDAA JavaScript MSE fMP4 playback engine. The vendor product page does not establish all codec+container+delivery+audio combinations or seek capability.

## Plex comparison

- No verifiable official, model-exact Plex `43A6K` client profile was found in the public material reviewed.
- Community Plex VIDAA XML examples declare combinations including H.264/HEVC with MP4/MKV and AAC/AC-3/E-AC-3 audio, but vary by model, firmware and author. They are **third-party hints**, not a VDR-Suite runtime capability fact.
- Plex VIDAA users have reported failures for HEVC in HLS MPEG-TS while another route played, and subtitle/failure variability across VIDAA versions. Reports concern other Hisense models and cannot be extrapolated directly to the 43A6K.

Sources:
- https://forums.plex.tv/t/persistent-buffering-issues-on-vidaa-os-hisense-tvs-lifetime-plex-pass/938553
- https://forums.plex.tv/t/direct-play-issues-on-hisense-vidaa-tv-75u63kavs-constant-buffering-client-profile-not-detected/914088
- https://gist.github.com/MMeldrum/505fc48c0579f1b9c33e638a9e6f0e0c (older non-matching model)

## Proposed VIDAA runtime verification matrix

| Video | Audio | Container/delivery/engine | Initial confidence |
| --- | --- | --- | --- |
| H.264 SDR | AAC | HLS-fMP4/MSE | Existing VDR-Suite path; still needs exact real-TV evidence per build |
| HEVC SDR | AAC | HLS-fMP4/MSE | Unknown — independent MSE+hardware test required |
| HEVC HDR10/HLG | AAC | HLS-fMP4/MSE | Unknown — verify actual HDR rendering and decoder |
| HEVC SDR | AC-3 | HLS-fMP4/MSE | Unknown — test audio as well as video |
| H.264 SDR | AAC | MP4/native | Unknown for the VDR-Suite app route |
| HEVC SDR | AAC | MPEG-TS/HLS | No implicit fallback; other-model Plex failure report |

Never assume that a positive `MediaSource.isTypeSupported`, `canPlayType`, or manufacturer specification guarantees real playback. Record `runtime-reported` separately from `verified-playback` with app version, TV firmware, container, protocol, engine, video profile/level, bit depth, resolution, frame rate, audio format/channels and observed seek/AV stability.

The existing Device playback credential/grants, `MediaPresentationSelector`, `MediaTranscodePolicy`, protected MediaSession artifacts and truthful index-based seek semantics remain authoritative. This document changes no running profile and authorizes no transcoding, index generation, install or deployment.

## Real Hisense VIDAA 9 runtime acceptance (2026-10-12)

After the initial product/profile research, the real Hisense 43A6K TV reported in the VIDAA app: Chromium 111, MediaSource available and positive `isTypeSupported` for multiple H.264/AAC, HEVC hvc1/hev1/Main10 AAC and HEVC/AC3 fMP4 MIME combinations. This remains **runtime reported support**, not hardware proof for each codec tuple.

At actual playback on VIDAA b20261011.23, the native HLS-TS route failed `Player-Start NotSupportedError` even though the authorized session and HLS-Master HTTP 200 were valid. The user reported the progressive MP4 technical test did not play; the explicit HLS-fMP4 MSE test did. On VIDAA b20261011.24 the user confirmed ordinary recording playback through the existing MSE transport, audio output, relative seeking and direct time jumping. The default MSE client currently requests **H.264/AAC**, so this is *not* verification of HEVC/MSE decoding or HEVC Direct Play. Teletext subtitles were visible as a track type but not selectable; treat subtitle delivery independently of the video codec.

Any future HEVC/AC3/Main10 direct-copy decision must compare exact trusted source ffprobe fields, selected media profile (copy vs transcode), authorized output stream properties and observed TV playback for that precise engine/path. This PR only exposes additional bounded trusted **source** metadata; it does not certify the **output** stream or enable a new codec.
