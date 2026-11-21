# MLD-Suchtimer: Forum feedback, 18 November 2026

Branch: `MLD-Suchtimer`. Before implementation, fast-forwarded from
`feed676f92d0e86a3c49c729aaee75e387a01ce3` to current main
`83f729330b2d0f1ce2fac6ace9f7dfad85fcc5c5`; no unique branch commits lost.
Main is unchanged by this work.

## Browser slice

- Operation inspection now displays loading and completed-read feedback, even
  when the persisted state remains `outcome_unknown`. This is an Operation GET,
  **not** a new native reconciliation or a second CREATE. No success inferred
  from a matching search term. Existing one-shot submission remains in force.
- Back/cancel collapses the form without dropping a pending operation or its
  duplicate-submit protection. Replacing the view closes its SSE subscription;
  reconnect remains with the existing EventSource implementation.
- Channel ranges expose named start/end selectors. Selecting a group first is
  optional. Preview and saved-rule channel labels resolve through the existing
  channel list, retaining IDs if that read fails.
- Folder choices exclude the browse API's `singleRecordingLeaf` entries;
  existing configured/current paths are retained. No inferred filesystem scan.
- German visible labels use Suchtimer. Further settings have a larger heading;
  duration and repeat controls are separate fieldsets.

Acceptance: user-style form submit/inspect/back/reopen must produce one CREATE;
sender range must retain both IDs; recording-title leaves must not be offered.
Existing security and Public-v1 paths remain unchanged.

UNIT/CONTRACT: Node tests passed: `test_searchtimer_workflows_runtime`,
`test_phase69f_searchtimer_fallback_removal`,
`test_searchtimer_maintenance_security_runtime`,
`test_searchtimer_execution_security_runtime`, `test_epg_timeline_enhancements`,
`test_i18n_runtime`. `git diff --check` passed.
PRODUCTION-INTEGRATION/REAL-VDR: not verified for this candidate. No installation.

## Remaining contract questions

- Full-field UPDATE is not present in the current Public-v1 mutation contract;
  only query changes on inactive rules are supported and proven on yaVDR.
- The current provider body maps `compareTitle/Subtitle/Summary` to both
  `use_title/subtitle/description` and `compare_title/subtitle/summary`.
  Independent search fields and repeat comparisons cannot be represented
  losslessly by those shared fields. Do not add misleading independent UI
  controls before the request, durable payload, mapper and readback agree.
- MLD CREATE failure cause needs the actual provider response/readback and
  operation evidence. A persisted unknown status alone cannot establish which
  requested field mismatched. Main already ignores inactive fuzzy tolerance
  during CREATE comparison; no claim that this resolves the reported MLD case.

Comparison reference: Pit/Skippy's supplied epgsearch feedback describes named
from/to channels and independent repeat suboptions. Named ranges now follow
that reported behavior; full epgsearch repeat parity remains unverified.
