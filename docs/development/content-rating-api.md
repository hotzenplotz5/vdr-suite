# Content Rating API

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This document describes the current content rating API contract.

The current implementation provides a REST-facing controller boundary and JSON serialization contract for resolved content ratings.

It does not yet expose an ApiRouter route.

---

## Current Scope

Implemented:

- ContentRating domain object
- ContentRatingCollection evidence container
- ContentRatingResolver
- ContentRatingResolutionResult
- ContentRatingResolutionJsonSerializer
- ContentRatingController

Not implemented yet:

- ApiRouter endpoint
- database persistence
- provider integration
- profile policy
- access enforcement
- real VDR metadata validation

---

## JSON Contract

The serialized content rating resolution contains:

- resolved
- primaryRating
- evidence

Example fields for a rating object:

- source
- system
- originalValue
- minimumAge
- confidence
- providerReference

The primary rating is the currently selected rating.

The evidence list preserves all known rating facts.

---

## Resolution Rules

The current resolver uses deterministic rules:

- explicit confidence is preferred
- higher confidence wins
- user ratings act as manual overrides
- otherwise the higher minimum age wins
- source priority is used as final tie-breaker

This keeps rating resolution predictable while preserving all source evidence.

---

## REST Boundary

The current controller boundary returns an ApiResponse with:

- statusCode 200
- contentType application/json
- body containing the serialized content rating resolution

The current boundary is intentionally isolated and not yet connected to ApiRouter.

---

## Future Direction

Future phases may add:

- ApiRouter route
- query/request model
- event/recording metadata integration
- rating provider mapping
- profile visibility policy
- FSK-style filtering
- real VDR metadata validation

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
