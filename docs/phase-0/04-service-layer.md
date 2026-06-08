# Service Layer

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)

---

## Ziel

Der Service Layer bildet die zentrale Fachlogik der VDR-Suite.

Module, Adapter, UI-Komponenten und APIs greifen nicht direkt auf Datenquellen zu, sondern verwenden definierte Services.

Dadurch entsteht eine saubere Trennung zwischen Datenhaltung, Logik und Darstellung.

## Architektur

    UI
     |
    API
     |
    Services
     |
    Database / Adapter
     |
    VDR / Plugins / Filesystem

## RecordingService

Verwaltet Aufnahmen.

Aufgaben:

- Aufnahmen auflisten
- Aufnahmeinformationen liefern
- Aufnahmeaktionen starten
- Aufnahmestatus verwalten

## MetadataService

Verwaltet Metadaten.

Aufgaben:

- Filmdaten
- Serieninformationen
- Personen
- Genres
- externe Datenquellen

## ArtworkService

Verwaltet Bilder.

Aufgaben:

- Poster
- Fanart
- Banner
- Thumbnails

## JobService

Verwaltet Hintergrundaufgaben.

Aufgaben:

- Queue
- Status
- Prioritäten
- Ergebnisse

## SearchService

Zentrale Suchfunktion.

Aufgaben:

- Aufnahmen
- Filme
- Serien
- Personen
- Metadaten

## Regel

Services kommunizieren untereinander über definierte Schnittstellen.

Direkte Datenbankzugriffe aus UI oder Modulen sind zu vermeiden.
---

## Back

- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
