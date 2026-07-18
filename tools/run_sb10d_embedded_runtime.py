#!/usr/bin/env python3

from pathlib import Path

updater = Path(__file__).with_name("apply_sb10d_embedded_runtime.py")
source = updater.read_text(encoding="utf-8")
patched = source.replace("\n    '''", "\n    r'''")

try:
    exec(
        compile(patched, str(updater), "exec"),
        {"__name__": "__main__", "__file__": str(updater)},
    )

    fixture = Path("core/vdr/tests/test_backend_runtime_context.cpp")
    fixture_text = fixture.read_text(encoding="utf-8")
    old_fixture = """    std::vector<VdrEvent> getEvents() const override
    {
        return {};
    }
"""
    new_fixture = """    std::vector<VdrEvent> getEvents() const override
    {
        return {};
    }

    std::vector<VdrEvent> getEvents(
        const VdrEventQuery&) const override
    {
        return {};
    }
"""

    if fixture_text.count(old_fixture) != 1:
        raise RuntimeError(
            "BackendRuntimeContext adapter fixture marker is not unique"
        )

    fixture.write_text(
        fixture_text.replace(old_fixture, new_fixture, 1),
        encoding="utf-8",
    )

    makefile = Path("mk/runtime-api-tests.mk")
    makefile_text = makefile.read_text(encoding="utf-8")
    old_target = """\t\t$(AGENT_SRC) \\
\t\tcore/vdr/src/VdrRecordingCacheRepository.cpp \\
\t\tcore/vdr/tests/test_backend_runtime_context.cpp \\
"""
    new_target = """\t\t$(AGENT_SRC) \\
\t\tcore/daemon/src/RestfulApiEventStreamClient.cpp \\
\t\tcore/vdr/src/VdrRecordingCacheRepository.cpp \\
\t\tcore/vdr/tests/test_backend_runtime_context.cpp \\
"""

    if makefile_text.count(old_target) != 1:
        raise RuntimeError(
            "BackendRuntimeContext test target marker is not unique"
        )

    makefile.write_text(
        makefile_text.replace(old_target, new_target, 1),
        encoding="utf-8",
    )
except Exception as error:
    print(f"SB10D_IMPLEMENTATION_ERROR: {error}", flush=True)
    raise SystemExit(1)
