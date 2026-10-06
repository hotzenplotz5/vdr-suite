CXXFLAGS += -Icore/security/include
LDFLAGS += -lcrypt

SECURITY_REPOSITORY_SRC := \
	core/security/src/AccountabilityEventRepository.cpp \
	core/security/src/BrowserSessionCredentialRepository.cpp \
	core/security/src/BrowserSessionRetentionRepository.cpp \
	core/security/src/SecurityPermissionGrantRepository.cpp \
	core/security/src/CredentialVerifierRepository.cpp \
	core/security/src/DevicePairingRequestRepository.cpp \
	core/security/src/FirstAdminBootstrapRepository.cpp \
	core/security/src/HumanAccountRepository.cpp \
	core/security/src/HumanAccountAdministrationRepository.cpp \
	core/security/src/HumanAccountCreationRepository.cpp \
	core/security/src/HumanAccountCredentialSessionReadRepository.cpp \
	core/security/src/SecurityIdentityIssuanceRepository.cpp \
	core/security/src/SecurityIdentityProvisioningRepository.cpp \
	core/security/src/SecurityIdentityRepository.cpp \
	core/security/src/SecurityIdentityRetentionRepository.cpp

SECURITY_SERVICE_SRC := \
	core/security/src/BrowserSessionHttpGate.cpp \
	core/security/src/BrowserSessionIssuanceService.cpp \
	core/security/src/BrowserSessionLifecycleService.cpp \
	core/security/src/BrowserSessionRetentionService.cpp \
	core/security/src/DevicePairingRequestService.cpp \
	core/security/src/FirstAdminBootstrapIssuanceService.cpp \
	core/security/src/FirstAdminClaimService.cpp \
	core/security/src/HumanAccountAdministrationService.cpp \
	core/security/src/HumanAccountCreationService.cpp \
	core/security/src/HumanAccountCredentialSessionReadService.cpp \
	core/security/src/HumanAccountCredentialAdministrationService.cpp \
	core/security/src/HumanAccountSessionAdministrationService.cpp \
	core/security/src/HumanAccountGrantAdministrationService.cpp \
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

.PHONY: first-admin-bootstrap-issuer human-account-recovery test-security-device-pairing-request test-security test-security-architecture test-security-human-account-credential-session-read test-security-human-account-credential-administration test-security-human-account-session-administration test-security-public-account-security-metadata test-security-public-account-session-revoke test-security-public-account-credential-revoke test-security-legacy-basic-retirement-acceptance test-security-authorization test-security-configuration test-security-accountability-event-repository test-security-identity-repository test-security-permission-grant-repository test-security-first-admin-bootstrap-repository test-security-first-admin-bootstrap-issuance-service test-security-first-admin-claim-service test-security-first-admin-claim-http-service test-security-human-account-read-foundation test-security-human-account-administration test-security-human-account-creation test-security-human-account-grant-administration test-security-public-account-collection test-security-public-account-grants test-security-managed-basic-authenticator test-security-human-password-browser-session test-security-human-account-recovery test-security-browser-session-authenticator test-security-browser-session-issuer-binding test-security-browser-session-issuance-service test-security-browser-session-concurrency-limit test-security-browser-session-idle-expiry test-security-browser-session-retention-cleanup test-security-browser-session-http-service test-security-browser-session-csrf-recovery test-security-browser-session-http-gate test-security-http-gate test-security-recording-marks test-security-series-artwork-route-scope test-security-teletext-read test-security-hbbtv-read test-security-hbbtv-session test-security-osd-session test-security-searchtimer-maintenance test-security-searchtimer-execution test-security-native-fuzzy-refresh test-security-safe-post test-security-manual-recording-metadata

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
	python3 tools/check_mu6d_account_create_idempotency.py
	python3 tools/check_mu7_account_grant_administration.py
	python3 tools/check_mu8a_credential_session_metadata.py
	python3 tools/check_mu8b_session_revoke.py
	python3 tools/check_mu8c_credential_revoke.py
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


test-security-device-pairing-request:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/DevicePairingRequestRepository.cpp \
		core/security/src/DevicePairingRequestService.cpp \
		core/security/tests/test_device_pairing_request_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_device_pairing_request_service
	$(BUILD_DIR)/test_device_pairing_request_service


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


test-security-human-account-creation:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/src/HumanAccountCreationService.cpp \
		core/security/tests/test_human_account_creation_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_creation_service
	$(BUILD_DIR)/test_human_account_creation_service


test-security-human-account-grant-administration:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/src/HumanAccountGrantAdministrationService.cpp \
		core/security/tests/test_human_account_grant_administration_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_grant_administration_service
	$(BUILD_DIR)/test_human_account_grant_administration_service


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
	$(MAKE) test-security-human-account-creation
	node clients/reference-js/tests/test_public_v1_account_client.js
	node clients/reference-js/tests/test_public_v1_account_create_client.js
	python3 tools/check_p2_public_account_collection.py
	python3 tools/check_mu6b_public_account_item.py
	python3 tools/check_mu6c_public_account_lifecycle_mutation.py
	python3 tools/check_mu6d_account_create_idempotency.py


test-security-public-account-grants:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_account_grants.cpp \
		-o $(BUILD_DIR)/test_public_account_grants
	$(BUILD_DIR)/test_public_account_grants
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_account_grants_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_account_grants_security
	$(BUILD_DIR)/test_public_account_grants_security
	$(MAKE) test-security-human-account-grant-administration
	node clients/reference-js/tests/test_public_v1_account_grants_client.js
	python3 tools/check_mu7_account_grant_administration.py


test-security-human-account-credential-session-read:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/src/HumanAccountCredentialSessionReadService.cpp \
		core/security/tests/test_human_account_credential_session_read_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_credential_session_read_service
	$(BUILD_DIR)/test_human_account_credential_session_read_service


test-security-human-account-credential-administration:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/src/BrowserSessionLifecycleService.cpp \
		core/security/src/HumanAccountCredentialSessionReadService.cpp \
		core/security/src/HumanAccountCredentialAdministrationService.cpp \
		core/security/tests/test_human_account_credential_administration_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_credential_administration_service
	$(BUILD_DIR)/test_human_account_credential_administration_service


test-security-human-account-session-administration:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/src/BrowserSessionLifecycleService.cpp \
		core/security/src/HumanAccountCredentialSessionReadService.cpp \
		core/security/src/HumanAccountSessionAdministrationService.cpp \
		core/security/tests/test_human_account_session_administration_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_human_account_session_administration_service
	$(BUILD_DIR)/test_human_account_session_administration_service


test-security-public-account-security-metadata: test-security-human-account-credential-session-read
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_account_security_metadata.cpp \
		-o $(BUILD_DIR)/test_public_account_security_metadata
	$(BUILD_DIR)/test_public_account_security_metadata
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_account_security_metadata_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_account_security_metadata_security
	$(BUILD_DIR)/test_public_account_security_metadata_security
	node clients/reference-js/tests/test_public_v1_account_security_metadata_client.js
	python3 tools/check_mu8a_credential_session_metadata.py


test-security-public-account-session-revoke: test-security-human-account-session-administration
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_account_session_revoke.cpp \
		-o $(BUILD_DIR)/test_public_account_session_revoke
	$(BUILD_DIR)/test_public_account_session_revoke
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_account_session_revoke_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_account_session_revoke_security
	$(BUILD_DIR)/test_public_account_session_revoke_security
	node clients/reference-js/tests/test_public_v1_account_session_revoke_client.js
	python3 tools/check_mu8b_session_revoke.py


test-security-public-account-credential-revoke: test-security-human-account-credential-administration
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_account_credential_revoke.cpp \
		-o $(BUILD_DIR)/test_public_account_credential_revoke
	$(BUILD_DIR)/test_public_account_credential_revoke
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_account_credential_revoke_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_account_credential_revoke_security
	$(BUILD_DIR)/test_public_account_credential_revoke_security
	node clients/reference-js/tests/test_public_v1_account_credential_revoke_client.js
	python3 tools/check_mu8c_credential_revoke.py


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
	test-security-device-pairing-request \
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
	test-security-public-account-security-metadata \
	test-security-public-account-session-revoke \
	test-security-public-account-credential-revoke \
	test-security-human-account-credential-administration \
	test-security-human-account-session-administration \
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
