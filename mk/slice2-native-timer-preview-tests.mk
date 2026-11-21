# Conservative, non-mutating, source-only native candidate preview kernel.
# A green result DOES NOT activate a public CREATE or attest a live VDR.
.PHONY: test-slice2-native-timer-preview-kernel

test-slice2-native-timer-preview-kernel:
	$(BUILD_CXX) $(CXXFLAGS) -Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_native_timer_preview_core.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_preview_core.cpp \
		-o $(BUILD_DIR)/test_suitebridge_native_timer_preview_core
	$(BUILD_DIR)/test_suitebridge_native_timer_preview_core

test-fast: test-slice2-native-timer-preview-kernel

.PHONY: test-slice2-native-timer-preview-source-architecture

test-slice2-native-timer-preview-source-architecture:
	python3 tools/check_slice2_native_timer_preview_safety.py

test-architecture: test-slice2-native-timer-preview-source-architecture
test-fast: test-slice2-native-timer-preview-source-architecture

.PHONY: test-slice2-native-timer-preview-service

test-slice2-native-timer-preview-service:
	$(BUILD_CXX) $(CXXFLAGS) -Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_native_timer_preview_core.cpp \
		vdr-plugin-suite-bridge/suitebridge_native_timer_preview_service.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_preview_service.cpp \
		-o $(BUILD_DIR)/test_suitebridge_native_timer_preview_service
	$(BUILD_DIR)/test_suitebridge_native_timer_preview_service

test-fast: test-slice2-native-timer-preview-service
