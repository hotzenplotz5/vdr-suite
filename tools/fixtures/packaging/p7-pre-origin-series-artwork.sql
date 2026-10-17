-- Synthetic pre-origin series artwork table; CI-only and NOT source-attested.
CREATE TABLE epg_series_artwork_fallback (
    backend_id TEXT NOT NULL,
    channel_id TEXT NOT NULL,
    event_id TEXT NOT NULL,
    provider TEXT NOT NULL,
    path TEXT NOT NULL,
    width INTEGER NOT NULL,
    height INTEGER NOT NULL,
    resolved_at INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (backend_id, channel_id, event_id),
    CHECK (provider <> '' AND provider <> 'none' AND provider <> 'tvscraper'),
    CHECK (path <> ''),
    CHECK (width > 0 AND height > 0),
    CHECK (resolved_at > 0)
);
INSERT INTO epg_series_artwork_fallback(
    backend_id, channel_id, event_id, provider, path, width, height, resolved_at
) VALUES (
    'p7_pre_backend','p7_pre_channel','p7_pre_event','tvmaze',
    '/p7/synthetic-prior-artwork.jpg',320,480,100
);
