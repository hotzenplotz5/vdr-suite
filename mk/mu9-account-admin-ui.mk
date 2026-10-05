.PHONY: test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui test-mu9c-session-revoke-ui test-mu9d-credential-revoke-ui test-mu9e-grant-mutation-ui

test-mu9a-account-admin-read-ui:
	node web/frontend/tests/test_mu9a_account_admin_read_ui.js
	python3 tools/check_mu9a_account_admin_read_ui.py

test-mu9b-account-lifecycle-ui:
	node web/frontend/tests/test_mu9b_account_lifecycle_mutation_ui.js
	python3 tools/check_mu9b_account_lifecycle_ui.py

test-mu9c-session-revoke-ui:
	node web/frontend/tests/test_mu9c_session_revoke_ui.js
	python3 tools/check_mu9c_session_revoke_ui.py

test-mu9d-credential-revoke-ui:
	node web/frontend/tests/test_mu9d_credential_revoke_ui.js
	python3 tools/check_mu9d_credential_revoke_ui.py

test-mu9e-grant-mutation-ui:
	node web/frontend/tests/test_mu9e_grant_mutation_ui.js
	python3 tools/check_mu9e_grant_mutation_ui.py

test-frontend-contracts: test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui test-mu9c-session-revoke-ui test-mu9d-credential-revoke-ui test-mu9e-grant-mutation-ui
test-ci-frontend: test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui test-mu9c-session-revoke-ui test-mu9d-credential-revoke-ui test-mu9e-grant-mutation-ui
