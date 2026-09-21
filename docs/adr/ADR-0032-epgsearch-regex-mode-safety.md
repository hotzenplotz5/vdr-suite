# ADR-0032: EPGSearch Regex Mode Safety

## Navigation

- [ADR Index](index.md)
- [Development Index](../development/index.md)
- [EPGSearch Capability Matrix](../development/epgsearch-capability-matrix.md)
- [EPGSearch Test Coverage Audit](../development/epgsearch-test-coverage-audit.md)

---

## Status

Accepted.

---

## Context

VDR-Suite now supports deterministic EPGSearch modes for exact, all-words and any-word matching.

The EPGSearch query model already contains a regular-expression mode, but regular-expression matching has different risk characteristics from deterministic text matching.

Regex mode can introduce:

- invalid-pattern errors
- unexpectedly expensive matching behavior
- ambiguous user expectations around anchoring and field scope
- unclear HTTP error semantics if invalid regex input reaches the service layer

Therefore regex must not be silently treated like normal phrase matching once exposed through the REST API.

---

## Decision

EPGSearch regex support is allowed only under an explicit mode.

The accepted public mode name is:

- `mode=regex`

Regex mode must follow these rules:

- default EPGSearch behavior remains phrase/contains matching
- regex mode is never enabled implicitly
- invalid regex patterns must return HTTP 400
- invalid regex patterns must not crash the process
- invalid regex patterns must not fall back to phrase matching
- regex evaluation must remain scoped to the configured searchable fields
- exact/all/any modes remain deterministic and independent from regex
- fuzzy search remains a separate future decision

The implementation phase after this ADR may activate regex mode in the matcher and REST/controller boundary.

---

## Consequences

Positive consequences:

- regex behavior has explicit API semantics before implementation
- invalid user input has a deterministic error contract
- existing phrase/exact/all/any behavior remains stable
- future tests can target accepted/invalid regex behavior directly

Trade-offs:

- regex support is delayed by one phase
- URL encoding and escaping requirements remain a client/API concern
- performance guards may need further refinement after real-world validation

---

## Implementation Guidance

The following implementation shape is recommended:

- extend the REST/controller search-mode validation to accept `regex`
- map `mode=regex` to `EpgSearchMode::RegularExpression`
- compile regex patterns at the controller/service boundary or in a matcher path that can report invalid input
- return HTTP 400 with a stable JSON error for invalid regex patterns
- keep `std::regex_error` contained
- add router/controller regression tests for valid and invalid regex patterns
- keep fuzzy search out of the regex implementation phase

---

## Back

- [Back to ADR Index](index.md)
- [Back to Development Index](../development/index.md)
