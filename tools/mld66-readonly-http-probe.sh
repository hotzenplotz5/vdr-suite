#!/bin/sh
# MLD.1A: explicitly selected, bounded local RESTfulAPI diagnostics.
# Prints timings, status and bytes only. Never prints or saves response bodies.
set -eu

usage() {
    echo 'Usage: sh tools/mld66-readonly-http-probe.sh --metadata|--recordings-sample' >&2
    exit 2
}

[ "$#" -eq 1 ] || usage
case "$1" in
    --metadata) mode=metadata ;;
    --recordings-sample) mode=recordings-sample ;;
    *) usage ;;
esac

if ! command -v curl >/dev/null 2>&1; then
    echo 'MLD66_HTTP_PROBE=CURL_UNAVAILABLE' >&2
    exit 3
fi

probe() {
    label=$1
    path=$2
    deadline=$3
    # -q first: ignore ~/.curlrc. Fixed loopback URL, no proxy, redirects,
    # credentials, uploaded content or request bodies. A single GET only.
    # Output is deliberately discarded, including on HTTP failure.
    if metrics=$(curl -q --silent --noproxy '*' --proto '=http' \
        --connect-timeout 2 --max-time "$deadline" --max-redirs 0 \
        --output /dev/null \
        --write-out 'http=%{http_code} seconds=%{time_total} bytes=%{size_download}' \
        "http://127.0.0.1:8002/$path" 2>/dev/null); then
        exit_code=0
    else
        exit_code=$?
    fi
    printf 'REQUEST=%s CURL_EXIT=%s %s\n' "$label" "$exit_code" "$metrics"
}

printf 'MLD66_HTTP_PROBE=READ_ONLY MODE=%s\n' "$mode"
case "$mode" in
    metadata)
        probe info info.json 5
        probe change-state change-state.json 5
        ;;
    recordings-sample)
        # Explicit opt-in: even limit=1 can require a full VDR list scan.
        # Only one request, never automatically retry or query the full list.
        probe recordings-sample 'recordings.json?start=0&limit=1' 30
        ;;
esac
printf 'MLD66_HTTP_PROBE=END\n'
