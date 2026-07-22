VDR_RECORDING_NATIVE_IDENTITY_SRC := \
	core/vdr/src/VdrRecordingNativeIdentity.cpp

VDR_RECORDING_NATIVE_METADATA_SRC := \
	$(VDR_RECORDING_NATIVE_IDENTITY_SRC) \
	core/vdr/src/SuiteBridgeRecordingMetadataResolver.cpp \
	core/vdr/src/VdrRecordingNativeMetadataRepositoryInternal.cpp \
	core/vdr/src/VdrRecordingNativeMetadataRepositorySchema.cpp \
	core/vdr/src/VdrRecordingNativeMetadataRepositoryStorage.cpp \
	core/vdr/src/VdrRecordingNativeMetadataRepositoryRead.cpp \
	core/vdr/src/VdrRecordingNativeMetadataRepositorySearch.cpp \
	core/vdr/src/VdrRecordingNativeMetadataRepositoryMaintenance.cpp \
	core/vdr/src/VdrRecordingNativeMetadataEnrichmentService.cpp \
	core/vdr/src/VdrRecordingNativePersonSearchService.cpp

DAEMON_SRC += $(VDR_RECORDING_NATIVE_METADATA_SRC)

.PHONY: test-vdr-recording-native-identity test-vdr-recording-metadata-type-coexistence test-suite-bridge-svdrp-recording-metadata-transport test-suite-bridge-recording-metadata-resolver test-vdr-recording-native-metadata-repository test-vdr-recording-native-metadata-enrichment-service test-vdr-recording-native-person-search-service test-recording-person-persistent-search-controller check-vdr-recording-native-metadata-runtime-wiring test-recording-native-metadata-contracts

test-vdr-recording-native-identity:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		core/vdr/tests/test_vdr_recording_native_identity.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_native_identity
	$(BUILD_DIR)/test_vdr_recording_native_identity

test-vdr-recording-metadata-type-coexistence:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/vdr/include \
		core/vdr/tests/test_vdr_recording_metadata_type_coexistence.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_metadata_type_coexistence
	$(BUILD_DIR)/test_vdr_recording_metadata_type_coexistence

test-suite-bridge-svdrp-recording-metadata-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Icore/agent/include \
		-Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_recording_metadata_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_recording_metadata_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_recording_metadata_transport

test-suite-bridge-recording-metadata-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		core/vdr/src/SuiteBridgeRecordingMetadataResolver.cpp \
		core/vdr/tests/test_suite_bridge_recording_metadata_resolver.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_recording_metadata_resolver
	$(BUILD_DIR)/test_suite_bridge_recording_metadata_resolver

test-vdr-recording-native-metadata-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryInternal.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositorySchema.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryStorage.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryRead.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositorySearch.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryMaintenance.cpp \
		core/vdr/tests/test_vdr_recording_native_metadata_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_native_metadata_repository
	$(BUILD_DIR)/test_vdr_recording_native_metadata_repository

test-vdr-recording-native-metadata-enrichment-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryInternal.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositorySchema.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryStorage.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryRead.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositorySearch.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryMaintenance.cpp \
		core/vdr/src/VdrRecordingNativeMetadataEnrichmentService.cpp \
		core/vdr/tests/test_vdr_recording_native_metadata_enrichment_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_native_metadata_enrichment_service
	$(BUILD_DIR)/test_vdr_recording_native_metadata_enrichment_service

test-vdr-recording-native-person-search-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryInternal.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositorySchema.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryStorage.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryRead.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositorySearch.cpp \
		core/vdr/src/VdrRecordingNativeMetadataRepositoryMaintenance.cpp \
		core/vdr/src/VdrRecordingNativePersonSearchService.cpp \
		core/vdr/tests/test_vdr_recording_native_person_search_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_native_person_search_service
	$(BUILD_DIR)/test_vdr_recording_native_person_search_service

test-recording-person-persistent-search-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/PersonQueryMatcher.cpp \
		core/vdr/src/RecordingPersonSearchService.cpp \
		core/vdr/src/VdrRecordingArtworkIdentity.cpp \
		core/vdr/src/RecordingPersonSearchResultJsonSerializer.cpp \
		api/rest/src/RecordingPersonSearchController.cpp \
		api/rest/tests/test_recording_person_persistent_search_controller.cpp \
		-o $(BUILD_DIR)/test_recording_person_persistent_search_controller
	$(BUILD_DIR)/test_recording_person_persistent_search_controller

check-vdr-recording-native-metadata-runtime-wiring:
	python3 tools/check_recording_native_metadata_runtime_wiring.py

test-recording-native-metadata-contracts: \
	test-vdr-recording-native-identity \
	test-vdr-recording-metadata-type-coexistence \
	test-suite-bridge-svdrp-recording-metadata-transport \
	test-suite-bridge-recording-metadata-resolver \
	test-vdr-recording-native-metadata-repository \
	test-vdr-recording-native-metadata-enrichment-service \
	test-vdr-recording-native-person-search-service \
	test-recording-person-persistent-search-controller \
	check-vdr-recording-native-metadata-runtime-wiring
