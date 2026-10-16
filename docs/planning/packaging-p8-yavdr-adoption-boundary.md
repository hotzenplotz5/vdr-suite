# P8 — observed existing-yaVDR adoption boundary (no install approval)

Status: **read-only host evidence; migration policy incomplete; deployment BLOCKED**.
This ledger records the operator-supplied yaVDR inspection on 2026-10-16.
It does not disclose configuration contents, tokens, or SQLite rows. It
does not certify the exact installed VDR-Suite binary version, source commit,
recorded database schema, VDR plugin ABI, or service recovery procedure.

## Verified observations and preservation constraints

| Observed existing path or behavior | Ownership / operational evidence | Mandatory boundary |
| --- | --- | --- |
| `/etc/nginx/snippets/vdr-suite.conf` | Existing file; no dpkg file owner reported; collides with daemon package target | **BLOCKED:** preserve contents and operator ownership; never install over it |
| `/etc/nginx/conf.d/vidaa-https.conf` | Operator HTTPS configuration references VDR-Suite | REVIEW: preserve HTTPS routing; no automatic activation, include editing or nginx reload |
| `/etc/nginx/snippets/vdr-suite-browser-sessions.conf` | Existing operator snippet referenced by nginx discovery | REVIEW: preserve and inspect routing dependencies before migration; no copying into package |
| `/etc/default/vdr-suite-daemon` | Existing root-owned mode 0600 file, no dpkg owner reported | **BLOCKED:** never replace, print or relax access; compare non-secret settings only with approval |
| `/etc/vdr-suite/backend-agent.conf` | Existing root-owned configuration, no dpkg owner reported | **BLOCKED:** preserve and handle as potentially sensitive configuration |
| `/usr/sbin/vdr-suite-daemon` | Existing unowned binary; observed systemd `ExecStart` resolves to this path, running as root at inspection | **BLOCKED:** do not unpack another daemon there while live; determine exact source provenance first |
| `/usr/local/sbin/vdr-suite-daemon` | Older, separate manual binary; not the observed service executable | REVIEW: do not delete or implicitly select via PATH |
| `/usr/lib/systemd/system/vdr-suite-daemon.service` | Existing systemd fragment; `/lib/systemd/system/...` also visible | REVIEW: map realpath and ownership; do not replace or restart service |
| `/etc/systemd/system/vdr-suite-daemon.service.d/recording-artwork.conf` | Active systemd drop-in (`DropInPaths`) | **First package install blocked:** preserve semantics and directory; no automatic disable or migration |
| `/var/lib/vdr-suite` | Existing application state; scanner reports REVIEW | **First package install blocked:** no backup, schema migration, purge or rollback without separate approval and writer-quiesce protocol |
| Other daemon, agent, doc and manpage targets | P6 observed multiple existing files without a dpkg owner | **BLOCKED:** reconcile each canonical `debian/*.install` destination before any unpack |
| VDR 2.8.3 package provenance | Ubuntu `26.04` `resolute` amd64; `vdr` and `vdr-dev` report `2.8.3-0yavdr0~resolute`; `apt-cache policy` identifies `ppa:seahawk1986-hotmail/vdr-2.8.3` at `https://ppa.launchpadcontent.net/seahawk1986-hotmail/vdr-2.8.3/ubuntu`, `resolute/main`, priority 500 | **Exact-source P4 build PASS:** CI #118 produced API-14 plugin `.deb` with the correct generated ABI dependency; full Resolute daemon/agent and functional runtime tests still open |
| SuiteBridge VDR plugin | Five files include `.so.11`, `.so.14` and `.so.11.*` historical variants; `pkg-config vdr` is `2.8.3`, API `14`, libdir `/usr/lib/vdr/plugins`; running VDR MainPID `1757` maps the `.so.14` file; device/inode equality with disk confirmed (`MAPPED_FILE_IDENTITY=PASS`) | **Loaded-module/disk-file identity verified for inspected process; control socket listener separately confirmed; protocol not exercised.** Both `.so.14` and `.so.11` lack reported dpkg ownership. Preserve all versions; **package collision remains BLOCKED** |
| SQLite database and WAL | `vdr-suite.db`: 3,357,405,184 bytes; `-wal`: 198,295,632 bytes; `-shm`: 393,216 bytes | **P7 BLOCKED:** live WAL sidecars present at inspection; no direct copy of `.db`, checkpoint, `immutable=1` schema check, migration, or restore on live state |
| Daemon binary fingerprints | `/usr/sbin/vdr-suite-daemon`: 17,939,328 bytes, SHA-256 `0bfa78635215bd46363ec86bbe356d4426aef9fc3a6430f329ae69658f0ffc57`; `/usr/local/sbin/vdr-suite-daemon`: 6,503,208 bytes, SHA-256 `a5e08254ed8e4956fe31651981070098a7c5cc57de59e58a1da1d2f4bef227f2` | Fingerprints identify observed file bytes only, not a known source SHA or supported predecessor |
| Network/runtime | Daemon active; prior `ss` showed TCP 18080 listening on 0.0.0.0. SuiteBridge private `/run/vdr/vdr-suite-control/control.sock` is a `SOCK_SEQPACKET` Unix listener owned by VDR PID `1757`, fd `28`; directory `0700 vdr:vdr`, socket `0660 vdr:vdr`. Passive journal: `CONTROL_PLANE_START_ACCEPTED=1`, `CONTROL_PLANE_START_REJECTED=0`, `PLUGIN_LIFECYCLE_START_ACCEPTED=1` | **Control-plane/lifecycle initialization PASS at observed boot; listener and ownership confirmed.** No peer-authentication handshake or functional command executed. Preserve socket, service and nginx configurations; no listener edits |

These are observations at inspection time, not immutable facts. Do not
infer that a process ID, binary hash or active route is unchanged later.

## Separation of mandatory gates

1. **P6 inventory:** the read-only scanner returned `verdict=BLOCKED`,
   `install_approved=false`. Existing unowned package destinations
   prohibit normal takeover regardless of successful CI.
2. **P8 entry point:** `tools/packaging_p8_installer.py --install`
   always refuses. There is no approved user-facing `--install`
   implementation or alternate authorized installer.
3. **Native package guard:** Debian `preinst` scripts generated from
   canonical manifests reject enumerated conflicts before `dpkg` unpack.
   They are defense-in-depth, **not** a source/DB/nginx compatibility policy.
   A direct `apt`/`dpkg` command is not approved just because it might fail
   safely on one known collision.
4. **P7 state:** the old daemon-created SQLite version, consistent
   WAL-inclusive quiesced snapshot, compatibility policy and rollback
   validation are still missing. Synthetic CI fixtures cannot stand in
   for an actual supported predecessor.

## Required operator-reviewed takeover plan, not yet authorized

- Establish the source SHA/version and ownership for **every** present
  package target and systemd unit, including symlink-equivalent locations.
  Produce a non-secret inventory of nginx includes, systemd environment and
  override precedence; do not print secrets or use automatic config merging.
- Treat the existing nginx snippets and HTTPS site as operator-managed
  configuration. A future package must not claim or rewrite their unowned
  paths without a separately reviewed, reversible transition. Preserve the
  custom browser-session location behavior.
- Design an explicit daemon service transition for the observed root-run
  binary and its recording-artwork drop-in, with a maintenance window and
  restart/recovery acceptance. Existing `/usr/local` binaries must not be
  silently removed.
- Ubuntu 26.04 / amd64, VDR 2.8.3, API 14 and the provided VDR ABI are
  observed on this host. [CI #118](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38936881893)
  passed all 15 jobs at code SHA
  `c957f0c0a6c6b21e152315da31152f572c1e59b8`, including an isolated
  SuiteBridge `.deb` built for that exact Resolute VDR package, but it
  was **not installed** on this host. The subsequent read-only host
  examination confirmed the running VDR process mapped `.so.14`,
  that its mapped device/inode matched the on-disk library at inspection
  time, and that the installed `vdr`/`vdr-dev` packages came from
  Seahawk's `resolute/main` PPA. The next boundary is functional
  SuiteBridge health/handshake and source-version provenance. A matching
  device/inode and a disk hash do **not** prove this binary is identical
  to the CI artifact or that its protocol works correctly.
- Name a supported older manual-install source and obtain a safely quiesced
  offline SQLite/WAL sample in a **private** workspace. Validate old-to-new
  schema, all writers, backup/restore and failure recovery there first.
- Verify the target VDR ABI and SuiteBridge plugin/agent lifecycle independently,
  including service activation policy. No new startup, enable, or nginx routing
  may be performed automatically.
- Only after all blockers are reconciled and an **explicit host-install approval**
  is issued may an install path be implemented and acceptance tested. There is
  currently no such approval.

## Evidence limits

The direct `dpkg --unpack` nginx sentinel refusal and existing package
lifecycle experiments passed in
[CI #109](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38926400765)
on code SHA `643166983f961bcf450d9c785cf1e628dc31d684`.
That verification is isolated and does not assert that the live host has
changed. The expanded P8 six-case package-native collision matrix and
retained-conffile safeguards passed in
[CI #111](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38929697400), all 11 jobs SUCCESS at code SHA
`288dedef378570625e6b30e2a346146b2392abf8` (128 packaging regression tests). This confirms only disposable
package behavior, not real yaVDR transition safety.
The observed old SuiteBridge filename variants were also refused before
unpack in the isolated package matrix: [CI #112](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38932136305),
**14/14 jobs SUCCESS**, **130 packaging regressions**, code SHA
`178b00e7252747de22b1c7d90113a3aac9dee15c`.
This proves collision refusal, not the ABI of the plugin loaded by VDR.

The 2026-10-16 operator audit of the active VDR (`MainPID=1757`) read
`/proc/1757/maps` and found the mapped file
`/usr/lib/vdr/plugins/libvdr-suitebridge.so.14`. This establishes the
loaded library path only at that instant. An existing, unowned `.so.11`
and the unowned `.so.14` still trigger the package-native pre-unpack
collision guard. A later read-only `/proc/1757/maps` device/inode check returned
`MAPPED_FILE_IDENTITY=PASS` against the actual on-disk `.so.14`
(`stat` inode `1704446`, size `13523792` bytes, disk-file SHA-256
`8158311f874f0e1b10620192eb3e35f6f62d10e7723571d33635e7ecc1858580`).
This establishes file identity at the inspection time only, not that
the in-memory bytes have been independently hashed, nor any source
commit, package artifact or functional protocol compatibility.

A further read-only inspection observed the native control-plane directory
`/run/vdr/vdr-suite-control` as `0700 vdr:vdr` and its `control.sock`
as `0660 vdr:vdr`, a regular Unix socket (not a symlink).
`ss -xlnpH` reported a listening `u_seq`/SOCK_SEQPACKET endpoint
at that path owned by `vdr` PID `1757`, fd `28`, kernel socket
identifier `21642`. A separate read-only `journalctl` query scoped to
the current VDR PID `1757` found exactly one
`suitebridge: control-plane event=start result=accepted` and one
`suitebridge: lifecycle event=start result=accepted`, with no control-plane
start rejection (`START_LOG_EVIDENCE=PASS`). Taken together these
observations attest successful startup and socket-listener initialization,
not SO_PEERCRED admission, protocol replies, agent health, authenticated
authorization, or continued operation after a restart.

No productive files, service states, permissions, nginx includes or databases
were changed during the reported read-only host inspections.
