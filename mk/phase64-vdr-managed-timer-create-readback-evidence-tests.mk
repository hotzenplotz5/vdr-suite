.PHONY: test-phase64-vdr-managed-timer-create-readback-evidence-architecture test-phase64-vdr-managed-timer-create-readback-evidence

test-phase64-vdr-managed-timer-create-readback-evidence-architecture:
	python3 tools/check_phase64_vdr_managed_timer_create_readback_evidence.py

test-phase64-vdr-managed-timer-create-readback-evidence: test-phase64-vdr-managed-timer-create-readback-evidence-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include -Icore/vdr/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerInventoryEvidence.cpp \
		core/timers/src/NativeTimerCreateReadbackEvidence.cpp \
		core/vdr/src/VdrNativeTimerObservationMapper.cpp \
		core/vdr/src/VdrTimerManagedCorrelation.cpp \
		core/vdr/src/VdrManagedTimerCreateReadbackEvidenceBuilder.cpp \
		core/vdr/tests/test_vdr_managed_timer_create_readback_evidence_builder.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_managed_timer_create_readback_evidence_builder
	$(BUILD_DIR)/test_vdr_managed_timer_create_readback_evidence_builder

test-fast: test-phase64-vdr-managed-timer-create-readback-evidence
test-architecture: test-phase64-vdr-managed-timer-create-readback-evidence-architecture
