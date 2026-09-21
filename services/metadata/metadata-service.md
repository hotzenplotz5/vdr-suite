# MetadataService

## Ziel

Der MetadataService verwaltet alle Metadaten der VDR-Suite.

Er stellt Informationen zu Filmen, Serien, Episoden, Personen und Genres bereit.

Der Service bildet die zentrale Schnittstelle zwischen Aufnahmen und externen Datenquellen.

## Verantwortlichkeiten

- Metadaten laden
- Metadaten aktualisieren
- Metadaten suchen
- Aufnahmen mit Metadaten verknüpfen
- externe Quellen integrieren

## Datenquellen

- SQLite
- TVScraper Adapter
- TMDB
- TVDB
- IMDb
- lokale Metadaten

## Geplante Methoden

### getMetadata

Lädt Metadaten anhand einer ID.

### getMetadataForRecording

Lädt Metadaten für eine Aufnahme.

### searchMetadata

Durchsucht Filme, Serien und Episoden.

### updateMetadata

Aktualisiert vorhandene Metadaten.

### linkRecording

Verknüpft eine Aufnahme mit Metadaten.

### importMetadata

Importiert Daten aus externen Quellen.

## Verwaltete Objekte

- Movie
- Series
- Episode
- Person
- Genre

## Architekturregeln

- keine direkte Kommunikation mit UI
- keine direkte Kommunikation mit Plugins
- Zugriff ausschließlich über definierte Services

## Phase-1-Ziel

In Phase 1 wird zunächst nur die Service-Schnittstelle definiert.

Eine Implementierung erfolgt in einer späteren Phase.
