# Public API v1: VIDAA-Device-Suchberechtigungen

Verbindlicher produktspezifischer Vertrag für Public-v1- und VIDAA-Arbeiten.
Vor Änderungen an Device-Suche, Grant-Listen und Geräteanmeldung lesen.
Zusätzlich gelten die Sicherheits-, Architektur- und Genehmigungsregeln
aus [AGENTS.md](../../AGENTS.md).

## Public-v1 VIDAA search permission contract (2026-10-15)

Public-v1 Device `/search/recordings` requires `recordings.view`, and
`/search/epg` plus `/epg/now-next` requires `epg.view`, independently
of `channels.view` or `media.live.play`. The product grant vocabulary,
Device supported-grant allowlist and Device-grant HTTP `supportedPermissions`
**must include both read-only permissions**, and tests must prove assignment
through the normal administration service and Device authentication. Never
work around missing grants with a weaker HTTP gate, inherited browser grants,
implicit pairing rights, or unauthenticated Client fallback. Existing devices
are granted rights only by an explicit authorized administrator mutation.

