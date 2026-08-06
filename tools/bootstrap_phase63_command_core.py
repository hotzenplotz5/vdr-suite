#!/usr/bin/env python3
from pathlib import Path

source = Path("core/agent/src/BackendAgentCommandDelivery.cpp")
text = source.read_text()
broken = '''        "CHECK(assignment_state IN ('pending','receipted','resulted','reconciliation')),
        "CHECK(dispatch_state IN ('not_started','starting','accepted_by_executor','effect_reported'))"'''
fixed = '''        "CHECK(assignment_state IN ('pending','receipted','resulted','reconciliation')),"
        "CHECK(dispatch_state IN ('not_started','starting','accepted_by_executor','effect_reported'))"'''
if broken not in text:
    raise SystemExit("expected schema string marker missing")
source.write_text(text.replace(broken, fixed))

sources = Path("mk/agent-sources.mk")
text = sources.read_text()
marker = "AGENT_CHANNEL_JSON_SRC := \\\n\tcore/agent/src/BackendAgentChannelObservationJson.cpp\n"
addition = marker + "\nAGENT_COMMAND_DELIVERY_SRC := \\\n\tcore/agent/src/BackendAgentCommandDelivery.cpp\n"
if marker not in text or "AGENT_COMMAND_DELIVERY_SRC :=" in text:
    raise SystemExit("agent source marker mismatch")
sources.write_text(text.replace(marker, addition))

tests = Path("mk/agent-tests.mk")
text = tests.read_text()
phony = ".PHONY: test-backend-agent-foundation test-backend-agent-client test-backend-agent-enrollment-tool test-backend-agent-admin-tool test-backend-agent-foundation-architecture"
if phony not in text or "test-backend-agent-command-delivery:" in text:
    raise SystemExit("agent test phony marker mismatch")
text = text.replace(phony, phony + " test-backend-agent-command-delivery")
marker = "test-backend-agent-foundation-architecture:\n\tpython3 tools/check_backend_agent_foundation.py\n"
target = marker + """
test-backend-agent-command-delivery:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \\
		$(SQLITE_SRC) \\
		$(AGENT_COMMAND_DELIVERY_SRC) \\
		core/agent/tests/test_backend_agent_command_delivery.cpp \\
		$(LDFLAGS) \\
		-o $(BUILD_DIR)/test_backend_agent_command_delivery
	$(BUILD_DIR)/test_backend_agent_command_delivery
"""
if marker not in text:
    raise SystemExit("agent test target marker mismatch")
text = text.replace(marker, target)
old_fast = "test-fast: test-backend-agent-foundation test-backend-agent-client test-backend-agent-enrollment-tool test-backend-agent-admin-tool"
if old_fast not in text:
    raise SystemExit("test-fast marker mismatch")
text = text.replace(old_fast, old_fast + " test-backend-agent-command-delivery")
tests.write_text(text)
