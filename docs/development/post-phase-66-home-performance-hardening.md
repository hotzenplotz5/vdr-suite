# Post-Phase-66 Home Performance Hardening

Status: **COMPLETED / MERGED THROUGH PR #265.** This is a bounded, non-numbered hardening block after Phase 66. It does not reopen Phase 66 and does not start Phase 67.

## Boundary

The work keeps the existing Home Live-Hero, EPG, playback, artwork, metadata and Recording-discovery ownership unchanged. No parallel cache authority, metadata/artwork source, playback path or Phase-67 feature is introduced.

## Accepted runtime evidence

```text
pr=265
branch=work/post-phase66-home-performance
accepted_runtime_head=00d7245126c89b79216ece3004eca7ad59849dff
runtime_acceptance_ci_run_number=8577
runtime_acceptance_ci_run_id=33735740131
runtime_acceptance_ci_result=PASS
final_pr_head=6267e2dab81dd1ab48f7fa61e5fa4a92fad2e6e9
final_pr_ci_run_number=8593
merge_commit=f5efafc7d0a6d862c8eb1ba95ff649b5317cea23
```

The earlier runtime head carried the real-system acceptance. The final PR head added the remaining repository reconciliation/guards and also passed hosted CI before merge.

## Implemented hot-path hardening

1. Pure Live-Hero left/right/touch browsing rerenders only the Hero projection instead of rebuilding both programme rails.
2. Canonical EPG events are indexed once per data change by channel id instead of repeatedly filtering/sorting the complete event list.
3. Same-backend Home return within the warm interval reuses the complete existing projection rather than unnecessarily restarting EPG work.
4. Recent-Movies work coalesces same-backend in-flight scans and preserves rail position during incremental expansion.
5. The canonical full Recording projection remains authoritative where content kind/release-date semantics are required.

## Real-system acceptance

```text
HOME_PROGRAMME_PAGING_SCROLL=PASS
HOME_PERFORMANCE_HERO=PASS
HOME_PERFORMANCE_WARM_RETURN=PASS
```

The accepted observations covered programme paging, rapid Hero browsing, warm Home return, selected Hero state/rail position preservation and continued interaction after repeated module transitions.

## Follow-on status

The former `Remaining gate` is closed: PR #265 was merged. Recording Discovery, Series metadata/artwork and later Home correctness work continued in subsequent bounded post-Phase-66 workstreams and is consolidated in [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md).

Phase 67 remains a separate explicit runtime authorization boundary.
