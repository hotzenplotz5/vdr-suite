#!/usr/bin/env bash
umask 077

fail() {
    printf 'PHASE_64_MANAGED_TIMER_FULFILLMENT_ACCEPTANCE=FAIL\n' >&2
    printf 'REASON=%s\n' "$1" >&2
    if [[ -n "${EVIDENCE_DIR:-}" ]]; then
        printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR" >&2
    fi
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 ||
        fail "missing_command_$1"
}

run_logged() {
    local name="$1"
    shift
    "$@" >"$EVIDENCE_DIR/$name.log" 2>&1 || {
        tail -n 120 "$EVIDENCE_DIR/$name.log" >&2 || true
        fail "${name}_failed"
    }
}

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)" ||
    fail repository_root_unavailable
cd "$REPO_ROOT" || fail repository_unavailable

EXPECTED_HEAD="${PHASE64_EXPECTED_HEAD:-}"
if [[ ! "$EXPECTED_HEAD" =~ ^[0-9a-f]{40}$ ]]; then
    fail expected_head_missing_or_invalid
fi

for command in git make python3 g++ sha256sum readelf pkg-config; do
    require_command "$command"
done

ACTUAL_HEAD="$(git rev-parse HEAD 2>/dev/null)" ||
    fail head_unavailable
if [[ "$ACTUAL_HEAD" != "$EXPECTED_HEAD" ]]; then
    fail exact_candidate_head_mismatch
fi

BRANCH="$(git branch --show-current 2>/dev/null)" ||
    fail branch_unavailable
if [[ "$BRANCH" != "work/phase64-managed-timer-fulfillment" ]]; then
    fail unexpected_branch
fi

if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
    fail tracked_worktree_not_clean
fi

if ! git diff --check; then
    fail diff_check_failed
fi

EVIDENCE_DIR="/tmp/vdr-suite-phase64-${ACTUAL_HEAD}"
if [[ -e "$EVIDENCE_DIR" ]]; then
    fail evidence_directory_already_exists
fi
mkdir -m 700 "$EVIDENCE_DIR" ||
    fail evidence_directory_create_failed

{
    printf 'HEAD=%s\n' "$ACTUAL_HEAD"
    printf 'BRANCH=%s\n' "$BRANCH"
    printf 'UTC_STARTED=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'HOST=%s\n' "$(hostname)"
    printf 'KERNEL=%s\n' "$(uname -sr)"
    printf 'VDR_PC=%s\n' "$(pkg-config --modversion vdr 2>/dev/null || printf unavailable)"
} >"$EVIDENCE_DIR/context.txt" ||
    fail context_capture_failed

run_logged clean make clean
run_logged plugin-clean make -C vdr-plugin-suite-bridge clean
run_logged architecture make -j2 test-architecture
run_logged fast-regression make -j2 test-ci-fast
run_logged production-build make -j2 \
    daemon \
    backend-agent \
    backend-agent-enrollment \
    backend-agent-admin \
    backend-agent-command-admin
run_logged suitebridge-check make -C vdr-plugin-suite-bridge -j2 check

for candidate in \
    .build/vdr-suite-daemon \
    .build/vdr-suite-backend-agent \
    .build/vdr-suite-backend-agent-enroll \
    .build/vdr-suite-backend-agent-admin \
    .build/vdr-suite-backend-agent-command-admin \
    vdr-plugin-suite-bridge/libvdr-suitebridge.so
do
    if [[ ! -f "$candidate" ]]; then
        fail "candidate_missing_$(printf '%s' "$candidate" | tr '/.' '__')"
    fi
    sha256sum "$candidate" >>"$EVIDENCE_DIR/candidates.sha256" ||
        fail candidate_hash_failed
done

readelf -h vdr-plugin-suite-bridge/libvdr-suitebridge.so \
    >"$EVIDENCE_DIR/suitebridge.elf.txt" 2>&1 ||
    fail suitebridge_elf_check_failed

python3 - \
    packaging/systemd/backend-agent.conf \
    vdr-plugin-suite-bridge/suitebridge_svdrp.cpp \
    "$EVIDENCE_DIR/closed-advertisement.txt" <<'PY_CLOSED_ADVERTISEMENT' ||
    fail closed_advertisement_check_failed
from pathlib import Path
import sys

config = Path(sys.argv[1]).read_text(encoding="utf-8")
svdrp = Path(sys.argv[2]).read_text(encoding="utf-8")
output = Path(sys.argv[3])

if "COMMAND_TYPES=\n" not in config:
    raise SystemExit("packaged command advertisement is not closed")

help_start = svdrp.find("SVDRPHelpPages")
command_start = svdrp.find("SVDRPCommand", help_start)
if help_start < 0 or command_start < 0:
    raise SystemExit("SuiteBridge help boundary missing")
help_text = svdrp[help_start:command_start]
for command in ("NTCREATE", "NTMOD", "NTDELETE"):
    if command in help_text:
        raise SystemExit(f"private write advertised in help: {command}")

output.write_text(
    "PACKAGED_COMMAND_TYPES=closed\n"
    "PUBLIC_SVDRP_TIMER_WRITES=closed\n",
    encoding="utf-8",
)
PY_CLOSED_ADVERTISEMENT

if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
    fail tracked_worktree_changed_by_acceptance
fi
git diff --check ||
    fail final_diff_check_failed

{
    printf 'UTC_COMPLETED=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'RESULT=PASS\n'
} >>"$EVIDENCE_DIR/context.txt" ||
    fail completion_capture_failed

printf 'PHASE_64_MANAGED_TIMER_FULFILLMENT_ACCEPTANCE=PASS\n'
printf 'HEAD=%s\n' "$ACTUAL_HEAD"
printf 'ADVERTISEMENT=closed\n'
printf 'EVIDENCE=%s\n' "$EVIDENCE_DIR"
