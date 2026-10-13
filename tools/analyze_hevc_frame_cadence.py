#!/usr/bin/env python3
"""Offline HEVC cadence diagnosis at five positions in a *single* VDR TS file.

Input file is never changed. No root/sudo switching, permission edits,
downloads, cache writes, temporary media, daemon calls or TV use.
Uses bounded ffprobe packet timing and FFmpeg decoded framemd5 checksums.
"""
import argparse
from collections import Counter
from fractions import Fraction
import json
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import sys

DEFAULT_SOURCE = Path(
    "/srv/vdr/video.00/Historienfilm/Der_Medicus_II/"
    "2026-05-18.19.00.1-0.rec/00001.ts"
)
FRACTIONS = (0.02, 0.25, 0.50, 0.75, 0.95)
WINDOW = 10
MAX_STDOUT = 3 * 1024 * 1024
HASH_RE = re.compile(r"^[0-9a-fA-F]{32}$")
TB_RE = re.compile(r"^#tb 0:\s*(\d+/\d+)\s*$")


def process(argv, timeout):
    try:
        proc = subprocess.run(
            argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=timeout, check=False)
    except subprocess.TimeoutExpired:
        raise RuntimeError("STOP_TIMEOUT")
    except OSError as exc:
        raise RuntimeError("STOP_EXEC " + str(exc))
    if len(proc.stdout) > MAX_STDOUT or len(proc.stderr) > MAX_STDOUT:
        raise RuntimeError("STOP_OUTPUT_OVERSIZE")
    return proc.returncode, proc.stdout.decode("utf-8", "replace"), \
        proc.stderr.decode("utf-8", "replace")


def metadata(path):
    cmd = [
        "ffprobe", "-v", "error", "-show_entries",
        "format=duration:stream=index,codec_type,codec_name,avg_frame_rate,"
        "width,height,profile,pix_fmt", "-of", "json", str(path)
    ]
    rc, data, err = process(cmd, timeout=45)
    if rc:
        raise RuntimeError("STOP_METADATA_FFPROBE " + err[:240])
    try:
        value = json.loads(data)
        duration = float(value["format"]["duration"])
        video = next(s for s in value["streams"]
                     if s.get("codec_type") == "video")
    except (ValueError, KeyError, TypeError, StopIteration, OverflowError):
        raise RuntimeError("STOP_INVALID_MEDIA_METADATA")
    if not 60 <= duration <= 24 * 3600:
        raise RuntimeError("STOP_DURATION_OUT_OF_RANGE")
    if video.get("codec_name") != "hevc":
        raise RuntimeError("STOP_EXPECTED_HEVC")
    return duration, video


def get_positions(duration):
    # Stay at least 15 s away from the end. Distinct samples are required.
    starts = [round(max(0, min(duration-15, duration * f)), 1)
              for f in FRACTIONS]
    if any(b-a < WINDOW + 1 for a,b in zip(starts, starts[1:])):
        raise RuntimeError("STOP_WINDOWS_OVERLAP")
    return starts


def parse_framemd5(text):
    tb = None
    samples = []
    for line in text.splitlines():
        found = TB_RE.match(line)
        if found:
            tb = Fraction(found.group(1))
        if line.startswith("#"):
            continue
        parts = [piece.strip() for piece in line.split(",")]
        if len(parts) != 6 or not HASH_RE.fullmatch(parts[5]):
            continue
        try:
            pts = int(parts[2])
        except ValueError:
            continue
        samples.append((pts, parts[5].lower()))
    if not tb or len(samples) < 10 or len(samples) > 270:
        raise RuntimeError("STOP_BAD_FRAMEMD5")
    return tb, samples


def summarize_frames(tb, samples):
    duplicates = [i for i in range(1, len(samples))
                  if samples[i][1] == samples[i-1][1]]
    repeated = set(duplicates)
    # Consecutive identical frames form runs. A standstill creates one
    # long run; persistent cadence errors may create short periodic runs.
    run_lengths = []
    length = 1
    for i in range(1, len(samples)):
        if i in repeated:
            length += 1
        else:
            if length > 1:
                run_lengths.append(length)
            length = 1
    if length > 1:
        run_lengths.append(length)
    second_buckets = Counter(
        min(9, max(0, int((samples[i][0]-samples[0][0])*float(tb))))
        for i in duplicates)
    times = [float(v[0]*tb) for v in samples]
    diffs = [b-a for a,b in zip(times,times[1:])]
    gaps = Counter(round(v*1000, 2) for v in diffs)
    return {
        "decoded": len(samples),
        "exact_duplicate_pairs": len(duplicates),
        "duplicate_pct": round(100*len(duplicates)/max(1,len(samples)-1),2),
        "duplicate_buckets_per_sec": [second_buckets[i] for i in range(10)],
        "duplicate_indices_first40": duplicates[:40],
        "repeat_runs": len(run_lengths),
        "longest_identical_run": max(run_lengths, default=1),
        "repeat_run_distribution": dict(sorted(Counter(run_lengths).items())),
        "frame_pts_median_ms": round(statistics.median(diffs)*1000,3),
        "frame_pts_delta_top": gaps.most_common(5)
    }


def packet_timing(path, start):
    # ffprobe seeks to a demuxer start point, which may be a nearby keyframe.
    rc, data, err = process([
        "ffprobe", "-v", "error", "-read_intervals",
        f"{start:.1f}%+{WINDOW}", "-select_streams", "v:0",
        "-show_packets", "-show_entries", "packet=pts_time,dts_time",
        "-of", "json", str(path)
    ], timeout=45)
    if rc:
        raise RuntimeError("STOP_PACKETS " + err[:240])
    try:
        packets = json.loads(data)["packets"]
    except (ValueError, KeyError, TypeError):
        raise RuntimeError("STOP_PACKET_JSON")
    def number(p, key):
        try:
            return float(p[key])
        except (KeyError, ValueError, TypeError):
            return None
    pts = sorted(x for p in packets if (x := number(p, "pts_time")) is not None)
    dts = [number(p, "dts_time") for p in packets]
    dts_bad = sum(b <= a for a,b in zip(dts,dts[1:])
                  if a is not None and b is not None)
    diffs = [b-a for a,b in zip(pts,pts[1:])]
    return {
        "packets": len(packets),
        "pts_missing": len(packets)-len(pts),
        "dts_missing": dts.count(None),
        "dts_non_increasing": dts_bad,
        "pts_gaps_gt_75ms": sum(x > .075 for x in diffs),
        "packet_pts_delta_top_ms": Counter(round(x*1000,2) for x in diffs).most_common(5)
    }


def one_window(source, start):
    # -ss before input makes random-access seeks practical on large TS files.
    # No -vsync cfr, no framerate filter and no transcoded output file.
    rc, out, err = process([
        "ffmpeg", "-hide_banner", "-nostdin", "-loglevel", "error",
        "-threads", "2", "-ss", f"{start:.1f}", "-i", str(source),
        "-t", str(WINDOW), "-map", "0:v:0", "-an", "-sn", "-dn",
        "-fps_mode", "passthrough", "-frames:v", "270",
        "-f", "framemd5", "-"
    ], timeout=150)
    if rc:
        raise RuntimeError("STOP_FRAMEMD5 " + err[-250:])
    tb, frames = parse_framemd5(out)
    return summarize_frames(tb, frames)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    args = parser.parse_args()
    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        print("STOP_FFMPEG_MISSING")
        return 2
    if not args.source.is_file():
        print("STOP_SOURCE_NOT_FOUND", args.source)
        return 2
    try:
        source = args.source.resolve(strict=True)
        print("=== HEVC_FIVE_WINDOW_CADENCE ===", flush=True)
        print("SOURCE=", source, "READ_ONLY=YES NO_TV=YES NO_REMUX=YES")
        duration, video = metadata(source)
        print("SOURCE_DURATION_SECONDS=", round(duration,2))
        print("VIDEO=", json.dumps(video, sort_keys=True))
        starts = get_positions(duration)
        print("WINDOWS_START_SECONDS=", starts, flush=True)
        results = []
        for index, start in enumerate(starts, 1):
            print(f"=== WINDOW_{index} start={start:.1f}s ===", flush=True)
            frame_report = one_window(source,start)
            packet_report = packet_timing(source,start)
            print("FRAMES=", json.dumps(frame_report,sort_keys=True), flush=True)
            print("PACKETS=", json.dumps(packet_report,sort_keys=True), flush=True)
            results.append(frame_report)
        print("=== SUMMARY ===")
        print("EXACT_DUPLICATES_BY_WINDOW=",
              [r["exact_duplicate_pairs"] for r in results])
        print("DUPLICATE_RATIOS_PCT=",
              [r["duplicate_pct"] for r in results])
        print("LONGEST_RUN_BY_WINDOW=",
              [r["longest_identical_run"] for r in results])
        print("Interpretation: consecutive exact duplicates can be scene-dependent;")
        print("do not infer a bad encode from one static scene.")
        print("SOURCE_FILES_MODIFIED=NO")
        print("=== HEVC_FIVE_WINDOW_REPORT_END ===")
        return 0
    except (RuntimeError, OSError) as exc:
        print("STOP_ANALYSIS:", str(exc)[:350])
        return 2


if __name__ == "__main__":
    sys.exit(main())
