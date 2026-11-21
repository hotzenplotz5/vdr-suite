-- P7 synthetic older device-pairing shape, NOT an attested historical release.
-- Contains deliberately unusable fake secrets, no real device identity.
-- Only the three ALTER columns missing below are relevant to this regression.
CREATE TABLE security_device_pairing_requests (
    pairing_request_id TEXT PRIMARY KEY,
    user_code_hash TEXT NOT NULL UNIQUE,
    pairing_token_hash TEXT NOT NULL UNIQUE,
    display_name TEXT NOT NULL,
    client_kind TEXT NOT NULL,
    app_version TEXT NOT NULL DEFAULT '',
    state TEXT NOT NULL DEFAULT 'pending',
    expires_at TEXT NOT NULL,
    invalidated_at TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO security_device_pairing_requests (
    pairing_request_id, user_code_hash, pairing_token_hash, display_name,
    client_kind, app_version, state, expires_at, invalidated_at
) VALUES (
    'p7_pre_pairing', 'SYNTHETIC_HASH_1', 'SYNTHETIC_TOKEN_1',
    'P7 older fixture device', 'vidaa', 'test-only', 'pending',
    '2099-01-01 00:00:00', ''
);
