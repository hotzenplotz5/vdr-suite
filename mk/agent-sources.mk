AGENT_HANDSHAKE_SRC := \
	core/agent/src/SuiteBridgeHandshake.cpp \
	core/agent/src/SuiteBridgeLocalContractParser.cpp \
	core/agent/src/SuiteBridgeHandshakeService.cpp

AGENT_SVDRP_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpTransport.cpp

AGENT_SRC := \
	$(AGENT_HANDSHAKE_SRC) \
	$(AGENT_SVDRP_TRANSPORT_SRC)
