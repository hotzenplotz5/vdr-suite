# Recording Folder and Manual Genre Management

Status: **Runtime accepted on yaVDR; merge to `main` authorized.**

Branch: `work/recordings-folder-genre-management`

Base updated by merge: `main@881a0f08015c35b37a4e71c78e32bf86dcc105b3`

## Purpose

This non-numbered Recording product slice adds two user-facing capabilities to
Recordings 2 without changing the numbered Phase-70 or active Multiuser
roadmaps:

- create a VDR recording folder as part of moving a Recording;
- add or remove one explicit manual canonical Genre for a Recording.

## VDR source boundary

The implementation follows the current VDR recording model rather than
inventing a parallel empty-folder store.

VDR represents Recording folder hierarchy in the logical Recording name and
maps hierarchy separators to the filesystem Recording tree. Its folder
selection/editing model supports selecting or entering a target folder and
moving an existing Recording into that target. A folder therefore becomes a
meaningful Recording folder when it contains a Recording; this slice does not
persist empty synthetic folders outside VDR.

VDR also persists DVB/Event content descriptors in Recording information, but
its native Recording edit workflow does not provide a free-form manual Genre
editor. VDR-Suite therefore does **not** rewrite `info.vdr` for a user Genre.

## Folder behavior

Recordings 2 keeps the existing protected `MOVE` action owner.

The Move editor now exposes an explicit **Neuen Ordner als Ziel** control. It
builds a normalized target path under the selected/current folder, validates
the existing MOVE contract and lets the native Recording move materialize the
folder.

A single new folder name may not contain `/`, `\` or VDR's hierarchy
separator `~`. Nested paths remain possible by first choosing/entering the
parent and then creating the final folder segment.

There is deliberately no create-empty-folder mutation.

## Manual Genre behavior

The Genre editor is part of the selected Recording metadata surface. It uses
the existing canonical Genre registry and the existing Suite-owned Genre index.

Private browser route:

```text
GET  /api/backends/{backendId}/recordings/metadata/genre?resourceKey=...
POST /api/backends/{backendId}/recordings/metadata/genre
```

POST body:

```json
{
  "resourceKey": "<recording backend-native id or cache key>",
  "genreId": "science-fiction"
}
```

An empty `genreId` removes only the manual Genre.

The repository resolves the supplied Recording identity to the canonical
`vdr_recording_cache.cache_key` and writes Suite-owned Genre evidence with:

```text
provider_id = manual-recording-genre
source_kind = recording-manual-genre
target_type = recording
confidence  = 1.0
```

Manual Genre evidence is additive. Automatic VDR/TVScraper/folder evidence is
not deleted or overwritten. Recording-cache synchronization does not delete
the manual source, and conflict reconciliation does not mark it conflicting
merely because automatic evidence differs.

## Security

The POST mutation stays behind the existing browser mutation boundary:

- permission: `metadata.recording.assign`;
- backend scope is taken from the route;
- browser CSRF validation remains mandatory;
- normal accountability logging remains owned by `SecurityHttpGate`.

No public-v1 contract is added by this slice.

## Verification and real-system acceptance

Repository tests cover:

- new-folder target composition and invalid hierarchy characters;
- Recordings 2 Genre editor request wiring;
- security recognition of the `genre` mutation;
- REST set/get/invalid/clear lifecycle;
- manual Genre survival across Recording-cache synchronization;
- Recording action request parsing for titles containing commas/escaped quotes;
- JSON-safe serialization of multiline upstream Recording action responses.

Real yaVDR acceptance on 2026-10-02 passed for the complete slice:

- moving `Oskar/Disney, eine Weihnachtsgeschichte` into the newly entered
  `Oskar/Klassiker` folder created the VDR folder and moved the Recording;
- after the move, native `LSTT`, RESTfulAPI `/timers.json` and the
  VDR-Suite Recording-folder endpoint all remained responsive;
- the manual Genre could be added, survived detail reload and a
  `vdr-suite-daemon` restart, and could be removed again without deleting
  automatic Genre evidence.

During acceptance, the first real move exposed a deadlock in the current
RESTfulAPI Recording move executor: it held `TIMERS_WRITE` while waiting
indefinitely for `RECORDINGS_WRITE`. GDB evidence identified
`RecordingMoveExecutor::executeNormalCase()` waiting in
`cRecordings::GetRecordingsWrite()` while concurrent timer readers blocked in
`cTimers::GetTimersRead()`.

The accepted runtime used the separate RESTfulAPI fix
`hotzenplotz5/vdr-plugin-restfulapi:fix/recording-move-lock-timeout` at
`eb5b40fc1444b1d2ed12d075d5ee7542d729616c`. That fix preserves VDR's required
Timers-before-Recordings lock order, bounds both lock acquisitions and releases
the Timer lock if the Recording lock cannot be acquired. Until that fix is
available in the deployed RESTfulAPI version, the native Recording move path
must not be treated as deadlock-safe.

The VDR-Suite product/runtime evidence above remains accepted for documentation-
only closeout changes; no additional build or runtime retest is required unless
a directly relevant product input changes.
