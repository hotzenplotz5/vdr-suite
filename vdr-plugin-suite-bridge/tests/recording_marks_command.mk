# Isolated regression for the production RMARKS handler.
# The fake VDR headers are used only by this test executable. Neither the
# installed plugin nor the running VDR is linked to the fake implementation.
RECORDING_MARKS_COMMAND_TEST := $(TMPDIR)/test_suitebridge_recording_marks_command

.PHONY: test-recording-marks-command

test-recording-marks-command:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -pthread \
		-Itests/fakes -I. \
		suitebridge_recording_identity.cpp \
		suitebridge_recording_marks.cpp \
		suitebridge_recording_marks_contract.cpp \
		suitebridge_recording_marks_command.cpp \
		tests/test_suitebridge_recording_marks_command.cpp \
		-o $(RECORDING_MARKS_COMMAND_TEST)
	$(RECORDING_MARKS_COMMAND_TEST)
	rm -f $(RECORDING_MARKS_COMMAND_TEST)

check: test-recording-marks-command
