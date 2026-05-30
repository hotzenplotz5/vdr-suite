# VDR-Suite

VDR-Suite ist ein langfristiges Architektur- und Entwicklungsprojekt zur Modernisierung des Video Disk Recorders (VDR).

Das Projekt verfolgt das Ziel, bestehende VDR-Funktionalität mit einer gemeinsamen Service-Schicht, zentralen Metadaten, modernen APIs und zeitgemäßen Benutzeroberflächen zu erweitern.

VDR bleibt dabei das Basissystem.

Die Suite ergänzt VDR, ersetzt ihn jedoch nicht.

---

# Projektstatus

Aktuelle Phase:

**Phase 0 – Architektur und Planung**

Status:

- Repository erstellt
- Architektur definiert
- Datenmodell definiert
- Service-Layer definiert
- ADR-System eingeführt
- Roadmap erstellt
- Meilensteine definiert

Aktuell befindet sich das Projekt noch vor der eigentlichen Implementierung.

---

# Langfristige Ziele

## Core Services

- RecordingService
- MetadataService
- ArtworkService
- JobService
- SearchService

## Datenhaltung

- zentrale SQLite-Datenbank
- gemeinsame Metadatenbasis
- einheitliche Suchfunktionen

## Schnittstellen

- REST API
- OSD Integration
- WebUI
- externe Clients

## Medienbereiche

- Live-TV
- Aufnahmen
- Filme
- Serien
- Musik
- Bilder

---

# Architekturprinzipien

- VDR bleibt das Basissystem
- Service Layer vor UI
- API vor WebUI
- SQLite als zentrale Metadatenquelle
- OSD als First-Class Citizen
- lose Kopplung über Services und Adapter
- langfristige Wartbarkeit vor kurzfristigen Workarounds

Weitere Details:

- docs/project-principles.md
- docs/phase-0/
- docs/adr/

---

# Externe Projekte

Die VDR-Suite integriert bestehende Projekte soweit sinnvoll.

Aktuell geplant:

## VDR-Rectools

Eigenständiges Repository.

Integration über Adapter geplant.

## TVScraper

Eigenständiges Repository.

Integration über MetadataService geplant.

---

# Roadmap

## Phase 0

Architektur und Planung

## Phase 1

Core Foundation

- SQLite
- Core Services
- Basis-Infrastruktur

## Phase 2

Adapter Layer

- Rectools
- TVScraper

## Phase 3

REST API

## Phase 4

Dashboard

## Phase 5

Modernes OSD

## Phase 6

Media Center

Weitere Details:

- docs/planning/roadmap.md
- docs/planning/milestones.md

---

# Dokumentation

## Einstieg

- docs/phase-0/00-overview.md

## Architektur

- docs/phase-0/01-vision.md
- docs/phase-0/02-architecture.md
- docs/phase-0/03-data-model.md
- docs/phase-0/04-service-layer.md

## Entscheidungen

- docs/adr/

## Planung

- docs/planning/

---

# Lizenz

Siehe LICENSE.

---

# Projektstatus

Dieses Repository befindet sich derzeit im Aufbau.

Architektur und Dokumentation haben aktuell Priorität vor Implementierungscode.
