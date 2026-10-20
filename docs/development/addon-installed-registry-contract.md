# Add-on inventory and activation boundary — read-only slice

## Background and scope

ADR-0068 established inert, separately packageable modules with per-module
`AGENTS.md`. C++ Media Tools source/package preview is **not** a live enabled
VDR-Suite module. This slice adds a working, deterministic **installed metadata
inventory**, not an add-on loader, HTTP endpoint or activation service.

### Source and installed directories

- Source definition: `modules/<id>/{addon.json,README.md,AGENTS.md}`
- Independently installed package metadata: `/usr/share/vdr-suite/addons/<id>/`
- Isolated package root is supplied explicitly with `--root <root>`; the
  command is entirely read-only, with no startup side effects.
- The three reviewed package names are core-owned; unknown package
  directories are reported as rejected rather than self-registered.
- Corrupt, mismatched, incomplete or symlinked package records are rejected.
- A valid manifest is reported as **installed-disabled**, never `active`.
  Inventory means **metadata present**, not that `dpkg` has verified package
  ownership, installation provenance or binary integrity.

### Minimal machine-readable contract

`registrySchemaVersion: 1`, `modules: []`. An item contains `id`,
`package`, `installed`, `state`, `reason`, `active` and `capabilities`;
valid installs additionally have `version`.

`active=false` and `capabilities=[]` are unconditionally enforced in this
slice. `plan-enable` produces a denied, side-effect-free decision. A package
cannot create an HTTP endpoint, execute code or gain a permission by editing
its manifest. This inventory is **not safe to expose as a public API** without
the existing actor authorization and response contracts.

### Offline commands

```bash
cd /home/yavdr/vdr-suite
make check-addon-registry
python3 tools/addons/installed_registry.py inventory --root /path/to/package-stage
python3 tools/addons/installed_registry.py plan-enable --root /path/to/package-stage --module rectools
```

Tests cover the three reviewed module identities, rejection of legacy
TVScraper add-on paths, malformed package metadata, fake rights, symlinks,
unknown packages, missing installed modules and zero-side-effect enable
previews. No installed VDR path is touched. TVScraper metadata continues
through the native SuiteBridge/Core provider integration (ADR-0036);
existing installations are not modified by this source-only change.

## What must still be implemented before activation

1. **Trust**: signed/admin-installed `dpkg` package provenance, compatible
   Suite version, ownership and immutable package-to-handler identity.
2. **Server ownership**: approved backend-scoped capability/handler registry,
   no manifest-selected executable/shell path and no public C++ ABI.
3. **Authorization**: Core-owned actor grants for administrator module
   management, enabled module operations, and backend/source-specific scopes.
   Install status is not a substitute for permission.
4. **Lifecycle**: explicit administrator intent stored durably, concurrency,
   compatibility/dependency resolution, enable/disable and crash recovery.
5. **Client discovery**: a scoped API returning only currently authorized,
   healthy effective capabilities, not raw manifests or filesystem paths.
6. **Package migration**: upgrade/downgrade, state schema changes, uninstall
   semantics, audit and rollback, documented per module.

All these are **future implementation gates**. Do not call this inventory an
operational manager until they are satisfied.

## Suite Core C++ policy evidence (next pre-integration slice)

The C++17 `core/addons/AddonAccessPolicy` independently evaluates global-administrator inventory visibility and the strict conjunction for future backend-local Media Tools import. The evaluation uses canonical `RequestSecurityContext` (authenticated, active actor/device/session/credential, resolved grants) and `BackendAccessDecision`; it does **not** reimplement backend read-only/write classification. Even a package with valid metadata remains unusable without independent package provenance, compatible version, explicit administrator enable, reviewed healthy execution handler and an exact backend-scoped `addons.media.import` grant. Generic admin or `recordings.execute` is not an import grant. This new permission is a prospective name **not currently grantable** by the canonical Human Account/Device Grant Administration vocabulary. This isolated evaluator is not connected to the daemon, Public API, worker or persistent settings, so it grants no live capability.

Validation: `make test-addon-access-policy` compiles the actual C++ service and exercises the above permits/denials in a standalone binary. No live media, VDR or daemon services are involved. Productive enable/disable and API wiring need separately audited authentication, persistence, operation fencing and readback/reconciliation.

## Durable desired state proof (not wired to runtime)

`core/addons/AddonActivationIntentRepository` implements a narrowly scoped,
source-only C++17 SQLite persistence model for administrator **intent**,
not effective module activation. It stores only reviewed module identities
and explicitly scoped backend IDs. Defaults are disabled, revision 0;
every authorized edit compares its expected revision under an SQLite
`BEGIN IMMEDIATE` transaction and increments a revision. Read/write
admission requires resolved global `role.admin@*`. Enabling even the
known `rectools` module requires separately supplied evidence of exact
package identity, manifest validity, package provenance, Suite compatibility,
trusted/healthy core handler and canonical writable backend. No other
scaffold can be marked desired-enabled. Disabling is intentionally possible
when a provider has disappeared.

The repository is intentionally **not wired** into `DaemonRuntime`,
`PublicApiRuntime`, the Python inventory, package installation or any
executor. Stored intent cannot enable an executable, grant permissions,
refresh the VDR cache or expose a client capability. The first runtime
consumer must derive effective availability on every use from freshly
verified package, handler, backend, actor and stored-state evidence, with
fail-closed behavior if any input becomes unavailable. Future API/activation
mutations also need accountability logging, durable permission vocabulary,
CSRF, revision/idempotency and upgrade/removal policy.

Test `make test-addon-activation-intent` creates a temporary isolated SQLite
file; verifies default-disabled read, forbidden actors, missing trust/handler,
read-only backend, invalid identities, optimistic conflict, persistence
across repository reconstruction, and disabling while backend is offline.
It does not touch production SQLite or VDR.

## SQLite-to-C++ policy read bridge (2026-10-09)

The internal `AddonAdmissionReadService` supplies a **non-executing**
Media Tools eligibility preview. It first applies existing
`RequestSecurityContext` actor/grant and `BackendAccessDecision`
backend gates with separately supplied package/handler evidence. If
those gates pass, it reads the desired state from the SQLite repository
and **overwrites** the caller-provided `administratorEnabled` flag.
An absent row is disabled, an inaccessible/invalid table denies,
and revocation of backend authority, package trust, handler health or
actor grant denies regardless of a previously enabled row.

Only the bounded core-internal preview can read SQLite state without
a global administrative role. A global administrator grant never
substitutes for the backend-scoped `addons.media.import` right.
Its reported `policy.allowed=true` means a **hypothetically eligible
combination of injected evidence**, not an installed executable provider.
`executable=false` is always returned. It does not dispatch jobs,
expose an HTTP route, load modules or start any VDR operation.
Its observed revision is diagnostic only, not a valid operation lease.

`make test-addon-admission-preview` compiles the C++ service with SQLite
against a newly created temporary database. Nothing is installed on
the running yaVDR host.

This slice still does **not** consume live `dpkg` proof, nor does it
connect the Python installed-metadata inventory to the C++ runtime.
A future runtime must independently verify package provenance, owner,
schema/version compatibility, handler identity and backend generation,
and recheck desired-state/revision at the fenced operation boundary.

## Read-only Debian package-ledger consistency (2026-10-20)

The installed inventory now accepts the optional `--verify-dpkg` diagnostic,
always scoped to the **same absolute `--root`** as the metadata tree. The
`dpkgEvidence` item contains `consistent` and a bounded reason. The
verifier reads `var/lib/dpkg/status`, the exact reviewed package's
`info/<package>.list` and `info/<package>.md5sums` under that root.
It requires `install ok installed`, an approved scaffold/CPP-preview version,
the applicable architecture, claimed ownership of all three metadata files
and matching file hashes. Missing ledgers, duplicate entries, symlinks and
checksum mismatches fail closed. No `dpkg` command or external program runs.

On the current conventionally/source-installed yaVDR, **do not require or
run this dpkg check** for VDR-Suite or Media Tools development. Current
package builds and daily builds are build artifacts, not proof of an
installed add-on or Suite package. The offline fixture exercises the optional
diagnostic using an isolated root with synthetic dpkg files; the real host's
`/var/lib/dpkg` is not inspected by routine add-on tests.

The regular `--root <isolated>` metadata-only mode works **without**
a dpkg database and emits no `dpkgEvidence` field. It preserves the
existing registry schema-1 response shape. Later, for a genuinely Debian-
installed add-on, an operator can explicitly request `--verify-dpkg`.
A manual/source-built handler will need a *separate*, independently approved
install identity and integrity mechanism; it must not pretend to be a
Debian package.

**This is local dpkg ledger *consistency*, not trusted package provenance.**
The dpkg status/list/MD5 files and package content are mutable local data;
MD5 is not a cryptographic authenticity guarantee. `installed` continues
to mean manifest metadata exists. `active=false` and `capabilities=[]`
are unconditional, even with consistent ledger data. No value from this
offline CLI is permitted to set C++ `packageTrusted` or authorize a worker.
Actual production trust still needs independently verified package source/
signature policy, immutable handler identity, revocation and backend fencing.

The isolated regression target `make check-addon-registry` now also verifies
the dpkg-ledger checks against a synthetic, caller-owned root.

## Manual/source handler integrity diagnostic (non-authorizing)

`installed_registry.py inventory --root <isolated-root> --manual-sha256 <independently-approved-64-hex-digest>` adds an optional `manualHandlerEvidence` diagnostic. The default schema-1 response remains unchanged. Only the fixed, reviewed Media Tools binary path `/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import` is examined. Images, Music, and unknown modules never acquire a handler through this option.

The checker requires an existing regular executable with root ownership, no group/other writes, safe non-symlink path components and a bounded SHA-256 match. The expected digest MUST originate outside the inspected install root from independent release or administrator approval; a value entered at the CLI is not itself trusted provenance. It never runs code, creates an installation, activates a module, sets `packageTrusted` or registers a handler. The currently staged C++ prototype and its preview package are still disabled.

A digest computed from the installed file itself proves nothing beyond self-consistency. Even a pinned digest check is subject to filesystem replacement after inspection: a future backend executor must acquire/revalidate its own execution descriptor, verify core-owned approval and maintain lifecycle fencing. The Python CLI is an offline diagnostic, **not** a security authority for the daemon.

`make check-addon-registry` runs the additional isolated manual-install negative tests without reading the live filesystem.

## Core handler identity scaffold

`core/addons/include/AddonHandlerRegistry.h` fixes the Media Tools identity,
package, binary path and intended backend-Agent transport at compile time.
The declared handler is **not executable**; no daemon route, worker, or
installation handler consumes this definition yet. Unknown module and package
pairings have no mapping. This is a reviewable identity
definition only, not provider trust, permission or activation. The isolated
`make test-addon-handler-registry` test verifies the immutable mapping and
fail-closed unknown identities, without starting the daemon.
