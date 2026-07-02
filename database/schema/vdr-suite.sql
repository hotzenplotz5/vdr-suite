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
CREATE INDEX IF NOT EXISTS idx_epg_events_backend_title
    ON epg_events (backend_id, title);

CREATE TABLE IF NOT EXISTS epgsearch_native_fuzzy_capability_probes (
    backend_id TEXT PRIMARY KEY,
    create_accepted INTEGER NOT NULL DEFAULT 0,
    readback_available INTEGER NOT NULL DEFAULT 0,
    mode_preserved INTEGER NOT NULL DEFAULT 0,
    tolerance_preserved INTEGER NOT NULL DEFAULT 0,
    cleanup_succeeded INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);
