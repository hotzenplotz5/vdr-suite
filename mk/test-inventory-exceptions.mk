# Legacy test sources that predate the strict Make inventory audit.
#
# Keeping this list explicit prevents the audit from hiding these files while
# still rejecting every newly orphaned test source. Each entry must either be
# restored to an executable Make target or removed in a dedicated cleanup.
LEGACY_UNWIRED_TEST_SOURCES := \
	core/vdr/tests/test_local_partial_refresh_validation.cpp \
	core/vdr/tests/test_local_restfulapi_integration.cpp \
	core/vdr/tests/test_local_snapshot_runtime_integration.cpp \
	core/vdr/tests/test_real_change_state.cpp \
	core/vdr/tests/test_real_polling_initial_snapshot.cpp \
	core/vdr/tests/test_real_polling_stability.cpp \
	core/vdr/tests/test_real_restfulapi_integration.cpp \
	core/vdr/tests/test_real_snapshot_builder.cpp \
	core/vdr/tests/test_vdr_snapshot_read_service.cpp
