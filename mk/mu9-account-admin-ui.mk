.PHONY: test-device-admin-ui test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui test-mu9c-session-revoke-ui test-mu9d-credential-revoke-ui test-mu9e-grant-mutation-ui test-mu9f-account-create-ui test-multiuser-admin-ux-guidance

test-device-admin-ui:
	node web/frontend/tests/test_device_admin_ui.js
	node clients/reference-js/tests/test_public_v1_device_collection_client.js

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

test-mu9f-account-create-ui:
	node web/frontend/tests/test_mu9f_account_create_ui.js
	python3 tools/check_mu9f_account_create_ui.py

test-multiuser-admin-ux-guidance:
	python3 tools/check_multiuser_admin_ux_guidance.py

test-frontend-contracts: test-device-admin-ui test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui test-mu9c-session-revoke-ui test-mu9d-credential-revoke-ui test-mu9e-grant-mutation-ui test-mu9f-account-create-ui test-multiuser-admin-ux-guidance test-addon-media-import-ui
test-ci-frontend: test-device-admin-ui test-mu9a-account-admin-read-ui test-mu9b-account-lifecycle-ui test-mu9c-session-revoke-ui test-mu9d-credential-revoke-ui test-mu9e-grant-mutation-ui test-mu9f-account-create-ui test-multiuser-admin-ux-guidance test-addon-media-import-ui

.PHONY: test-addon-media-import-ui
test-addon-media-import-ui:
	node --check web/frontend/api/public-v1-client.js
	node --check web/frontend/modules/media-import.js
	node --check web/frontend/app.js
	node web/frontend/tests/test_media_import_composition.js
