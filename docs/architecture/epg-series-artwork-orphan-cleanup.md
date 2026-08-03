# EPG series-artwork managed-cache orphan cleanup

## Scope

This slice is stacked on the guarded public-delivery boundary from Draft PR
#128. It adds only bounded cleanup for files already stored in the managed
series-artwork fallback cache.

It does not change provider selection, network access, materialization, public
JSON, image delivery, authentication, frontend behavior or TV-client behavior.

## Activation and schedule

Cleanup is independently opt-in:

```text
VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_ENABLED=false
```

When enabled together with the existing series-artwork fallback runtime, one
bounded cleanup pass runs during backend-context construction after the fallback
persistence schema is ready and before the SuiteBridge Agent or provider can
materialize new files.

There is no periodic worker, timer, HTTP endpoint or operator-triggered action in
this slice. A restart is the only scheduling boundary.

## Managed namespace

The cleaner accepts only the exact namespace created by
`FilesystemSeriesArtworkFallbackMaterializer`:

```text
<cache-root>/
  <hex-backend>/
    <hex-channel>/
      <hex-event>/
        series.png
        series.jpg
```

Directory names must be lowercase, even-length hexadecimal components within
the materializer's existing component limits. Only the two reserved filenames
are candidates. Provider incoming files, temporary materializer files, foreign
filenames and arbitrary directory trees are not cleanup inputs.

## Reference decision

The authoritative reference is an exact `path` match in
`epg_series_artwork_fallback`.

The repository lookup is tri-state:

- `Referenced`: retain the file;
- `Unreferenced`: it may proceed to the remaining deletion guards;
- `Error`: retain the file and report an error.

A database or schema failure is therefore never interpreted as absence.
Cleanup does not delete persistence rows and does not infer references from
provider state, public JSON or filename identities.

## Age and batch bounds

A candidate must be at least the configured age according to its file
modification time. Defaults are:

```text
minimum age: 604800 seconds (7 days)
maximum files removed per startup: 64
```

Runtime parsing constrains the minimum age to one hour through 365 days and the
batch to one through 1024 files. Reaching the batch limit stops further deletion
for that startup.

The age guard protects recently replaced artwork and files whose persistence
write may still be recovering from an interrupted process boundary. It is not a
migration rule and does not qualify older unguarded layouts.

## Filesystem safety

The configured cache root must be an absolute normalized non-root path already
constrained by `RuntimeConfig` to a strict descendant of
`/var/cache/vdr-suite/epg-artwork`.

Cleanup opens the root and every accepted directory component with
`O_NOFOLLOW`. Candidate files are opened descriptor-relative with
`openat(..., O_NOFOLLOW)`, must be regular files, and are rechecked immediately
before deletion with `fstatat(..., AT_SYMLINK_NOFOLLOW)`.

The opened and current device, inode, mode, size and modification time must
remain identical. Only then is descriptor-relative `unlinkat` used. File and
directory symlinks, non-regular entries, changed identities and unsafe
components are retained.

Empty materializer-owned event, channel and backend directories are removed
best-effort with `unlinkat(..., AT_REMOVEDIR)`. Non-empty or foreign directories
are left untouched.

## Failure behavior

Cleanup is fail-safe for file retention:

- disabled cleanup performs no scan;
- a missing cache root is a successful no-op;
- an invalid or symlinked root fails without following it;
- persistence lookup errors retain the candidate;
- filesystem identity or deletion errors retain the candidate when deletion has
  not already completed;
- cleanup failure does not disable the existing fallback resolver, provider or
  delivery runtime.

The startup log contains only bounded counters and the backend identifier. It
does not expose local paths, provider URLs, credentials or file contents.

## Compatibility

Packaged defaults keep both external fallback and orphan cleanup disabled.
Systems without fallback files or without the new settings behave as before.
There is no database migration beyond the existing guarded fallback schema and
no public API or client contract change.

## Deliberately deferred

The following remain separate review boundaries:

- cleanup of stale provider incoming files;
- periodic, timer-driven or operator-triggered cleanup;
- detailed telemetry, metrics and operator dashboards;
- additional artwork providers;
- automatic migration or adoption of old unguarded files;
- broader frontend and TV-client work.
