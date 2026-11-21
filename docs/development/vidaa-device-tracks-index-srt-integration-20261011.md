# VIDAA Device track integration — merge provenance, 2026-10-11

**Current repository status (2026-10-11):** The consolidated R2 source was merged into `main` at `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`. The original branch/provenance notes below are **historical** and must not be read as a claim that this code is still unmerged. This source checkpoint alone does **not** prove which daemon binary is installed on yaVDR, nor successful real Hisense audio, SRT/WebVTT or STOP/cleanup acceptance. A deployment/restart still needs separate explicit authorization.

**Safety:** Do not deploy or replace the live daemon solely because this source branch merged.

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

## Historical compatibility guard reconciliation (PR #450 CI)

The first complete PR CI observed three pre-existing historical guard discrepancies, which are not grounds to discard the R2 code or weaken security:

- The public route inventory was missing the **actual already implemented** `/api/v1/epg/now-next`, `/api/v1/genres`, and `/api/v1/genres/recordings` handler literals. They are now explicitly inventoried as public-v1 paths; no new route is created here.
- The P2 account guard incorrectly counted the candidate Recording reference as an accepted 69.F contract (10 vs 9). It now explicitly proves **nine accepted resources covered by seven accepted references, plus the single named R2 Recording candidate**. Unknown resources/references fail closed.
- The Backend collection guard's literal assertion of the removed historical print phrase is updated to the **actual 9 accepted + 1 candidate** report and exact method/resource-set validation.

The original accepted 69.F contract semantics remain frozen; the newer Recording candidate is **not silently promoted to accepted**. At the time of the original candidate, the branch still required full CI before merging; the focused Device track CI alone was insufficient. That sentence describes the **historical pre-merge gate**, not the current `main` state.
