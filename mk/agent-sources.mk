# Suite Bridge handshake, transport, observation and embedded runtime stay Agent-owned.
AGENT_HANDSHAKE_SRC := \
	core/agent/src/SuiteBridgeHandshake.cpp \
	core/agent/src/SuiteBridgeLocalContractParser.cpp \
	core/agent/src/SuiteBridgeHandshakeService.cpp

AGENT_SVDRP_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpEpgTypeSnapshotTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpMetadataTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpRecordingMetadataTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpRecordingMarksTransport.cpp

AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpNativeTimerCreateTransport.cpp

AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpNativeTimerDeleteTransport.cpp

AGENT_NATIVE_TIMER_MODIFY_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpNativeTimerModifyTransport.cpp

AGENT_RECORDING_MARKS_MODIFY_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpRecordingMarksModifyTransport.cpp

AGENT_OBSERVATION_SRC := \
	core/agent/src/SuiteBridgeObservation.cpp \
	core/agent/src/SuiteBridgeObservationService.cpp \
	core/agent/src/SuiteBridgeObservationWorker.cpp

AGENT_EMBEDDED_RUNTIME_SRC := \
	core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp

AGENT_CHANNEL_DOMAIN_SRC := \
	core/agent/src/BackendAgentChannelObservation.cpp

AGENT_CHANNEL_JSON_SRC := \
	core/agent/src/BackendAgentChannelObservationJson.cpp

AGENT_NATIVE_PROBE_SRC := \
	core/agent/src/BackendAgentLocalProvider.cpp \
	core/agent/src/BackendAgentNativeProbe.cpp

AGENT_LIVE_PROVIDER_SRC := \
	core/agent/src/BackendAgentLiveProviderAuthority.cpp \
	core/agent/src/BackendAgentLiveProviderRuntime.cpp

AGENT_COMMAND_DOMAIN_SRC := \
	core/agent/src/BackendAgentCommand.cpp \
	core/agent/src/BackendAgentNativeTimerCreate.cpp \
	core/agent/src/BackendAgentNativeTimerCreatePayload.cpp \
	core/agent/src/BackendAgentNativeTimerModify.cpp \
	core/agent/src/BackendAgentNativeTimerModifyPayload.cpp \
	core/agent/src/BackendAgentRecordingMarksModify.cpp \
	core/agent/src/BackendAgentRecordingMarksModifyPayload.cpp \
	$(AGENT_NATIVE_PROBE_SRC)

AGENT_COMMAND_JSON_SRC := \
	core/agent/src/BackendAgentCommandJson.cpp

AGENT_COMMAND_DELIVERY_SRC := \
	core/agent/src/BackendAgentCommandDelivery.cpp \
	core/agent/src/BackendAgentNativeProbeDelivery.cpp \
	core/agent/src/BackendAgentNativeTimerModifyAssignment.cpp \
	core/agent/src/BackendAgentRecordingMarksModifyAssignment.cpp

AGENT_COMMAND_STATE_SRC := \
	core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp \
	core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp \
	core/agent/src/BackendAgentNativeTimerDelete.cpp \
	core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp \
	core/agent/src/BackendAgentNativeTimerModifyLocalState.cpp \
	core/agent/src/BackendAgentRecordingMarksModifyLocalState.cpp \
	core/agent/src/BackendAgentCommandStateExtension.cpp \
	core/agent/src/BackendAgentCommandStateStore.cpp

AGENT_TIMER_DELETE_EXECUTOR_SRC := \
	core/agent/src/BackendAgentNativeTimerDeleteExecutor.cpp

AGENT_TIMER_MODIFY_EXECUTOR_SRC := \
	core/agent/src/BackendAgentNativeTimerModifyExecutor.cpp

AGENT_RECORDING_MARKS_MODIFY_EXECUTOR_SRC := \
	core/agent/src/BackendAgentRecordingMarksModifyExecutor.cpp

AGENT_NATIVE_PROBE_COMMAND_HANDLER_SRC := \
	core/agent/src/BackendAgentNativeProbeCommandHandler.cpp

AGENT_TIMER_CREATE_EXECUTOR_SRC := \
	core/agent/src/BackendAgentNativeTimerCreateExecutor.cpp

AGENT_NATIVE_TIMER_CREATE_COMMAND_HANDLER_SRC := \
	core/agent/src/BackendAgentNativeTimerCreateCommandHandler.cpp

AGENT_NATIVE_TIMER_DELETE_COMMAND_HANDLER_SRC := \
	core/agent/src/BackendAgentNativeTimerDeleteCommandHandler.cpp

AGENT_NATIVE_TIMER_MODIFY_COMMAND_HANDLER_SRC := \
	core/agent/src/BackendAgentNativeTimerModifyCommandHandler.cpp

AGENT_RECORDING_MARKS_MODIFY_COMMAND_HANDLER_SRC := \
	core/agent/src/BackendAgentRecordingMarksModifyCommandHandler.cpp

AGENT_COMMAND_CLIENT_SRC := \
	$(AGENT_COMMAND_STATE_SRC) \
	$(AGENT_TIMER_CREATE_EXECUTOR_SRC) \
	$(AGENT_TIMER_DELETE_EXECUTOR_SRC) \
	$(AGENT_TIMER_MODIFY_EXECUTOR_SRC) \
	$(AGENT_RECORDING_MARKS_MODIFY_EXECUTOR_SRC) \
	$(AGENT_NATIVE_PROBE_COMMAND_HANDLER_SRC) \
	$(AGENT_NATIVE_TIMER_CREATE_COMMAND_HANDLER_SRC) \
	$(AGENT_NATIVE_TIMER_DELETE_COMMAND_HANDLER_SRC) \
	$(AGENT_NATIVE_TIMER_MODIFY_COMMAND_HANDLER_SRC) \
	$(AGENT_RECORDING_MARKS_MODIFY_COMMAND_HANDLER_SRC) \
	core/agent/src/BackendAgentCommandClient.cpp

AGENT_CONTROL_PLANE_DOMAIN_SRC := \
	core/agent/src/BackendAgentRepository.cpp \
	core/agent/src/BackendAgentLifecycle.cpp \
	$(AGENT_CHANNEL_DOMAIN_SRC) \
	$(AGENT_COMMAND_DOMAIN_SRC) \
	$(AGENT_COMMAND_DELIVERY_SRC) \
	$(AGENT_LIVE_PROVIDER_SRC)

AGENT_CONTROL_PLANE_HTTP_SRC := \
	core/agent/src/BackendAgentHttpServer.cpp \
	$(AGENT_CHANNEL_JSON_SRC) \
	$(AGENT_COMMAND_JSON_SRC)

AGENT_CONTROL_PLANE_SRC := \
	$(AGENT_CONTROL_PLANE_DOMAIN_SRC) \
	$(AGENT_CONTROL_PLANE_HTTP_SRC)

AGENT_CLIENT_SRC := \
	core/agent/src/BackendAgentClient.cpp \
	$(AGENT_CHANNEL_JSON_SRC) \
	$(AGENT_COMMAND_JSON_SRC) \
	$(AGENT_COMMAND_CLIENT_SRC)

AGENT_SRC := \
	$(AGENT_HANDSHAKE_SRC) \
	$(AGENT_SVDRP_TRANSPORT_SRC) \
	$(AGENT_OBSERVATION_SRC) \
	$(AGENT_EMBEDDED_RUNTIME_SRC)

# Standalone agent binaries do not link the daemon-owned recording metadata
# module. They still need the native recording identity implementation because
# SuiteBridgeSvdrpRecordingMetadataTransport and the native marks transport
# validate opaque recording keys before issuing SVDRP requests.
AGENT_SVDRP_TRANSPORT_STANDALONE_SRC = \
	$(AGENT_SVDRP_TRANSPORT_SRC) \
	$(VDR_RECORDING_NATIVE_IDENTITY_SRC)

AGENT_STANDALONE_SRC = \
	$(AGENT_SRC) \
	$(VDR_RECORDING_NATIVE_IDENTITY_SRC)
