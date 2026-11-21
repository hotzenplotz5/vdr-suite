# VDR-Suite TV Frontends

This directory is reserved for TV-oriented frontend targets.

Planned structure:

```text
web/tv/
  samsung/
  hisense/
```

The daemon serves the web frontend from `web/frontend/` first. TV-specific frontends should stay separate from the daemon and from the standard web frontend.
