# Post-Phase-69 EPG Current Context and Live Action Hardening

Status: **non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

## Reported UX gap

In the horizontal EPG timeline the orange current-time marker could sit almost
at the left edge even though its percentage calculation matched the event-card
time scale. The current 24-hour page began at the current half-hour anchor with
no historical context.

The EPG event detail card also lacked a direct way to open the selected
channel in the established Live-TV surface.

## Root cause

The marker geometry was not independently offset. Both event cards and the
current-time marker already use `epgTimelinePercent(..., bounds)`.

The actual problem was the bound itself:

- `EPG_TIMELINE_CONTEXT_BEFORE_SECONDS` was zero;
- the first page therefore began at the current 30-minute anchor;
- completed predecessor events were still eligible for rendering whenever the
  fetched window included them.

This made the current marker look displaced even though it was mathematically
correct.

The detail card had Timer/SearchTimer actions but no delegation to the
canonical `VdrSuiteLiveTvView` owner.

## Bounded fix

For the current page:

- fetch enough overlap for at most 60 minutes of current-programme context;
- render at least 30 minutes and at most 60 minutes before now;
- if the earliest running programme started inside that bound, retain its real
  start;
- if a running programme started earlier, clip its left edge rather than
  shifting the whole viewport indefinitely;
- omit events that already ended before now from the current-page projection;
- retain all fetched events for the explicit next-24-hours page;
- align grid ticks to clock boundaries independently from the contextual
  viewport start;
- keep the Now marker and programme cards on the same percentage function.

This is a presentation/read-model projection only. It does not change EPG
authority, cache storage or server time semantics.

## EPG detail Live action

The detail card exposes **Sender live**.

The action:

1. enters the existing Live-TV presentation;
2. delegates to `VdrSuiteLiveTvView.startChannel(channel)`;
3. creates no second player, MediaSession owner or transport path.

There is intentionally no extra EPG action because the card is already inside
the EPG owner.

There is intentionally no pre-tune HbbTV action. ADR-0054 keeps HbbTV
application lifecycle tied to an authorized channel/application session.
HbbTV availability and launch therefore remain in the tuned Live-TV context
where the provider capability is authoritative.

## Retained architecture

This hardening does not:

- create a second playback lifecycle;
- call `/api/media/sessions` directly from the EPG detail action;
- create a parallel HbbTV launch path;
- change SuiteBridge, daemon, provider or public-v1 contracts;
- reopen Phase 66, Phase 67 or Phase 69;
- start Phase 70.

## Regression boundary

Focused regression coverage must prove:

- current-page context stays between 30 and 60 minutes;
- a very long running programme is clipped instead of moving Now arbitrarily
  far right;
- a recently started programme still receives at least 30 minutes of context;
- completed predecessor programmes are not forced into the current timeline;
- next-page browsing keeps the complete fetched event set;
- Now and event cards retain the same time-axis percentage function;
- Sender live delegates only to the canonical Live-TV owner;
- the EPG detail card does not invent EPG or HbbTV duplicate actions.
