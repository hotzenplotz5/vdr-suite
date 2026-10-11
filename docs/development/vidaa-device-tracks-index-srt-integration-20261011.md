# VIDAA Device track integration — isolated candidate, 2026-10-11

**Development only. DO NOT DEPLOY, MERGE, or replace the live daemon without separate production authorization.**

## Provenance

- Parent `499b20a01174a3bc5944921fed011897e697d48b` (PR #448) retains PR #445's index-backed duration, Device status projection, authorized track inventory and Device subtitle route.
- Named `.srt` fallback and its unit test are reused verbatim from `6cf5e5e488d76346e993a121ee261cef0966a69f` (PR #449).
- Overwriting the parent `RecordingMediaHttpRuntime.cpp` with the isolated .9 draft would erase the index/duration projection; this integration changes only the Device track/subtitle blocks.
- The Device `track-status` projection is now bounded at 64 KiB and checks extracted object boundaries; successful Device WebVTT bodies are limited to 1 MiB with text/vtt content type and WEBVTT signature.
- Existing browser controller and VDR recording/index logic are not modified; no Device privileges are broadened. Existing Device proof, active play grant, backend scope and session ownership remain required.

## Acceptance and open gates

- Focused workflow `.github/workflows/vidaa-device-tracks-integrated-ci.yml` runs existing C++ media, subtitle, parser, public response, and SecurityHttpGate checks on this branch.
- Not yet proven: exact revision of running yaVDR daemon; registered Device routes there; end-to-end authorization and real hardware Audio/SRT/WebVTT selection. Complete review and separate deployment approval remain required.
- Running binary fingerprint: `cda10a7d1c51c63a600bf18a80c520077122abe3af28e90cab6b663f4dc46386`. A binary SHA-256 is not a source commit.
