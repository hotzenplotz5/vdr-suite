#!/usr/bin/env python3
"""Binding production-lifecycle checks for Recording playback frontend integration."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FRONTEND = ROOT / "web" / "frontend"


def read(path: Path) -> str:
    if not path.exists():
        raise SystemExit(f"playback frontend integration contract missing file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"playback frontend integration contract failed: {message}")


def main() -> int:
    agents = read(ROOT / "AGENTS.md")
    contract = read(ROOT / "docs/development/frontend-playback-integration-contract.md")
    browser_view = read(FRONTEND / "recordings2-browser-view.js")
    fast_owner = read(FRONTEND / "api/session-frontend-sync.js")
    track_owner = read(FRONTEND / "api/recording-track-controls.js")
    lifecycle_test = read(
        FRONTEND / "tests/test_phase65d_recording_progressive_hls_track_owner.js"
    )

    require(
        "docs/development/frontend-playback-integration-contract.md" in agents,
        "AGENTS.md must bind the playback integration contract",
    )
    for token in (
        "Method interception is not a lifecycle contract",
        "Observe the whole owner lifetime",
        "Real action-to-request proof",
        "Subtitle pre-implementation gate",
    ):
        require(token in contract, f"binding contract is missing section: {token}")

    require(
        "const playback = global.VdrSuiteRecordings2Playback" in browser_view
        and "activePlayback = playback.createPanel(recording, currentState.backendId)" in browser_view,
        "Recordings 2 detail view must compose the public Recording playback owner",
    )

    # The current production owner binds its visible initial Start directly to
    # the internal closure. As long as that topology exists, extensions cannot
    # rely on interception of the exported start() method for lifecycle truth.
    direct_internal_start = "startButton.addEventListener('click', startPlayback);" in fast_owner
    if direct_internal_start:
        require(
            "scheduleSessionWatch();" in track_owner,
            "track owner must start canonical session observation at owner creation",
        )
        require(
            "else if (currentId && currentId !== activeSessionId)" in track_owner,
            "track owner must discover the first internally-started MediaSession",
        )
        require(
            "SESSION_WATCH_MAX_ATTEMPTS" not in track_owner,
            "track owner observation must not expire before a delayed user Start",
        )
        require(
            "internalStartButton.dispatch('click');" in lifecycle_test,
            "integration test must exercise the owner-internal production-style Start path",
        )
        require(
            "await playback.start();" not in lifecycle_test,
            "integration test must not shortcut initial lifecycle coverage through decorated start()",
        )

    require(
        "fastElement.replaceWith(fallbackElement);" in lifecycle_test,
        "integration test must cover progressive-to-HLS transport replacement",
    )

    # Once HLS owns the transport, the outer fast owner intentionally reports
    # only the presentation state `fallback`. Cross-cutting controls must use
    # the published HLS owner for actual playing/paused truth.
    if "if (fallbackPanel) return 'fallback';" in fast_owner:
        require(
            "const playbackState = usingHlsOwner ? text(hlsOwner.state()) : text(panel.state());" in track_owner,
            "HLS track selection must resolve playing/paused from the active HLS owner",
        )
        require(
            "assert.strictEqual(basePanel.state(), 'fallback'" in lifecycle_test,
            "integration test must model the outer fast owner's production fallback state",
        )

    require(
        "requests.some(body => body.sessionId === 'progressive-session-1')" in lifecycle_test,
        "integration test must prove first-session track-status request",
    )
    require(
        "requests.some(body => body.sessionId === 'hls-session-1')" in lifecycle_test,
        "integration test must prove replacement-session track-status request",
    )
    require(
        "fallbackSelections, 1" in lifecycle_test,
        "integration test must prove audio selection delegates to the existing HLS owner",
    )

    print("playback frontend integration contracts ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
