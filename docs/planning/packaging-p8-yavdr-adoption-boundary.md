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
| SuiteBridge VDR plugin | Exact installed .so and ABI not yet established from this host report | REVIEW: verify installed path and target VDR ABI independently; no VDR restart |
| Network/runtime | Daemon active; prior `ss` showed TCP 18080 listening on 0.0.0.0 | REVIEW: binding and proxy path need security/runtime acceptance; no listener edits |

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

No productive files, service states, permissions, nginx includes or databases
were changed during the reported read-only host inspections.
