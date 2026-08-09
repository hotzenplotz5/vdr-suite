.PHONY: test-phase64-timer-intent-contract-architecture test-phase64-timer-intent-contract

test-phase64-timer-intent-contract-architecture:
	python3 tools/check_phase64_timer_intent_contract.py

test-phase64-timer-intent-contract: test-phase64-timer-intent-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/TimerIntent.cpp \
		core/timers/tests/test_timer_intent.cpp \
		-o $(BUILD_DIR)/test_timer_intent
	$(BUILD_DIR)/test_timer_intent

test-fast: test-phase64-timer-intent-contract
test-architecture: test-phase64-timer-intent-contract-architecture
