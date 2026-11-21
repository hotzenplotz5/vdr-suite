.PHONY: test-phase64-native-timer-modify-delivery test-phase64-native-timer-modify-delivery-architecture

test-phase64-native-timer-modify-delivery:
	python3 tools/check_phase64_native_timer_modify_delivery.py

test-phase64-native-timer-modify-delivery-architecture: test-phase64-native-timer-modify-delivery

test-fast: test-phase64-native-timer-modify-delivery
test-architecture: test-phase64-native-timer-modify-delivery-architecture
