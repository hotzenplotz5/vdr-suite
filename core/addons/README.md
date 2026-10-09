# Core Add-on access decisions

This is an internal C++17 source-only policy component, not an optional
module and not a public C++ ABI.

`AddonAccessPolicy` consumes VDR-Suite's existing
`RequestSecurityContext` and `BackendAccessDecision`. It authorizes
global-admin inventory inspection and models the future Media Tools import
admission conjunction. The actual VDR-Suite daemon, package registry,
actor grant administration and Backend Agent do **not** call it yet.
No package, module or capability becomes enabled by compiling the library.

The future `addons.media.import` right is deliberately not yet in the
canonical grant administration vocabulary. Therefore any positive access
example in this unit test is an isolated injected fixture and **not** a
production capability. Do not add an HTTP endpoint, handler launcher or
persisted enable switch on the strength of this evaluator alone.

Run `make test-addon-access-policy` on an isolated build path.
See [registry boundary](../../docs/development/addon-installed-registry-contract.md).
