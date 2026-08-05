-- VDR-Suite SQLite Schema
-- Phase 1: Database Foundation

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS recordings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    subtitle TEXT,
    description TEXT,
    channel TEXT,
    start_time TEXT,
    end_time TEXT,
    duration_seconds INTEGER,
    recording_path TEXT NOT NULL UNIQUE,
    recording_format TEXT CHECK(recording_format IN ('PES', 'TS', 'UNKNOWN')) DEFAULT 'UNKNOWN',
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS metadata (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    recording_id INTEGER,
    media_type TEXT CHECK(media_type IN ('MOVIE', 'SERIES', 'EPISODE', 'UNKNOWN')) DEFAULT 'UNKNOWN',
    title TEXT NOT NULL,
    original_title TEXT,
    year INTEGER,
    season_number INTEGER,
    episode_number INTEGER,
    genre TEXT,
    description TEXT,
    source TEXT,
    external_id TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (recording_id) REFERENCES recordings(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS artwork (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    metadata_id INTEGER,
    recording_id INTEGER,
    poster_path TEXT,
    fanart_path TEXT,
    banner_path TEXT,
    thumbnail_path TEXT,
    source TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (metadata_id) REFERENCES metadata(id) ON DELETE CASCADE,
    FOREIGN KEY (recording_id) REFERENCES recordings(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS jobs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    recording_id INTEGER,
    job_type TEXT NOT NULL,
    status TEXT CHECK(status IN ('PENDING', 'RUNNING', 'DONE', 'FAILED', 'CANCELLED')) DEFAULT 'PENDING',
    priority INTEGER DEFAULT 0,
    message TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    started_at TEXT,
    finished_at TEXT,

    FOREIGN KEY (recording_id) REFERENCES recordings(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_recordings_title ON recordings(title);
CREATE INDEX IF NOT EXISTS idx_recordings_path ON recordings(recording_path);
CREATE INDEX IF NOT EXISTS idx_metadata_title ON metadata(title);
CREATE INDEX IF NOT EXISTS idx_metadata_type ON metadata(media_type);
CREATE INDEX IF NOT EXISTS idx_jobs_status ON jobs(status);
CREATE INDEX IF NOT EXISTS idx_jobs_type ON jobs(job_type);

CREATE TABLE IF NOT EXISTS epg_events (
    backend_id TEXT NOT NULL,
    channel_id TEXT NOT NULL,
    event_id TEXT NOT NULL,
    title TEXT NOT NULL,
    subtitle TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    start_time TEXT NOT NULL,
    end_time TEXT NOT NULL,
    duration_seconds INTEGER NOT NULL DEFAULT 0,
    parental_rating INTEGER NOT NULL DEFAULT 0,
    content_descriptors TEXT NOT NULL DEFAULT '',
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (backend_id, channel_id, event_id)
);

CREATE INDEX IF NOT EXISTS idx_epg_events_backend_time
    ON epg_events (backend_id, start_time, end_time);
CREATE INDEX IF NOT EXISTS idx_epg_events_backend_channel_time
    ON epg_events (backend_id, channel_id, start_time, end_time);
CREATE INDEX IF NOT EXISTS idx_epg_events_backend_end_epoch
    ON epg_events (
        backend_id,
        CAST(end_time AS INTEGER),
        CAST(start_time AS INTEGER),
        channel_id,
        event_id
    );
CREATE INDEX IF NOT EXISTS idx_epg_events_backend_title
    ON epg_events (backend_id, title);

CREATE TABLE IF NOT EXISTS epg_event_artwork (
    backend_id TEXT NOT NULL,
    channel_id TEXT NOT NULL,
    event_id TEXT NOT NULL,
    provider TEXT NOT NULL,
    path TEXT NOT NULL,
    width INTEGER NOT NULL DEFAULT 0,
    height INTEGER NOT NULL DEFAULT 0,
    resolved_at INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (backend_id, channel_id, event_id)
);

CREATE INDEX IF NOT EXISTS idx_epg_event_artwork_provider
    ON epg_event_artwork (backend_id, provider);

CREATE TABLE IF NOT EXISTS vdr_recording_cache (
    backend_id TEXT NOT NULL,
    cache_key TEXT NOT NULL,
    recording_id TEXT NOT NULL DEFAULT '',
    backend_native_id TEXT NOT NULL DEFAULT '',
    title TEXT NOT NULL DEFAULT '',
    path TEXT NOT NULL DEFAULT '',
    start_time TEXT NOT NULL DEFAULT '',
    duration_seconds INTEGER NOT NULL DEFAULT 0,
    size_mb INTEGER NOT NULL DEFAULT 0,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    last_seen_at TEXT DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (backend_id, cache_key)
);

CREATE INDEX IF NOT EXISTS idx_vdr_recording_cache_backend_title
    ON vdr_recording_cache (backend_id, title);
CREATE INDEX IF NOT EXISTS idx_vdr_recording_cache_backend_start
    ON vdr_recording_cache (backend_id, start_time);
CREATE INDEX IF NOT EXISTS idx_vdr_recording_cache_backend_path
    ON vdr_recording_cache (backend_id, path);

CREATE TABLE IF NOT EXISTS epgsearch_native_fuzzy_capability_probes (
    backend_id TEXT PRIMARY KEY,
    create_accepted INTEGER NOT NULL DEFAULT 0,
    readback_available INTEGER NOT NULL DEFAULT 0,
    mode_preserved INTEGER NOT NULL DEFAULT 0,
    tolerance_preserved INTEGER NOT NULL DEFAULT 0,
    cleanup_succeeded INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS vdr_recording_cache_status (
    backend_id TEXT PRIMARY KEY,
    state TEXT NOT NULL DEFAULT 'empty',
    total_count INTEGER NOT NULL DEFAULT 0,
    started_at TEXT NOT NULL DEFAULT '',
    finished_at TEXT NOT NULL DEFAULT '',
    last_error TEXT NOT NULL DEFAULT '',
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);

-- Phase 63: Backend Agent enrollment and lease foundation.
-- Runtime bootstrap remains additive and idempotent; normal reads never run DDL.
CREATE TABLE IF NOT EXISTS backend_agent_enrollments (
    enrollment_id TEXT PRIMARY KEY,
    backend_id TEXT NOT NULL,
    token_hash TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'pending',
    expires_at INTEGER NOT NULL,
    agent_id TEXT NOT NULL DEFAULT '',
    created_at INTEGER NOT NULL,
    consumed_at INTEGER NOT NULL DEFAULT 0,
    revoked_at INTEGER NOT NULL DEFAULT 0,
    revocation_reason TEXT NOT NULL DEFAULT '',
    CHECK(status IN ('pending','consumed','revoked','expired'))
);

CREATE INDEX IF NOT EXISTS idx_backend_agent_enrollment_backend
    ON backend_agent_enrollments (backend_id, status, expires_at);

CREATE TABLE IF NOT EXISTS backend_agents (
    agent_id TEXT PRIMARY KEY,
    backend_id TEXT NOT NULL,
    actor_id TEXT NOT NULL UNIQUE,
    device_id TEXT NOT NULL UNIQUE,
    credential_id TEXT NOT NULL UNIQUE,
    credential_generation INTEGER NOT NULL DEFAULT 1,
    agent_instance_id TEXT NOT NULL DEFAULT '',
    backend_generation INTEGER NOT NULL DEFAULT 0,
    protocol_version TEXT NOT NULL DEFAULT '',
    software_version TEXT NOT NULL DEFAULT '',
    heartbeat_sequence INTEGER NOT NULL DEFAULT 0,
    capability_revision INTEGER NOT NULL DEFAULT 0,
    last_connected_at INTEGER NOT NULL DEFAULT 0,
    last_heartbeat_at INTEGER NOT NULL DEFAULT 0,
    lease_expires_at INTEGER NOT NULL DEFAULT 0,
    revoked_at INTEGER NOT NULL DEFAULT 0,
    revocation_reason TEXT NOT NULL DEFAULT '',
    incompatible INTEGER NOT NULL DEFAULT 0,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_backend_agents_active_backend
    ON backend_agents (backend_id)
    WHERE revoked_at = 0;

CREATE INDEX IF NOT EXISTS idx_backend_agents_backend_generation
    ON backend_agents (backend_id, backend_generation);

CREATE TABLE IF NOT EXISTS backend_agent_credential_rotations (
    rotation_id TEXT PRIMARY KEY,
    agent_id TEXT NOT NULL,
    from_generation INTEGER NOT NULL,
    to_generation INTEGER NOT NULL,
    rotated_at INTEGER NOT NULL,
    UNIQUE (agent_id, to_generation),
    FOREIGN KEY (agent_id) REFERENCES backend_agents(agent_id)
);

CREATE TABLE IF NOT EXISTS backend_agent_capabilities (
    agent_id TEXT NOT NULL,
    capability_revision INTEGER NOT NULL,
    capability_kind TEXT NOT NULL,
    capability_name TEXT NOT NULL,
    capability_value TEXT NOT NULL,
    PRIMARY KEY (agent_id, capability_kind, capability_name),
    FOREIGN KEY (agent_id) REFERENCES backend_agents(agent_id)
);
