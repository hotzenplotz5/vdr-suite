# Phase 55.3 - Timer Form Body URL Encoding

## Navigation

- [Development Index](index.md)
- [Phase 55.0 - VDR-Suite Feature-Parity and Adapter Audit](phase-55-vdr-parity-adapter-audit.md)
- [Phase 55.1 - SearchTimer Update RESTfulAPI Contract Fix](phase-55.1-searchtimer-update-restfulapi-contract.md)
- [Phase 55.2 - SearchTimer JSON Array Payload Parity](phase-55.2-searchtimer-json-array-payload-parity.md)
- [Current Project Status](current-status.md)

---

## Status

Completed. GitHub Actions passed for commit `ca74848f`.

## Purpose

Phase 55.0 identified a RESTfulAPI timer-action robustness gap.

VDR-Suite sends timer create/update requests as:

```text
Content-Type: application/x-www-form-urlencoded
```

Before this phase, form values were appended raw as `name=value` pairs. That is unsafe for real timer names and metadata because reserved form characters inside a value can be interpreted as separators or operators.

Problem examples:

```text
file=Film & Serie
aux=<vdr>foo=bar&baz+</vdr>
```

Without encoding, `&`, `=`, `+`, spaces, XML characters and percent signs can corrupt the request body.

## Behaviour after this phase

Timer form values are now encoded before being appended to the body:

- unreserved characters remain unchanged
- spaces become `+`
- reserved bytes become `%XX`
- literal plus signs become `%2B`
- XML/control characters such as `<`, `>`, `/`, `&`, `=` are encoded

Example encoded payload fragments:

```text
file=Doku+Archiv~Film+%26+Serie+%2B+Bonus+%3D+100%25
aux=%3Cvdr%3Efoo%3Dbar%26baz%2B%3C%2Fvdr%3E
timer_id=timer+42%2B7
```

## Changed files

```text
core/vdr/include/RestfulApiVdrTimerActionRequestBuilder.h
core/vdr/tests/test_restful_api_vdr_timer_action_executor.cpp
```

## Regression coverage

The existing target is extended:

```text
test-restful-api-vdr-timer-action-executor
```

New coverage:

- create request encodes a timer title containing `&`, `+`, `=`, `%` and spaces
- create request encodes a directory containing spaces
- create request encodes XML-like aux data containing `<`, `>`, `/`, `=`, `&` and `+`
- update request encodes `timer_id` containing a space and a literal plus sign
- raw dangerous fragments such as `Film & Serie` and `foo=bar&baz+` must not remain in the request body

## Remaining follow-up

This phase intentionally focuses on form body values only.

Potential follow-up work:

- Decide whether DELETE `/timers/<id>` needs path encoding for non-numeric backend timer IDs.
- Add a real yaVDR smoke test with a timer title containing spaces, umlauts and reserved form characters.
- Reuse or extract form encoding if another RESTfulAPI form endpoint is added.

## Verification

GitHub Actions passed for commit:

```text
ca74848f
```

Validated gates included:

```text
make test-docs
make test-phase
```

Narrow implementation target:

```bash
make test-restful-api-vdr-timer-action-executor
```

## Back

- [Back to Development Index](index.md)
- [Back to Phase 55.0 Audit](phase-55-vdr-parity-adapter-audit.md)
- [Back to Phase 55.2](phase-55.2-searchtimer-json-array-payload-parity.md)
