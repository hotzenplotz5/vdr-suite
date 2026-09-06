.PHONY: check-recording-cut-runtime-wiring

check-recording-cut-runtime-wiring:
	python3 tools/check_recording_cut_runtime_wiring.py

# Additive prerequisite only: keep the existing recording-native-editing
# test-fast recipe/graph untouched while making the Slice-3 boundary mandatory.
test-fast: check-recording-cut-runtime-wiring
