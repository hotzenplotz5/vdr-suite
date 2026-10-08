# Agent rules: TVScraper

Inherit all requirements from [root AGENTS.md](../../AGENTS.md).

1. Implement only an optional external metadata provider adapter. Do not migrate TVScraper's state into a second authoritative Suite database.
2. Preserve Suite recording identity, provider provenance, owner-selected metadata, conflict resolution and backend-aware permissions.
3. Read through bounded provider APIs with availability, version/capability probing and failure isolation. No direct browser-to-plugin calls or plugin-to-plugin communication as architecture.
4. Provider absence, incompatible versions and timeouts must degrade to unavailable status without blocking Suite.
5. No recording mutation, cut, rename, move, repair or import in this module.
6. Keep README, this AGENTS, addon.json and source-package stage in sync; no advertised runtime capabilities without implemented tested handlers.
7. Real provider probes, restarts, package changes or media writes require authorization and safety preflight.

8. An installed TVScraper add-on is not an available scraper provider until an explicitly authorized backend adapter is registered and healthy. The registry is read-only; no scraping job, credentials or metadata writes may be inferred from it.
