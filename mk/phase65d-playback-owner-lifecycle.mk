.PHONY: install-phase65d-playback-owner-lifecycle \
	test-phase65d-playback-owner-lifecycle \
	test-phase65d-playback-owner-lifecycle-install

install-runtime: install-phase65d-playback-owner-lifecycle

# ADR-0056 lifecycle publication is browser-local owner state. Prepend the
# publisher to the already composed playback bundle so every production owner
# sees it before any user-visible playback panel is created.
install-phase65d-playback-owner-lifecycle: install-phase65d-playback-contract-consumer
	cat \
		web/frontend/api/playback-owner-lifecycle.js \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js \
		> $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-lifecycle.js.tmp
	chmod 0644 $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-lifecycle.js.tmp
	mv -f \
		$(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-lifecycle.js.tmp \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js

test-phase65d-playback-owner-lifecycle:
	node --check web/frontend/api/playback-owner-lifecycle.js
	node web/frontend/tests/test_phase65d_playback_owner_lifecycle.js
	python3 tools/check_playback_frontend_integration_contract.py

test-phase65d-playback-owner-lifecycle-install: test-install-staging
	grep -F 'global.VdrSuitePlaybackOwnerLifecycle = Object.freeze' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js

test-frontend-contracts: test-phase65d-playback-owner-lifecycle
test-frontend-i18n: test-phase65d-playback-owner-lifecycle
test-recordings2-install-staging: test-phase65d-playback-owner-lifecycle-install
