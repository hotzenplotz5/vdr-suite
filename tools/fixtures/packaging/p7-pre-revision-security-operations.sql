-- Reconstructed P7 test-only pre-column-migration schema.
-- NOT a previously released package's SQL or a historical tagged SQL file.
-- Do not apply this fixture to any database outside a private CI scratch root.
PRAGMA foreign_keys=ON;
CREATE TABLE security_actors (
 actor_id TEXT PRIMARY KEY, actor_type TEXT NOT NULL,
 display_name TEXT NOT NULL, active INTEGER NOT NULL DEFAULT 1,
 revoked_at TEXT NOT NULL DEFAULT '',
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO security_actors(actor_id,actor_type,display_name)
 VALUES('p7_pre_user','user','Synthetic prior-schema actor');

CREATE TABLE security_credentials (
 credential_id TEXT PRIMARY KEY, actor_id TEXT NOT NULL,
 credential_type TEXT NOT NULL, active INTEGER NOT NULL DEFAULT 1,
 expires_at TEXT NOT NULL DEFAULT '', revoked_at TEXT NOT NULL DEFAULT '',
 rotated_from_credential_id TEXT NOT NULL DEFAULT '',
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id)
);
INSERT INTO security_credentials(credential_id,actor_id,credential_type)
 VALUES('p7_pre_credential','p7_pre_user','basic');

CREATE TABLE security_basic_credential_verifiers (
 credential_id TEXT PRIMARY KEY, login_name TEXT NOT NULL UNIQUE,
 password_hash TEXT NOT NULL,
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 FOREIGN KEY(credential_id) REFERENCES security_credentials(credential_id)
);
-- Deliberately invalid and obviously artificial sentinel, not a password.
INSERT INTO security_basic_credential_verifiers
 (credential_id,login_name,password_hash)
 VALUES('p7_pre_credential','p7_pre_login','P7_NOT_A_VALID_PASSWORD_HASH');

-- Existing table before HumanAccountRepository added "revision".
CREATE TABLE security_human_accounts (
 account_id TEXT PRIMARY KEY, actor_id TEXT NOT NULL UNIQUE,
 display_name TEXT NOT NULL, active INTEGER NOT NULL DEFAULT 1,
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id)
);
INSERT INTO security_human_accounts(account_id,actor_id,display_name)
 VALUES('p7_pre_account','p7_pre_user','Synthetic old account');

-- Existing table before MutationOperationRepository added fingerprint.
CREATE TABLE mutation_operations (
 operation_id TEXT PRIMARY KEY, operation_revision INTEGER NOT NULL,
 idempotency_key TEXT NOT NULL, actor_id TEXT NOT NULL,
 backend_id TEXT NOT NULL, backend_generation INTEGER NOT NULL,
 resource_type TEXT NOT NULL, resource_id TEXT NOT NULL,
 expected_revision TEXT NOT NULL, action_family TEXT NOT NULL,
 request_fingerprint TEXT NOT NULL, requested_at INTEGER NOT NULL,
 deadline INTEGER NOT NULL, verification_policy TEXT NOT NULL,
 state TEXT NOT NULL, result_reference TEXT NOT NULL,
 updated_at INTEGER NOT NULL,
 CHECK(operation_revision>0), CHECK(backend_generation>0),
 CHECK(verification_policy IN
 ('none','readback_required','event_confirmation','reconciliation_required')),
 CHECK(state IN
 ('accepted','rejected','conflict','queued','dispatching',
  'executed_unverified','succeeded','failed_before_dispatch',
  'failed_verified','outcome_unknown','cancelled'))
);
INSERT INTO mutation_operations(
 operation_id,operation_revision,idempotency_key,actor_id,
 backend_id,backend_generation,resource_type,resource_id,
 expected_revision,action_family,request_fingerprint,requested_at,
 deadline,verification_policy,state,result_reference,updated_at
) VALUES('p7_pre_operation',1,'pre_once','p7_pre_user',
         'p7_backend',1,'timer','synthetic','pre_rev',
         'timer.create','P7_REQ',100,200,'readback_required',
         'queued','',100);
