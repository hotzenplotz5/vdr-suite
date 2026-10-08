# TVScraper – optional VDR-Suite provider adapter

**State:** source/packaging scaffold only; no backend provider handler or UI is enabled.

Scope: consume supported metadata from the external TVScraper/VDR plugin through an explicit read adapter, normalize identity and provenance, and feed the existing Suite metadata policy and recording presentation. VDR-Suite remains the owner of metadata selection and persistence.

Never call plugin internals directly from browser clients, never create a second metadata database authority and never mutate recordings as a side effect of scraping. Missing/incompatible providers degrade to an unavailable capability without breaking the core.

`addon.json` advertises zero callable capabilities and no entrypoints. Stage metadata only with `make stage-addon MODULE=tvscraper DESTDIR=/isolated/root PREFIX=/usr`. See [AGENTS.md](AGENTS.md) and [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md).
