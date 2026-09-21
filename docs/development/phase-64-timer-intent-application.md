# Phase 64 TimerIntent application entry point

The minimal application entry point accepts one complete TimerIntent plus stable
intent and assignment identifiers. It durably creates and activates the intent,
runs deterministic primary scheduling against explicit capability, health,
authority, channel and conflict facts, then moves the selected assignment into
provisioning.

Every step is restart-safe. Repeating the exact request resumes from the
persisted intent or assignment and returns the already-provisioning result.
Changed semantic intent content, a competing assignment identity, stale
revisions or an ineligible backend fail closed. No reassignment or failover is
introduced by this entry point.
