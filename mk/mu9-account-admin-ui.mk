.PHONY: test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui

test-mu9a-account-admin-read-ui:
	node web/frontend/tests/test_mu9a_account_admin_read_ui.js
	python3 tools/check_mu9a_account_admin_read_ui.py

test-mu9b-account-lifecycle-ui:
	node web/frontend/tests/test_mu9b_account_lifecycle_mutation_ui.js
	python3 tools/check_mu9b_account_lifecycle_ui.py

test-frontend-contracts: test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui
test-ci-frontend: test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui
