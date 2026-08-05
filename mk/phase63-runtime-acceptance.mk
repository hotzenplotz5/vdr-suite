.PHONY: test-phase63-runtime-acceptance-harness phase63-backend-agent-runtime-acceptance

PHASE63_ACCEPTANCE_RUNNER := tools/phase63-runtime-acceptance/backend-agent-foundation.sh

PHASE63_EXPECTED_BRANCH ?=
PHASE63_EXPECTED_HEAD ?=
PHASE63_CONTROL_PLANE_URL ?=
PHASE63_CA_CERTIFICATE_PATH ?=
PHASE63_EVIDENCE_DIR ?=
PHASE63_BACKEND_ID ?= default
PHASE63_DATABASE ?= /var/lib/vdr-suite/vdr-suite.db
PHASE63_VDR_VIDEO_DIR ?= /srv/vdr/video.00

test-phase63-runtime-acceptance-harness:
	bash -n "$(PHASE63_ACCEPTANCE_RUNNER)"
	python3 tools/check_phase63_runtime_acceptance.py

phase63-backend-agent-runtime-acceptance: test-phase63-runtime-acceptance-harness
	@test -n "$(PHASE63_EXPECTED_BRANCH)" || { echo "PHASE63_EXPECTED_BRANCH is required"; exit 2; }
	@test -n "$(PHASE63_EXPECTED_HEAD)" || { echo "PHASE63_EXPECTED_HEAD is required"; exit 2; }
	@test -n "$(PHASE63_CONTROL_PLANE_URL)" || { echo "PHASE63_CONTROL_PLANE_URL is required"; exit 2; }
	@test -n "$(PHASE63_EVIDENCE_DIR)" || { echo "PHASE63_EVIDENCE_DIR is required"; exit 2; }
	PHASE63_EXPECTED_BRANCH="$(PHASE63_EXPECTED_BRANCH)" \
	PHASE63_EXPECTED_HEAD="$(PHASE63_EXPECTED_HEAD)" \
	PHASE63_CONTROL_PLANE_URL="$(PHASE63_CONTROL_PLANE_URL)" \
	PHASE63_CA_CERTIFICATE_PATH="$(PHASE63_CA_CERTIFICATE_PATH)" \
	PHASE63_EVIDENCE_DIR="$(PHASE63_EVIDENCE_DIR)" \
	PHASE63_BACKEND_ID="$(PHASE63_BACKEND_ID)" \
	PHASE63_DATABASE="$(PHASE63_DATABASE)" \
	PHASE63_VDR_VIDEO_DIR="$(PHASE63_VDR_VIDEO_DIR)" \
	"$(PHASE63_ACCEPTANCE_RUNNER)"
