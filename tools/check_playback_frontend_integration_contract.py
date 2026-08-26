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
    lifecycle_publisher = read(FRONTEND / "api/playback-owner-lifecycle.js")
    fast_owner = read(FRONTEND / "api/session-frontend-sync.js")
    fallback_owner = read(FRONTEND / "api/recording-fallback-controls.js")
    track_owner = read(FRONTEND / "api/recording-track-controls.js")
    volume_owner = read(FRONTEND / "api/playback-volume-controls.js")
    lifecycle_publication_test = read(
        FRONTEND / "tests/test_phase65d_playback_owner_lifecycle.js"
    )
    lifecycle_test = read(
        FRONTEND / "tests/test_phase65d_recording_progressive_hls_track_owner.js"
    )
    subtitle_lifecycle_test = read(
        FRONTEND / "tests/test_phase65d_recording_subtitle_track_controls.js"
    )
    volume_lifecycle_test = read(
        FRONTEND / "tests/test_phase65d_playback_volume_controls.js"
    )

    require(
        "docs/development/frontend-playback-integration-contract.md" in agents,
        "AGENTS.md must bind the playback integration contract",
    )
    for token in (
        "Method interception is not a lifecycle contract",
        "Observe the whole owner lifetime",
        "Real action-to-request proof",
        "Required test shape for client-local Volume/Mute",
        "Subtitle pre-implementation gate",
    ):
        require(token in contract, f"binding contract is missing section: {token}")

    require(
        "const playback = global.VdrSuiteRecordings2Playback" in browser_view
        and "activePlayback = playback.createPanel(recording, currentState.backendId)" in browser_view,
        "Recordings 2 detail view must compose the public Recording playback owner",
    )

    # ADR-0056 Slice 2 publishes lifecycle directly from the persistent owner.
    # A local monotonic lifecycle revision is observation ordering only; it must
    # not be confused with Slice-3 playback-presentation generation semantics.
    for token, message in (
        ("lifecycleVersion: LIFECYCLE_VERSION", "lifecycle snapshots must be versioned"),
        ("lifecycleRevision: revision", "lifecycle snapshots must expose local publication ordering"),
        ("function snapshot()", "lifecycle publisher must expose snapshot()"),
        ("function subscribe(listener)", "lifecycle publisher must expose subscribe()"),
        ("listeners.forEach", "lifecycle publisher must notify subscribers without server polling"),
    ):
        require(token in lifecycle_publisher, message)
    for forbidden, message in (
        ("VdrSuiteClientApi", "lifecycle publisher must not call the Suite API"),
        ("/api/media/sessions", "lifecycle publisher must not own MediaSessions"),
        ("generation:", "Slice 2 lifecycle publisher must not invent presentation generation"),
    ):
        require(forbidden not in lifecycle_publisher, message)

    # The production owner still binds its visible initial Start directly to an
    # internal closure. Lifecycle truth must therefore come from the owner
    # itself rather than exported-method interception or timer reconstruction.
    direct_internal_start = "startButton.addEventListener('click', startPlayback);" in fast_owner
    if direct_internal_start:
        require(
            "snapshot: lifecycleSnapshot" in fast_owner
            and "subscribe: subscribeLifecycle" in fast_owner,
            "persistent Recording owner must publish snapshot()/subscribe()",
        )
        require(
            "publishLifecycle('start-requested'" in fast_owner
            and "publishSession(id, 'progressive-fmp4'" in fast_owner,
            "owner-internal Start path must publish lifecycle and the first MediaSession",
        )
        require(
            "snapshot: lifecycleSnapshot" in fallback_owner
            and "subscribe: subscribeLifecycle" in fallback_owner,
            "HLS compatibility owner must publish the same lifecycle interface",
        )
        require(
            "panel.subscribe(handleOwnerLifecycle)" in track_owner
            and "function bindOwnerLifecycle()" in track_owner,
            "session-bound track controls must consume canonical owner publication",
        )
        require(
            "if (!bindOwnerLifecycle()) scheduleSessionWatch();" in track_owner,
            "legacy sessionId polling may remain only as compatibility fallback",
        )
        require(
            "if (usingCanonicalLifecycle) return result;" in track_owner,
            "canonical track integration must not depend on wrapped start() completion",
        )
        require(
            "SESSION_WATCH_MAX_ATTEMPTS" not in track_owner,
            "compatibility observation must not expire before a delayed user Start",
        )
        require(
            "internalStartButton.dispatch('click');" in lifecycle_publication_test,
            "lifecycle publication test must exercise the owner-internal visible Start path",
        )
        require(
            "await playback.start();" not in lifecycle_publication_test,
            "lifecycle publication test must not shortcut initial coverage through decorated start()",
        )
        require(
            "timerCalls, 0" in lifecycle_publication_test,
            "canonical lifecycle test must prove session discovery does not use a watch timer",
        )
        require(
            "internalStartButton.dispatch('click');" in subtitle_lifecycle_test,
            "subtitle integration test must exercise the owner-internal production-style Start path",
        )
        require(
            "await playback.start();" not in subtitle_lifecycle_test,
            "subtitle integration test must not shortcut initial lifecycle coverage through decorated start()",
        )

    require(
        "publishLifecycle('transport-replaced'" in fast_owner
        and "followFallbackLifecycle(fallbackPanel)" in fast_owner,
        "persistent owner must publish progressive-to-HLS transport replacement and follow child lifecycle",
    )
    require(
        "fastElement.replaceWith(fallbackElement);" in lifecycle_publication_test,
        "canonical lifecycle test must cover progressive-to-HLS transport replacement",
    )
    require(
        "transition: 'session-replaced'" in lifecycle_publication_test,
        "canonical lifecycle test must publish replacement MediaSession identity",
    )
    require(
        "fastElement.replaceWith(fallbackElement);" in lifecycle_test,
        "legacy topology integration test must retain progressive-to-HLS transport replacement coverage",
    )
    require(
        "fastElement.replaceWith(fallbackElement);" in subtitle_lifecycle_test,
        "subtitle integration test must cover progressive-to-HLS transport replacement",
    )

    # Once HLS owns the transport, the outer fast owner intentionally reports
    # only the presentation state `fallback`. Cross-cutting controls must use
    # the published HLS owner for actual playing/paused truth and for absolute
    # Recording position; the outer fast position is stale after replacement.
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
            "position: position" in fallback_owner,
            "published HLS owner must expose its canonical absolute Recording position",
        )
        require(
            "const positionOwner = hlsOwner && typeof hlsOwner.position === 'function'" in track_owner
            and "Number(positionOwner.position())" in track_owner,
            "subtitle stream base must resolve absolute position from the active HLS owner",
        )
        require(
            "outer fast-owner position intentionally stays stale after fallback" in subtitle_lifecycle_test,
            "subtitle integration test must model stale outer position after HLS replacement",
        )

    require(
        "requests.some(body => body.sessionId === 'progressive-session-1')" in lifecycle_publication_test,
        "canonical lifecycle test must prove first-session track-status request",
    )
    require(
        "requests.some(body => body.sessionId === 'hls-session-1')" in lifecycle_publication_test,
        "canonical lifecycle test must prove replacement-session track-status request",
    )
    require(
        "requests.some(body => body.sessionId === 'hls-session-2')" in lifecycle_publication_test,
        "canonical lifecycle test must verify a session replaced by HLS track selection",
    )
    require(
        "fallbackSelections, 1" in lifecycle_publication_test,
        "canonical lifecycle test must prove audio selection delegates to the established HLS owner",
    )
    require(
        "snapshot.generation === undefined" in lifecycle_publication_test,
        "Slice 2 test must prove publication does not invent Slice-3 generation",
    )

    # Subtitle delivery is session- and stream-base-bound. The production-style
    # test must prove normalized IDs, explicit OFF, browser-native WebVTT,
    # same-session base rebinding and replacement-session preference retention.
    for token, message in (
        ("['off', 'subtitle-1']", "subtitle test must keep unsupported DVB/Teletext tracks out of the selector"),
        ("subtitleTrackId, 'subtitle-1'", "subtitle test must prove normalized subtitle-N request IDs"),
        ("subtitleTrackId, 'off'", "subtitle test must prove explicit OFF request semantics"),
        ("streamBasePositionSeconds, 0", "subtitle test must prove initial Recording stream base"),
        ("streamBasePositionSeconds === 120", "subtitle test must prove same-session seek/base rebinding"),
        ("streamBasePositionSeconds === 300", "subtitle test must prove replacement-session stream base"),
        ("streamBasePositionSeconds === 337", "subtitle test must prove audio replacement uses the active HLS absolute position"),
        ("normal HLS playback progress must not continuously regenerate WebVTT", "subtitle test must keep a stable HLS stream base during normal playback"),
        ("revokeObjectURL", "subtitle test must prove stale WebVTT Blob cleanup"),
        ("mountedTrack.track.mode, 'showing'", "subtitle test must prove browser-native text-track activation"),
        ("fallbackSelections, 1", "subtitle test must keep HLS audio selection on the existing fallback owner"),
    ):
        require(token in subtitle_lifecycle_test, message)

    require(
        "subtitlePreferenceTrackId" in track_owner
        and "managedSubtitleElement" in track_owner
        and "streamBasePosition()" in track_owner,
        "subtitle delivery must remain attached to the established Recording track owner",
    )
    require(
        "select-subtitle-track" in track_owner
        and "streamBasePositionSeconds" in track_owner,
        "track owner must use the normalized session-bound subtitle selection contract",
    )

    # Volume/Mute is deliberately not session-bound. ADR-0053 makes it local
    # HTMLMediaElement state, so the shared decorator must wrap both Recording
    # and Live factories without acquiring server or transport authority.
    require(
        "decorated.createPanel" in volume_owner and "decorated.createLivePanel" in volume_owner,
        "Volume/Mute must use one shared Recording/Live playback decorator",
    )
    require(
        "const clientPreference" in volume_owner,
        "Volume/Mute must retain confirmed page-local state across clean owner replacement",
    )
    require(
        "video.volume" in volume_owner
        and "video.muted" in volume_owner
        and "volumechange" in volume_owner,
        "Volume/Mute must derive state from the active HTMLMediaElement",
    )
    require(
        "panel.replaceWith(fallbackPanel.element);" in fast_owner,
        "production Recording fallback must remain modeled as complete panel replacement",
    )
    require(
        "const next = firstVideo(shell);" in volume_owner,
        "Volume/Mute must resolve the active media element across complete panel replacement",
    )
    require(
        "observer.observe(shell, {childList: true, subtree: true});" in volume_owner,
        "Volume/Mute replacement observer must watch the stable owner shell across fallback replacement",
    )
    require(
        "if (firstVideo(shell) !== activeVideo) bindCurrentVideo();" in volume_owner,
        "Volume/Mute shell observer must remain inert unless the active media element actually changes",
    )
    for forbidden, message in (
        ("VdrSuiteClientApi", "Volume/Mute must not call the Suite server API"),
        ("/api/media/sessions", "Volume/Mute must not mutate MediaSessions"),
        ("video.play(", "Volume/Mute must not start playback"),
        ("video.pause(", "Volume/Mute must not pause playback"),
        ("video.load(", "Volume/Mute must not reload playback"),
    ):
        require(forbidden not in volume_owner, message)

    for token, message in (
        ("api.volumeFromPercent(25), 0.25", "volume test must prove UI 0..100 to media 0..1 mapping"),
        ("recordingMute.dispatch('click');", "volume test must exercise mute/unmute through visible UI"),
        ("recording.element.replaceChild(replacement, oldPresentation);", "volume test must exercise complete progressive-to-HLS panel replacement"),
        ("fake browser must deliver Volume/Mute textContent mutations", "volume test must model real-browser observer delivery from control text updates"),
        ("must not create an observer feedback loop", "volume test must fail on a self-triggering shell observer"),
        ("replacement observer must watch the stable outer owner shell", "volume test must keep replacement observation on the persistent owner shell"),
        ("miniPlayer.appendChild(live.element);", "volume test must exercise persistent presentation reparenting"),
        ("replacement Live owner must keep confirmed volume", "volume test must exercise clean Live owner replacement handoff"),
        ("runtime.metrics.startCalls(), 0", "volume test must prove changes do not start/restart playback"),
        ("volume decorator must not create a second media element", "volume test must prove single-media-element ownership"),
        ("ignoreVolumeWrites", "volume test must cover capability/read-back failure handling"),
    ):
        require(token in volume_lifecycle_test, message)

    print("playback frontend integration contracts ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
