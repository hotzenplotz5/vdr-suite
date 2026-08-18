#!/usr/bin/env python3
"""Calibrate VDR-Suite software video-transcode workloads for auto policy."""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Dict, Optional

PRESETS = ("superfast", "veryfast", "faster", "fast")
QUALITY_ORDER = ("fast", "faster", "veryfast", "superfast")
MINIMUM_REALTIME_SPEED = 1.25
PROFILE_VERSION = 3
DEFAULT_REAL_SOURCE_SECONDS = 30
DEFAULT_REAL_SOURCE_START = 15
SPEED_RE = re.compile(r"speed=\s*([0-9]+(?:\.[0-9]+)?)x")

WORKLOADS = {
    "standard": {
        "filter": "scale=1920:1080",
        "fallback": "veryfast",
    },
    "deinterlace": {
        "filter": (
            "bwdif=mode=send_frame:parity=auto:deint=all,"
            "scale=1920:1080"
        ),
        "fallback": "superfast",
    },
    "uhd-source": {
        "filter": "scale=1920:1080",
        "fallback": "veryfast",
    },
}


def run_checked(command: list[str], timeout: int, description: str) -> None:
    completed = subprocess.run(
        command,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout,
        check=False,
    )
    if completed.returncode != 0:
        tail = "\n".join(completed.stderr.splitlines()[-12:])
        raise RuntimeError(f"{description} failed:\n{tail}")


def ffmpeg_has_encoder(ffmpeg: str, encoder: str) -> bool:
    completed = subprocess.run(
        [ffmpeg, "-hide_banner", "-encoders"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=30,
        check=False,
    )
    if completed.returncode != 0:
        return False
    return any(
        line.split()[1:2] == [encoder]
        for line in completed.stdout.splitlines()
        if line.strip()
    )


def resolve_reference(path_text: Optional[str]) -> Optional[pathlib.Path]:
    if not path_text:
        return None
    path = pathlib.Path(path_text)
    if path.is_dir():
        segments = sorted(path.glob("[0-9][0-9][0-9][0-9][0-9].ts"))
        if not segments:
            raise RuntimeError(f"no VDR TS segment found below {path}")
        path = segments[0]
    if not path.is_file():
        raise RuntimeError(f"reference source is not a regular file: {path}")
    return path.resolve()


def generate_fixture(
    ffmpeg: str,
    workload: str,
    seconds: int,
    root: pathlib.Path,
    has_libx265: bool,
) -> Optional[pathlib.Path]:
    duration = seconds + 2
    output = root / f"{workload}.mkv"
    base = [
        ffmpeg,
        "-hide_banner",
        "-v",
        "error",
        "-y",
        "-f",
        "lavfi",
    ]

    if workload == "standard":
        command = base + [
            "-i", "testsrc2=size=1920x1080:rate=25",
            "-t", str(duration),
            "-an",
            "-c:v", "libx264",
            "-preset", "ultrafast",
            "-crf", "18",
            "-pix_fmt", "yuv420p",
            str(output),
        ]
    elif workload == "deinterlace":
        command = base + [
            "-i", "testsrc2=size=1920x1080:rate=50",
            "-t", str(duration),
            "-an",
            "-vf", "tinterlace=mode=interleave_top",
            "-c:v", "libx264",
            "-preset", "ultrafast",
            "-crf", "18",
            "-flags", "+ilme+ildct",
            "-x264-params", "tff=1",
            "-pix_fmt", "yuv420p",
            str(output),
        ]
    elif workload == "uhd-source":
        if not has_libx265:
            return None
        command = base + [
            "-i", "testsrc2=size=3840x2160:rate=25",
            "-t", str(duration),
            "-an",
            "-c:v", "libx265",
            "-preset", "ultrafast",
            "-crf", "20",
            "-x265-params", "log-level=error",
            "-pix_fmt", "yuv420p",
            str(output),
        ]
    else:
        raise RuntimeError(f"unknown workload: {workload}")

    run_checked(
        command,
        timeout=max(120, duration * 30),
        description=f"fixture generation for {workload}",
    )
    return output


def benchmark(
    ffmpeg: str,
    workload: str,
    preset: str,
    seconds: int,
    source: pathlib.Path,
    include_audio: bool,
    start_seconds: int,
) -> float:
    command = [
        ffmpeg,
        "-hide_banner",
        "-stats",
        "-stats_period",
        "0.5",
        "-v",
        "warning",
    ]
    if start_seconds > 0:
        command += ["-ss", str(start_seconds)]
    command += [
        "-i",
        str(source),
        "-map",
        "0:v:0",
    ]
    if include_audio:
        command += ["-map", "0:a:0?"]
    command += [
        "-t",
        str(seconds),
        "-sn",
        "-c:v",
        "libx264",
        "-preset",
        preset,
        "-crf",
        "20",
        "-vf",
        WORKLOADS[workload]["filter"],
        "-pix_fmt",
        "yuv420p",
    ]
    if include_audio:
        command += ["-c:a", "aac", "-b:a", "192k", "-ac", "2"]
    else:
        command += ["-an"]
    command += ["-f", "null", "-"]

    completed = subprocess.run(
        command,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
        timeout=max(60, seconds * 20),
        check=False,
    )
    if completed.returncode != 0:
        tail = "\n".join(completed.stderr.splitlines()[-12:])
        raise RuntimeError(
            f"ffmpeg benchmark failed for {workload}/{preset}:\n{tail}"
        )
    speeds = [float(match.group(1)) for match in SPEED_RE.finditer(completed.stderr)]
    if not speeds:
        raise RuntimeError(
            f"ffmpeg produced no speed metric for {workload}/{preset}"
        )
    return speeds[-1]


def policy_choice(workload: str, samples: Dict[str, float]) -> tuple[str, bool]:
    for preset in QUALITY_ORDER:
        if samples.get(preset, 0.0) >= MINIMUM_REALTIME_SPEED:
            return preset, True
    return str(WORKLOADS[workload]["fallback"]), False


def write_profile(
    path: pathlib.Path,
    results: Dict[str, Dict[str, float]],
    source_kinds: Dict[str, str],
    durations: Dict[str, int],
    starts: Dict[str, int],
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Generated by vdr-suite-media-calibrate.",
        "# Measurements include source decode plus the workload transform and x264 encode.",
        f"version={PROFILE_VERSION}",
    ]
    for workload in WORKLOADS:
        if workload not in results:
            continue
        lines.append(f"# {workload}.source={source_kinds[workload]}")
        lines.append(f"# {workload}.start={starts[workload]}")
        lines.append(f"# {workload}.seconds={durations[workload]}")
        for preset in PRESETS:
            lines.append(f"{workload}.{preset}={results[workload][preset]:.3f}")
    lines.append("")

    handle = tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        dir=str(path.parent),
        prefix=f".{path.name}.",
        delete=False,
    )
    temporary = pathlib.Path(handle.name)
    try:
        with handle:
            handle.write("\n".join(lines))
            handle.flush()
            os.fsync(handle.fileno())
        os.chmod(temporary, 0o644)
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Benchmark representative decoded VDR-Suite x264 workloads and write "
            "the server performance profile used by VDR_SUITE_MEDIA_X264_PRESET=auto."
        )
    )
    parser.add_argument(
        "--output",
        default="/var/lib/vdr-suite/media-transcode-performance.conf",
        help="performance profile path",
    )
    parser.add_argument(
        "--seconds",
        type=int,
        default=6,
        help="media seconds per generated-fixture benchmark (default: 6)",
    )
    parser.add_argument(
        "--real-seconds",
        type=int,
        default=DEFAULT_REAL_SOURCE_SECONDS,
        help="media seconds per real-reference benchmark (default: 30)",
    )
    parser.add_argument(
        "--real-start",
        type=int,
        default=DEFAULT_REAL_SOURCE_START,
        help=(
            "seconds to skip at the beginning of real references before measuring "
            "(default: 15; use 0 to include recording pre-roll)"
        ),
    )
    parser.add_argument(
        "--standard-source",
        help="optional real file or .rec directory for the standard workload",
    )
    parser.add_argument(
        "--deinterlace-source",
        help="optional real interlaced file or .rec directory for deinterlace",
    )
    parser.add_argument(
        "--uhd-source",
        help="optional real UHD file or .rec directory for the UHD workload",
    )
    args = parser.parse_args()

    if args.seconds < 3 or args.seconds > 30:
        parser.error("--seconds must be between 3 and 30")
    if args.real_seconds < 15 or args.real_seconds > 60:
        parser.error("--real-seconds must be between 15 and 60")
    if args.real_start < 0 or args.real_start > 3600:
        parser.error("--real-start must be between 0 and 3600")

    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        print("vdr-suite-media-calibrate: ffmpeg not found", file=sys.stderr)
        return 2
    if not ffmpeg_has_encoder(ffmpeg, "libx264"):
        print("vdr-suite-media-calibrate: libx264 encoder not available", file=sys.stderr)
        return 2

    references = {
        "standard": args.standard_source,
        "deinterlace": args.deinterlace_source,
        "uhd-source": args.uhd_source,
    }
    has_libx265 = ffmpeg_has_encoder(ffmpeg, "libx265")
    results: Dict[str, Dict[str, float]] = {}
    source_kinds: Dict[str, str] = {}
    durations: Dict[str, int] = {}
    starts: Dict[str, int] = {}

    try:
        with tempfile.TemporaryDirectory(prefix="vdr-suite-media-calibrate-") as temp:
            root = pathlib.Path(temp)
            for workload in WORKLOADS:
                reference = resolve_reference(references[workload])
                real_reference = reference is not None
                if real_reference:
                    source = reference
                    source_kinds[workload] = f"real:{reference}"
                    durations[workload] = args.real_seconds
                    starts[workload] = args.real_start
                else:
                    source = generate_fixture(
                        ffmpeg, workload, args.seconds, root, has_libx265
                    )
                    if source is None:
                        print(
                            f"[{workload}] skipped: no representative fixture encoder available"
                        )
                        continue
                    source_kinds[workload] = "generated-compressed-fixture"
                    durations[workload] = args.seconds
                    starts[workload] = 0

                results[workload] = {}
                print(
                    f"[{workload}] source={source_kinds[workload]} "
                    f"start={starts[workload]} seconds={durations[workload]}"
                )
                for preset in PRESETS:
                    speed = benchmark(
                        ffmpeg,
                        workload,
                        preset,
                        durations[workload],
                        source,
                        include_audio=real_reference,
                        start_seconds=starts[workload],
                    )
                    results[workload][preset] = speed
                    verdict = "PASS" if speed >= MINIMUM_REALTIME_SPEED else "slow"
                    print(f"  {preset:9s} {speed:5.3f}x  {verdict}")

                choice, measured = policy_choice(workload, results[workload])
                if measured:
                    print(
                        f"  auto -> {choice} "
                        f"(minimum {MINIMUM_REALTIME_SPEED:.2f}x)"
                    )
                else:
                    print(
                        f"  auto -> fallback {choice} "
                        f"(no measured preset reaches {MINIMUM_REALTIME_SPEED:.2f}x)"
                    )
    except (RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"vdr-suite-media-calibrate: {error}", file=sys.stderr)
        return 3

    if not results:
        print("vdr-suite-media-calibrate: no workload could be measured", file=sys.stderr)
        return 3

    output = pathlib.Path(args.output)
    try:
        write_profile(output, results, source_kinds, durations, starts)
    except OSError as error:
        print(
            f"vdr-suite-media-calibrate: cannot write {output}: {error}",
            file=sys.stderr,
        )
        if str(output).startswith("/var/lib/"):
            print("Run the calibrator with sudo for the default system path.", file=sys.stderr)
        return 4

    print(f"Wrote {output}")
    print("Restart vdr-suite-daemon to load the new auto policy.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
