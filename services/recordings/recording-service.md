# RecordingService

## Ziel

Der RecordingService ist die zentrale Schnittstelle für VDR-Aufnahmen innerhalb der VDR-Suite.

Er kapselt den Zugriff auf Aufnahmedaten und verhindert direkte Zugriffe von UI, API oder Modulen auf Dateisystem oder Datenbank.

## Verantwortlichkeiten

Der RecordingService verwaltet:

- Aufnahmen auflisten
- einzelne Aufnahme laden
- Aufnahmen suchen
- Aufnahme umbenennen
- Aufnahme in den Papierkorb verschieben
- Aufnahmeformat erkennen
- Aufnahmeaktionen starten
- Verknüpfung zu Metadaten herstellen

## Datenquellen

Der Service kann langfristig Daten aus mehreren Quellen zusammenführen:

- SQLite
- VDR-Aufnahmeverzeichnis
- info-Dateien
- Rectools Adapter
- TVScraper Adapter

## Geplante Methoden

### listRecordings

Gibt eine Liste aller bekannten Aufnahmen zurück.

### getRecording

Lädt eine einzelne Aufnahme anhand ihrer ID.

### searchRecordings

Sucht Aufnahmen nach Titel, Untertitel oder Beschreibung.

### renameRecording

Benennt eine Aufnahme kontrolliert um.

Wichtig:

- Dateisystem-Änderung
- Datenbank-Update
- spätere Rollback-Möglichkeit

### moveRecordingToTrash

Verschiebt eine Aufnahme in den Papierkorb.

Direktes Löschen ist nicht Standardverhalten.

### detectRecordingFormat

Erkennt das Aufnahmeformat.

Mögliche Werte:

- PES
- TS
- UNKNOWN

### startRecordingJob

Startet eine Hintergrundaufgabe für eine Aufnahme.

Beispiele:

- CHECK
- REPAIR
- SHRINK
- CUT
- PES2TS

## Architekturregeln

- Kein UI-Code im Service
- Keine direkte Plugin-zu-Plugin-Kommunikation
- Keine direkten Dateisystemzugriffe aus UI oder API
- Änderungen an Aufnahmen laufen über den Service

## Phase-1-Ziel

In Phase 1 wird zunächst nur das Service-Design beschrieben.

Eine Implementierung folgt erst nach Stabilisierung von Datenbank und Schnittstellen.
