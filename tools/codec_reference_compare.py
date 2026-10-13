#!/usr/bin/env python3
"""Bounded, non-root, offline analysis of local recordings vs public reference video.

A selected, published open-film encode is downloaded to a private disposable
directory under /home/yavdr. Local source recordings are opened read-only.
No VDR runtime, source, permission, or service state is changed.

This is a cadence/timestamp/bitstream comparison, NOT a display-fps measurement.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
import urllib.parse
import urllib.request

# Curated and deliberately explicit. Never crawl or mass-download by default.
# Sizes reported by the original publisher; deviations cause a hard stop.
REFERENCES = {
    "bbb-hevc-1080p24": {
        "url": "https://lafibre.info/videos/test/201411_blender_big_buck_bunny_24fps_1080p_hevc.mp4",
        "bytes": 137605655, "codec": "hevc", "fps": "24/1", "width": 1920,
        "height": 1080, "description": "Big Buck Bunny, HEVC 8-bit 1080p24 AAC",
    },
    "bbb-h264-1080p24": {
        "url": "https://lafibre.info/videos/test/201411_blender_big_buck_bunny_24fps_1080p_h264-high.mp4",
        "bytes": 160978435, "codec": "h264", "fps": "24/1", "width": 1920,
        "height": 1080, "description": "Big Buck Bunny, H.264 High 1080p24 AAC",
    },
    "bbb-mpeg2-480p24": {
        "url": "https://lafibre.info/videos/test/201411_blender_big_buck_bunny_24fps_480p_mpeg2.mpg",
        "bytes": 258988032, "codec": "mpeg2video", "fps": "24/1", "width": 854,
        "height": 480, "description": "Big Buck Bunny, MPEG-2 480p24 AC-3",
    },
}

# Historical case paths already supplied by the operator. Only checked when
# explicitly selected; the tool never enumerates or scans the entire archive.
KNOWN = {
    "medicus": "Historienfilm/Der_Medicus_II/2026-05-18.19.00.1-0.rec/00001.ts",
    "solo": "SciFi/Solo_a_Star_Wars_Story/2026-05-06.17.48.1-0.rec/00001.ts",
    "angel": "Action/Angel_Has_Fallen/2026-05-21.22.36.1-0.rec/00001.ts",
    "1917": "Krieg/1917/2026-05-12.10.59.1-0.rec/00001.ts",
    "bushwick": "Action/Bushwick/2026-04-22.23.53.1-0.rec/00001.ts",
}


def command(argv, timeout=120, cwd=None):
    try:
        p = subprocess.run(argv, stdin=subprocess.DEVNULL,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           timeout=timeout, cwd=cwd, check=False)
        return p.returncode, p.stdout.decode("utf-8", "replace"), p.stderr.decode("utf-8", "replace")
    except (OSError, subprocess.TimeoutExpired) as exc:
        return 125, "", type(exc).__name__


def stream_metadata(path):
    cmd = ["ffprobe", "-v", "error", "-select_streams", "v:0",
           "-show_entries", "stream=codec_name,profile,level,pix_fmt,width,height,"
           "avg_frame_rate,r_frame_rate,time_base,has_b_frames:"
           "format=duration",
           "-of", "json", str(path)]
    rc, out, err = command(cmd, timeout=35)
    if rc:
        raise RuntimeError("ffprobe rc={} {}".format(rc, err[:250]))
    data = json.loads(out)
    stream = (data.get("streams") or [{}])[0]
    return stream


def packets(path, seconds=15):
    rc, out, err = command(["ffprobe", "-v", "error",
        "-read_intervals", "%+{}".format(seconds), "-select_streams", "v:0",
        "-show_packets", "-show_entries", "packet=pts_time,dts_time,duration_time",
        "-of", "json", str(path)], timeout=70)
    if rc:
        print("TIMESTAMP_PROBE_FAILED rc={} {}".format(rc, err[:200]))
        return
    ps = json.loads(out).get("packets", [])
    def floats(key):
        result = []
        for packet in ps:
            try:
                value = float(packet[key])
                if abs(value) < 1e12: result.append(value)
            except (ValueError, TypeError, KeyError):
                pass
        return result
    pts, dts = sorted(floats("pts_time")), floats("dts_time")
    intervals = [b - a for a, b in zip(pts, pts[1:])]
    dts_bad = sum(b <= a for a, b in zip(dts, dts[1:]))
    print("TIMING packets={} pts_missing={} dts_missing={} dts_non_increasing={} "
          "pts_gaps_gt_75ms={}".format(len(ps), len(ps)-len(pts),
          len(ps)-len(dts), dts_bad, sum(v > .075 for v in intervals)))
    if intervals:
        common = Counter(round(v*1000, 1) for v in intervals)
        print("TIMING_PTS_DELTA_MS median={:.3f} frequent={}".format(
            statistics.median(intervals)*1000, common.most_common(5)))


def exact_frame_hashes(path, start, duration=10):
    # ffmpeg framemd5 hashes decoded, unscaled video frames. In particular,
    # this is NOT mpdecimate's scene-dependent near-duplicate heuristic.
    cmd = ["ffmpeg", "-hide_banner", "-nostdin", "-loglevel", "error",
           "-threads", "2", "-ss", str(start), "-i", str(path),
           "-t", str(duration), "-map", "0:v:0", "-an", "-sn", "-dn",
           "-fps_mode", "passthrough", "-frames:v", "250", "-f", "framemd5", "-"]
    rc, out, err = command(cmd, timeout=140)
    if rc:
        print("FRAMEHASH_FAILED t={} rc={} {}".format(start, rc, err[:250]))
        return
    digests = []
    for line in out.splitlines():
        if line.startswith("#"): continue
        parts = [part.strip() for part in line.split(",")]
        if len(parts) == 6 and re.fullmatch(r"[0-9a-fA-F]{32}", parts[5]):
            digests.append(parts[5].lower())
    repeated = sum(a == b for a,b in zip(digests, digests[1:]))
    print("FRAMEHASH start={}s decoded={} consecutive_exact_duplicate={} "
          "ratio={:.2%} (decoded original pixels; content-dependent)".format(
          start, len(digests), repeated,
          repeated / max(1, len(digests)-1)))


def download(ref, destination):
    url = ref["url"]
    request = urllib.request.Request(url, headers={
        "User-Agent": "VDR-Suite-codec-reference-check/1.0"})
    expected = ref["bytes"]
    maximum = expected + 1024
    started = time.monotonic()
    size, checksum = 0, hashlib.sha256()
    # A missing/altered publisher file must fail; never accept an HTML error
    # document or a page with a similar-looking name.
    with urllib.request.urlopen(request, timeout=40) as response:
        final = urllib.parse.urlparse(response.geturl())
        if (final.scheme != "https" or final.hostname not in
                ("lafibre.info", "www.lafibre.info")):
            raise RuntimeError("STOP_REFERENCE_REDIRECT")
        announced = response.headers.get("Content-Length")
        if announced and int(announced) != expected:
            raise RuntimeError("STOP_REFERENCE_CONTENT_LENGTH")
        with destination.open("xb") as output:
            while True:
                if time.monotonic() - started > 600:
                    raise RuntimeError("STOP_DOWNLOAD_TIMEOUT")
                chunk = response.read(1024 * 1024)
                if not chunk: break
                size += len(chunk)
                if size > maximum:
                    raise RuntimeError("STOP_REFERENCE_TOO_LARGE")
                checksum.update(chunk)
                output.write(chunk)
    print("REFERENCE_DOWNLOAD_BYTES={} SHA256={}".format(size, checksum.hexdigest()))
    if size != expected:
        raise RuntimeError("STOP_REFERENCE_SIZE_MISMATCH")


def ensure_source(path):
    if not path.is_file() or not os.access(path, os.R_OK):
        raise RuntimeError("STOP_SOURCE_UNREADABLE: " + str(path))
    return path.resolve(strict=True)


def remux_reference(reference, folder, codec):
    # Observe the same MP4 -> TS -> concat -> fMP4 chain using a bounded
    # excerpt. No re-encoding of video, and only temporary new files.
    if codec not in ("hevc", "h264"):
        print("REMUX=SKIPPED_NOT_FMP4_ELIGIBLE")
        return
    ts = folder / "reference-excerpt.ts"
    rc, _, err = command(["ffmpeg", "-hide_banner", "-nostdin", "-loglevel",
        "warning", "-n", "-ss", "12", "-i", str(reference), "-t", "24",
        "-map", "0:v:0", "-map", "0:a:0?", "-sn", "-c", "copy",
        "-f", "mpegts", str(ts)], timeout=150)
    if rc or not ts.is_file():
        print("REMUX_TO_TS_FAILED rc={} {}".format(rc, err[-350:]))
        return
    print("REFERENCE_TS_EXCERPT_BYTES={}".format(ts.stat().st_size))
    for label, flags in [
        ("split", "delete_segments+split_by_time+temp_file"),
        ("keyframe", "delete_segments+independent_segments+temp_file"),
    ]:
        folder_path = folder / label
        folder_path.mkdir()
        # Construct only a relative ffconcat entry to remain safe=1.
        (folder_path / "source.ts").symlink_to(ts)
        (folder_path / "input.ffconcat").write_text(
            "ffconcat version 1.0\nfile 'source.ts'\n", encoding="ascii")
        rc, _, logs = command(["ffmpeg", "-hide_banner", "-nostdin",
            "-loglevel", "warning", "-n", "-f", "concat", "-safe", "1",
            "-i", "input.ffconcat", "-map", "0:v:0?", "-map", "0:a:0?",
            "-sn", "-c:v", "copy", "-c:a", "aac", "-b:a", "192k",
            "-ac", "2", "-f", "hls", "-hls_time", "4",
            "-hls_list_size", "0", "-hls_flags", flags,
            "-hls_segment_type", "fmp4", "-hls_fmp4_init_filename",
            "init.mp4", "-hls_segment_filename", "segment-%06d.m4s",
            "master.m3u8"], cwd=folder_path, timeout=160)
        nonmono = logs.count("Non-monotonic DTS")
        negative = len(re.findall(r"Packet duration: -\d+", logs))
        print("REFERENCE_REMUX mode={} rc={} nonmonotonic_DTS={} "
              "negative_packet_duration={}".format(
              label, rc, nonmono, negative))
        if rc:
            print("  REMUX_ERROR:", logs[-350:])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--list", action="store_true", help="list curated reference variants")
    p.add_argument("--reference", choices=sorted(REFERENCES),
                   default="bbb-hevc-1080p24")
    p.add_argument("--known", action="append", choices=sorted(KNOWN),
                   help="select one prior local VDR example (repeatable)")
    p.add_argument("--source", type=Path, action="append",
                   help="additional source video file; repeatable")
    p.add_argument("--scratch-dir", type=Path, default=Path("/home/yavdr"),
                   help="temporary scratch filesystem (default /home/yavdr)")
    p.add_argument("--skip-remux", action="store_true")
    args = p.parse_args()
    if args.list:
        for name, entry in REFERENCES.items():
            print(name, entry["description"], "bytes=", entry["bytes"])
        print("Known local source labels:", ", ".join(KNOWN))
        return 0
    # Respect the operator's existing shell identity. In particular, do not
    # switch users or modify filesystem permissions/ownership. Temporary
    # artifacts are confined to our new private scratch subdirectory.
    print("RUNNING_UID={} PRIVILEGE_SWITCH=NO RIGHTS_CHANGES=NO".format(
        os.geteuid()), flush=True)
    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        print("STOP_DEPENDENCY: ffmpeg and ffprobe required")
        return 2
    if not (args.scratch_dir.is_dir() and
            os.access(args.scratch_dir, os.W_OK | os.X_OK)):
        print("STOP_SCRATCH_UNAVAILABLE")
        return 2
    targets = list(args.source or [])
    for label in args.known or (["medicus"] if not targets else []):
        # /srv/vdr/video is symlinked to /srv/vdr/video.00 on yaVDR.
        targets.append(Path("/srv/vdr/video") / KNOWN[label])
    if not targets or len(targets) > 8:
        print("STOP_SOURCE_COUNT (1..8)")
        return 2
    try:
        targets = [ensure_source(x) for x in targets]
    except (RuntimeError, OSError) as exc:
        print(exc)
        return 2
    ref = REFERENCES[args.reference]
    usage = shutil.disk_usage(args.scratch_dir)
    reserve = max(4 * 1024**3, (usage.total * 15 + 99) // 100)
    # Bound concurrent download + TS+fMP4 copies + framehash subprocesses.
    budget = 1024**3
    print("RESOURCE_PREFLIGHT total={} free={} reserve={} budget={}".format(
        usage.total, usage.free, reserve, budget), flush=True)
    if usage.free < reserve + budget:
        print("STOP_DISK")
        return 2
    print("RESOURCE_PREFLIGHT=PASS NO_SUDO=YES SOURCE_READ_ONLY=YES", flush=True)
    try:
        with tempfile.TemporaryDirectory(
                prefix="vdr-codec-reference-", dir=args.scratch_dir) as tmp:
            work = Path(tmp)
            reference = work / ("reference" + Path(
                urllib.parse.urlparse(ref["url"]).path).suffix)
            print("REFERENCE_NAME={} PUBLISHER={}".format(
                args.reference, ref["url"]), flush=True)
            download(ref, reference)
            metadata = stream_metadata(reference)
            print("REFERENCE_META=" + json.dumps(metadata, sort_keys=True))
            for field, expected in (
                    ("codec_name", ref["codec"]), ("width", ref["width"]),
                    ("height", ref["height"]),
                    ("avg_frame_rate", ref["fps"])):
                if metadata.get(field) != expected:
                    raise RuntimeError("STOP_REFERENCE_CODEC_MISMATCH: " + field)
            print("REFERENCE_IDENTITY=PASS", flush=True)
            print("=== REFERENCE CONTENT/TIMING ===", flush=True)
            packets(reference)
            exact_frame_hashes(reference, 10)
            exact_frame_hashes(reference, 70)
            if not args.skip_remux:
                remux_reference(reference, work, ref["codec"])
            for source in targets:
                print("=== LOCAL_SOURCE {} ===".format(source), flush=True)
                local_meta = stream_metadata(source)
                print("SOURCE_META=" + json.dumps(local_meta, sort_keys=True))
                if local_meta.get("codec_name") != ref["codec"]:
                    print("COMPARABILITY=DIFFERENT_VIDEO_CODEC")
                elif local_meta.get("pix_fmt") != metadata.get("pix_fmt"):
                    print("COMPARABILITY=VIDEO_CODEC_MATCH_BITDEPTH_DIFFERS")
                else:
                    print("COMPARABILITY=VIDEO_CODEC_AND_PIXEL_FORMAT_MATCH")
                packets(source)
                exact_frame_hashes(source, 10)
                exact_frame_hashes(source, 70)
        free_after = shutil.disk_usage(args.scratch_dir).free
        print("RESOURCE_POSTCHECK free={} reserve={}".format(
            free_after, reserve))
        print("RESOURCE_POSTCHECK=" + (
            "PASS" if free_after >= reserve else "FAIL"))
        print("NOTE=Identical frames depend on scene motion. "
              "A different film is not a frame-by-frame ground truth.")
        print("NOTE=No TV presentation data was collected.")
        print("=== CODEC_REFERENCE_REPORT_END ===", flush=True)
        return 0 if free_after >= reserve else 2
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as exc:
        print("STOP_REFERENCE_ANALYSIS:", type(exc).__name__, str(exc)[:300])
        return 2


if __name__ == "__main__":
    sys.exit(main())
