.PHONY: check-addon-contract check-addon-deb check-addon-registry stage-addon build-addon-deb

# Optional add-ons are staged individually, never in base make install.
check-addon-contract:
	python3 tools/addons/addon_contract.py check
	python3 -m unittest discover -s tools/addons -p 'test_addon_contract.py'

check-addon-deb:
	python3 -m unittest discover -s tools/addons -p 'test_addon_deb.py'

# Pure metadata inventory with no installation, activation or daemon interaction.
check-addon-registry:
	python3 -m unittest discover -s tools/addons -p 'test_installed_registry.py'
	python3 -m unittest discover -s tools/addons -p 'test_dpkg_evidence.py'
	python3 -m unittest discover -s tools/addons -p 'test_manual_handler_evidence.py'
	python3 -m unittest discover -s tools/addons -p 'test_admin_approval_evidence.py'

stage-addon:
	python3 tools/addons/addon_contract.py stage --module "$(MODULE)" --destdir "$(DESTDIR)" --prefix "$(PREFIX)"

# Build a standalone, explicitly nonfunctional Debian metadata scaffold package.
# No install, dependency changes or daemon access; caller owns output directory.
build-addon-deb:
	python3 tools/addons/build_deb.py --module "$(MODULE)" --output-dir "$(OUTPUT_DIR)" --maintainer "$(ADDON_MAINTAINER)"

# Reuse existing documentation and packaging regression CI entrypoints.
test-docs: check-addon-contract check-addon-registry
test-install-staging: check-addon-contract check-addon-deb check-addon-registry

# Standalone C++ source implementation, deliberately not part of default install
# or metadata-only add-on package until backend security/activation is complete.
.PHONY: addon-media-import test-addon-media-import
addon-media-import:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I addons/media-tools/include \
		addons/media-tools/src/MediaImport.cpp addons/media-tools/src/main.cpp \
		-o "$(BUILD_DIR)/addons/vdr-suite-media-import"

test-addon-media-import:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I addons/media-tools/include \
		addons/media-tools/src/MediaImport.cpp addons/media-tools/tests/test_media_import.cpp \
		-o "$(BUILD_DIR)/addons/test_media_import"
	"$(BUILD_DIR)/addons/test_media_import"

# Isolated real-media smoke: FFmpeg/FFprobe are real; VDR index is a
# synthetic fixture. No live VDR files or installed service are accessed.
.PHONY: test-addon-media-import-ffmpeg
test-addon-media-import-ffmpeg:
	test -x /usr/bin/ffmpeg
	test -x /usr/bin/ffprobe
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I addons/media-tools/include \
		addons/media-tools/src/MediaImport.cpp \
		addons/media-tools/src/MediaCheck.cpp \
		addons/media-tools/src/MediaShrink.cpp \
		addons/media-tools/tests/test_media_import_ffmpeg.cpp \
		-o "$(BUILD_DIR)/addons/test_media_import_ffmpeg"
	"$(BUILD_DIR)/addons/test_media_import_ffmpeg"

# Production source for the non-destructive HEVC review workflow.
# Local admin invocation is explicit; no Agent/API shrink mutation is enabled.
.PHONY: addon-media-shrink test-addon-media-shrink
addon-media-shrink:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I addons/media-tools/include \
		addons/media-tools/src/MediaImport.cpp \
		addons/media-tools/src/MediaShrink.cpp \
		addons/media-tools/src/media_shrink_main.cpp \
		-o "$(BUILD_DIR)/addons/vdr-suite-media-shrink"

test-addon-media-shrink:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I addons/media-tools/include \
		addons/media-tools/src/MediaImport.cpp \
		addons/media-tools/src/MediaShrink.cpp \
		addons/media-tools/tests/test_media_shrink.cpp \
		-o "$(BUILD_DIR)/addons/test_media_shrink"
	"$(BUILD_DIR)/addons/test_media_shrink"

# Protected Agent-local opaque review binding and durable one-shot publish
# start fence. Synthetic directories only; never VDR production data.
.PHONY: test-addon-media-shrink-local-review
test-addon-media-shrink-local-review:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I addons/media-tools/include \
		addons/media-tools/src/LocalMediaShrinkReviewStore.cpp \
		addons/media-tools/tests/test_local_media_shrink_review_store.cpp \
		-o "$(BUILD_DIR)/addons/test_local_media_shrink_review_store"
	"$(BUILD_DIR)/addons/test_local_media_shrink_review_store"

test-ci-fast: test-addon-media-shrink-local-review

.PHONY: test-addon-media-shrink-plan-api
test-addon-media-shrink-plan-api:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I addons/media-tools/include -I api/rest/include \
		-I core/daemon/include -I core/addons/include \
		-I core/sqlite/include -I core/operations/include \
		core/sqlite/src/Database.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/addons/src/MediaShrinkReviewRepository.cpp \
		addons/media-tools/src/MediaImport.cpp \
		addons/media-tools/src/MediaShrink.cpp \
		addons/media-tools/tests/test_media_shrink_plan_api.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_media_shrink_plan_api"
	"$(BUILD_DIR)/addons/test_media_shrink_plan_api"

test-ci-fast: test-addon-media-shrink-plan-api

test-ci-fast: test-addon-media-shrink

# Staged HEVC review identity is bound to canonical ADR-0042 Operations.
# This never dispatches a worker or touches installed VDR recordings.
.PHONY: test-addon-media-shrink-core-review
test-addon-media-shrink-core-review:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/sqlite/include -I core/operations/include \
		-I core/addons/include \
		core/sqlite/src/Database.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/addons/src/MediaShrinkReviewRepository.cpp \
		core/addons/tests/test_media_shrink_review_repository.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_media_shrink_review_repository"
	"$(BUILD_DIR)/addons/test_media_shrink_review_repository"

test-ci-fast: test-addon-media-shrink-core-review

# Reserve immutable HEVC stage/publish Operations using the canonical
# Core repository. Disposable in-memory SQLite; no recording files touched.
.PHONY: test-addon-media-shrink-core-admission
test-addon-media-shrink-core-admission:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I addons/media-tools/include -I core/agent/include \
		-I core/sqlite/include -I core/operations/include \
		-I core/addons/include \
		core/sqlite/src/Database.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationIdentity.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/addons/src/MediaShrinkReviewRepository.cpp \
		core/addons/src/MediaShrinkOperationAdmission.cpp \
		core/addons/tests/test_media_shrink_operation_admission.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_media_shrink_operation_admission"
	"$(BUILD_DIR)/addons/test_media_shrink_operation_admission"

test-ci-fast: test-addon-media-shrink-core-admission


# Validate the structural HEVC command boundary before any Agent runtime is
# allowed to consider this command type. No filesystem or command execution.
.PHONY: test-addon-media-shrink-command-fence
test-addon-media-shrink-command-fence:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I addons/media-tools/include -I core/addons/include \
		-I core/agent/include -I core/operations/include \
		core/addons/tests/test_media_shrink_command_fence.cpp \
		-o "$(BUILD_DIR)/addons/test_media_shrink_command_fence"
	"$(BUILD_DIR)/addons/test_media_shrink_command_fence"

test-ci-fast: test-addon-media-shrink-command-fence

# M2: read-only TS and FFmpeg engine, exercised via standalone CLI and
# Suite API v1 integration on disposable fixtures; never touch real recordings.
.PHONY: addon-media-check test-addon-media-check
addon-media-check:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread -I addons/media-tools/include \
		addons/media-tools/src/MediaCheck.cpp \
		addons/media-tools/src/MediaCheckFfmpeg.cpp \
		addons/media-tools/src/media_check_main.cpp \
		-o "$(BUILD_DIR)/addons/vdr-suite-media-check"

test-addon-media-check:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread -I addons/media-tools/include \
		addons/media-tools/src/MediaCheck.cpp \
		addons/media-tools/src/MediaCheckFfmpeg.cpp \
		addons/media-tools/tests/test_media_check.cpp \
		-o "$(BUILD_DIR)/addons/test_media_check"
	"$(BUILD_DIR)/addons/test_media_check"

# Actual opaque Public API request -> local backend worker -> TS report.
.PHONY: test-addon-media-check-api
test-addon-media-check-api:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I addons/media-tools/include -I api/rest/include \
		-I core/daemon/include -I core/sqlite/include \
		core/sqlite/src/Database.cpp \
		addons/media-tools/src/MediaCheck.cpp \
		addons/media-tools/src/MediaCheckFfmpeg.cpp \
		addons/media-tools/tests/test_media_check_api.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_media_check_api"
	"$(BUILD_DIR)/addons/test_media_check_api"

test-ci-fast: test-addon-media-check test-addon-media-check-api

# This package contains the C++ prototype under private libexec. It advertises
# zero Suite capabilities and is neither installed nor started by this target.
.PHONY: build-media-tools-cpp-deb test-addon-media-tools-package
build-media-tools-cpp-deb: addon-media-import addon-media-shrink addon-media-check
	python3 tools/addons/build_media_tools_cpp_deb.py \
		--binary "$(BUILD_DIR)/addons/vdr-suite-media-import" \
		--shrink-binary "$(BUILD_DIR)/addons/vdr-suite-media-shrink" \
		--check-binary "$(BUILD_DIR)/addons/vdr-suite-media-check" \
		--output-dir "$(OUTPUT_DIR)" --maintainer "$(ADDON_MAINTAINER)"

test-addon-media-tools-package:
	python3 -m unittest discover -s tools/addons -p 'test_media_tools_cpp_deb.py'

test-ci-fast: test-addon-media-import
test-install-staging: test-addon-media-tools-package

# Core-owned, standalone access-decision library. No daemon/HTTP wiring.
.PHONY: test-addon-access-policy
test-addon-access-policy:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I core/addons/include -I core/security/include -I core/vdr/include \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/tests/test_addon_access_policy.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_access_policy"
	"$(BUILD_DIR)/addons/test_addon_access_policy"

test-ci-fast: test-addon-access-policy

# Durable *desired* state only: SQLite CAS, no live daemon, routes or jobs.
.PHONY: test-addon-activation-intent
test-addon-activation-intent:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/addons/include -I core/security/include \
		-I core/vdr/include -I core/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/src/AddonActivationIntentRepository.cpp \
		core/addons/tests/test_addon_activation_intent_repository.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_addon_activation_intent"
	"$(BUILD_DIR)/addons/test_addon_activation_intent"

test-ci-fast: test-addon-activation-intent

# Dry-run C++ bridge between persisted intent and existing actor/backend gate.
# No runtime worker, provider discovery, API route or package installation.
.PHONY: test-addon-admission-preview
test-addon-admission-preview:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/addons/include -I core/security/include \
		-I core/vdr/include -I core/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/src/AddonActivationIntentRepository.cpp \
		core/addons/src/AddonAdmissionReadService.cpp \
		core/addons/tests/test_addon_admission_read_service.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_addon_admission_preview"
	"$(BUILD_DIR)/addons/test_addon_admission_preview"

test-ci-fast: test-addon-admission-preview

# Immutable reviewed handler identity; no runtime integration/dispatch.
.PHONY: test-addon-handler-registry
test-addon-handler-registry:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/tests/test_addon_handler_registry.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_handler_registry"
	"$(BUILD_DIR)/addons/test_addon_handler_registry"

test-ci-fast: test-addon-handler-registry

# Non-executable common source/deb provider admission contract.
.PHONY: test-addon-provider-admission
test-addon-provider-admission:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/tests/test_addon_provider_admission.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_provider_admission"
	"$(BUILD_DIR)/addons/test_addon_provider_admission"

test-ci-fast: test-addon-provider-admission

# Non-authorizing provider snapshot: pinned binary/version + backend generation.
.PHONY: test-addon-provider-binding
test-addon-provider-binding:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/tests/test_addon_provider_binding.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_provider_binding"
	"$(BUILD_DIR)/addons/test_addon_provider_binding"

test-ci-fast: test-addon-provider-binding

# Fail-closed bridge into the established Core RuntimeEvidence contract.
.PHONY: test-addon-provider-read-service
test-addon-provider-read-service:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I core/addons/include -I core/security/include -I core/vdr/include \
		core/addons/tests/test_addon_provider_read_service.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_provider_read_service"
	"$(BUILD_DIR)/addons/test_addon_provider_read_service"

test-ci-fast: test-addon-provider-read-service

# Independent approval matching; no runtime authority or execution.
.PHONY: test-addon-provider-approval
test-addon-provider-approval:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/tests/test_addon_provider_approval.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_provider_approval"
	"$(BUILD_DIR)/addons/test_addon_provider_approval"

test-ci-fast: test-addon-provider-approval

# Internal admin coordinator: same-transaction Suite accountability + intent CAS.
.PHONY: test-addon-administration-service
test-addon-administration-service:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/addons/include -I core/security/include \
		-I core/vdr/include -I core/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/security/src/AccountabilityEventRepository.cpp \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/src/AddonActivationIntentRepository.cpp \
		core/addons/src/AddonAdministrationService.cpp \
		core/addons/tests/test_addon_administration_service.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_addon_administration_service"
	"$(BUILD_DIR)/addons/test_addon_administration_service"

test-ci-fast: test-addon-administration-service

# Read-only Media Tools maintenance diagnostic using the real durable
# activation and MutationOperation repositories. No package action or lock.
.PHONY: test-addon-maintenance-read
test-addon-maintenance-read:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/addons/include -I core/security/include \
		-I core/vdr/include -I core/sqlite/include \
		-I core/operations/include \
		core/sqlite/src/Database.cpp \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/src/AddonActivationIntentRepository.cpp \
		core/addons/src/AddonMediaToolsMaintenanceReadService.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/addons/tests/test_addon_media_tools_maintenance_read_service.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_addon_maintenance_read"
	"$(BUILD_DIR)/addons/test_addon_maintenance_read"

test-ci-fast: test-addon-maintenance-read

# Core-owned global Media Import reservation fence. The same SQLite write
# transaction owns the freeze and the creation of every media.import operation.
# No host package maintenance, worker execution or live activation.
.PHONY: test-addon-maintenance-freeze
test-addon-maintenance-freeze:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/addons/include -I addons/media-tools/include \
		-I core/security/include \
		-I core/vdr/include -I core/sqlite/include \
		-I core/operations/include \
		core/sqlite/src/Database.cpp \
		core/security/src/AccountabilityEventRepository.cpp \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/src/AddonMediaImportMaintenanceFence.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/addons/tests/test_addon_media_import_maintenance_fence.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_addon_maintenance_freeze"
	"$(BUILD_DIR)/addons/test_addon_maintenance_freeze"

test-ci-fast: test-addon-maintenance-freeze

# Global cross-backend source-only maintenance DIAGNOSTIC. Synthetic Agent
# command rows, real Core SQLite owners, root-only isolated lock fixture.
# Never grants a package install, updates live state, or opens an API route.
.PHONY: build-addon-global-maintenance-inspection-test
build-addon-global-maintenance-inspection-test:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/addons/include -I addons/media-tools/include \
		-I core/security/include -I core/vdr/include \
		-I core/sqlite/include -I core/operations/include \
		core/sqlite/src/Database.cpp \
		core/security/src/AccountabilityEventRepository.cpp \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/src/AddonActivationIntentRepository.cpp \
		core/addons/src/AddonMediaImportMaintenanceFence.cpp \
		core/addons/src/AddonMediaToolsMaintenanceInspection.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/addons/tests/test_addon_media_tools_maintenance_inspection.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_addon_global_maintenance_inspection"

# Admin-only desired-state inventory. Effective/active capabilities remain empty.
.PHONY: test-addon-activation-catalog
test-addon-activation-catalog:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -pthread \
		-I core/addons/include -I core/security/include \
		-I core/vdr/include -I core/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/addons/src/AddonAccessPolicy.cpp \
		core/addons/src/AddonActivationIntentRepository.cpp \
		core/addons/src/AddonActivationCatalogReadService.cpp \
		core/addons/tests/test_addon_activation_catalog_read_service.cpp \
		-lsqlite3 -o "$(BUILD_DIR)/addons/test_addon_activation_catalog"
	"$(BUILD_DIR)/addons/test_addon_activation_catalog"

test-ci-fast: test-addon-activation-catalog

# Read-only binding to existing operation/Agent identities; never dispatches.
.PHONY: test-addon-agent-dispatch-gate
test-addon-agent-dispatch-gate:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I core/addons/include -I core/agent/include -I core/operations/include \
		core/addons/tests/test_addon_agent_dispatch_gate.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_agent_dispatch_gate"
	"$(BUILD_DIR)/addons/test_addon_agent_dispatch_gate"

test-ci-fast: test-addon-agent-dispatch-gate

# Build only: privileged fixture executes ONLY in disposable CI under /root.
.PHONY: build-addon-pinned-handler-test
build-addon-pinned-handler-test:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/src/AddonPinnedHandler.cpp \
		core/addons/tests/test_addon_pinned_handler.cpp -lcrypto \
		-o "$(BUILD_DIR)/addons/test_addon_pinned_handler"

# Build only. The real root-owned fixture is run in disposable CI.
.PHONY: build-addon-root-approval-test
build-addon-root-approval-test:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/src/AddonRootApproval.cpp \
		core/addons/src/AddonPinnedHandler.cpp \
		core/addons/src/AddonInstallSnapshot.cpp \
		core/addons/tests/test_addon_root_approval.cpp -lcrypto \
		-o "$(BUILD_DIR)/addons/test_addon_root_approval"

# Build only; drops privileges in a root-owned disposable CI fixture.
.PHONY: build-addon-nonroot-snapshot-test
build-addon-nonroot-snapshot-test:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/src/AddonRootApproval.cpp \
		core/addons/src/AddonPinnedHandler.cpp \
		core/addons/src/AddonInstallSnapshot.cpp \
		core/addons/tests/test_addon_nonroot_snapshot.cpp -lcrypto \
		-o "$(BUILD_DIR)/addons/test_addon_nonroot_snapshot"

# Reuse the actual canonical Agent poll implementation, not a mock registry.
.PHONY: test-addon-agent-advertisement
test-addon-agent-advertisement:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_JSON_SRC) \
		$(AGENT_COMMAND_CLIENT_SRC) \
		core/agent/tests/test_backend_agent_command_client.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_agent_advertisement"
	"$(BUILD_DIR)/addons/test_addon_agent_advertisement"

# Read-only permission descriptor catalog for settings, no grant mutation.
.PHONY: test-addon-permission-catalog
test-addon-permission-catalog:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I core/addons/include \
		core/addons/tests/test_addon_permission_catalog.cpp \
		-o "$(BUILD_DIR)/addons/test_addon_permission_catalog"
	"$(BUILD_DIR)/addons/test_addon_permission_catalog"

test-ci-fast: test-addon-permission-catalog

# Inert add-on-owned request envelope; no Core execution or package install.
.PHONY: test-media-tools-agent-request
test-media-tools-agent-request:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I addons/media-tools/include \
		addons/media-tools/tests/test_media_tools_agent_request.cpp \
		-o "$(BUILD_DIR)/addons/test_media_tools_agent_request"
	"$(BUILD_DIR)/addons/test_media_tools_agent_request"

test-ci-fast: test-media-tools-agent-request

# Media Tools consumes canonical Suite operation and Agent contracts; no job creation.
.PHONY: test-media-tools-canonical-agent-bridge
test-media-tools-canonical-agent-bridge:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I addons/media-tools/include -I core/agent/include \
		-I core/operations/include \
		addons/media-tools/tests/test_media_tools_canonical_agent_bridge.cpp \
		-o "$(BUILD_DIR)/addons/test_media_tools_canonical_agent_bridge"
	"$(BUILD_DIR)/addons/test_media_tools_canonical_agent_bridge"

test-ci-fast: test-media-tools-canonical-agent-bridge


# A6 integration: genuine durable Agent polling/receipts/recovery, isolated
# injected executor only. No production activation or recording mutation.
.PHONY: test-media-tools-agent-dispatch
test-media-tools-agent-dispatch:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) $(CXXFLAGS) -Werror \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_JSON_SRC) \
		$(AGENT_COMMAND_CLIENT_SRC) \
		addons/media-tools/tests/test_media_tools_agent_dispatch.cpp \
		-lcrypto -o "$(BUILD_DIR)/addons/test_media_tools_agent_dispatch"
	"$(BUILD_DIR)/addons/test_media_tools_agent_dispatch"

test-ci-fast: test-media-tools-agent-dispatch

# Build-only: root-owned mock installation and /bin/true executable in CI.
.PHONY: build-media-tools-pinned-agent-executor-test
build-media-tools-pinned-agent-executor-test:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		addons/media-tools/tests/fake_pinned_media_import_receipt.cpp \
		-o "$(BUILD_DIR)/addons/fake_pinned_media_import_receipt"
	$(CXX) $(CXXFLAGS) -Werror -Icore/addons/include \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		core/addons/src/AddonRootApproval.cpp \
		core/addons/src/AddonPinnedHandler.cpp \
		core/addons/src/AddonInstallSnapshot.cpp \
		addons/media-tools/src/PinnedMediaImportAgentExecutor.cpp \
		addons/media-tools/tests/test_pinned_media_import_agent_executor.cpp \
		-lcrypto -o "$(BUILD_DIR)/addons/test_pinned_media_import_agent_executor"

# A6.1: root-owned backend-local opaque source map. Only CI runs this test
# as root in a disposable /run fixture; no productive import is performed.
.PHONY: build-media-tools-local-inventory-test
build-media-tools-local-inventory-test:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) $(CXXFLAGS) -Werror -Icore/addons/include \
		addons/media-tools/src/MediaImport.cpp \
		addons/media-tools/src/LocalMediaImportInventory.cpp \
		addons/media-tools/tests/test_local_media_import_inventory.cpp \
		-o "$(BUILD_DIR)/addons/test_local_media_import_inventory"

# A6.2 Core Grant adapter: actual durable receipt and authenticated server recheck.
.PHONY: test-media-tools-effective-authorization
test-media-tools-effective-authorization:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) $(CXXFLAGS) -Werror \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_JSON_SRC) \
		$(AGENT_COMMAND_CLIENT_SRC) \
		core/agent/src/BackendAgentMediaImportAuthorization.cpp \
		core/agent/tests/test_backend_agent_media_import_authorization.cpp \
		-lcrypto -o "$(BUILD_DIR)/addons/test_backend_agent_media_import_authorization"
	"$(BUILD_DIR)/addons/test_backend_agent_media_import_authorization"

# A6.3: the exact opted-in Agent composition, verified in root-only
# disposable CI with a non-recording synthetic ELF handler.
.PHONY: build-media-tools-agent-composition-test
build-media-tools-agent-composition-test:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) $(CXXFLAGS) -Werror \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_JSON_SRC) \
		$(AGENT_COMMAND_CLIENT_SRC) \
		core/addons/src/AddonRootApproval.cpp \
		core/addons/src/AddonPinnedHandler.cpp \
		core/addons/src/AddonInstallSnapshot.cpp \
		addons/media-tools/src/MediaImport.cpp \
		addons/media-tools/src/LocalMediaImportInventory.cpp \
		addons/media-tools/src/PinnedMediaImportAgentExecutor.cpp \
		core/agent/src/BackendAgentMediaImportAuthorization.cpp \
		core/agent/src/BackendAgentMediaImportRuntime.cpp \
		core/agent/tests/test_backend_agent_media_import_runtime.cpp \
		-lcrypto -o "$(BUILD_DIR)/addons/test_backend_agent_media_import_runtime"
