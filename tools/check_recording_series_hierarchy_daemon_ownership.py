#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

initialization = (
    ROOT
    / "core/daemon/src/DaemonRuntimeInitialization.cpp"
).read_text(encoding="utf-8")

backend_context = (
    ROOT
    / "core/daemon/src/DaemonRuntimeBackendContext.cpp"
).read_text(encoding="utf-8")

api_source = (
    ROOT
    / "api/rest/src/RecordingSeriesHierarchyApiRuntime.cpp"
).read_text(encoding="utf-8")

api_test = (
    ROOT
    / "api/rest/tests/test_recording_series_hierarchy_api_runtime.cpp"
).read_text(encoding="utf-8")


def require(condition, message):
    if not condition:
        raise SystemExit(
            "recording Series hierarchy daemon ownership: "
            + message
        )


configure = (
    "RecordingSeriesHierarchyApiRuntime::instance().configure(database_)"
)

require(
    initialization.count(configure) == 1,
    "global initialization must own exactly one configure(database_) call",
)

require(
    configure not in backend_context,
    "backend context must not own hierarchy runtime configuration",
)

require(
    '#include "RecordingSeriesHierarchyApiRuntime.h"'
    in initialization,
    "global initialization must include hierarchy runtime",
)

database_open = initialization.index(
    "database_.open(config_.databasePath())"
)

database_opened = initialization.index(
    'std::cout << "database opened"'
)

hierarchy_configure = initialization.index(
    configure
)

backend_context_creation = initialization.index(
    "createBackendRuntimeContext(runtimeBackend)"
)

require(
    database_open <
    database_opened <
    hierarchy_configure <
    backend_context_creation,
    "hierarchy configure must run after DB open and before backend contexts",
)

configure_window = initialization[
    hierarchy_configure:
    hierarchy_configure + 700
]

require(
    "return false;" in configure_window,
    "failed global hierarchy configuration must fail daemon initialization",
)

require(
    "failed to initialize recording Series hierarchy override runtime"
    in configure_window,
    "global initialization failure must remain diagnosable",
)

require(
    "recording Series hierarchy override runtime initialized"
    in initialization,
    "successful global initialization must remain visible in startup diagnostics",
)

require(
    "RecordingSeriesHierarchyOverrideRepository"
    in api_source,
    "API runtime must remain backed by hierarchy repository",
)

require(
    ".configure(" in api_test,
    "semantic API regression must exercise configure",
)

require(
    "recording_series_hierarchy_overrides"
    in api_test
    or "series-hierarchy" in api_test,
    "semantic API regression must retain hierarchy persistence coverage",
)

print(
    "recording Series hierarchy global database ownership contract ok"
)
