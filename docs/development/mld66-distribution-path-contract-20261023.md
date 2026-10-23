# MLD 6.6 distribution path contract — Ubuntu/yaVDR stays zero-config

**Implementation branch:** `work/mld66-platform-integration`; work-in-progress candidate following Pit's pinned `SRCREV 9a28cd77a74c85516fc9147cc84bdb1b66edfdf5`. These new code changes are **not** present in Pit's pinned MLD build until he explicitly updates the recipe's `SRCREV`.

## MLD maintainer runtime paths (2026-10-23)

Pit reports a VDR configuration root at `/etc/vdr/` and a
`channels.conf` symlink to the user's selected channel list. His
recording/video root is `/data/tv`, a **user-selected mount/share**
which may be backed by a different local partition/disk or a NAS.
**Never treat `/data/tv` as universal for every MLD installation.**
Recording-sidecar files such as `tvscraper.json`, `poster.jpg`,
`fanart.jpg`, `marks` and `info` were observed inside his
individual `.rec` directory and should be preserved. Exact TVScraper
plugin cache/config/resource roots remain unverified because Pit's
referenced screenshot/attachment was not available in the conversation.

See [Pit filesystem evidence](mld66-pit-filesystem-evidence-20261023.md).

**Native discovery candidate:** SuiteBridge now exposes a versioned read-only
`PLUG suitebridge VPATH 1` that obtains `cVideoDirectory::Name()` inside VDR.
The daemon probes only its matching local SuiteBridge backend when no explicit
recording-root map is provided. A valid absolute, non-root, traversal-free
response becomes the authoritative native mutation root with the existing
cross-root safety checks. A malformed success response or the new command's 451/504 rejection disables recording writes;
old/offline plugins retain legacy Ubuntu behavior. Explicit operator roots always
take precedence, including invalid explicit roots failing closed. The command
does not execute filesystem mutations. End-to-end MLD acceptance and a future
VDR-owned recording mutation implementation remain **not yet validated**.
Ubuntu/yaVDR must remain zero-config.

## Mandatory compatibility invariant

A clean Ubuntu/yaVDR installation and an existing upgraded Ubuntu/yaVDR
installation must continue to use the **same paths and behavior without any
new configuration edits**. No automatic migration or change to existing
operator `/etc` files is required.

- When the new variables below are **unset or empty**, the previous yaVDR
  fallback values and behavior remain in effect.
- Other distribution recipes may explicitly set overrides for **their own**
  layouts; they must not move VDR-managed `channels.conf`, `setup.conf`,
  `timers.conf`, caches or recordings into an Ubuntu layout.
- An **invalid explicit nonempty override** fails closed for the affected
  operation. No silent fallback to the yaVDR recording directory or to a
  permissive local socket/artwork root.
- In-tree Ubuntu defaults deliberately keep all three new variables
  **commented out**. Avoid changing live VDR service or installed yaVDR
  services without an independently approved runtime gate.

## Configuration matrix

| Setting | Scope | Absent (Ubuntu/yaVDR) | MLD/other distro opt-in |
| --- | --- | --- | --- |
| `VDR_SUITE_RECORDING_VIDEO_ROOTS` | Daemon process; backend ID to physical VDR video-root map | Existing `/srv/vdr/video` and legacy source-path behavior unchanged | `default=/ACTUAL/MLD/VDR-VIDEO`; semicolon-delimited for multiple backends. Invalid mapping or missing backend disables that backend's recording mutation adapter |
| `VDR_SUITE_CONTROL_SOCKET_PATH` | **VDR process hosting SuiteBridge**, daemon and local clients | `/run/vdr/vdr-suite-control/control.sock` | One identical, explicitly approved socket path **below `/run/`** in each participating process's environment. Malformed nonempty override rejects socket startup/connection |
| `SUITEBRIDGE_CONTROL_SOCKET_PATH` | Optional Backend Agent `backend-agent.conf` key | Agent uses its existing local transport default (or the shared VDR environment when provided) | An explicit `/run/...` pathname takes precedence for local Agent OSD/probe transports. Align with the VDR SuiteBridge socket |
| `VDR_SUITE_EPG_ARTWORK_ROOTS` | Daemon-process local artwork allowlist | Original roots `/var/cache/vdr/plugins/tvscraper` and `/var/cache/vdr-suite/epg-artwork` | One or more absolute trusted roots separated by semicolons; **replaces** default list; invalid entries deny all local EPG artwork access |
| `VDR_SUITE_RECORDING_ARTWORK_ROOTS` | Daemon; existing per-backend Recording artwork path mapping | Existing behavior unchanged | Already supported mapping; set explicitly where required |
| `VDR_SUITE_DATABASE_PATH` | Daemon; SQLite | Existing systemd package path `/var/lib/vdr-suite/vdr-suite.db` | Set persistent package state path; requires correct owner and permissions |
| `VDR_SUITE_FRONTEND_ROOT` | Daemon static frontend | Existing packaged `/usr/share/vdr-suite/web/frontend` | Set MLD package's installed frontend directory |
| `CHANNELS_CONF_PATH` | Backend Agent, **only when explicitly using** `channels-conf` adapter | Existing `/var/lib/vdr/channels.conf` example default | Set the real VDR `channels.conf`, preferably use VDR service API otherwise; no automatic path guessing |

### Native recording mutation safety

An explicit backend recording root is authoritative in the safe RESTfulAPI
mutation adapter. The adapter uses it to build safe-move targets and checks
`backendNativeId` is lexically inside the configured root **before HTTP
preview/validate/execute dispatch**. A missing per-backend mapping disables
the adapter. Existing Ubuntu/yaVDR behavior remains unchanged when the map is
unset. Legacy special treatment of `Recordings_on_yavdr(nfs)` applies only
to legacy, non-explicit recording path transformations.

**Not a recording-detection fix:** This contract does *not* fix Pit's
`GET /recordings.json` 5s timeout. That separate plugin provider defect
must still be investigated before native Suite recording acceptance.
Symlink/mount changes and real VDR recording-side behaviour require independent
end-to-end security and filesystem acceptance.

### Socket permissions and the MLD root-running VDR

The socket override changes the **pathname**, not the ACL or service identity.
Migrations require matching real paths, service unit/environment ownership,
Unix group/mode and parent directory execute permission. An `0700` parent
directory owned by VDR prevents a different non-root Agent from entering, even
if the socket itself is `0660`. The MLD package must establish a safe
explicit user/group/permissions topology; do not simply run the Agent as root
or apply broad `chmod 777`. The VDR process and daemon/Agent do not inherit
one another's systemd `EnvironmentFile` automatically: set the socket path
in **each relevant service**, or deliberately preserve the shared default.
No service changes are made by this source branch.

### Example opt-in MLD package settings (NOT actual discovered MLD paths)

These are schematic values only. Do not copy until Pit confirms the real
MLD paths and access model.

```ini
# Environment for the daemon
VDR_SUITE_RECORDING_VIDEO_ROOTS=default=/REPLACE/WITH/ACTUAL/MLD/VIDEO
VDR_SUITE_EPG_ARTWORK_ROOTS=/REPLACE/WITH/ACTUAL/MLD/TVSCRAPER
VDR_SUITE_CONTROL_SOCKET_PATH=/run/REPLACE-WITH-APPROVED-PRIVATE-DIR/control.sock
```

The VDR service hosting SuiteBridge must receive the **same**
`VDR_SUITE_CONTROL_SOCKET_PATH`, while the Backend Agent may set
`SUITEBRIDGE_CONTROL_SOCKET_PATH` in `/etc/vdr-suite/backend-agent.conf`.
Leave the variables unset on Ubuntu/yaVDR.

## Verification boundary

Unit tests cover the default paths, explicit paths, invalid/missing video
root mappings, cross-root/traversal source rejection, MLD-safe target directory
encoding, Agent config validation, private socket env/default behavior, and
Artwork allowlist overrides. The existing yaVDR recording move regression
remains in the suite.

Tests and a real MLD build must be run on the **new source head** before
calling this accepted. The direct MLD BitBake recipes, target service
identities, VDR sysroot and native runtime remain **unverified**.
No main-branch merge, MLD recipe SRCREV change or installed host
modification has been made here.
