# VDR Suite Agent Plugin

`vdr-plugin-suite-agent` is the native VDR execution bridge for operations that must be performed against live VDR objects.

It does not replace the VDR-Suite control plane, RESTfulAPI, or existing read adapters.

## Responsibility boundary

VDR-Suite owns authorization, backend policy, validation, jobs, audit context, user confirmation and multi-site orchestration.

The plugin owns only the final local VDR mutation:

- resolve the recording through VDR's recording list
- preserve the real `cRecording::Name()` leaf
- construct the destination in VDR's logical naming format
- call `cRecording::ChangeName()`
- update VDR's recordings state
- return a structured success or failure result

The plugin must never reconstruct a recording name from EPG metadata or a frontend display title.

## Initial transport

The first transport is a local SVDRP plugin command. This deliberately reuses VDR's existing local control channel and does not expose a new network listener.

Command:

```text
PLUG suiteagent MOVR <source-file-name><TAB><target-folder>
```

`source-file-name` is the backend-native value returned by `cRecording::FileName()`.

`target-folder` is a logical VDR folder. `/` and an empty value mean the recording root. Slash-separated and tilde-separated folders are accepted and normalized inside the plugin.

The command moves the recording while preserving the final component of the real `cRecording::Name()` value.

Example:

```text
source file name:
/srv/vdr/video/Archive/My_Record/2026-07-17.20.15.1-0.rec

cRecording::Name():
Archive~My Record

target folder:
Movies/Drama

new cRecording::Name():
Movies~Drama~My Record
```

## Safety rules

- no raw destination filesystem path is accepted
- `.` and `..` folder components are rejected
- control characters are rejected
- the source must resolve to an existing VDR recording
- an existing destination is rejected by VDR's `ChangeName()` implementation
- no RESTfulAPI source is modified
- no VDR source is modified

## Build

The plugin uses the standard VDR plugin Makefile contract:

```bash
make VDRDIR=/usr/include/vdr
```

Packaging and runtime activation are intentionally separate from this foundation. The VDR-Suite executor must not prefer this backend until a real end-to-end move and readback test has passed.
