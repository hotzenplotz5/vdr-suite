.PHONY: install-phase65d-playback-owner-lifecycle \
	test-phase65d-playback-failure-classification \
	test-phase65d-playback-owner-lifecycle \
	test-phase65d-timeline-continuity \
	test-phase65d-recording-network-recovery \
	test-phase65d-playback-owner-lifecycle-install

install-runtime: install-phase65d-playback-owner-lifecycle

# ADR-0056 lifecycle publication and classified client failure semantics are
# browser-local owner state/evidence. ADR-0057 adds one bounded policy for a
# demonstrated completed-Recording network interruption. The recovery guard is
# prepended before the established playback composition so the already-owned HLS
# compatibility factory can be suppressed only during an authorized recovery
# attempt. The recovery policy is appended after timeline/continuity so it
# consumes the final canonical owner surface and delegates start/seek/play back
# to that same owner.
install-phase65d-playback-owner-lifecycle: install-phase65d-playback-contract-consumer
	cat \
		web/frontend/api/recording-network-recovery-guard.js \
		web/frontend/api/playback-owner-lifecycle.js \
		web/frontend/api/playback-failure-classification.js \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js \
		web/frontend/api/playback-timeline-continuity.js \
		web/frontend/api/recording-network-recovery.js \
		> $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-lifecycle.js.tmp
	chmod 0644 $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-lifecycle.js.tmp
	mv -f \
		$(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-lifecycle.js.tmp \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js

test-phase65d-playback-failure-classification:
	node --check web/frontend/api/playback-failure-classification.js
	node web/frontend/tests/test_phase65d_playback_failure_classification.js

test-phase65d-playback-owner-lifecycle: test-phase65d-playback-failure-classification
	node --check web/frontend/api/playback-owner-lifecycle.js
	node web/frontend/tests/test_phase65d_playback_owner_lifecycle.js
	node web/frontend/tests/test_phase65d_restart_choice_owner_shell.js
	python3 tools/check_playback_frontend_integration_contract.py

test-phase65d-timeline-continuity:
	node --check web/frontend/api/playback-owner-lifecycle.js
	node --check web/frontend/api/playback-timeline-continuity.js
	node web/frontend/tests/test_phase65d_playback_presentation_generation.js
	node web/frontend/tests/test_phase65d_timeline_continuity_progressive.js
	node web/frontend/tests/test_phase65d_timeline_continuity_hls.js
	python3 tools/check_playback_frontend_integration_contract.py

test-phase65d-recording-network-recovery:
	node --check web/frontend/api/recording-network-recovery-guard.js
	node --check web/frontend/api/recording-network-recovery.js
	node web/frontend/tests/test_phase65d_recording_network_recovery.js
	python3 tools/check_playback_frontend_integration_contract.py

test-phase65d-playback-owner-lifecycle-install: test-install-staging
	grep -F '__vdrSuiteRecordingNetworkRecoveryGuardBound' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	grep -F 'global.VdrSuitePlaybackOwnerLifecycle = Object.freeze' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	grep -F 'global.VdrSuitePlaybackFailureClassification = Object.freeze' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	grep -F '__vdrSuitePlaybackTimelineContinuityBound' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	grep -F '__vdrSuiteRecordingNetworkRecoveryBound' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js

test-frontend-contracts: test-phase65d-playback-owner-lifecycle test-phase65d-timeline-continuity test-phase65d-recording-network-recovery
test-frontend-i18n: test-phase65d-playback-owner-lifecycle test-phase65d-timeline-continuity test-phase65d-recording-network-recovery
test-recordings2-install-staging: test-phase65d-playback-owner-lifecycle-install
