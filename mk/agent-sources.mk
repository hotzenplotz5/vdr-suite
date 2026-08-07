# Suite Bridge handshake, transport, observation and embedded runtime stay Agent-owned.
AGENT_HANDSHAKE_SRC := \
	core/agent/src/SuiteBridgeHandshake.cpp \
	core/agent/src/SuiteBridgeLocalContractParser.cpp \
	core/agent/src/SuiteBridgeHandshakeService.cpp

AGENT_SVDRP_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpEpgTypeSnapshotTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpMetadataTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpRecordingMetadataTransport.cpp

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

AGENT_COMMAND_DOMAIN_SRC := \
	core/agent/src/BackendAgentCommand.cpp \
	$(AGENT_NATIVE_PROBE_SRC)

AGENT_COMMAND_JSON_SRC := \
	core/agent/src/BackendAgentCommandJson.cpp

AGENT_COMMAND_DELIVERY_SRC := \
	core/agent/src/BackendAgentCommandDelivery.cpp \
	core/agent/src/BackendAgentNativeProbeDelivery.cpp

AGENT_COMMAND_CLIENT_SRC := \
	core/agent/src/BackendAgentCommandClient.cpp

AGENT_CONTROL_PLANE_DOMAIN_SRC := \
	core/agent/src/BackendAgentRepository.cpp \
	core/agent/src/BackendAgentLifecycle.cpp \
	$(AGENT_CHANNEL_DOMAIN_SRC) \
	$(AGENT_COMMAND_DOMAIN_SRC) \
	$(AGENT_COMMAND_DELIVERY_SRC)

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
# SuiteBridgeSvdrpRecordingMetadataTransport validates opaque recording keys.
AGENT_SVDRP_TRANSPORT_STANDALONE_SRC = \
	$(AGENT_SVDRP_TRANSPORT_SRC) \
	$(VDR_RECORDING_NATIVE_IDENTITY_SRC)

AGENT_STANDALONE_SRC = \
	$(AGENT_SRC) \
	$(VDR_RECORDING_NATIVE_IDENTITY_SRC)
