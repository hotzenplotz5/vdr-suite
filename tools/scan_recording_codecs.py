#!/usr/bin/env python3
"""Read-only, bounded codec inventory for local VDR recordings.

One non-symlink first segment per .rec. No indexing, writes, playback, FFmpeg
transcoding, remote URL, or VDR daemon interaction. Stream-header ffprobe only.
"""
import argparse
from collections import Counter, defaultdict
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import time

SEGMENT = re.compile(r"^(?:\d{5}\.ts|\d{3}\.vdr)$", re.I)
MOVIES = {".mkv", ".mp4", ".m4v", ".mov", ".ts", ".m2ts"}
HDR = {"smpte2084": "HDR10/PQ", "arib-std-b67": "HLG"}


def is_local_regular(path):
    try:
        st = path.lstat()
        return stat.S_ISREG(st.st_mode) and st.st_size > 0
    except OSError:
        return False


def discover(root):
    items = []
    loose_segments = set()
    for here, dirs, files in os.walk(root, followlinks=False):
        folder = Path(here)
        dirs[:] = [d for d in dirs
                   if not d.startswith(".") and not (folder / d).is_symlink()]
        if folder.name.endswith(".rec"):
            parts = sorted(folder / f for f in files if SEGMENT.fullmatch(f))
            head = next((f for f in parts if is_local_regular(f)), None)
            if head is not None:
                items.append((folder.parent.relative_to(root).as_posix(),
                              head, len(parts)))
            dirs[:] = []
            continue
        for name in files:
            file = folder / name
            if name.startswith(".") or file.suffix.lower() not in MOVIES:
                continue
            if SEGMENT.fullmatch(name):
                if folder in loose_segments:
                    continue
                loose_segments.add(folder)
            if is_local_regular(file):
                items.append((file.relative_to(root).as_posix(), file, 1))
    # Balance across top-level genres so a short scan covers all categories.
    genre = defaultdict(list)
    for item in items:
        genre[item[0].split("/", 1)[0]].append(item)
    order = sorted(genre)
    balanced = []
    while order:
        remaining = []
        for name in order:
            balanced.append(genre[name].pop(0))
            if genre[name]:
                remaining.append(name)
        order = remaining
    return balanced


def ffprobe(path, seconds):
    args = ["/usr/bin/ffprobe", "-v", "error",
            "-probesize", "6000000", "-analyzeduration", "2500000",
            "-show_entries",
            "stream=codec_type,codec_name,pix_fmt,width,height,field_order,color_transfer,channels:stream_tags=language",
            "-of", "json", "-i", str(path)]
    try:
        proc = subprocess.run(args, capture_output=True, timeout=seconds,
                              check=False)
        if proc.returncode != 0 or len(proc.stdout) > 128 * 1024:
            return None, "ffprobe-error"
        streams = json.loads(proc.stdout).get("streams", [])
        if not isinstance(streams, list) or len(streams) > 100:
            return None, "malformed-report"
        return streams, None
    except subprocess.TimeoutExpired:
        return None, "timeout"
    except (OSError, ValueError, TypeError):
        return None, "parse-error"


def describe(streams):
    videos = [s for s in streams if s.get("codec_type") == "video"]
    audios = [s for s in streams if s.get("codec_type") == "audio"]
    subtitles = [s for s in streams if s.get("codec_type") == "subtitle"]
    v = videos[0] if videos else {}
    codec = v.get("codec_name", "none")
    pix = v.get("pix_fmt", "?")
    transfer = HDR.get(v.get("color_transfer"), "-")
    a = sorted({s.get("codec_name", "?") for s in audios})
    subs = sorted({s.get("codec_name", "?") for s in subtitles})
    groups = []
    if codec in ("hevc", "h265"):
        groups.append("HEVC Main10/HDR" if "10" in pix or transfer != "-" else "HEVC sonstige")
    if codec == "h264": groups.append("H264")
    if codec == "mpeg2video": groups.append("MPEG2")
    if codec == "h264" and any(c in a for c in ("ac3", "eac3")):
        groups.append("H264+AC3/EAC3")
    if codec == "h264" and "aac" in a: groups.append("H264+AAC")
    if len(audios) > 1: groups.append("Mehrere Tonspuren")
    if any(c in subs for c in ("dvb_teletext", "dvb_subtitle")):
        groups.append("Videotext/DVB Untertitel")
    if (v.get("width") or 0) >= 3840: groups.append("4K")
    return dict(codec=codec, pixel=pix, hdr=transfer, audio=a,
                subtitles=subs, groups=groups)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("root", nargs="?", default="/srv/vdr/video.00")
    p.add_argument("--limit", type=int, default=800)
    p.add_argument("--budget", type=int, default=240,
                   help="global limit in seconds (default: four minutes)")
    p.add_argument("--timeout", type=int, default=6)
    opt = p.parse_args()
    root = Path(opt.root)
    if not root.is_dir() or root.is_symlink() or not Path("/usr/bin/ffprobe").is_file():
        p.error("local video dir / ffprobe unavailable")
    if not 1 <= opt.limit <= 5000 or not 10 <= opt.budget <= 3600 or not 1 <= opt.timeout <= 20:
        p.error("invalid scan budget/limit")
    candidates = discover(root)
    chosen = candidates[:opt.limit]
    print("READ_ONLY=YES\nINDEX_WRITES=NONE\nTRANSCODING=NONE")
    print("RECORDINGS_FOUND={} PROBES_PLANNED={}".format(len(candidates), len(chosen)), flush=True)
    start = time.monotonic()
    successes = []
    errors = Counter()
    scanned = 0
    for title, path, segments in chosen:
        elapsed = time.monotonic() - start
        if elapsed >= opt.budget:
            print("SCAN_TIME_BUDGET_REACHED={}s".format(opt.budget))
            break
        scanned += 1
        streams, err = ffprobe(path, max(1, min(opt.timeout, opt.budget - elapsed)))
        if err:
            errors[err] += 1
        else:
            item = describe(streams)
            item.update(title=title, path=str(path), segments=segments)
            successes.append(item)
        if scanned % 25 == 0:
            print("PROGRESS={}/{} OK={}".format(scanned, len(chosen), len(successes)), flush=True)
    print("\n=== VERTEILUNG ===")
    print("PROBED_OK={} ERRORS={} NOT_SCANNED={}".format(
        len(successes), sum(errors.values()), len(candidates) - scanned))
    print("VIDEO=" + json.dumps(dict(Counter(i["codec"] for i in successes))))
    print("AUDIO=" + json.dumps(dict(Counter(c for i in successes for c in i["audio"]))))
    print("SUBTITLES=" + json.dumps(dict(Counter(c for i in successes for c in i["subtitles"]))))
    print("PROBE_ERRORS=" + json.dumps(dict(errors)))
    print("\n=== TESTKANDIDATEN (max. 3 je Format) ===")
    groups = ["HEVC Main10/HDR", "HEVC sonstige", "H264+AC3/EAC3",
              "H264+AAC", "MPEG2", "Mehrere Tonspuren",
              "Videotext/DVB Untertitel", "4K"]
    for group in groups:
        matches = [i for i in successes if group in i["groups"]]
        print("\n{}: {} Treffer".format(group, len(matches)))
        for i in matches[:3]:
            print("  {} | {} / {} | HDR {} | Audio {} | Subs {} | {}".format(
                i["title"], i["codec"], i["pixel"], i["hdr"],
                ",".join(i["audio"]) or "-", ",".join(i["subtitles"]) or "-",
                i["path"]))
    print("\nRESULT=" + ("INVENTORY_COMPLETE" if not errors and scanned == len(candidates)
                         else "INVENTORY_PARTIAL"))
    print("NOTE=Only first local segment inspected, not full-recording/direct-play proof.")


if __name__ == "__main__":
    main()
