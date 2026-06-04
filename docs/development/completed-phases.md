# VDR-Suite – Completed Phases

This file keeps the longer phase history out of `docs/development/current-status.md`.

## Phase 8 status history

### Phase 8.90 – change-state polling integration

Integrated change-state aware polling into PollingService.

### Phase 8.91 – change event generation

Introduced backend-neutral VdrChangeEvent generation from VdrChangeState transitions.

### Phase 8.92 – snapshot refresh decision model

Introduced SnapshotRefreshDecision and SnapshotRefreshDecisionService.

### Phase 8.93 – snapshot cache model

Introduced SnapshotCache and SnapshotCacheService.

### Phase 8.94 – snapshot cache integration (planned)

Move snapshot ownership from PollingService into SnapshotCache and integrate SnapshotCacheService into runtime wiring.
