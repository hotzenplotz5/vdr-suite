# Architektur

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)

---

## Überblick

Die VDR-Suite folgt einer klaren Schichtenarchitektur.

    VDR Core
       |
    VDR-Suite Core
       |
    Service Layer
       |
    Modules / Adapters
       |
    UI / API

## Schichten

### VDR Core

Der bestehende VDR bleibt das Fundament.

Er verwaltet unter anderem:

- Live-TV
- Aufnahmen
- Timer
- Kanäle
- Plugins

### VDR-Suite Core

Der Suite-Core stellt gemeinsame Infrastruktur bereit:

- Initialisierung
- Konfiguration
- Logging
- zentrale Schnittstellen
- Modulverwaltung

### Service Layer

Der Service Layer kapselt Fachlogik.

Beispiele:

- RecordingService
- MetadataService
- ArtworkService
- JobService
- SearchService

### Modules

Module erweitern die Suite funktional.

Beispiele:

- Rectools-Integration
- TVScraper-Integration
- Image-Modul
- Music-Modul

### Adapter

Adapter verbinden externe Plugins oder Tools mit der Suite.

Wichtig:

Externe Projekte werden nicht direkt abhängig vom Suite-Core gemacht.

### UI / API

Die Darstellung erfolgt über klar definierte Schnittstellen.

Zielsysteme:

- VDR OSD
- WebUI
- REST API
- spätere externe Clients

## Architekturregel

Module dürfen nicht direkt hart miteinander gekoppelt werden.

Kommunikation erfolgt über Services oder definierte Adapter.
---

## Back

- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
