#!/usr/bin/env python3
"""Read-only VDR-Suite media diagnosis. Run as an unprivileged user."""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile

MATCH = re.compile(r"ffmpeg|ffprobe|vdr-suite|trickplay|trickspeed|codec|hevc|h264|h265|transcod|422|error|failed|buffer|fps|speed=|frame=|media.?session|seek|hls|fmp4|mse|hdr", re.I)
REDACTIONS = [
    (re.compile(r"(?i)(authorization\s*[:=]\s*(?:bearer|basic)\s+)\S+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)((?:access[_-]?token|refresh[_-]?token|api[_-]?key|client[_-]?secret|password|passwd|session[_-]?token)\s*[:=]\s*)[^\s,;&]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)([?&](?:token|key|sig|signature|password|secret|auth|grant)=)[^\s&#]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(https?://)[^/@\s]+:[^/@\s]+@"), r"\1[REDACTED]@"),
    (re.compile(r"(?i)(cookie\s*[:=]\s*)[^\r\n]+"), r"\1[REDACTED]"),
]

def redact(value):
    for pattern, substitute in REDACTIONS:
        value = pattern.sub(substitute, value)
    return value

def command(argv, timeout=18):
    try:
        result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout, check=False,
                                text=True, errors="replace")
        return result.returncode, result.stdout[:1000000]
    except (OSError, subprocess.TimeoutExpired) as exc:
        return 127, str(exc)

def write(path, content):
    path.write_text(redact(content)[:1000000], encoding="utf-8")
    path.chmod(0o600)

def first_segment(recording):
    path = Path(recording)
    if path.is_symlink():
        raise ValueError("Symlink verweigert")
    if path.is_dir():
        segments = sorted(child for child in path.iterdir()
                          if re.fullmatch(r"\d{5}\.(ts|vdr)", child.name)
                          and child.is_file() and not child.is_symlink())
        if not segments:
            raise ValueError("Kein VDR-Segment vorhanden")
        path = segments[0]
    if path.is_symlink() or not path.is_file():
        raise ValueError("Keine regulaere Datei")
    return path.resolve()

def main():
    parser = argparse.ArgumentParser(description="VDR-Suite Media-/FFmpeg-/HTTP422-Diagnose, ohne sudo")
    parser.add_argument("--hours", type=int, default=2)
    parser.add_argument("--recording", help="Optionaler .rec-Ordner oder TS/VDR-Segment")
    parser.add_argument("--ffmpeg-log", action="append", default=[], help="Vorhandenes FFmpeg-Log; mehrfach moeglich")
    parser.add_argument("--out", help="Existierendes Ausgabeverzeichnis (Standard: aktuelles Verzeichnis)")
    args = parser.parse_args()
    if os.geteuid() == 0:
        parser.error("Nicht mit root/sudo ausfuehren")
    if not 1 <= args.hours <= 24:
        parser.error("--hours muss zwischen 1 und 24 liegen")
    base = Path(args.out or os.getcwd())
    if base.is_symlink() or not base.is_dir():
        parser.error("Ausgabe muss ein existierendes regulaeres Verzeichnis sein")
    base = base.resolve()
    os.umask(0o077)
    folder = Path(tempfile.mkdtemp(prefix="vdr-media-diagnose-", dir=base))
    summary = [
        "VDR-Suite Diagnose (read-only)",
        "Zeit: " + datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
        f"Journal-Rueckblick: {args.hours} Stunden",
        "Keine Echtzeit-FPS-Messung; nur bereits protokollierte Werte.",
    ]
    try:
        for name in ("ffmpeg", "ffprobe"):
            status, output = command([name, "-hide_banner", "-version"], timeout=8)
            write(folder / (name + "-version.txt"), output[:2400] if status == 0 else "Nicht verfuegbar: " + output)
        status, output = command(["systemctl", "list-units", "--all", "--type=service",
                                  "--no-legend", "--no-pager", "--plain"], timeout=10)
        services = []
        if status == 0:
            for line in output.splitlines():
                unit = line.strip().split(" ", 1)[0]
                if re.fullmatch(r"[A-Za-z0-9_.@-]+\.service", unit) and ("vdr" in unit.lower() or "suite" in unit.lower()):
                    services.append(unit)
        for unit in ("vdr-suite-daemon.service", "vdr.service"):
            if unit not in services:
                services.append(unit)
        for unit in services[:15]:
            status, output = command(["journalctl", "--no-pager", "--output=short-iso",
                                      "--since", f"{args.hours} hours ago", "-n", "2500", "-u", unit])
            lines = output.splitlines()
            selected = [line for line in lines if MATCH.search(line)]
            write(folder / ("journal-" + unit + ".txt"),
                  "\n".join(selected) if selected else "(Keine Treffer bzw. kein Journal-Zugriff)\n" + output[:2500])
            summary.append(f"{unit}: {len(lines)} gelesen, {len(selected)} Treffer; Exit={status}")
        for index, filename in enumerate(args.ffmpeg_log, 1):
            try:
                path = Path(filename)
                if path.is_symlink() or not path.is_file() or path.stat().st_size > 20000000:
                    raise ValueError("Symlink, Nicht-Datei oder Datei ueber 20 MB")
                with path.open("r", encoding="utf-8", errors="replace") as stream:
                    data = stream.read(4000000)
                matches = [line for line in data.splitlines() if MATCH.search(line)]
                write(folder / f"ffmpeg-log-{index}.txt", "\n".join(matches[-5000:]))
                summary.append(f"FFmpeg-Log {index}: {len(matches)} Treffer")
            except (OSError, ValueError) as exc:
                summary.append(f"FFmpeg-Log {index}: {exc}")
        if args.recording:
            try:
                file = first_segment(args.recording)
                fields = "format=format_name,duration,bit_rate:stream=index,codec_name,codec_type,profile,level,width,height,pix_fmt,color_space,color_transfer,color_primaries,r_frame_rate,avg_frame_rate,bit_rate,channels,channel_layout"
                status, output = command(["ffprobe", "-v", "error", "-probesize", "1048576",
                                          "-analyzeduration", "3000000", "-show_entries", fields,
                                          "-of", "json", "-i", str(file)], timeout=18)
                if status:
                    summary.append("ffprobe fehlgeschlagen: " + redact(output[:300]))
                else:
                    payload = json.loads(output)
                    streams = [{key: value for key, value in entry.items()
                                if key in ("index", "codec_name", "codec_type", "profile", "level",
                                           "width", "height", "pix_fmt", "color_space",
                                           "color_transfer", "color_primaries", "r_frame_rate",
                                           "avg_frame_rate", "bit_rate", "channels", "channel_layout")}
                               for entry in payload.get("streams", [])]
                    metadata = {key: value for key, value in payload.get("format", {}).items()
                                if key in ("format_name", "duration", "bit_rate")}
                    write(folder / "recording-ffprobe.json", json.dumps({"streams": streams, "format": metadata},
                                                                       ensure_ascii=False, indent=2))
                    summary.append("Aufnahme untersucht (erstes Segment): " + str(file))
            except (OSError, ValueError, json.JSONDecodeError) as exc:
                summary.append("Aufnahmefehler: " + str(exc))
        write(folder / "SUMMARY.txt", "\n".join(summary) + "\n")
        archive = base / (folder.name + ".tar.gz")
        with tarfile.open(archive, "w:gz") as output:
            for file in sorted(folder.iterdir()):
                output.add(file, arcname=folder.name + "/" + file.name, recursive=False)
        archive.chmod(0o600)
        print("FERTIG:", archive)
        print("Archiv vor Weitergabe auf private Angaben pruefen; Logtexte koennen Pfade/IP-Adressen enthalten.")
    finally:
        shutil.rmtree(folder)

if __name__ == "__main__":
    main()
