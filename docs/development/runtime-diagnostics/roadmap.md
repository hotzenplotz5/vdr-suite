# Runtime Diagnostics Roadmap

This document tracks runtime diagnostics work that is intentionally not part of the current Phase 10.20 implementation.

## Not implemented yet

The following topics are not implemented yet:

- persisted runtime measurements
- long-term runtime history
- rolling averages
- percentile calculations
- diagnostics access control
- diagnostics export formats
- external metrics integration

## Candidate future phases

Potential future work:

```text
Phase 10.21: diagnostics API validation and hardening
Phase 10.22: rolling summary values
Phase 10.23: diagnostics retention configuration
Phase 10.24: runtime diagnostics export strategy
```

## Current boundary

The current endpoint remains a read-only view over the daemon-owned runtime diagnostics state.

The diagnostics subsystem must not become a general debug endpoint.
