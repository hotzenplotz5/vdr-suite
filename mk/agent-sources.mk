# Suite Bridge handshake, transport, observation and embedded runtime stay Agent-owned.
AGENT_HANDSHAKE_SRC := \
	core/agent/src/SuiteBridgeHandshake.cpp \
	core/agent/src/SuiteBridgeLocalContractParser.cpp \
	core/agent/src/SuiteBridgeHandshakeService.cpp

AGENT_SVDRP_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpMetadataTransport.cpp \
	core/agent/src/SuiteBridgeSvdrpRecordingMetadataTransport.cpp

AGENT_OBSERVATION_SRC := \
	core/agent/src/SuiteBridgeObservation.cpp \
	core/agent/src/SuiteBridgeObservationService.cpp \
	core/agent/src/SuiteBridgeObservationWorker.cpp

AGENT_EMBEDDED_RUNTIME_SRC := \
	core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp

AGENT_SRC := \
	$(AGENT_HANDSHAKE_SRC) \
	$(AGENT_SVDRP_TRANSPORT_SRC) \
	$(AGENT_OBSERVATION_SRC) \
	$(AGENT_EMBEDDED_RUNTIME_SRC)
