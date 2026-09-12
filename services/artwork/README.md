# Home artwork previews

Home recording cards opt into `variant=home` on the existing recording-artwork
and recording metadata image routes. The existing HTTP boundary first performs
authentication, identity selection and allowed-root checks, then supplies the
validated original bytes to its preview cache. No client path is passed to a
decoder. Requests without the variant, including recording details, retain the
original representation. Other variant values return 400 with no-store.

The fixed variant fits within 400 x 600 pixels, preserves aspect ratio to integer
pixel rounding and never enlarges an image. JPEG, PNG and static WebP are supported;
small images are passed through unchanged. Output is JPEG at FFmpeg quality 4;
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
the original content length and ETag.

Resource limits: 16 MiB input, 40 million source pixels, 16384 pixels per source
dimension, 512 KiB output, 768 MiB decoder address space, two CPU seconds and three
wall seconds. Decode is single-threaded, uses a forced image demuxer and a local
file/pipe protocol allowlist. Existing valid cache hits are served before converter locking. On a cache miss,
there is at most one active conversion per cache directory; contention still
returns the original without queuing. Failed conversions
have a bounded 256-entry, 30-second retry cooldown in each daemon instance.

The persistent cache retains at most 256 previews and 32 MiB, evicting oldest
published files before atomic publication. One input (up to 16 MiB) and one output
(up to 512 KiB) may exist transiently in addition to the retained cache. Directory
scans are bounded; an unexpectedly overfilled directory disables new publication.
Directory and file descriptors reject symlinks; an advisory directory lock also
serializes independent daemon instances. Cache hits survive daemon restarts.

Focused validation: `make test-artwork-preview-cache
test-recording-artwork-http-server test-phase65-media-process-runner
test-phase66-continue-watching-frontend`. These tests use isolated temporary
fixtures; they do not require or contact a running VDR.
