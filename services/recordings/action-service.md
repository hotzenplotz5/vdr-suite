# ActionService

## Ziel

Der ActionService verwaltet Aktionen, die auf VDR-Aufnahmen ausgeführt werden können.

Er bildet die Brücke zwischen RecordingService, JobService und externen Werkzeugen wie VDR-Rectools.

## Beispiele für Aktionen

- CHECK
- REPAIR
- SHRINK
- CUT
- PES2TS
- RENAME

## Architektur

RecordingService
        |
ActionService
        |
JobService
        |
Adapter
        |
Rectools / externe Tools

## Verantwortlichkeiten

- verfügbare Aktionen für eine Aufnahme bestimmen
- Aktionen validieren
- Jobs für Aktionen erzeugen
- Statusinformationen bereitstellen

## Phase-1-Ziel

In Phase 1 wird zunächst nur das Domainmodell vorbereitet.

Die technische Job-Erzeugung folgt in einem späteren Schritt.
