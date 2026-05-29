# Plugin-Strategie

## Grundsatz

Bestehende VDR-Plugins sollen möglichst integriert statt ersetzt werden.

Die VDR-Suite soll nicht gegen die bestehende Plugin-Landschaft arbeiten, sondern sie strukturieren, verbinden und modern nutzbar machen.

## Integrationsmodell

    Externes Plugin / Tool
            |
         Adapter
            |
       Suite Service
            |
       SQLite / API / UI

## Rectools

Rectools bleibt zunächst ein eigenständiges Repository.

Die Suite integriert Rectools später über einen Adapter.

Ziel:

- keine harte Migration in Phase 0
- bestehende Nutzung bleibt möglich
- spätere C++-Migration einzelner Funktionen bleibt offen

Status:

    extern

## TVScraper

TVScraper bleibt zunächst ebenfalls extern.

Die Suite soll später Scraper-Daten übernehmen, vereinheitlichen und über MetadataService sowie SQLite nutzbar machen.

Status:

    extern

## Image / Music / weitere Medienmodule

Diese Bereiche werden als Suite-Module vorbereitet.

Sie können später entweder eigene Implementierungen enthalten oder bestehende VDR-Plugins integrieren.

## Legacy Adapter

Für ältere Plugins oder Tools wird eine Adapter-Schicht vorgesehen.

Ziel ist nicht, jedes Plugin umzubauen.

Ziel ist, saubere Übergänge zu schaffen.

## Regel

Keine direkte Plugin-zu-Plugin-Kommunikation als Architekturgrundlage.

Wenn Daten geteilt werden müssen, geschieht das über:

- Services
- Adapter
- Datenbank
- API
