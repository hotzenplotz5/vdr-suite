.PHONY: test-phase68-legacy-osd-domain \
	test-phase68-suitebridge-osd-observation \
	test-phase68-legacy-osd-observation

test-phase68-legacy-osd-domain:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_legacy_osd_domain.cpp \
		-o $(BUILD_DIR)/test_legacy_osd_domain
	$(BUILD_DIR)/test_legacy_osd_domain

test-phase68-suitebridge-osd-observation:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -pedantic \
		-Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_counter_continuity.cpp \
		vdr-plugin-suite-bridge/suitebridge_osd_state.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_osd_state.cpp \
		-o $(BUILD_DIR)/test_suitebridge_osd_state
	$(BUILD_DIR)/test_suitebridge_osd_state

test-phase68-legacy-osd-observation: \
	test-phase68-legacy-osd-domain \
	test-phase68-suitebridge-osd-observation
	python3 tools/check_phase68_legacy_osd_observation.py

# Phase 68.A is a read-only backend/domain slice and belongs to normal fast/VDR CI.
test-ci-fast: test-phase68-legacy-osd-observation
test-vdr: test-phase68-legacy-osd-observation
