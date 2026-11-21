.PHONY: test-phase64-restfulapi-native-timer-inventory-reader-architecture test-phase64-restfulapi-native-timer-inventory-reader

test-phase64-restfulapi-native-timer-inventory-reader-architecture:
	python3 tools/check_phase64_restfulapi_native_timer_inventory_reader.py

test-phase64-restfulapi-native-timer-inventory-reader: test-phase64-restfulapi-native-timer-inventory-reader-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/http/include -Icore/timers/include -Icore/vdr/include \
		core/timers/src/NativeTimerInventoryEvidence.cpp \
		core/vdr/src/RestfulApiNativeTimerInventoryReader.cpp \
		core/vdr/tests/test_restfulapi_native_timer_inventory_reader.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restfulapi_native_timer_inventory_reader
	$(BUILD_DIR)/test_restfulapi_native_timer_inventory_reader

test-fast: test-phase64-restfulapi-native-timer-inventory-reader
test-architecture: test-phase64-restfulapi-native-timer-inventory-reader-architecture
