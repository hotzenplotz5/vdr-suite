#!/usr/bin/env python3
"""Guard the Slice-3 real-system preflight as strictly read-only."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/run_recording_cut_acceptance_preflight.sh"
errors: list[str] = []

if not RUNNER.is_file():
    errors.append("recording cut acceptance preflight runner is missing")
    text = ""
else:
    text = RUNNER.read_text(encoding="utf-8")

required = (
    "git branch --show-current",
    "git rev-parse HEAD",
    "git status --porcelain",
    "pkg-config --variable=libdir vdr",
    "pkg-config --variable=apiversion vdr",
    'PLUGIN_CANDIDATE="vdr-plugin-suite-bridge/libvdr-suitebridge.so"',
    'PLUGIN_INSTALLED="$VDR_LIBDIR/libvdr-suitebridge.so.$VDR_APIVERSION"',
    "cmp -s \"$DAEMON_CANDIDATE\" \"$DAEMON_INSTALLED\"",
    "cmp -s \"$AGENT_CANDIDATE\" \"$AGENT_INSTALLED\"",
    "cmp -s \"$PLUGIN_CANDIDATE\" \"$PLUGIN_INSTALLED\"",
    '"$DAEMON_CANDIDATE" "$AGENT_CANDIDATE" "$PLUGIN_CANDIDATE"',
    '"$DAEMON_INSTALLED" "$AGENT_INSTALLED" "$PLUGIN_INSTALLED"',
    "PLUG suitebridge CAPS 1",
    "PLUG suitebridge NCUT CAP 1 start",
    '"id":"recording-cut-state","state":"available"',
    "vdr-suite-ncut-cap/1 vdr.recording.cut 1 recording-cut enabled suitebridge",
    "/api/vdr/recordings/marks?$QUERY_STRING",
    "/api/vdr/recordings/cut?$QUERY_STRING",
    "curl_arguments+=(--get --request GET)",
    '"cut_ready": cut.get("ready") is True and cut.get("reason") == "ready"',
    '"cut_revision_matches": cut.get("marksRevision") == revision',
    '"cut_handler_idle": int(cut.get("handlerUsage", -1)) == 0',
    '"cut_destination_free": cut.get("editedDestinationExists") is False',
    '"cut_result_absent": cut.get("editedRecordingFound") is False',
    "HTTP_MUTATION=not_executed",
    "NCUT_EXEC=not_executed",
    "SERVICE_MUTATION=not_executed",
)
for token in required:
    if token not in text:
        errors.append(f"preflight missing required read-only token: {token}")

for pattern, label in (
    (r"PLUG\s+suitebridge\s+NCUT\s+EXEC\b", "native cut execution"),
    (r"PLUG\s+suitebridge\s+NMARKS\b", "manual marks mutation"),
    (r"--request(?:=|\s+)POST\b", "HTTP POST"),
    (r"(?:^|\s)-X\s*POST\b", "HTTP POST shorthand"),
    (r"(?:^|\s)--data(?:-raw|-binary)?(?:=|\s)", "HTTP request body"),
    (r"(?:^|\s)-d(?:\s|$)", "HTTP request body shorthand"),
    (r"\bsystemctl\s+(?:start|stop|restart|reload|enable|disable)\b", "service mutation"),
    (r"\bsqlite3\b", "raw SQLite access"),
    (r"(?:/srv/vdr|/srv/video|/var/lib/vdr).*(?:marks|\.rec)", "direct VDR recording filesystem access"),
):
    if re.search(pattern, text, flags=re.MULTILINE | re.IGNORECASE):
        errors.append(f"preflight contains forbidden {label}")

curl_lines = [line.strip() for line in text.splitlines() if line.lstrip().startswith("curl ")]
if len(curl_lines) != 2:
    errors.append("preflight must contain exactly two public API curl calls")
for line in curl_lines:
    if '"${curl_arguments[@]}"' not in line:
        errors.append("every preflight curl call must use the forced GET argument vector")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("recording cut acceptance preflight is read-only")
