test-backend-registry-capability-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/tests/test_backend_registry_capability_resolver.cpp \
		-o $(BUILD_DIR)/test_backend_registry_capability_resolver
	$(BUILD_DIR)/test_backend_registry_capability_resolver

test-backend-registry-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		api/rest/src/BackendRegistryController.cpp \
		api/rest/tests/test_backend_registry_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_registry_controller
	$(BUILD_DIR)/test_backend_registry_controller

test-runtime-diagnostics-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RUNTIME_SRC) \
		$(REST_RUNTIME_SRC) \
		api/rest/tests/test_runtime_diagnostics_controller.cpp \
		-o $(BUILD_DIR)/test_runtime_diagnostics_controller
	$(BUILD_DIR)/test_runtime_diagnostics_controller

test-snapshot-change-feed-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		$(REST_SNAPSHOT_CHANGE_FEED_SRC) \
		api/rest/tests/test_snapshot_change_feed_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_change_feed_controller
	$(BUILD_DIR)/test_snapshot_change_feed_controller

test-live-transport-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		$(REST_LIVE_TRANSPORT_SRC) \
		api/rest/tests/test_live_transport_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_live_transport_controller
	$(BUILD_DIR)/test_live_transport_controller

test-epgsearch-native-fuzzy-startup-restore-validation: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStartupRestoreService.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_startup_restore_validation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_startup_restore_validation
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_startup_restore_validation

test-epgsearch-native-fuzzy-capability-detector:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/vdr/include \
		core/vdr/tests/test_epgsearch_native_fuzzy_capability_detector.cpp \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_detector
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_detector

test-epg-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchMatcher.cpp \
		core/vdr/src/EpgSearchRequestMapper.cpp core/vdr/src/EpgSearchService.cpp \
		core/vdr/src/EpgSearchResultJsonSerializer.cpp \
		api/rest/src/EpgController.cpp \
		api/rest/tests/test_epg_controller.cpp \
		-o $(BUILD_DIR)/test_epg_controller
	$(BUILD_DIR)/test_epg_controller

test-genre-classification:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_genre_classification.cpp \
		-o $(BUILD_DIR)/test_genre_classification
	$(BUILD_DIR)/test_genre_classification

test-genre-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_genre_resolver.cpp \
		-o $(BUILD_DIR)/test_genre_resolver
	$(BUILD_DIR)/test_genre_resolver

test-canonical-genre-registry:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_canonical_genre_registry.cpp \
		-o $(BUILD_DIR)/test_canonical_genre_registry
	$(BUILD_DIR)/test_canonical_genre_registry

test-genre-localization:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_genre_localization.cpp \
		-o $(BUILD_DIR)/test_genre_localization
	$(BUILD_DIR)/test_genre_localization

test-genre-resolution-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/GenreResolutionJsonSerializer.cpp \
		core/vdr/tests/test_genre_resolution_json_serializer.cpp \
		-o $(BUILD_DIR)/test_genre_resolution_json_serializer
	$(BUILD_DIR)/test_genre_resolution_json_serializer

test-content-rating:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_content_rating.cpp \
		-o $(BUILD_DIR)/test_content_rating
	$(BUILD_DIR)/test_content_rating

test-content-rating-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_content_rating_resolver.cpp \
		-o $(BUILD_DIR)/test_content_rating_resolver
	$(BUILD_DIR)/test_content_rating_resolver

test-content-rating-resolution-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/ContentRatingResolutionJsonSerializer.cpp \
		core/vdr/tests/test_content_rating_resolution_json_serializer.cpp \
		-o $(BUILD_DIR)/test_content_rating_resolution_json_serializer
	$(BUILD_DIR)/test_content_rating_resolution_json_serializer

test-content-rating-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/ContentRatingResolutionJsonSerializer.cpp \
		api/rest/src/ContentRatingController.cpp \
		api/rest/tests/test_content_rating_controller.cpp \
		-o $(BUILD_DIR)/test_content_rating_controller
	$(BUILD_DIR)/test_content_rating_controller

test-person:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_person.cpp \
		-o $(BUILD_DIR)/test_person
	$(BUILD_DIR)/test_person


test-person-query:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_person_query.cpp \
		-o $(BUILD_DIR)/test_person_query
	$(BUILD_DIR)/test_person_query




test-person-search-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryMatcher.cpp \
		core/vdr/src/PersonSearchService.cpp \
		core/vdr/tests/test_person_search_service.cpp \
		-o $(BUILD_DIR)/test_person_search_service
	$(BUILD_DIR)/test_person_search_service

test-person-query-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryResultJsonSerializer.cpp \
		core/vdr/tests/test_person_query_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_person_query_result_json_serializer
	$(BUILD_DIR)/test_person_query_result_json_serializer

test-person-query-matcher:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryMatcher.cpp \
		core/vdr/tests/test_person_query_matcher.cpp \
		-o $(BUILD_DIR)/test_person_query_matcher
	$(BUILD_DIR)/test_person_query_matcher

test-person-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_person_resolver.cpp \
		-o $(BUILD_DIR)/test_person_resolver
	$(BUILD_DIR)/test_person_resolver

test-person-resolution-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonResolutionJsonSerializer.cpp \
		core/vdr/tests/test_person_resolution_json_serializer.cpp \
		-o $(BUILD_DIR)/test_person_resolution_json_serializer
	$(BUILD_DIR)/test_person_resolution_json_serializer

test-person-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryMatcher.cpp \
		core/vdr/src/PersonQueryResultJsonSerializer.cpp \
		core/vdr/src/PersonResolutionJsonSerializer.cpp \
		core/vdr/src/PersonSearchService.cpp \
		api/rest/src/PersonController.cpp \
		api/rest/tests/test_person_controller.cpp \
		-o $(BUILD_DIR)/test_person_controller
	$(BUILD_DIR)/test_person_controller


test-person-query-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryMatcher.cpp \
		core/vdr/src/PersonQueryResultJsonSerializer.cpp \
		core/vdr/src/PersonResolutionJsonSerializer.cpp \
		core/vdr/src/PersonSearchService.cpp \
		api/rest/src/PersonController.cpp \
		api/rest/tests/test_person_query_controller.cpp \
		-o $(BUILD_DIR)/test_person_query_controller
	$(BUILD_DIR)/test_person_query_controller

test-recording-person-search-result:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_recording_person_search_result.cpp \
		-o $(BUILD_DIR)/test_recording_person_search_result
	$(BUILD_DIR)/test_recording_person_search_result

test-recording-person-search-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryMatcher.cpp \
		core/vdr/src/RecordingPersonSearchService.cpp \
		core/vdr/tests/test_recording_person_search_service.cpp \
		-o $(BUILD_DIR)/test_recording_person_search_service
	$(BUILD_DIR)/test_recording_person_search_service

test-recording-person-search-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RecordingPersonSearchResultJsonSerializer.cpp \
		core/vdr/tests/test_recording_person_search_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_recording_person_search_result_json_serializer
	$(BUILD_DIR)/test_recording_person_search_result_json_serializer

test-recording-person-search-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryMatcher.cpp \
		core/vdr/src/RecordingPersonSearchService.cpp \
		core/vdr/src/RecordingPersonSearchResultJsonSerializer.cpp \
		api/rest/src/RecordingPersonSearchController.cpp \
		api/rest/tests/test_recording_person_search_controller.cpp \
		-o $(BUILD_DIR)/test_recording_person_search_controller
	$(BUILD_DIR)/test_recording_person_search_controller

test-epg-search-request:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_epg_search_request.cpp \
		-o $(BUILD_DIR)/test_epg_search_request
	$(BUILD_DIR)/test_epg_search_request

test-epg-search-result:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_epg_search_result.cpp \
		-o $(BUILD_DIR)/test_epg_search_result
	$(BUILD_DIR)/test_epg_search_result

