-- VDR-Suite Sample Data
-- Schema Version 1

INSERT INTO recordings (
    title,
    subtitle,
    description,
    channel,
    recording_path,
    recording_format
)
VALUES (
    'Tatort',
    'Die letzte Reise',
    'Kriminalfilm',
    'Das Erste',
    '/srv/vdr/video/Tatort.rec',
    'TS'
);

INSERT INTO recordings (
    title,
    subtitle,
    description,
    channel,
    recording_path,
    recording_format
)
VALUES (
    'Tagesschau',
    NULL,
    'Nachrichten',
    'Das Erste',
    '/srv/vdr/video/Tagesschau.rec',
    'TS'
);

INSERT INTO metadata (
    recording_id,
    media_type,
    title,
    original_title,
    year,
    genre,
    source
)
VALUES (
    1,
    'MOVIE',
    'Tatort',
    'Tatort',
    2026,
    'Krimi',
    'sample'
);

INSERT INTO jobs (
    recording_id,
    job_type,
    status,
    priority
)
VALUES (
    1,
    'SHRINK',
    'PENDING',
    10
);

INSERT INTO jobs (
    recording_id,
    job_type,
    status,
    priority
)
VALUES (
    1,
    'CHECK',
    'DONE',
    5
);
