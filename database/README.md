# Database — SQL-Verträge und Testdaten

## Navigation

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
- [Schema-Index](schema/README.md)
- [Metadaten-Schemaarchitektur](../docs/architecture/metadata-platform-schema-v1.md)
- [SQLite-Core](../core/sqlite/README.md)

## Bestand

Dieses Verzeichnis enthält versionierte **SQL-Quellen und Beispieldaten**, nicht automatisch die vollständige produktive Migrationshistorie. Der Quellbaum unterscheidet:

- [`schema/vdr-suite.sql`](schema/vdr-suite.sql) – ursprüngliches Phase-1-Basisschema einschließlich `recordings`, `metadata`, `artwork` und `jobs`.
- [`schema/metadata-platform-v1.sql`](schema/metadata-platform-v1.sql) – additive Suite-Metadatenplattform mit eigenem Versions- und Identitätsmodell; kein Ersatz für die Legacy-Tabellen.
- [`testdata/sample-data.sql`](testdata/sample-data.sql) – Beispiel-/Testdaten; keine produktive Importanweisung.
- `migrations/` – enthält aktuell nur einen leeren README-Platzhalter. **Daraus darf keine vollständige Migrationsimplementierung abgeleitet werden.**

## Verbindliche Abgrenzung

Das tatsächliche produktive Schema, Upgrades, Identity-/Security- und Playback-Daten sind auch in den zuständigen Laufzeit-/Repositoryquellen dokumentiert. Die SQL-Dateien hier sind **nicht** die alleinige Quelle aller später hinzugekommenen Tabellen und dürfen nicht ungeprüft in eine bestehende yaVDR-Datenbank importiert oder gegen ihre Daten ausgeführt werden.

Für Datenbankänderungen immer die konkrete Runtime-Migration, Vorwärtskompatibilität, vorhandene Tabellen, das Backup-/Rollback-Konzept und passende Tests der betroffenen Domäne prüfen. Die [Datenbank-Entwurfsskizze](../docs/database-design.md) beschreibt ausdrücklich einen frühen, historischen Stand; der [aktuelle Architekturstand](../docs/development/current-architecture-state.md) ist für die heutige Einordnung maßgeblich.

## Back

- [Schema-Index](schema/README.md)
- [Projekt-README](../README.md)
