# Recording Folder and Manual Genre Management

Status: **Working branch candidate; not merged and not accepted yet.**

Branch: `work/recordings-folder-genre-management`

Base: `main@9e5e65f55b91f0e90fc832c4cfa99d2f237bdb8c`

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

## Focused verification

Repository tests cover:

- new-folder target composition and invalid hierarchy characters;
- Recordings 2 Genre editor request wiring;
- security recognition of the `genre` mutation;
- REST set/get/invalid/clear lifecycle;
- manual Genre survival across Recording-cache synchronization.

Real-system acceptance must still verify on yaVDR that:

1. moving a Recording into a newly named folder creates and displays that
   folder after readback;
2. adding a manual Genre is visible after detail refresh and daemon/cache
   refresh;
3. removing the manual Genre leaves automatically detected Genres intact.

No merge or PR is implied by this working document.
