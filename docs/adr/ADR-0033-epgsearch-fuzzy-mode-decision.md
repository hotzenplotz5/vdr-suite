# ADR-0033: EPGSearch Fuzzy Mode Decision

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

VDR-Suite now supports deterministic EPGSearch-style modes and explicit regex mode.

LIVE/epgsearch exposes fuzzy search as a SearchTimer search mode with an additional tolerance value.

The relevant LIVE/epgsearch-compatible model is:

- mode 0: phrase
- mode 1: all words
- mode 2: at least one word
- mode 3: match exactly
- mode 4: regular expression
- mode 5: fuzzy
- tolerance: integer fuzzy tolerance used when mode is fuzzy

Fuzzy search differs from phrase/exact/all/any/regex because it needs a tolerance contract and can be interpreted as a boolean match, a ranked match, or a backend-native epgsearch operation.

---

## Decision

VDR-Suite will support fuzzy search as an explicit mode, but it must remain backend-neutral.

The public REST shape is:

- mode=fuzzy
- tolerance=<int>

The internal SearchTimer-compatible mapping is:

- mode=fuzzy maps to the epgsearch-compatible fuzzy mode
- native epgsearch adapters may map it to numeric mode 5
- tolerance maps to the epgsearch-compatible fuzzy tolerance field

Rules:

- fuzzy is never enabled implicitly
- default search behavior remains phrase/contains
- tolerance must be explicit or default to 1
- tolerance must be non-negative
- invalid tolerance must return HTTP 400 once exposed through REST
- fuzzy initially produces boolean match/no-match behavior, not ranking
- VDR-Suite must not invent a public scoring model before the SearchTimer runtime needs it
- backend-native epgsearch support may be used later when available
- VDR-Suite fallback behavior may be implemented for backends without epgsearch
- native and fallback behavior must be documented as capability-dependent if they diverge

---

## Consequences

Positive consequences:

- VDR-Suite remains usable without the epgsearch plugin installed.
- Existing epgsearch/LIVE semantics can still be mapped by future native adapters.
- Multi-backend installations can expose fuzzy capability per backend.
- The REST API stays stable and backend-neutral.

Trade-offs:

- Fuzzy behavior may not be bit-identical to native epgsearch when using the fallback matcher.
- Native epgsearch passthrough and VDR-Suite fallback must be distinguished by backend capability metadata.
- Ranking remains intentionally deferred.

---

## Implementation Guidance

The follow-up implementation should:

- accept mode=fuzzy at the REST/controller boundary
- accept tolerance=<int> for fuzzy mode
- reject negative tolerance with HTTP 400
- map fuzzy mode to EpgSearchMode::Fuzzy
- add a fallback matcher with deterministic tests
- keep fallback matching conservative and boolean
- keep native epgsearch passthrough as a later adapter concern
- add capability documentation for native vs fallback fuzzy support

---

## Back

- [Back to ADR Index](index.md)
- [Back to Development Index](../development/index.md)
