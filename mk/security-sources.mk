CXXFLAGS += -Icore/security/include
LDFLAGS += -lcrypt

SECURITY_REPOSITORY_SRC := \
	core/security/src/AccountabilityEventRepository.cpp \
	core/security/src/BrowserSessionCredentialRepository.cpp \
	core/security/src/BrowserSessionRetentionRepository.cpp \
	core/security/src/SecurityPermissionGrantRepository.cpp \
	core/security/src/CredentialVerifierRepository.cpp \
	core/security/src/FirstAdminBootstrapRepository.cpp \
	core/security/src/HumanAccountRepository.cpp \
	core/security/src/HumanAccountAdministrationRepository.cpp \
	core/security/src/SecurityIdentityIssuanceRepository.cpp \
	core/security/src/SecurityIdentityProvisioningRepository.cpp \
	core/security/src/SecurityIdentityRepository.cpp \
	core/security/src/SecurityIdentityRetentionRepository.cpp

SECURITY_SERVICE_SRC := \
	core/security/src/BrowserSessionHttpGate.cpp \
	core/security/src/BrowserSessionIssuanceService.cpp \
	core/security/src/BrowserSessionLifecycleService.cpp \
	core/security/src/BrowserSessionRetentionService.cpp \
	core/security/src/FirstAdminBootstrapIssuanceService.cpp \
	core/security/src/FirstAdminClaimService.cpp \
	core/security/src/HumanAccountAdministrationService.cpp \
	core/security/src/HumanAccountRecoveryService.cpp \
	core/security/src/HumanAccountReadService.cpp

SECURITY_SRC := \
	$(SECURITY_REPOSITORY_SRC) \
	$(SECURITY_SERVICE_SRC)

BROWSER_SESSION_HTTP_SRC := \
	core/http/src/BrowserSessionCsrfRecoveryService.cpp \
	core/http/src/BrowserSessionHttpService.cpp

FIRST_ADMIN_HTTP_SRC := \
	core/http/src/FirstAdminClaimHttpService.cpp

.PHONY: first-admin-bootstrap-issuer human-account-recovery test-security test-security-architecture test-security-legacy-basic-retirement-acceptance test-security-authorization test-security-configuration test-security-accountability-event-repository test-security-identity-repository test-security-permission-grant-repository test-security-first-admin-bootstrap-repository test-security-first-admin-bootstrap-issuance-service test-security-first-admin-claim-service test-security-first-admin-claim-http-service test-security-human-account-read-foundation test-security-human-account-administration test-security-public-account-collection test-security-managed-basic-authenticator test-security-human-password-browser-session test-security-human-account-recovery test-security-browser-session-authenticator test-security-browser-session-issuer-binding test-security-browser-session-issuance-service test-security-browser-session-concurrency-limit test-security-browser-session-idle-expiry test-security-browser-session-retention-cleanup test-security-browser-session-http-service test-security-browser-session-csrf-recovery test-security-browser-session-http-gate test-security-http-gate test-security-recording-marks test-security-series-artwork-route-scope test-security-teletext-read test-security-hbbtv-read test-security-hbbtv-session test-security-osd-session test-security-searchtimer-maintenance test-security-searchtimer-execution test-security-native-fuzzy-refresh test-security-safe-post test-security-manual-recording-metadata

test-security-architecture:
	python3 tools/check_security_identity_architecture.py
	python3 tools/check_p1_identity_authority_audit.py
	python3 tools/check_p2_human_account_read_foundation.py
	python3 tools/check_p2_public_account_collection.py
	python3 tools/check_p2_first_admin_bootstrap_runtime.py
	python3 tools/check_p2_first_admin_bootstrap_issuer.py
	python3 tools/check_p2_first_admin_claim.py
	python3 tools/check_p2_first_admin_browser_claim.py
	python3 tools/check_p2_human_password_browser_session.py
	python3 tools/check_p2_human_account_recovery.py
	python3 tools/check_p2_authentication_default_migration.py
	python3 tools/check_p2_enforced_legacy_basic_retirement.py
	python3 tools/check_p2_legacy_basic_retirement_acceptance.py
	python3 tools/check_mu6_account_lifecycle_foundation.py
	python3 tools/check_mu6b_public_account_item.py
	python3 tools/check_mu6c_public_account_lifecycle_mutation.py
	python3 tools/check_browser_session_issuance_architecture.py
	python3 tools/check_browser_session_issuer_binding.py
	python3 tools/check_browser_session_concurrency_limit.py
	python3 tools/check_browser_session_idle_expiry.py
	python3 tools/check_browser_session_retention_cleanup.py
	python3 tools/check_browser_session_outcome_accountability.py
	python3 tools/check_searchtimer_maintenance_security.py
	python3 tools/check_searchtimer_execution_security.py
	python3 tools/check_native_fuzzy_refresh_security.py
	python3 tools/check_safe_post_security.py


test-security-authorization:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/security/tests/test_authorization_service.cpp \
		-o $(BUILD_DIR)/test_authorization_service
	$(BUILD_DIR)/test_authorization_service


test-security-configuration:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/security/tests/test_security_configuration.cpp \
		-o $(BUILD_DIR)/test_security_configuration
	$(BUILD_DIR)/test_security_configuration


test-security-accountability-event-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_accountability_event_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_accountability_event_repository
	$(BUILD_DIR)/test_accountability_event_repository


test-security-identity-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_security_identity_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_security_identity_repository
	$(BUILD_DIR)/test_security_identity_repository


test-security-permission-grant-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_security_permission_grant_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_security_permission_grant_repository
	$(BUILD_DIR)/test_security_permission_grant_repository


first-admin-bootstrap-issuer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityPermissionGrantRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/FirstAdminBootstrapRepository.cpp \
		core/security/src/FirstAdminBootstrapIssuanceService.cpp \
		apps/tools/first_admin_bootstrap_issue.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-first-admin-bootstrap


human-account-recovery:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/BrowserSessionCredentialRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityIssuanceRepository.cpp \
		core/security/src/HumanAccountRecoveryService.cpp \
		apps/tools/human_account_recover.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-human-account-recover


test-security-first-admin-bootstrap-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityPermissionGrantRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/FirstAdminBootstrapRepository.cpp \
		core/security/tests/test_first_admin_bootstrap_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_first_admin_bootstrap_repository
	$(BUILD_DIR)/test_first_admin_bootstrap_repository


test-security-first-admin-bootstrap-issuance-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityPermissionGrantRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/FirstAdminBootstrapRepository.cpp \
		core/security/src/FirstAdminBootstrapIssuanceService.cpp \
		core/security/tests/test_first_admin_bootstrap_issuance_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_first_admin_bootstrap_issuance_service
	$(BUILD_DIR)/test_first_admin_bootstrap_issuance_service


test-security-first-admin-claim-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/security/src/SecurityPermissionGrantRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/FirstAdminBootstrapRepository.cpp \
		core/security/src/FirstAdminClaimService.cpp \
		core/security/tests/test_first_admin_claim_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_first_admin_claim_service
	$(BUILD_DIR)/test_first_admin_claim_service


test-security-first-admin-claim-http-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/security/src/SecurityPermissionGrantRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/FirstAdminBootstrapRepository.cpp \
		core/security/src/FirstAdminClaimService.cpp \
		$(FIRST_ADMIN_HTTP_SRC) \
		core/http/tests/test_first_admin_claim_http_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_first_admin_claim_http_service
	$(BUILD_DIR)/test_first_admin_claim_http_service


test-security-human-account-read-foundation:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/HumanAccountReadService.cpp \
		core/security/tests/test_human_account_read_foundation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_read_foundation
	$(BUILD_DIR)/test_human_account_read_foundation


test-security-human-account-administration:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/src/BrowserSessionLifecycleService.cpp \
		core/security/src/HumanAccountAdministrationService.cpp \
		core/security/tests/test_human_account_administration_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_administration_service
	$(BUILD_DIR)/test_human_account_administration_service


test-security-public-account-collection:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_account_collection.cpp \
		-o $(BUILD_DIR)/test_public_account_collection
	$(BUILD_DIR)/test_public_account_collection
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_account_collection_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_account_collection_security
	$(BUILD_DIR)/test_public_account_collection_security
	node clients/reference-js/tests/test_public_v1_account_client.js
	python3 tools/check_p2_public_account_collection.py
	python3 tools/check_mu6b_public_account_item.py
	python3 tools/check_mu6c_public_account_lifecycle_mutation.py


test-security-managed-basic-authenticator:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_managed_basic_authenticator.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_managed_basic_authenticator
	$(BUILD_DIR)/test_managed_basic_authenticator


test-security-human-password-browser-session:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		$(BROWSER_SESSION_HTTP_SRC) \
		core/security/tests/test_human_password_browser_session.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_password_browser_session
	$(BUILD_DIR)/test_human_password_browser_session


test-security-human-account-recovery:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/BrowserSessionCredentialRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/HumanAccountRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityIssuanceRepository.cpp \
		core/security/src/SecurityPermissionGrantRepository.cpp \
		core/security/src/BrowserSessionIssuanceService.cpp \
		core/security/src/HumanAccountRecoveryService.cpp \
		core/security/tests/test_human_account_recovery_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_recovery_service
	$(BUILD_DIR)/test_human_account_recovery_service


test-security-browser-session-authenticator:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_browser_session_authenticator.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_authenticator
	$(BUILD_DIR)/test_browser_session_authenticator


test-security-browser-session-issuer-binding:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_browser_session_issuer_binding.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_issuer_binding
	$(BUILD_DIR)/test_browser_session_issuer_binding


test-security-browser-session-issuance-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_browser_session_issuance_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_issuance_service
	$(BUILD_DIR)/test_browser_session_issuance_service


test-security-browser-session-concurrency-limit:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		$(BROWSER_SESSION_HTTP_SRC) \
		core/security/tests/test_browser_session_concurrency_limit.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_concurrency_limit
	$(BUILD_DIR)/test_browser_session_concurrency_limit


test-security-browser-session-idle-expiry:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		$(BROWSER_SESSION_HTTP_SRC) \
		core/security/tests/test_browser_session_idle_expiry.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_idle_expiry
	$(BUILD_DIR)/test_browser_session_idle_expiry


test-security-browser-session-retention-cleanup:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_browser_session_retention_cleanup.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_retention_cleanup
	$(BUILD_DIR)/test_browser_session_retention_cleanup


test-security-browser-session-http-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		$(BROWSER_SESSION_HTTP_SRC) \
		core/http/tests/test_browser_session_http_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_http_service
	$(BUILD_DIR)/test_browser_session_http_service


test-security-browser-session-csrf-recovery:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		$(BROWSER_SESSION_HTTP_SRC) \
		core/http/tests/test_browser_session_csrf_recovery_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_csrf_recovery_service
	$(BUILD_DIR)/test_browser_session_csrf_recovery_service


test-security-browser-session-http-gate:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_browser_session_http_gate.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_http_gate
	$(BUILD_DIR)/test_browser_session_http_gate


test-security-http-gate:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_security_http_gate.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_security_http_gate
	$(BUILD_DIR)/test_security_http_gate


test-security-recording-marks:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_recording_marks_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recording_marks_security
	$(BUILD_DIR)/test_recording_marks_security


test-security-series-artwork-route-scope:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_series_artwork_settings_route_scope_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_series_artwork_settings_route_scope_security
	$(BUILD_DIR)/test_series_artwork_settings_route_scope_security


test-security-teletext-read:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_teletext_read_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_teletext_read_security
	$(BUILD_DIR)/test_teletext_read_security


test-security-hbbtv-read:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_hbbtv_read_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_hbbtv_read_security
	$(BUILD_DIR)/test_hbbtv_read_security

test-security-hbbtv-session:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_hbbtv_session_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_hbbtv_session_security
	$(BUILD_DIR)/test_hbbtv_session_security

test-security-osd-session:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_legacy_osd_session_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_legacy_osd_session_security
	$(BUILD_DIR)/test_legacy_osd_session_security


test-security-searchtimer-maintenance:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_searchtimer_maintenance_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_searchtimer_maintenance_security
	$(BUILD_DIR)/test_searchtimer_maintenance_security


test-security-searchtimer-execution:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_searchtimer_execution_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_searchtimer_execution_security
	$(BUILD_DIR)/test_searchtimer_execution_security


test-security-native-fuzzy-refresh:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_native_fuzzy_refresh_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_fuzzy_refresh_security
	$(BUILD_DIR)/test_native_fuzzy_refresh_security


test-security-safe-post:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_safe_post_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_safe_post_security
	$(BUILD_DIR)/test_safe_post_security


test-security-legacy-basic-retirement-acceptance:
	python3 tools/p2_legacy_basic_retirement_acceptance.py --self-test
	python3 tools/check_p2_legacy_basic_retirement_acceptance.py


test-security-manual-recording-metadata:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_manual_recording_metadata_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_manual_recording_metadata_security
	$(BUILD_DIR)/test_manual_recording_metadata_security


test-security: \
	test-security-architecture \
	test-security-authorization \
	test-security-configuration \
	test-security-accountability-event-repository \
	test-security-identity-repository \
	test-security-permission-grant-repository \
	test-security-first-admin-bootstrap-repository \
	test-security-first-admin-bootstrap-issuance-service \
	test-security-first-admin-claim-service \
	test-security-first-admin-claim-http-service \
	test-security-human-account-read-foundation \
	test-security-human-account-administration \
	test-security-public-account-collection \
	test-security-managed-basic-authenticator \
	test-security-human-password-browser-session \
	test-security-human-account-recovery \
	test-security-legacy-basic-retirement-acceptance \
	test-security-browser-session-authenticator \
	test-security-browser-session-issuer-binding \
	test-security-browser-session-issuance-service \
	test-security-browser-session-concurrency-limit \
	test-security-browser-session-idle-expiry \
	test-security-browser-session-retention-cleanup \
	test-security-browser-session-http-service \
	test-security-browser-session-csrf-recovery \
	test-security-browser-session-http-gate \
	test-security-http-gate \
	test-security-recording-marks \
	test-security-series-artwork-route-scope \
	test-security-teletext-read \
	test-security-hbbtv-read \
	test-security-hbbtv-session \
	test-security-searchtimer-maintenance \
	test-security-searchtimer-execution \
	test-security-native-fuzzy-refresh \
	test-security-safe-post \
	test-security-manual-recording-metadata

test: test-security
test-fast: test-security

test-architecture: test-security-architecture
