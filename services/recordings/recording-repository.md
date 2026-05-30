# RecordingRepository

## Ziel

Das RecordingRepository kapselt den direkten Datenbankzugriff für Aufnahmen.

Der RecordingService kennt keine SQL-Statements.

Alle Datenbankzugriffe laufen über das Repository.

## Architektur

RecordingService
        |
RecordingRepository
        |
Database
        |
SQLite

## Verantwortlichkeiten

- Aufnahmen laden
- Aufnahmen speichern
- Aufnahmen aktualisieren
- Aufnahmen löschen
- Aufnahmen suchen

## Geplante Methoden

### getById

Lädt eine Aufnahme anhand ihrer ID.

### getAll

Lädt alle Aufnahmen.

### findByTitle

Sucht Aufnahmen nach Titel.

### insert

Legt eine neue Aufnahme an.

### update

Aktualisiert eine Aufnahme.

### remove

Entfernt eine Aufnahme.

## Architekturregel

Repository enthält SQL.

Services enthalten Geschäftslogik.

UI enthält Darstellung.
