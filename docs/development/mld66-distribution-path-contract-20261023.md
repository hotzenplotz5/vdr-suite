# MLD 6.6 distribution path contract — Ubuntu/yaVDR stays zero-config

**Source location (updated 2026-11-03):** The previously separate MLD 6.6 code is now contained in official `main`. Pit's pinned MLD recipe `SRCREV 9a28cd77a74c85516fc9147cc84bdb1b66edfdf5` does **not** automatically include later changes: the recipe, native build and installed package require separate approval and verification.

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

The recording root is owned by the configured VDR backend, **not** by
the Linux distribution. Existing operator `/etc` files remain untouched.

- Recording mutations require either an explicit backend-specific
  `VDR_SUITE_RECORDING_VIDEO_ROOTS` entry or the matching SuiteBridge
  `VPATH 1` discovery result. If neither exists, these mutations are
  unavailable rather than using a guessed distribution path.
- Other distribution recipes may configure **their own** layouts; they must
  not relocate VDR-managed files to fit an Ubuntu default.
- Invalid explicit nonempty overrides fail closed for the affected backend.
- Packaged service and agent root settings must be derived from the actual
  backend. Do not change any installed service automatically during upgrade.

## Configuration matrix

| Setting | Scope | Absent (Ubuntu/yaVDR) | MLD/other distro opt-in |
| --- | --- | --- | --- |
| `VDR_SUITE_RECORDING_VIDEO_ROOTS` | Daemon process; backend ID to physical VDR video-root map | Discover via the backend's SuiteBridge `VPATH 1` if enabled, otherwise recording mutations are unavailable until configured | `default=/ACTUAL/VDR/VIDEO`; semicolon-delimited for multiple backends. Invalid mapping or missing backend disables that backend's recording mutation adapter |
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
On every distribution, configure the recording root explicitly if SuiteBridge `VPATH 1` is not available. Do not infer it from a distribution name.

## MLD command validation update (2026-10-23)

Pit has now reported the read-only SuiteBridge `VPATH 1` command returning `900 /data/tv` on MLD VDR 2.8.3. This is a real-host **command-level** result, not proof of daemon root-probe adoption or recording mutation authority. His simultaneous RESTfulAPI five-second recording timeout and recurring VDR restart report remain independent blockers. See [latest evidence](mld66-pit-vpath-http-crash-evidence-20261023.md).

## Verification boundary

Unit tests cover explicit paths and discovery, invalid/missing video
root mappings, cross-root/traversal source rejection, MLD-safe target directory
encoding, Agent config validation, private socket env/default behavior, and
Artwork allowlist overrides. The existing yaVDR recording move regression
remains in the suite.

Tests and a real MLD build must be run on the **new source head** before
calling this accepted. The direct MLD BitBake recipes, target service
identities, VDR sysroot and native runtime remain **unverified**.
The source integration into `main` is complete; **no** MLD recipe
`SRCREV` change or installed host modification is implied.

## Senderlogos aus dem optionalen MLD-Paket (2026-10-26)

VDR-Suite checks `/usr/share/vdr/channellogos` as an additional **read-only**
logo directory if it exists. Explicit `VDR_SUITE_CHANNEL_LOGO_ROOT` and
`VDR_SUITE_CHANNEL_LOGO_CACHE` overrides, the Suite logo cache, and all
previous logo directories retain their precedence. PNG and SVG filenames
are supported; browser candidates are normalized to lower-case and include
known VDR aliases, including `~` in place of a channel-name slash.

MLD needs only to offer/install the optional `channellogos` data package
and grant the VDR-Suite daemon read access. No copying, renaming or relocation
of distro-owned assets is required. Missing package/directory and unknown
logos continue to use the browser's initial-letter fallback. Real MLD 6.6
runtime acceptance remains separate from repository regression tests.
