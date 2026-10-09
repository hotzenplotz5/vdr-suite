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
- The four provisionally known package names are core-owned; unknown package
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

Tests construct four isolated package roots, exercise malformed package
metadata, fake rights, symlinks, unknown packages, missing installed modules
and zero-side-effect enable previews. No installed VDR path is touched.

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
