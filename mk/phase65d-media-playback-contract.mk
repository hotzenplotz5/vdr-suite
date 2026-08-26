DAEMON_SRC += \
	core/media/src/MediaPlaybackContract.cpp \
	api/rest/src/MediaPlaybackContractResponse.cpp

.PHONY: install-phase65d-playback-contract-consumer \
	test-phase65d-media-playback-contract \
	test-phase65d-media-playback-contract-response \
	test-phase65d-playback-contract-consumer \
	test-phase65d-playback-contract-consumer-install

install-runtime: install-phase65d-playback-contract-consumer

# Compose after the established track + Volume/Mute owner chain. This adapter
# changes only semantic capability projection; it does not own MediaSessions,
# transports, media elements, restart sequencing or lifecycle observation.
install-phase65d-playback-contract-consumer: install-phase65d-playback-volume-controls
	cat \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js \
		web/frontend/api/playback-contract-consumer.js \
		> $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-playback-contract.js.tmp
	chmod 0644 $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-playback-contract.js.tmp
	mv -f \
		$(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-playback-contract.js.tmp \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js

test-phase65d-media-playback-contract:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaPlaybackContract.cpp \
		core/media/tests/test_media_playback_contract.cpp \
		-o $(BUILD_DIR)/test_phase65d_media_playback_contract
	$(BUILD_DIR)/test_phase65d_media_playback_contract

test-phase65d-media-playback-contract-response:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include -Iapi/rest/include \
		core/media/src/MediaPlaybackContract.cpp \
		api/rest/src/MediaPlaybackContractResponse.cpp \
		api/rest/tests/test_media_playback_contract_response.cpp \
		-o $(BUILD_DIR)/test_phase65d_media_playback_contract_response
	$(BUILD_DIR)/test_phase65d_media_playback_contract_response

test-phase65d-playback-contract-consumer:
	node --check web/frontend/api/playback-contract-consumer.js
	node web/frontend/tests/test_phase65d_playback_contract_consumer.js

test-phase65d-playback-contract-consumer-install: test-install-staging
	grep -F '__vdrSuitePlaybackContractConsumer' \
		/tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js >/dev/null
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js

test-fast: test-phase65d-media-playback-contract test-phase65d-media-playback-contract-response
test-frontend-i18n: test-phase65d-playback-contract-consumer
test-frontend-contracts: test-phase65d-playback-contract-consumer
test-recordings2-install-staging: test-phase65d-playback-contract-consumer-install
