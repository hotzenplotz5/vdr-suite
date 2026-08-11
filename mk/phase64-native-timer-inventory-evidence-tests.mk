.PHONY: test-phase64-native-timer-inventory-evidence-architecture test-phase64-native-timer-inventory-evidence

test-phase64-native-timer-inventory-evidence-architecture:
	python3 tools/check_phase64_native_timer_inventory_evidence.py

test-phase64-native-timer-inventory-evidence: test-phase64-native-timer-inventory-evidence-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/NativeTimerInventoryEvidence.cpp \
		core/timers/tests/test_native_timer_inventory_evidence.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_inventory_evidence
	$(BUILD_DIR)/test_native_timer_inventory_evidence

test-fast: test-phase64-native-timer-inventory-evidence
test-architecture: test-phase64-native-timer-inventory-evidence-architecture
