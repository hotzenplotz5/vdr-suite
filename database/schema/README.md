# Database Schema — versionierte SQL-Ausgangsverträge

## Navigation

- [Database-Index](../README.md)
- [Projekt-README](../../README.md)
- [Schema-v1-Architektur](../../docs/architecture/metadata-platform-schema-v1.md)
- [Metadaten-Datenbankstrategie ADR-0038](../../docs/adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)

## Verfügbare Schemadateien

| Datei | Wofür sie steht |
| --- | --- |
| [`vdr-suite.sql`](vdr-suite.sql) | Historisches Phase-1-Basisschema für Aufnahmen, Metadaten, Artwork und Jobs |
| [`metadata-platform-v1.sql`](metadata-platform-v1.sql) | Separates additives Metadaten-Schema mit `suite_metadata_*`-Tabellen und eigener Schema-Versionsverwaltung |

Die Metadata-Platform-Datei sieht das **Nebeneinander** mit den älteren `metadata`- und `artwork`-Tabellen vor. Ein Feldname oder Legacy-Primärschlüssel wird dadurch nicht automatisch zur kanonischen Suite-Metadaten-ID.

Diese Quellen dokumentieren den gespeicherten SQL-Vertrag ihrer jeweiligen Entwicklungsstufe. Sie sind weder vollständige Inventare aller produktiven Sicherheitstabellen noch eine Anleitung, SQL auf einer vorhandenen Installation erneut einzuspielen. Für produktive Anwendung stets den zugehörigen Runtime-Migrationspfad und den aktuellen Datenbankzustand nachweisen.

## Back

- [Database-Index](../README.md)
- [Dokumentationsindex](../../docs/index.md)
