bool executeFoundationalSchema(Database& database)
{
    static const char* schema = R"sql(
PRAGMA foreign_keys = ON;
CREATE TABLE IF NOT EXISTS suite_metadata_schema_versions (
    version INTEGER PRIMARY KEY,
    description TEXT NOT NULL,
    applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (version > 0)
);
INSERT OR IGNORE INTO suite_metadata_schema_versions (version, description)
VALUES (1, 'Suite metadata entity, target, provider, evidence and assignment foundation');
CREATE TABLE IF NOT EXISTS suite_metadata_entities (
    metadata_entity_id TEXT PRIMARY KEY,
    media_type TEXT NOT NULL,
    lifecycle_state TEXT NOT NULL DEFAULT 'active',
    revision INTEGER NOT NULL DEFAULT 1,
    merged_into_entity_id TEXT,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (length(metadata_entity_id) = 38 AND substr(metadata_entity_id, 1, 6) = 'mdent_' AND substr(metadata_entity_id, 7) NOT GLOB '*[^0-9a-f]*'),
    CHECK (media_type IN ('unknown','movie','series','season','episode','programme','person')),
    CHECK (lifecycle_state IN ('active','merged','retired')),
    CHECK (revision > 0),
    CHECK ((lifecycle_state = 'merged' AND merged_into_entity_id IS NOT NULL) OR (lifecycle_state <> 'merged' AND merged_into_entity_id IS NULL)),
    CHECK (merged_into_entity_id IS NULL OR merged_into_entity_id <> metadata_entity_id),
    FOREIGN KEY (merged_into_entity_id) REFERENCES suite_metadata_entities(metadata_entity_id) ON DELETE RESTRICT
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_entities_media_type ON suite_metadata_entities (media_type, lifecycle_state);
CREATE TABLE IF NOT EXISTS suite_metadata_targets (
    metadata_target_id TEXT PRIMARY KEY,
    target_type TEXT NOT NULL,
    lifecycle_state TEXT NOT NULL DEFAULT 'active',
    revision INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (length(metadata_target_id) = 38 AND substr(metadata_target_id, 1, 6) = 'mdtgt_' AND substr(metadata_target_id, 7) NOT GLOB '*[^0-9a-f]*'),
    CHECK (target_type IN ('recording','program-event','timer-intent')),
    CHECK (lifecycle_state IN ('active','retired')),
    CHECK (revision > 0)
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_targets_type ON suite_metadata_targets (target_type, lifecycle_state);
CREATE TABLE IF NOT EXISTS suite_metadata_providers (
    provider_id TEXT PRIMARY KEY,
    provider_kind TEXT NOT NULL,
    display_name TEXT NOT NULL DEFAULT '',
    lifecycle_state TEXT NOT NULL DEFAULT 'active',
    attribution_required INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (length(provider_id) BETWEEN 1 AND 64),
    CHECK (provider_id = lower(provider_id)),
    CHECK (provider_id NOT GLOB '*[^a-z0-9._-]*'),
    CHECK (substr(provider_id, 1, 1) GLOB '[a-z0-9]'),
    CHECK (substr(provider_id, -1, 1) GLOB '[a-z0-9]'),
    CHECK (provider_kind IN ('epg','plugin','external-catalog','sidecar','manual','suite-db')),
    CHECK (lifecycle_state IN ('active','disabled','retired')),
    CHECK (attribution_required IN (0,1))
);
CREATE TABLE IF NOT EXISTS suite_metadata_provider_scopes (
    provider_id TEXT NOT NULL,
    scope_type TEXT NOT NULL,
    backend_id TEXT NOT NULL DEFAULT '',
    enabled INTEGER NOT NULL DEFAULT 1,
    priority INTEGER NOT NULL DEFAULT 0,
    runtime_state TEXT NOT NULL DEFAULT 'active',
    last_error TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (provider_id, scope_type, backend_id),
    CHECK (scope_type IN ('global','backend')),
    CHECK ((scope_type = 'global' AND backend_id = '') OR (scope_type = 'backend' AND backend_id <> '')),
    CHECK (enabled IN (0,1)),
    CHECK (runtime_state IN ('active','degraded','disabled')),
    FOREIGN KEY (provider_id) REFERENCES suite_metadata_providers(provider_id) ON DELETE CASCADE
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_provider_scopes_backend ON suite_metadata_provider_scopes (scope_type, backend_id, enabled, priority);
CREATE TABLE IF NOT EXISTS suite_metadata_evidence (
    metadata_evidence_id TEXT PRIMARY KEY,
    metadata_target_id TEXT NOT NULL,
    provider_id TEXT NOT NULL,
    backend_id TEXT NOT NULL DEFAULT '',
    source_entity_type TEXT NOT NULL DEFAULT '',
    source_external_id TEXT NOT NULL DEFAULT '',
    observed_at TEXT NOT NULL,
    language TEXT NOT NULL DEFAULT '',
    provider_revision TEXT NOT NULL DEFAULT '',
    payload_schema_version INTEGER NOT NULL DEFAULT 1,
    normalized_payload TEXT NOT NULL DEFAULT '{}',
    payload_fingerprint TEXT NOT NULL DEFAULT '',
    confidence REAL NOT NULL DEFAULT 0.0,
    evidence_state TEXT NOT NULL DEFAULT 'observed',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (length(metadata_evidence_id) = 38 AND substr(metadata_evidence_id, 1, 6) = 'mdevd_' AND substr(metadata_evidence_id, 7) NOT GLOB '*[^0-9a-f]*'),
    CHECK (payload_schema_version > 0),
    CHECK (confidence >= 0.0 AND confidence <= 1.0),
    CHECK (evidence_state IN ('observed','stale','retracted')),
    FOREIGN KEY (metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE RESTRICT,
    FOREIGN KEY (provider_id) REFERENCES suite_metadata_providers(provider_id) ON DELETE RESTRICT
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_evidence_target_time ON suite_metadata_evidence (metadata_target_id, observed_at);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_evidence_provider_source ON suite_metadata_evidence (provider_id, backend_id, source_entity_type, source_external_id);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_evidence_fingerprint ON suite_metadata_evidence (provider_id, payload_fingerprint);
CREATE TRIGGER IF NOT EXISTS trg_suite_metadata_evidence_no_update BEFORE UPDATE ON suite_metadata_evidence BEGIN SELECT RAISE(ABORT, 'suite metadata evidence is immutable'); END;
CREATE TRIGGER IF NOT EXISTS trg_suite_metadata_evidence_no_delete BEFORE DELETE ON suite_metadata_evidence BEGIN SELECT RAISE(ABORT, 'suite metadata evidence is immutable'); END;
CREATE TABLE IF NOT EXISTS suite_metadata_assignments (
    metadata_assignment_id TEXT PRIMARY KEY,
    metadata_target_id TEXT NOT NULL,
    metadata_entity_id TEXT NOT NULL,
    assignment_state TEXT NOT NULL DEFAULT 'selected',
    confidence REAL NOT NULL DEFAULT 0.0,
    manual_assignment INTEGER NOT NULL DEFAULT 0,
    relationship_locked INTEGER NOT NULL DEFAULT 0,
    supersedes_assignment_id TEXT,
    created_by_ref TEXT NOT NULL DEFAULT '',
    revision INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (length(metadata_assignment_id) = 38 AND substr(metadata_assignment_id, 1, 6) = 'mdasg_' AND substr(metadata_assignment_id, 7) NOT GLOB '*[^0-9a-f]*'),
    CHECK (assignment_state IN ('selected','superseded','disputed','withdrawn')),
    CHECK (confidence >= 0.0 AND confidence <= 1.0),
    CHECK (manual_assignment IN (0,1)),
    CHECK (relationship_locked IN (0,1)),
    CHECK (revision > 0),
    CHECK (supersedes_assignment_id IS NULL OR supersedes_assignment_id <> metadata_assignment_id),
    FOREIGN KEY (metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE RESTRICT,
    FOREIGN KEY (metadata_entity_id) REFERENCES suite_metadata_entities(metadata_entity_id) ON DELETE RESTRICT,
    FOREIGN KEY (supersedes_assignment_id) REFERENCES suite_metadata_assignments(metadata_assignment_id) ON DELETE RESTRICT
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_suite_metadata_selected_assignment_target ON suite_metadata_assignments (metadata_target_id) WHERE assignment_state = 'selected';
CREATE INDEX IF NOT EXISTS idx_suite_metadata_assignments_entity ON suite_metadata_assignments (metadata_entity_id, assignment_state);
CREATE TABLE IF NOT EXISTS suite_metadata_assignment_evidence (
    metadata_assignment_id TEXT NOT NULL,
    metadata_evidence_id TEXT NOT NULL,
    evidence_role TEXT NOT NULL DEFAULT 'supporting',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (metadata_assignment_id, metadata_evidence_id),
    CHECK (evidence_role IN ('supporting','contradicting','manual-override')),
    FOREIGN KEY (metadata_assignment_id) REFERENCES suite_metadata_assignments(metadata_assignment_id) ON DELETE CASCADE,
    FOREIGN KEY (metadata_evidence_id) REFERENCES suite_metadata_evidence(metadata_evidence_id) ON DELETE RESTRICT
);
CREATE TABLE IF NOT EXISTS suite_metadata_entity_external_ids (
    metadata_entity_id TEXT NOT NULL,
    provider_id TEXT NOT NULL,
    external_namespace TEXT NOT NULL,
    external_id TEXT NOT NULL,
    metadata_evidence_id TEXT,
    binding_state TEXT NOT NULL DEFAULT 'active',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (metadata_entity_id, provider_id, external_namespace, external_id),
    CHECK (external_namespace <> ''),
    CHECK (external_id <> ''),
    CHECK (binding_state IN ('active','disputed','retracted')),
    FOREIGN KEY (metadata_entity_id) REFERENCES suite_metadata_entities(metadata_entity_id) ON DELETE RESTRICT,
    FOREIGN KEY (provider_id) REFERENCES suite_metadata_providers(provider_id) ON DELETE RESTRICT,
    FOREIGN KEY (metadata_evidence_id) REFERENCES suite_metadata_evidence(metadata_evidence_id) ON DELETE RESTRICT
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_external_id_lookup ON suite_metadata_entity_external_ids (provider_id, external_namespace, external_id, binding_state);
)sql";
    return database.execute(schema);
}

bool executeGenreSchema(Database& database)
{
    return database.execute(
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(2,'Backend-scoped normalized genre bindings and read model');"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(3,'Backend-scoped VDR channel snapshot cache for metadata read models');"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(4,'Read-isolated indexed EPG genre access');"
        "CREATE TABLE IF NOT EXISTS suite_metadata_target_bindings("
        "metadata_target_id TEXT NOT NULL,target_type TEXT NOT NULL,backend_id TEXT NOT NULL,resource_key TEXT NOT NULL,native_id TEXT NOT NULL DEFAULT '',"
        "channel_id TEXT NOT NULL DEFAULT '',start_time INTEGER NOT NULL DEFAULT 0,end_time INTEGER NOT NULL DEFAULT 0,"
        "lifecycle_state TEXT NOT NULL DEFAULT 'active',created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(metadata_target_id),UNIQUE(target_type,backend_id,resource_key),"
        "CHECK(target_type IN('recording','program-event')),CHECK(lifecycle_state IN('active','stale','retired')),"
        "FOREIGN KEY(metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE CASCADE);"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_target_bindings_lookup "
        "ON suite_metadata_target_bindings(backend_id,target_type,lifecycle_state,resource_key);"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_target_bindings_time "
        "ON suite_metadata_target_bindings(backend_id,target_type,start_time,end_time);"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_target_bindings_epg_native "
        "ON suite_metadata_target_bindings(backend_id,target_type,channel_id,native_id,lifecycle_state);"
        "CREATE TABLE IF NOT EXISTS suite_metadata_genres("
        "genre_id TEXT PRIMARY KEY,label_de TEXT NOT NULL,label_en TEXT NOT NULL,known INTEGER NOT NULL DEFAULT 0,"
        "sort_priority INTEGER NOT NULL DEFAULT 1000,created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CHECK(known IN(0,1)));"
        "CREATE TABLE IF NOT EXISTS suite_metadata_genre_assignments("
        "metadata_target_id TEXT NOT NULL,backend_id TEXT NOT NULL,target_type TEXT NOT NULL,genre_id TEXT NOT NULL,provider_id TEXT NOT NULL,"
        "original_value TEXT NOT NULL DEFAULT '',normalization_rule TEXT NOT NULL DEFAULT 'canonical-alias-v1',source_kind TEXT NOT NULL DEFAULT '',"
        "assignment_state TEXT NOT NULL DEFAULT 'active',confidence REAL NOT NULL DEFAULT 0.0,ordinal INTEGER NOT NULL DEFAULT 0,observed_at INTEGER NOT NULL DEFAULT 0,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(metadata_target_id,provider_id,genre_id,original_value),"
        "CHECK(assignment_state IN('active','missing','unknown','stale','conflict')),CHECK(confidence>=0.0 AND confidence<=1.0),"
        "FOREIGN KEY(metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE CASCADE,"
        "FOREIGN KEY(genre_id) REFERENCES suite_metadata_genres(genre_id) ON DELETE RESTRICT,"
        "FOREIGN KEY(provider_id) REFERENCES suite_metadata_providers(provider_id) ON DELETE RESTRICT);"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_genre_assignments_query "
        "ON suite_metadata_genre_assignments(backend_id,target_type,genre_id,assignment_state,metadata_target_id);"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_genre_assignments_target "
        "ON suite_metadata_genre_assignments(metadata_target_id,assignment_state,ordinal);"
        "CREATE TABLE IF NOT EXISTS vdr_channel_cache("
        "backend_id TEXT NOT NULL,channel_id TEXT NOT NULL,channel_number INTEGER NOT NULL DEFAULT 0,"
        "name TEXT NOT NULL DEFAULT '',provider TEXT NOT NULL DEFAULT '',group_name TEXT NOT NULL DEFAULT '',"
        "radio INTEGER NOT NULL DEFAULT 0,encrypted INTEGER NOT NULL DEFAULT 0,enabled INTEGER NOT NULL DEFAULT 1,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,PRIMARY KEY(backend_id,channel_id),"
        "CHECK(radio IN(0,1)),CHECK(encrypted IN(0,1)),CHECK(enabled IN(0,1)));"
        "CREATE INDEX IF NOT EXISTS idx_vdr_channel_cache_backend_number "
        "ON vdr_channel_cache(backend_id,channel_number,channel_id);"
    );
}
}
