-- Synthetic pre-cast manual recording assignment. NOT source-attested.
-- Run on disposable private P7 SQLite after current metadata platform SQL.
INSERT INTO suite_metadata_entities(metadata_entity_id,media_type)
 VALUES('mdent_11111111111111111111111111111111','episode');
INSERT INTO suite_metadata_targets(metadata_target_id,target_type)
 VALUES('mdtgt_11111111111111111111111111111111','recording');
INSERT INTO suite_metadata_providers(provider_id,provider_kind,display_name)
 VALUES('p7fixture','external-catalog','P7 synthetic provider');
INSERT INTO suite_metadata_assignments(
    metadata_assignment_id,metadata_target_id,metadata_entity_id,
    manual_assignment)
 VALUES('mdasg_11111111111111111111111111111111',
        'mdtgt_11111111111111111111111111111111',
        'mdent_11111111111111111111111111111111',1);
CREATE TABLE suite_metadata_manual_assignment_values (
    metadata_assignment_id TEXT PRIMARY KEY,
    metadata_target_id TEXT NOT NULL,
    backend_id TEXT NOT NULL,
    resource_key TEXT NOT NULL,
    provider_id TEXT NOT NULL,
    external_namespace TEXT NOT NULL,
    external_id TEXT NOT NULL,
    media_type TEXT NOT NULL,
    title TEXT NOT NULL,
    original_title TEXT NOT NULL DEFAULT '',
    overview TEXT NOT NULL DEFAULT '',
    release_date TEXT NOT NULL DEFAULT '',
    poster_reference TEXT NOT NULL DEFAULT '',
    season_number INTEGER NOT NULL DEFAULT 0,
    episode_number INTEGER NOT NULL DEFAULT 0,
    actor_ref TEXT NOT NULL,
    revision INTEGER NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK(media_type IN('movie','series','episode')),
    CHECK(season_number>=0),
    CHECK(episode_number>=0),
    CHECK(revision>0),
    FOREIGN KEY(metadata_assignment_id)
      REFERENCES suite_metadata_assignments(metadata_assignment_id)
      ON DELETE CASCADE,
    FOREIGN KEY(metadata_target_id)
      REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE CASCADE,
    FOREIGN KEY(provider_id)
      REFERENCES suite_metadata_providers(provider_id) ON DELETE RESTRICT
);
INSERT INTO suite_metadata_manual_assignment_values(
    metadata_assignment_id,metadata_target_id,backend_id,resource_key,
    provider_id,external_namespace,external_id,media_type,title,
    season_number,episode_number,actor_ref,revision)
 VALUES('mdasg_11111111111111111111111111111111',
        'mdtgt_11111111111111111111111111111111',
        'p7_backend','p7_old_recording','p7fixture',
        'thetvdb','old-fixture','episode','P7 old manual metadata',
        2,4,'p7_pre_actor',1);
