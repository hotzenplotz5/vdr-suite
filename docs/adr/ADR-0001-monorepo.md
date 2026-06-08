# ADR-0001: Monorepo für VDR-Suite

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Angenommen

## Kontext

Die VDR-Suite besteht langfristig aus mehreren eng zusammenhängenden Bereichen:

- Core
- Services
- Datenbank
- REST API
- OSD
- WebUI
- Module
- Adapter
- Dokumentation
- Packaging

Diese Komponenten müssen architektonisch zusammenpassen.

## Entscheidung

VDR-Suite wird als zentrales Monorepo organisiert.

Externe Projekte wie Rectools oder TVScraper bleiben zunächst eigenständige Repositories und werden über Adapter integriert.

## Begründung

Ein Monorepo erleichtert:

- gemeinsame Architektur
- konsistente Dokumentation
- einheitliche Schnittstellen
- gemeinsame Roadmap
- bessere Orientierung für Entwickler
- bessere Nutzung durch KI-Assistenten

## Konsequenzen

Das Repository enthält zunächst viele Dokumentations- und Strukturdateien.

Nicht jeder Ordner enthält sofort Implementierungscode.

Die frühe Struktur dient bewusst als Architekturrahmen für spätere Phasen.

## Abgrenzung

Das Monorepo bedeutet nicht, dass bestehende VDR-Projekte sofort integriert werden.

Rectools und TVScraper bleiben zunächst eigenständige Projekte.
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
