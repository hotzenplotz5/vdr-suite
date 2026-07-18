AGENT_HANDSHAKE_SRC := \
	core/agent/src/SuiteBridgeHandshake.cpp \
	core/agent/src/SuiteBridgeLocalContractParser.cpp \
	core/agent/src/SuiteBridgeHandshakeService.cpp

AGENT_SVDRP_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpTransport.cpp

AGENT_OBSERVATION_SRC := \
	core/agent/src/SuiteBridgeObservation.cpp \
	core/agent/src/SuiteBridgeObservationService.cpp \
	core/agent/src/SuiteBridgeObservationWorker.cpp

AGENT_SRC := \
	$(AGENT_HANDSHAKE_SRC) \
	$(AGENT_SVDRP_TRANSPORT_SRC) \
	$(AGENT_OBSERVATION_SRC)
