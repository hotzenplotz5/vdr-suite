#!/usr/bin/env python3
"""Inspect a bounded number of VDR recording indexes, strictly read-only."""
import argparse
import collections
import os
from pathlib import Path
import re
import stat
import struct
import sys

MAX_SCAN_DIRS = 50000
MAX_INDEX_BYTES = 256 * 1024 * 1024
MAX_TAIL_BYTES = (3 * 1024 * 1024 // 188) * 188 + 64 * 188


def regular(path):
    try:
        value = path.lstat()
    except OSError:
        return None
    return value if stat.S_ISREG(value.st_mode) else None


def resolved_video_root(path):
    """Resolve a configured VDR video root, including symlink aliases, safely."""
    try:
        path = Path(path)
        if not path.is_absolute() or path == Path("/"):
            return None
        canonical = path.resolve(strict=True)
        if canonical == Path("/") or not canonical.is_dir():
            return None
        return canonical
    except (OSError, RuntimeError):
        return None


def configured_roots():
    roots = set()

    def add_candidate(value):
        path = resolved_video_root(value)
        if path is not None:
            roots.add(path)

    # VDR -v/-video arguments are authoritative when present. On yaVDR the
    # video directory is commonly selected by a config variable instead.
    for entry in Path("/proc").iterdir():
        if not entry.name.isdecimal():
            continue
        try:
            if (entry / "comm").read_text().strip() != "vdr":
                continue
            args = (entry / "cmdline").read_bytes().split(b"\x00")
            argv = [item.decode("utf-8", "replace") for item in args if item]
            for i, argument in enumerate(argv):
                value = ""
                if argument in ("-v", "--video", "--video-dir", "--video-directory"):
                    if i + 1 < len(argv):
                        value = argv[i + 1]
                elif argument.startswith("--video="):
                    value = argument.split("=", 1)[1]
                elif argument.startswith("-v") and len(argument) > 2:
                    value = argument[2:]
                if value:
                    add_candidate(value)
        except (OSError, ValueError):
            continue

    # Read-only parsing, never source/execute shell configuration.
    default_vdr = Path("/etc/default/vdr")
    if regular(default_vdr) is not None:
        try:
            for line in default_vdr.read_text(errors="replace")[:16384].splitlines():
                match = re.match(
                    r"""^\s*(?:export\s+)?VIDEO_DIR\s*=\s*(?:"([^"]+)"|'([^']+)'|([^#\s]+))""",
                    line)
                if match:
                    add_candidate(next(value for value in match.groups() if value))
        except OSError:
            pass

    # The prior implementation dropped /srv/vdr/video when it was a symlink
    # and omitted /srv/vdr/video.00 altogether. On yaVDR they may denote the
    # same real directory. Canonicalize both and deduplicate before traversal.
    for value in (
        "/srv/vdr/video",
        "/srv/vdr/video.00",
        "/srv/video",
        "/video",
        "/var/lib/video.00",
        "/var/lib/vdr/video",
        "/var/lib/vdr/recordings",
    ):
        add_candidate(value)
    return sorted(roots, key=lambda item: str(item))

def find_recordings(roots, term):
    matches = set()
    visited = set()
    count = 0
    for root in roots:
        for folder, directories, _files in os.walk(root, followlinks=False):
            count += 1
            if count > MAX_SCAN_DIRS:
                raise RuntimeError("SCAN_LIMIT_REACHED")
            path = Path(folder)
            if path in visited:
                directories[:] = []
                continue
            visited.add(path)
            directories[:] = [name for name in directories
                              if not (path / name).is_symlink()]
            if path.name.endswith(".rec"):
                if term.casefold() in str(path.parent.name).casefold():
                    matches.add(path)
                directories[:] = []
    return sorted(matches)


def inspect(folder):
    if folder.is_symlink() or not folder.is_dir():
        raise RuntimeError("RECORDING_NOT_REGULAR_DIRECTORY")
    entries = {}
    for file in folder.iterdir():
        match = re.fullmatch(r"([0-9]{5})\.ts", file.name)
        if not match:
            continue
        facts = regular(file)
        if facts is None:
            raise RuntimeError("TS_NOT_REGULAR")
        number = int(match.group(1))
        if number == 0 or number in entries:
            raise RuntimeError("TS_SEQUENCE_INVALID")
        entries[number] = facts.st_size
    print("TS_SEGMENT_COUNT=" + str(len(entries)))
    if not entries:
        legacy = sum(bool(re.fullmatch(r"[0-9]{3}\.vdr", x.name))
                     for x in folder.iterdir())
        print("LEGACY_PES_SEGMENTS=" + str(legacy))
        raise RuntimeError("MODERN_TS_SEGMENTS_MISSING")
    growing = (folder / ".timer").exists() or (folder / ".timer").is_symlink()
    print("SOURCE_GROWING=" + ("YES" if growing else "NO"))
    info = next((p for p in (folder / "info", folder / "info.vdr")
                 if regular(p) is not None), None)
    if info is None:
        raise RuntimeError("INFO_MISSING_OR_UNSAFE")
    with info.open("rb") as inp:
        lines = inp.read(65536).decode("utf-8", "replace").splitlines()
    fps = 0.0
    for line in lines:
        match = re.match(r"^F\s+([0-9]+(?:\.[0-9]+)?)", line)
        if match:
            fps = float(match.group(1))
            break
    if not 0 < fps <= 240:
        raise RuntimeError("INFO_FPS_INVALID")
    print("INFO_FPS=" + str(fps))
    index = next((p for p in (folder / "index", folder / "index.vdr")
                  if regular(p) is not None), None)
    if index is None:
        raise RuntimeError("INDEX_MISSING_OR_UNSAFE")
    before = regular(index)
    if before is None or before.st_size < 8 or before.st_size % 8 or \
            before.st_size > MAX_INDEX_BYTES or sys.byteorder != "little":
        raise RuntimeError("INDEX_SIZE_OR_LAYOUT_INVALID")
    frames = collections.Counter()
    count = 0
    final_number = 0
    final_offset = 0
    with index.open("rb") as inp:
        while True:
            block = inp.read(8192 * 8)
            if not block:
                break
            if len(block) % 8:
                raise RuntimeError("INDEX_TORN_ENTRY")
            for (value,) in struct.iter_unpack("<Q", block):
                number = (value >> 48) & 0xffff
                if number == 0:
                    raise RuntimeError("INDEX_SEGMENT_NUMBER_INVALID")
                frames[number] += 1
                final_number = number
                final_offset = value & ((1 << 40) - 1)
                count += 1
    after = regular(index)
    if after is None or (before.st_dev, before.st_ino, before.st_size,
                         before.st_mtime_ns) != (
                         after.st_dev, after.st_ino, after.st_size,
                         after.st_mtime_ns) or count * 8 != before.st_size:
        raise RuntimeError("INDEX_CHANGED_DURING_READ")
    last_size = entries[max(entries)]
    duration_usable = (
        not growing and final_number == max(entries) and
        final_offset <= last_size and
        last_size - final_offset <= MAX_TAIL_BYTES)
    all_mapped = (
        duration_usable and set(frames) == set(entries) and
        all(frames[number] > 0 for number in entries))
    print("INDEX_FRAMES=" + str(count))
    print("INDEX_DURATION_SECONDS=" +
          (str(int(count / fps)) if duration_usable else "UNAVAILABLE"))
    print("INDEX_LAST_TS=" + str(final_number))
    print("INDEX_END_COMPLETE=" + ("YES" if duration_usable else "NO"))
    print("INDEX_TS_FULLY_MAPPED=" + ("YES" if all_mapped else "NO"))
    print("TS_WITHOUT_INDEX_FRAMES=" + str(len(set(entries) - set(frames))))
    print("INDEX_SEGMENTS_WITHOUT_TS=" + str(len(set(frames) - set(entries))))
    print("HLS_SYNC_PROFILE=NOT_TESTED")
    print("SEEK_DIAGNOSIS=" +
          ("INDEX_SEGMENTS_MAPPED" if all_mapped
           else "INDEX_OR_SEGMENT_MAPPING_INCOMPLETE"))



MAX_MATCHED_RECORDINGS = 4


def diagnose_matches(matches, all_matches=False):
    """Keep the original unique-mode gate; explicitly inspect bounded peers."""
    print("MATCHED_RECORDINGS=" + str(len(matches)))
    if not matches:
        raise RuntimeError("RECORDING_NOT_FOUND")
    if len(matches) != 1 and not all_matches:
        raise RuntimeError("RECORDING_NOT_UNIQUE")
    if len(matches) > MAX_MATCHED_RECORDINGS:
        raise RuntimeError("TOO_MANY_RECORDINGS_FOR_BOUNDED_DIAGNOSIS")

    print("MODE=READ_ONLY")
    incomplete = False
    for i, path in enumerate(matches, 1):
        print("=== RECORDING_" + str(i) + "_OF_" + str(len(matches)) + " ===")
        # Labels are limited to local CLI output; never modify the source.
        print("RECORDING_DIRECTORY=" + str(path).replace(chr(10), " "))
        try:
            inspect(path)
            print("RECORDING_RESULT=READ_ONLY_COMPLETE")
        except (OSError, RuntimeError, ValueError) as error:
            incomplete = True
            print("RECORDING_STOP=" + str(error))
            print("RECORDING_RESULT=READ_ONLY_INCOMPLETE")
    if incomplete:
        raise RuntimeError("ONE_OR_MORE_RECORDINGS_INCOMPLETE")
    print("RESULT=READ_ONLY_COMPLETE")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", default="Pferd")
    parser.add_argument("--directory")
    parser.add_argument("--all-matches", action="store_true",
                        help="read-only checks of up to four matching recordings")
    args = parser.parse_args()
    if args.directory:
        matches = [Path(args.directory)]
    else:
        roots = configured_roots()
        if not roots:
            raise RuntimeError("VDR_VIDEO_ROOT_NOT_FOUND")
        matches = find_recordings(roots, args.title)
    diagnose_matches(matches, all_matches=args.all_matches)

if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError) as error:
        print("STOP=" + str(error), file=sys.stderr)
        print("RESULT=READ_ONLY_INCOMPLETE", file=sys.stderr)
        sys.exit(1)
