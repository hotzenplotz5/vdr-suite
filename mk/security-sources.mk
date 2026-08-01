CXXFLAGS += -Icore/security/include
LDFLAGS += -lcrypt

SECURITY_REPOSITORY_SRC := \
	core/security/src/AccountabilityEventRepository.cpp \
	core/security/src/BrowserSessionCredentialRepository.cpp \
	core/security/src/SecurityPermissionGrantRepository.cpp \
	core/security/src/CredentialVerifierRepository.cpp \
	core/security/src/SecurityIdentityIssuanceRepository.cpp \
	core/security/src/SecurityIdentityProvisioningRepository.cpp \
	core/security/src/SecurityIdentityRepository.cpp

SECURITY_SERVICE_SRC := \
	core/security/src/BrowserSessionHttpGate.cpp \
	core/security/src/BrowserSessionIssuanceService.cpp \
	core/security/src/BrowserSessionLifecycleService.cpp

SECURITY_SRC := \
	$(SECURITY_REPOSITORY_SRC) \
	$(SECURITY_SERVICE_SRC)

BROWSER_SESSION_HTTP_SRC := \
	core/http/src/BrowserSessionHttpService.cpp

.PHONY: test-security test-security-architecture test-security-authorization test-security-configuration test-security-accountability-event-repository test-security-identity-repository test-security-permission-grant-repository test-security-managed-basic-authenticator test-security-browser-session-authenticator test-security-browser-session-issuance-service test-security-browser-session-http-service test-security-browser-session-http-gate test-security-http-gate test-security-searchtimer-maintenance test-security-safe-post

test-security-architecture:
	python3 tools/check_security_identity_architecture.py
	python3 tools/check_browser_session_issuance_architecture.py
	python3 tools/check_searchtimer_maintenance_security.py
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


test-security-managed-basic-authenticator:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_managed_basic_authenticator.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_managed_basic_authenticator
	$(BUILD_DIR)/test_managed_basic_authenticator


test-security-browser-session-authenticator:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_browser_session_authenticator.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_authenticator
	$(BUILD_DIR)/test_browser_session_authenticator


test-security-browser-session-issuance-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_browser_session_issuance_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_issuance_service
	$(BUILD_DIR)/test_browser_session_issuance_service


test-security-browser-session-http-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		$(BROWSER_SESSION_HTTP_SRC) \
		core/http/tests/test_browser_session_http_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_browser_session_http_service
	$(BUILD_DIR)/test_browser_session_http_service


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


test-security-searchtimer-maintenance:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_searchtimer_maintenance_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_searchtimer_maintenance_security
	$(BUILD_DIR)/test_searchtimer_maintenance_security


test-security-safe-post:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_safe_post_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_safe_post_security
	$(BUILD_DIR)/test_safe_post_security


test-security: \
	test-security-architecture \
	test-security-authorization \
	test-security-configuration \
	test-security-accountability-event-repository \
	test-security-identity-repository \
	test-security-permission-grant-repository \
	test-security-managed-basic-authenticator \
	test-security-browser-session-authenticator \
	test-security-browser-session-issuance-service \
	test-security-browser-session-http-service \
	test-security-browser-session-http-gate \
	test-security-http-gate \
	test-security-searchtimer-maintenance \
	test-security-safe-post

test: test-security
test-fast: test-security

test-architecture: test-security-architecture
