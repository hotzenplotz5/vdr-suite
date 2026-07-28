CXXFLAGS += -Icore/security/include
LDFLAGS += -lcrypt

SECURITY_REPOSITORY_SRC := \
	core/security/src/AccountabilityEventRepository.cpp \
	core/security/src/BrowserSessionCredentialRepository.cpp \
	core/security/src/CredentialVerifierRepository.cpp \
	core/security/src/SecurityIdentityIssuanceRepository.cpp \
	core/security/src/SecurityIdentityProvisioningRepository.cpp \
	core/security/src/SecurityIdentityRepository.cpp

SECURITY_SERVICE_SRC := \
	core/security/src/BrowserSessionIssuanceService.cpp

SECURITY_SRC := \
	$(SECURITY_REPOSITORY_SRC) \
	$(SECURITY_SERVICE_SRC)

.PHONY: test-security test-security-architecture test-security-authorization test-security-configuration test-security-accountability-event-repository test-security-identity-repository test-security-managed-basic-authenticator test-security-browser-session-authenticator test-security-browser-session-issuance-service test-security-http-gate

test-security-architecture:
	python3 tools/check_security_identity_architecture.py
	python3 tools/check_browser_session_issuance_architecture.py


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


test-security-http-gate:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_REPOSITORY_SRC) \
		core/security/tests/test_security_http_gate.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_security_http_gate
	$(BUILD_DIR)/test_security_http_gate


test-security: \
	test-security-architecture \
	test-security-authorization \
	test-security-configuration \
	test-security-accountability-event-repository \
	test-security-identity-repository \
	test-security-managed-basic-authenticator \
	test-security-browser-session-authenticator \
	test-security-browser-session-issuance-service \
	test-security-http-gate

test: test-security
test-fast: test-security

test-architecture: test-security-architecture
