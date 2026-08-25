# HEVC Recording Frame-Rate / VDR Length Investigation

## Status

**Deferred investigation — evidence recorded, root cause not yet closed.**

This note records a reproducible VDR recording-duration anomaly discovered during Phase 65 media work. It is intentionally **not** an ADR: no architecture decision is accepted here, and this investigation does not authorize or block unrelated Phase 65.D work such as normalized audio/subtitle selection.

The issue should be resumed after the current subtitle work or when recording import/index semantics are revisited.

## Observed case

Real yaVDR system:

- VDR `2.7.9`
- video codec: HEVC / Main 10
- dimensions: `3840x1606`
- TS duration from `ffprobe`: approximately `5821.23 s` (`1:37:01`)
- `r_frame_rate=24000/1001`
- `avg_frame_rate=48000/1001`

HEVC VUI trace:

```text
vui_num_units_in_tick = 1001
vui_time_scale = 48000
vui_poc_proportional_to_timing_flag = 0
```

## Reproduced VDR behavior

The recording index contained:

```text
index size    = 1116560 bytes
index entries = 139570
```

VDR `--genindex` regenerated the same index size / entry count, so the tested index itself was reproducibly stable.

With this `info` frame-rate line:

```text
F 47.95204795 3840 1606 i -
```

VDR reported the recording through `LSTR` as approximately:

```text
0:49
```

The arithmetic matches the observed display:

```text
139570 / 47.95204795 ~= 2910.6 s ~= 0:48:30
```

After deleting both `info` and `index`, fully removing the recording from VDR's recording cache, restoring the directory, and triggering a fresh recordings scan, VDR reported:

```text
0:00
```

and `LSTR <id>` exposed VDR's fallback/default recording information including:

```text
F 25
O -1
```

This confirmed that the earlier `0:49` result was not an independent TS-duration calculation.

The SVDRP Android app also showed `0:00` after a real refresh in this fresh no-`info` / no-`index` state, confirming that its recording-duration display follows VDR rather than independently deriving the TS duration.

## Controlled correction

The unchanged index was restored and only the frame rate in `info` was corrected to:

```text
F 23.97602398 3840 1606 i -
```

After forcing VDR to discard and freshly recreate the recording object, `LSTR` reported:

```text
1:37
```

This matches both the TS duration and:

```text
139570 / 23.97602398 ~= 5821 s ~= 1:37:01
```

## Proven conclusions

For this concrete recording:

1. The tested VDR index with `139570` entries is consistent with the real recording duration when interpreted at approximately `24000/1001` fps.
2. `F 47.95204795` causes VDR's displayed recording length to be approximately half of the real duration.
3. Replacing only that value with approximately `23.97602398` makes VDR report the correct `1:37` duration with the same index.
4. The SVDRP app follows VDR's recording-duration result; its earlier apparently correct value must not be used as evidence for an independent duration calculation.
5. A missing `info` and `index` does not cause VDR to derive the real TS duration during a normal recordings rescan; a freshly discovered recording showed `0:00` and default `F 25` metadata.

## Still open

The following points are deliberately **not** considered solved:

- Which component originally wrote `F 47.95204795` into the imported recording's `info` file.
- Whether the importer selected an unsuitable FFmpeg value such as `avg_frame_rate` instead of the effective picture rate.
- Why VDR's HEVC parsing / `--genindex` path accepts or derives approximately `48000/1001` for this stream even though the generated index count and TS duration are consistent with approximately `24000/1001`.
- The exact semantics VDR should use for HEVC VUI timing when `vui_poc_proportional_to_timing_flag=0`.
- Whether the behavior reproduces across a representative HEVC corpus or only specific encodes/remuxes.
- Whether any eventual fix belongs in VDR core, VDR-Suite import handling, metadata normalization, or more than one layer.

## Resume criteria / next investigation

When this investigation is resumed:

1. Identify the exact VDR-Suite/import code path that writes the `F` line.
2. Capture all FFmpeg/ffprobe frame-rate candidates used by that path (`r_frame_rate`, `avg_frame_rate`, time base, timestamps).
3. Compare VDR's HEVC parser result with PTS-derived picture cadence for the same stream.
4. Repeat against several HEVC samples, including 23.976/24/25/50/59.94 material where available.
5. Define the correction only after the source of truth and cross-stream behavior are proven.
6. Add a regression test before changing import or VDR-facing metadata generation.

## Safety note for the reproduced sample

Until the investigation is closed, do not treat `vdr --genindex` as a guaranteed repair for this class of recording. In the reproduced case VDR regenerated the same correct index count while the problematic approximately `47.952` fps metadata remained in effect.

The evidence above is a deferred diagnostic record, not a general rule to blindly halve HEVC frame rates.
