.PHONY: install-phase65d-playback-volume-controls \
	test-phase65d-playback-volume-controls \
	test-phase65d-playback-volume-install-staging

install-runtime: install-phase65d-playback-volume-controls

# Keep Volume/Mute outside the replaceable Recording transport while composing
# it after the established fallback + track-control owner chain. The installed
# session frontend runtime remains the one production composition entry.
install-phase65d-playback-volume-controls: install-phase65d-recording-track-controls
	cat \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js \
		web/frontend/api/playback-volume-controls.js \
		> $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-volume.js.tmp
	chmod 0644 $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-volume.js.tmp
	mv -f \
		$(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-volume.js.tmp \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js

test-phase65d-playback-volume-controls:
	node --check web/frontend/api/playback-volume-controls.js
	node web/frontend/tests/test_phase65d_playback_volume_controls.js
	python3 tools/check_playback_frontend_integration_contract.py

test-phase65d-playback-volume-install-staging: test-install-staging
	grep -F 'global.VdrSuitePlaybackVolumeControls = Object.freeze' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js

test-frontend-i18n: test-phase65d-playback-volume-controls

test-frontend-contracts: test-phase65d-playback-volume-controls

test-recordings2-install-staging: test-phase65d-playback-volume-install-staging
