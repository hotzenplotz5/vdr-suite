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
    old = """    std::vector<VdrEvent> getEvents() const override
    {
        return {};
    }
"""
    new = """    std::vector<VdrEvent> getEvents() const override
    {
        return {};
    }

    std::vector<VdrEvent> getEvents(
        const VdrEventQuery&) const override
    {
        return {};
    }
"""

    if fixture_text.count(old) != 1:
        raise RuntimeError(
            "BackendRuntimeContext adapter fixture marker is not unique"
        )

    fixture.write_text(
        fixture_text.replace(old, new, 1),
        encoding="utf-8",
    )
except Exception as error:
    print(f"SB10D_IMPLEMENTATION_ERROR: {error}", flush=True)
    raise SystemExit(1)
