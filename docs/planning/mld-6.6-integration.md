# MLD 6.6 integration — separate distribution target

**Status:** planned; no MLD build, package installation or end-to-end acceptance claimed.

**Dedicated branch:** `work/mld66-platform-integration`
**Branch baseline:** `main` at `01b74e30ec1a09ee68986059f508284564ef67e1` (2026-10-19).
**Explicit exception:** The project owner authorized a *separate branch* for MLD on 2026-10-19. This is one topic branch originating at current `main`, **not** a stack based on the ongoing API-v1, Timer, codec, Multiuser or VIDAA branches. Retain it for this bounded integration; do not merge, close a PR or delete the branch without explicit approval. Follow [branch lifecycle](../development/branch-pull-request-lifecycle.md).

## Goal and authority

Provide **MLD 6.6 as an alternative supported installation and update path** alongside Ubuntu/yaVDR, using the same VDR-Suite source and public API contract. MLD packaging must be a distribution integration, not a permanent feature fork, a new authentication authority, or a parallel HTTP API.

This is a **cross-cutting platform productization milestone**, provisionally `MLD.1`; it does not renumber or start [Phase 70](roadmap.md), reopen completed Phases 68/69, or alter current [multiuser/VIDAA](../CURRENT.md) acceptance. The primary [platform productization roadmap](platform-productization-roadmap.md) and [ADR-0037 install boundary](../adr/ADR-0037-packaging-install-api-boundary.md) remain applicable.

## External starting point and questions to verify

- The MLD team reports **MLD 6.6**, VDR running as **root**, and `vdr-plugin-restfulapi` **0.2.6.6**. These are integration inputs, not yet independently verified capabilities on an actual MLD 6.6 host.
- The [MLD-6 build repository](https://gitlab.com/MLD-6/mld-dev) documents Yocto/BitBake builds orchestrated with `kas`; the distribution builds its own packages and package index. Check the *actual target branch, recipes, machine, VDR package and dependency feed* before deciding package names or implementation paths. **Do not assume Ubuntu PPA binaries are compatible with the MLD build environment**, even if both package managers use `.deb`.
- RESTfulAPI is an **internal optional Suite backend provider**, not the public browser API. [Adapter contract](../architecture/restfulapi-integration.md). A plugin version string is not proof of compatible event/channel/timer/recording semantics.
- SuiteBridge must be built/packaged against the **exact VDR plugin API/ABI** of the selected MLD image; never copy an Ubuntu-built `.so` blindly.
- Running VDR as root is **not** authorization for running the Suite HTTP daemon, Agent, database or client sessions as root. Inventory permissions and process ownership; preserve the existing actor/grant and device-pairing security model. Do not weaken authentication or authorize native mutations by default.

## Implementation workstream

### MLD.1A — Reproducible read-only compatibility inventory

Collect versioned evidence for VDR core/API, plugin versions and loading, RESTfulAPI response semantics, SuiteBridge `CAPS` / native capability discovery, service manager, runtime paths, package feeds, toolchain, filesystem layout, service users/groups and privileged access needs. Map every known incompatibility and provide evidence before a workaround. Do not modify live MLD or yaVDR.

### MLD.1B — Native build and package ownership

Agree with MLD maintainers on the owning Yocto layer/recipe location and update path. Reuse the supported staged source/install contract from `mk/install.mk`, adapt it to MLD without an alternative manually maintained binary file list, and package daemon, web assets, Backend Agent and SuiteBridge with explicit dependencies, VDR ABI constraints and reproducible source SHA. Document config/state/log file ownership; optional VDR/client components remain deliberate packages. Test build in an isolated MLD development environment, **not** as a root script on production yaVDR.

### MLD.1C — Lifecycle and security

Verify init/service startup order, native service environment, restart/shutdown, locally managed secrets, First Admin bootstrap, account/Device grants, Web CSRF/auth gates, Agent trust and loopback/backend transport. For root-running VDR, allow only the minimum required local interoperability. Validate database migrations and upgrade/remove semantics without overwriting configuration or user data; preserve independent authorization and fail-closed native mutation gates.

### MLD.1D — Real-system functional acceptance

Against the exact installed MLD package set, test version discovery and public `/api/v1`, authenticated channels/EPG/timer reads, controlled Timer CREATE with safe validation and readback, recording reads/actions, SuiteBridge capability discovery, web assets, live/recording playback where supported, pairing/authorization and explicit degradation for absent plugins/capabilities. Capture logs, exact VDR and plugin versions, recipe/source SHA, CPU architecture, test fixture, expected versus actual results. **No blind live timer creation, database edits or root permission changes**.

### MLD.1E — Publication and maintenance

Publish a documented MLD-supported installation/update flow only after clean install, upgrade/rollback, plugin ABI failure handling, restart, and relevant Golden User Journeys pass on MLD. Maintain a version/feature matrix for MLD and Ubuntu independently; one public API and one source-owned product architecture. Coordinate package-feed maintenance with the MLD team.

## Coordination and acceptance

- **MLD maintainers:** confirm official 6.6 build baseline, recipe owner, package feed, expected supported architectures and distribution service policy.
- **VDR-Suite:** supply install manifest, source build boundaries, optional capability checks, API/authorization invariants, compatibility tests and necessary upstream fixes.
- A dedicated **“VDR-Suite goes MLD”** MLD-Forum thread is the proposed question/answer venue; the general VDR-Portal thread remains for Suite product discussion.
- MLD is **not supported** until the real clean-install and functional acceptance described above succeeds. Do not advertise the preparatory branch, recipe drafts or Ubuntu PPA CI as MLD runtime acceptance.

## Non-goals and safety

No migration/reinstallation of the existing yaVDR host, no root-level modifications to a live VDR, no direct `main` writes, no unrelated codec/timer/API-v1/VIDAA changes, no new universal VDR plugin ABI, and no deployment or publication from this planning document. Any later branch-to-main integration requires explicit approval, exact-commit tests and dependency checks.

## Source pointers

- [MLD-6 developer build tree](https://gitlab.com/MLD-6/mld-dev)
- [Platform productization roadmap](platform-productization-roadmap.md)
- [Current state](../CURRENT.md)
- [Packaging status](../../packaging/README.md)
- [RESTfulAPI adapter](../architecture/restfulapi-integration.md)
- [Branch/PR lifecycle](../development/branch-pull-request-lifecycle.md)
