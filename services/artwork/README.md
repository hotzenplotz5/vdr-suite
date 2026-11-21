# Home artwork previews

Home recording cards and Now/Next programme rail cards opt into `variant=home`
on the existing recording-artwork, recording metadata image and legacy EPG cache
artwork/metadata image routes. The large Live Hero, EPG detail and Public-v1
artwork requests remain unchanged. The existing HTTP boundary first performs
authentication, identity selection and allowed-root checks, then supplies the
validated original bytes to its preview cache. No client path is passed to a
decoder. Requests without the variant, including recording details, retain the
original representation. Other variant values return 400 with no-store.

The fixed variant fits within 400 x 600 pixels, preserves aspect ratio to integer
pixel rounding and never enlarges an image. JPEG, PNG and static WebP are supported;
small images already within 256 KiB are passed through unchanged. Larger small
images are recompressed. Output is JPEG at FFmpeg quality 6;
transparency is flattened. Unsupported, oversized, malformed or unhelpful output
(not smaller than the source) falls back to the original. Rotated images whose
decoded dimensions do not match the inspected dimensions also fall back.

Build dependency: OpenSSL development headers and libcrypto (`libssl-dev` on
Debian/Ubuntu). Optional runtime dependency: `/usr/bin/ffmpeg` with JPEG, PNG and
WebP decoders and the MJPEG encoder. Tests require FFmpeg and ffprobe. A missing
decoder does not prevent daemon startup or original artwork delivery.

The daemon creates `/var/cache/vdr-suite/artwork-previews` with mode 0700 beneath
the existing Suite cache root. If the parent is unavailable or the directory is
not private and owned by the daemon user, it delivers originals. The cache is
disposable and does not write to recording, manual-poster or TVScraper storage.
SHA-256 covers the variant version, full artwork request identity (including
assignment revision), MIME type and actual source bytes. Every cache lookup
therefore still validates and reads the source. Source replacement invalidates
previews even with unchanged size or timestamps. Existing browser cache lifetimes
and assignment revision semantics are retained; transformed responses discard
the original content length, ETag and Last-Modified.

Resource limits: 16 MiB input, 40 million source pixels, 16384 pixels per source
dimension, 256 KiB output, 768 MiB decoder address space, two CPU seconds and three
wall seconds. Decode is single-threaded, uses a forced image demuxer and a local
file/pipe protocol allowlist. Existing valid cache hits are served before converter locking. On a cache miss,
there is at most one active conversion per cache directory; contention still
returns the original without queuing. Failed conversions
have a bounded 1024-entry, 30-second retry cooldown in each daemon instance.

The persistent cache retains at most 1024 previews and 64 MiB, evicting oldest
published files before atomic publication. One input (up to 16 MiB) and one output
(up to 256 KiB) may exist transiently in addition to the retained cache. Directory
scans are bounded; an unexpectedly overfilled directory disables new publication.
Directory and file descriptors reject symlinks; an advisory directory lock also
serializes independent daemon instances. Cache hits survive daemon restarts.

Focused validation: `make test-artwork-preview-cache
test-recording-artwork-http-server test-phase65-media-process-runner
test-phase66-continue-watching-frontend`. These tests use isolated temporary
fixtures; they do not require or contact a running VDR.

## PWA.15.6 — bounded Home artwork response preparation

The native HTTP listener still parses requests, calls the existing server,
and runs every router, authentication, grant, database and source lookup on
its original serial thread. For a successful `variant=home` image response
only, it defers the preview and final scoped-byte revalidation until an
existing image-writer worker processes the already-authorized original bytes.
No additional request handler, decoder process, or worker pool is introduced.
The existing four image writers and finite pending queue bound this phase;
`ArtworkPreviewCache` still limits conversion to one FFmpeg process per
cache directory, preserves hits/prewarming and falls back to originals on
conversion failures. Non-image and unsuccessful responses, all other routes,
and direct `RecordingArtworkHttpServer::handleRequest()` callers keep their
synchronous behavior.

Deferred work receives only the source response and the original request
path/conditional ETag; it does not retain credentials or rerun authorization.
`Server-Timing` now separates `artwork-read` (serial authorized preparation),
`artwork-queue` (time awaiting an image worker), and `artwork-preview`
(cache lookup/conversion). The listener's existing
`HTTP_LISTENER_STALL phase=http-handler` journal entry still reports
unexpectedly slow serial source retrieval. An exception during deferred
processing retains the original representation with `no-store`, without
publishing an unvalidated cache partition.

Focused tests include `make test-http-listener-partial-request-timeout
test-recording-artwork-http-server test-artwork-preview-cache`. The listener
regression sends six slow image requests, checks that an unrelated health
request is served before those conversions finish, verifies bounded workers,
denial responses and original-byte fallback after a simulated conversion
failure. The artwork integration test checks deferred previews, scoped ETag
and 304 revalidation, and denial before deferral. These are deterministic
test fixtures, **not** a real yaVDR cold-start performance measurement.
Use new operator browser captures to compare first-byte timings, listener
stall diagnostics, total artwork time and Service Worker cache-miss causes.
Session isolation and SW authorization remain unchanged.

## PWA.15.13 — mehr persistente Home-Previews (2026-11-21)

Ein yaVDR-Lauf zeigte 256 Cache-Dateien bei nur 8,28 MiB auf Festplatte;
alle gespeicherten Previews lagen unter 256 KiB (groesstes 69.956 Byte).
Die Dateianzahl, nicht der Bytebedarf, erreichte damit die alte Grenze.
Der persistente serverseitige Preview-Cache nimmt jetzt hoechstens 1024 Dateien
und 64 MiB auf. Bestehende Dateien bleiben erhalten und sind weiterhin nach
Originalinhalt adressiert. Die Web-PWA-Grenze von 1024 Eintraegen und 256 KiB
pro Recording-Home-Cover bleibt unveraendert. Die bisherige
Nur-eine-Umwandlung-Regel und Original-Fallbacks bleiben ebenfalls erhalten.
Die Kapazitaetserweiterung allein beweist nicht, dass alle HTTP-Bilder
aus der Preview-Variante kommen; fuer Restfaelle sind die vorhandenen
Browser-Header- und Groessenmessungen massgeblich.
