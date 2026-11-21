# EPG Series Artwork Incoming Cleanup

## Status

This document defines the bounded cleanup boundary for stale TMDB series-artwork
files in the provider incoming directory.

The cleanup is optional, startup-only and disabled by default. It does not alter
the provider selection, fallback resolution, materialization, persistence or
public delivery contracts.

## Problem

The TMDB provider writes downloaded artwork atomically into the first configured
series-artwork source root. A successful provider result is materialized into the
separate managed cache immediately, but the original incoming candidate remains.
A daemon crash may also leave an old atomic temporary file behind.

Without a bounded cleanup contract these provider-owned files can accumulate.
Incoming files are not managed-cache references and therefore cannot use the
persisted-reference orphan-cleanup policy.

## Exact provider-owned namespace

The current TMDB provider owns exactly two flat filename forms below its incoming
root:

```text
tmdb-<positive-series-id>-<16-lowercase-hex>.candidate
.tmdb-<positive-series-id>-<16-lowercase-hex>.candidate.<positive-pid>.<sequence>.tmp
```

The cleanup does not recurse and does not infer ownership from file extensions
alone. Every other filename and every child directory is foreign and remains
untouched.

The namespace is checked against the provider source by the repository static
architecture contract so a future provider naming change cannot silently widen
or break cleanup eligibility.

## Startup lifecycle

For the configured SuiteBridge backend the daemon performs the following order:

1. Parse and normalize the series-artwork source roots.
2. Derive the TMDB incoming root from the first source root.
3. Run the optional incoming cleanup once.
4. Construct the provider cache, HTTP transport and TMDB provider when the
   existing provider activation requirements are satisfied.
5. Construct the materializer and persistence resolver.
6. Start the SuiteBridge Agent only after backend-context construction finishes.

This ordering means the in-process provider cannot be writing or consuming an
incoming file during cleanup. Cleanup does not require a TMDB token and may be
used to remove old TMDB-owned files after the provider has been disabled or
changed.

## Configuration

```text
VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_ENABLED=false
VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MIN_AGE_SECONDS=86400
VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MAX_FILES=64
```

The cleanup switch is independent from the managed-cache orphan-cleanup switch.
Packaged defaults perform no deletion.

Runtime bounds are:

- minimum age: one hour through 365 days;
- maximum successful deletions per startup: 1 through 1024.

Invalid values fall back to the conservative packaged defaults.

## Root handling

The configured root must be an absolute, normalized path other than `/`.

The cleaner opens `/` and every root component descriptor-relatively with:

```text
O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW
```

A missing root is a successful no-op. A malformed path, symlinked component,
permission failure or other root-opening error fails the cleanup without
following the path.

The root is never created by the cleaner.

## Candidate qualification

Entries are read from the single opened root directory and sorted by filename.
Sorting gives deterministic bounded progress over repeated daemon startups.

For a reserved filename the cleaner:

1. opens the entry with `openat(..., O_NOFOLLOW | O_NONBLOCK)`;
2. requires the opened object to be a regular file;
3. evaluates age from the opened descriptor's modification timestamp;
4. retains files younger than the configured minimum age;
5. resolves the directory entry again using
   `fstatat(..., AT_SYMLINK_NOFOLLOW)`;
6. requires device, inode, mode, size and modification timestamp to match the
   already opened file;
7. deletes only that exact directory entry with `unlinkat`.

File symlinks, directories, FIFOs, sockets, devices and entries whose identity
or metadata changed remain untouched.

The deletion counter advances only after a successful `unlinkat`. Once the
configured maximum is reached, the cleaner stops scanning and reports that the
limit was reached.

## Failure behavior

Cleanup errors are logged through bounded counters and do not disable the
existing provider, resolver, materializer, persistence or public delivery
runtime.

Startup logs contain only:

- backend ID;
- whether cleanup and the root were available;
- examined and recognized counts;
- young, removed, foreign, unsafe and error counts;
- whether the batch limit was reached.

No local path, provider URL, bearer token, downloaded content or public metadata
is logged or exposed.

## Compatibility

The cleanup adds no HTTP route, JSON field, frontend behavior, authentication
change, provider request, database schema or persistence migration.

With the cleanup switch disabled, runtime behavior is unchanged.

## Focused validation

Tests cover:

- disabled cleanup;
- missing-root no-op behavior;
- old final-candidate deletion;
- old atomic-temporary deletion;
- young-file retention;
- foreign and malformed filename retention;
- file-symlink and reserved-name directory retention;
- symlinked-root rejection;
- deterministic bounded deletion over repeated startup runs;
- invalid configuration rejection;
- runtime defaults, valid overrides and invalid-value fallback;
- source, test, packaging and startup-order architecture contracts.

## Deliberately deferred

This boundary does not add:

- periodic, timer-driven or operator-triggered cleanup;
- cleanup support for future providers or their namespaces;
- managed-cache orphan behavior beyond the separate existing cleaner;
- telemetry, metrics or operator dashboards;
- migration or adoption of old unguarded files;
- frontend or TV-client work.
