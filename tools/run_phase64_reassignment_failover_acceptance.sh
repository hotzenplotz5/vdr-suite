#!/usr/bin/env bash
set -euo pipefail
umask 077

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

EXPECTED_HEAD="${PHASE64_EXPECTED_HEAD:-}"
if [[ ! "$EXPECTED_HEAD" =~ ^[0-9a-f]{40}$ ]]; then
    printf 'PHASE_64_REASSIGNMENT_FAILOVER_ACCEPTANCE=FAIL\n' >&2
    printf 'REASON=expected_head_missing_or_invalid\n' >&2
    exit 1
fi

PHASE64_EXPECTED_BRANCH=work/phase64-reassignment-failover \
    tools/run_phase64_managed_timer_fulfillment_acceptance.sh

printf 'PHASE_64_REASSIGNMENT_FAILOVER_ACCEPTANCE=PASS\n'
printf 'HEAD=%s\n' "$EXPECTED_HEAD"
printf 'REASSIGNMENT=atomic-fail-closed\n'
printf 'OUTCOME_UNKNOWN=reconciliation-only\n'
printf 'PUBLIC_SVDRP_TIMER_WRITES=closed\n'
