-- P7 synthetic older browser-session and playback schema, NOT a released
-- binary fixture. All keys and secrets are deliberately fake.
-- Preserve the current real FK contract while omitting only ALTER columns.
CREATE TABLE security_devices (
    device_id TEXT PRIMARY KEY,
    actor_id TEXT NOT NULL,
    display_name TEXT NOT NULL,
    active INTEGER NOT NULL DEFAULT 1,
    revoked_at TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id)
);
INSERT INTO security_devices(device_id,actor_id,display_name)
 VALUES('p7_pre_device','p7_pre_user','P7 test device');

CREATE TABLE security_sessions (
    session_id TEXT PRIMARY KEY,
    actor_id TEXT NOT NULL,
    device_id TEXT NOT NULL,
    active INTEGER NOT NULL DEFAULT 1,
    expires_at TEXT NOT NULL DEFAULT '',
    revoked_at TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id),
    FOREIGN KEY(device_id) REFERENCES security_devices(device_id)
);
INSERT INTO security_sessions(session_id,actor_id,device_id)
 VALUES('p7_pre_session','p7_pre_user','p7_pre_device');

-- The browser-session repository later added last_seen_at and backfills it
-- from created_at when missing. These are not usable credentials.
CREATE TABLE security_browser_session_credentials (
    token_id TEXT PRIMARY KEY,
    session_id TEXT NOT NULL UNIQUE,
    actor_id TEXT NOT NULL,
    device_id TEXT NOT NULL,
    credential_id TEXT NOT NULL UNIQUE,
    issued_from_credential_id TEXT NOT NULL,
    session_secret_hash TEXT NOT NULL,
    csrf_secret_hash TEXT NOT NULL,
    active INTEGER NOT NULL DEFAULT 1,
    expires_at TEXT NOT NULL,
    revoked_at TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id),
    FOREIGN KEY(device_id) REFERENCES security_devices(device_id),
    FOREIGN KEY(session_id) REFERENCES security_sessions(session_id),
    FOREIGN KEY(credential_id) REFERENCES security_credentials(credential_id),
    FOREIGN KEY(issued_from_credential_id) REFERENCES security_credentials(credential_id)
);
INSERT INTO security_browser_session_credentials(
    token_id,session_id,actor_id,device_id,credential_id,
    issued_from_credential_id,session_secret_hash,csrf_secret_hash,expires_at)
 VALUES('p7_pre_token','p7_pre_session','p7_pre_user','p7_pre_device',
        'p7_pre_credential','p7_pre_credential','SYNTHETIC_HASH_PRE',
        'SYNTHETIC_CSRF_PRE','2099-01-01 00:00:00');

-- Playback rows created before backend_native_id was introduced.
CREATE TABLE continue_watching_state (
    actor_id TEXT NOT NULL,
    backend_id TEXT NOT NULL,
    recording_id TEXT NOT NULL,
    position_seconds INTEGER NOT NULL CHECK(position_seconds >= 0),
    last_activity_at TEXT NOT NULL,
    last_operation_id TEXT NOT NULL,
    PRIMARY KEY(actor_id, backend_id, recording_id)
);
INSERT INTO continue_watching_state(
    actor_id,backend_id,recording_id,position_seconds,
    last_activity_at,last_operation_id)
 VALUES('p7_pre_user','p7_backend','p7_pre_recording',72,
        '2026-01-01T00:00:00Z','p7_pre_operation');

CREATE TABLE recently_watched_state (
    actor_id TEXT NOT NULL,
    backend_id TEXT NOT NULL,
    recording_id TEXT NOT NULL,
    position_seconds INTEGER NOT NULL CHECK(position_seconds >= 0),
    position_known INTEGER NOT NULL CHECK(position_known IN (0,1)),
    completion_known INTEGER NOT NULL CHECK(completion_known IN (0,1)),
    completed INTEGER NOT NULL CHECK(completed IN (0,1)),
    resume_relevance_known INTEGER NOT NULL CHECK(resume_relevance_known IN (0,1)),
    resume_relevant INTEGER NOT NULL CHECK(resume_relevant IN (0,1)),
    source_evidence TEXT NOT NULL,
    last_activity_at TEXT NOT NULL,
    last_operation_id TEXT NOT NULL,
    PRIMARY KEY(actor_id, backend_id, recording_id)
);
INSERT INTO recently_watched_state(
    actor_id,backend_id,recording_id,position_seconds,position_known,
    completion_known,completed,resume_relevance_known,resume_relevant,
    source_evidence,last_activity_at,last_operation_id)
 VALUES('p7_pre_user','p7_backend','p7_pre_recording',72,1,1,0,1,1,
        'synthetic','2026-01-01T00:00:00Z','p7_pre_operation');
