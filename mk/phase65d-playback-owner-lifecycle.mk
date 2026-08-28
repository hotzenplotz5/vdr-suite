.PHONY: install-phase65d-playback-owner-lifecycle \
	test-phase65d-playback-failure-classification \
	test-phase65d-playback-owner-lifecycle \
	test-phase65d-timeline-continuity \
	test-phase65d-playback-owner-lifecycle-install

install-runtime: install-phase65d-playback-owner-lifecycle

# ADR-0056 lifecycle publication and classified client failure semantics are
# browser-local owner state/evidence. Prepend both helpers to the already
# composed playback bundle so the persistent production owner sees them before
# any user-visible playback panel is created. Classification is observational;
# recovery/replacement remains an explicit action of that existing owner.
# Slice 3 then appends its bounded Recording timeline/continuity projection
# after the full established owner/decorator chain.
install-phase65d-playback-owner-lifecycle: install-phase65d-playback-contract-consumer
	cat \
		web/frontend/api/playback-owner-lifecycle.js \
		web/frontend/api/playback-failure-classification.js \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js \
		web/frontend/api/playback-timeline-continuity.js \
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

test-phase65d-playback-owner-lifecycle-install: test-install-staging
	grep -F 'global.VdrSuitePlaybackOwnerLifecycle = Object.freeze' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	grep -F 'global.VdrSuitePlaybackFailureClassification = Object.freeze' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	grep -F '__vdrSuitePlaybackTimelineContinuityBound' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js

test-frontend-contracts: test-phase65d-playback-owner-lifecycle test-phase65d-timeline-continuity
test-frontend-i18n: test-phase65d-playback-owner-lifecycle test-phase65d-timeline-continuity
test-recordings2-install-staging: test-phase65d-playback-owner-lifecycle-install
