#!/usr/bin/env python3

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

backend = (
    ROOT / "core/daemon/src/DaemonRuntimeBackendContext.cpp"
).read_text(encoding="utf-8")

context = (
    ROOT / "core/daemon/include/BackendRuntimeContext.h"
).read_text(encoding="utf-8")

worker = (
    ROOT / "core/daemon/src/DaemonRuntimeEpgCache.cpp"
).read_text(encoding="utf-8")

errors = []

def require(condition, message):
    if not condition:
        errors.append(message)

# Separate EPG transport/service ownership must exist.
require(
    "std::unique_ptr<IHttpClient> epgHttpClient;" in context,
    "missing dedicated epgHttpClient"
)
require(
    "std::unique_ptr<IVdrAdapter> epgAdapter;" in context,
    "missing dedicated epgAdapter"
)
require(
    "std::unique_ptr<VdrService> epgService;" in context,
    "missing dedicated epgService"
)

# Normal and EPG client construction blocks are bounded by the following
# assignment, rather than by a regex ending at ");".  The cancellation lambda
# itself contains "load();", which would otherwise truncate the match.
normal_start = backend.find(
    "context->httpClient = std::make_unique<BasicHttpClient>("
)
normal_end = backend.find(
    "context->adapter = std::make_unique<RestfulApiVdrAdapter>(",
    normal_start,
)

require(
    normal_start >= 0 and normal_end > normal_start,
    "normal BasicHttpClient construction missing"
)

normal_client = (
    backend[normal_start:normal_end]
    if normal_start >= 0 and normal_end > normal_start
    else ""
)

require(
    "EpgRequestTimeout" not in normal_client,
    "normal backend client must not use the EPG timeout"
)

# EPG-only path must use the bounded 60 second timeout.
require(
    "constexpr std::chrono::seconds EpgRequestTimeout(60);" in backend,
    "EPG request timeout must be exactly 60 seconds"
)

epg_start = backend.find(
    "context->epgHttpClient = std::make_unique<BasicHttpClient>("
)
epg_end = backend.find(
    "context->epgAdapter = std::make_unique<RestfulApiVdrAdapter>(",
    epg_start,
)

require(
    epg_start >= 0 and epg_end > epg_start,
    "EPG BasicHttpClient construction missing"
)

epg_client = (
    backend[epg_start:epg_end]
    if epg_start >= 0 and epg_end > epg_start
    else ""
)

require(
    "EpgRequestTimeout" in epg_client,
    "EPG client does not use EpgRequestTimeout"
)

require(
    "*context->epgHttpClient" in backend,
    "EPG adapter is not wired to dedicated EPG HTTP client"
)
require(
    "*context->epgAdapter" in backend,
    "EPG service is not wired to dedicated EPG adapter"
)
require(
    "*context->epgService" in backend,
    "EPG cache service is not wired to dedicated EPG service"
)

# Worker must retry transient startup and dirty refresh failures.
require(
    "const int failureRetrySeconds = 120;" in worker,
    "worker retry interval must be 120 seconds"
)
require(
    'refreshEpgCacheForAllBackends("startup")' in worker,
    "startup EPG refresh missing"
)
require(
    "EPG cache startup refresh failed; retrying" in worker,
    "startup refresh retry handling missing"
)
require(
    'refreshEpgCacheForAllBackends("event-stream-dirty-hint")' in worker,
    "dirty-hint EPG refresh missing"
)
require(
    "epgCacheDirtyHint_.store(true);" in worker,
    "failed dirty refresh must restore dirty hint"
)
require(
    "EPG cache refresh failed; retrying" in worker,
    "dirty refresh retry handling missing"
)

# Retry delay must remain interruptible by shutdown.
require(
    worker.count("waitForStop(failureRetrySeconds)") >= 2,
    "retry waits must use shutdown-aware waitForStop"
)

# The old outer safety net may remain, but transient refresh exceptions must
# now be handled inside the worker before reaching it.
startup_pos = worker.find('refreshEpgCacheForAllBackends("startup")')
startup_retry_pos = worker.find("EPG cache startup refresh failed; retrying")
dirty_pos = worker.find(
    'refreshEpgCacheForAllBackends("event-stream-dirty-hint")'
)
dirty_retry_pos = worker.find("EPG cache refresh failed; retrying")

require(
    startup_pos >= 0 and startup_retry_pos > startup_pos,
    "startup retry handler ordering invalid"
)
require(
    dirty_pos >= 0 and dirty_retry_pos > dirty_pos,
    "dirty refresh retry handler ordering invalid"
)

if errors:
    for error in errors:
        print("FAIL:", error)
    sys.exit(1)

print("test_epg_cache_worker_recovery_contract passed")
