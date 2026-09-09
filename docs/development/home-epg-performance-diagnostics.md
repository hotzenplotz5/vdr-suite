# Home EPG performance diagnostics

Status: bounded opt-in diagnostics candidate.

This diagnostic extends the existing browser-performance page without changing Home, EPG, playback, recording discovery, artwork, cache or timer behavior.

`EPG-Home messen` invokes the existing `VdrSuiteHomeLiveHero.refresh()` owner and records only same-origin Channel and EPG fetch timing plus the DOM milestones for the existing `Was läuft jetzt` and `Was läuft danach` rails. The report contains aggregate timing, counts, byte totals and rail card counts; it does not export request URLs, channel IDs, event IDs, titles or credentials.

The focused regression is `node tools/test_browser_home_epg_diagnostics.js`. It verifies Channel/EPG resource classification, exclusion of EPG image resources, rail-ready and post-animation-frame milestones, privacy of the exported report and controller wiring.
