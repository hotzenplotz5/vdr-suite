# Database Design

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)

---

## Stand und Geltungsbereich

**Historischer Phase-1-Entwurf.** Die folgende „Schema Version 1“-Beschreibung bildet **nicht** die vollständige heutige VDR-Suite-Datenbank ab. Für tatsächlich versionierte SQL-Quellen siehe [Datenbank-Einstieg](../database/README.md) und [Schema-Index](../database/schema/README.md). Die additive Suite-Metadatenplattform ist in der [Schemaarchitektur](architecture/metadata-platform-schema-v1.md) spezifiziert. Credential-/Session-, Timer-, Operations- und spätere Metadaten-Repositories gehören zu ihren jeweiligen Runtime-Domänen; aus diesem historischen Tabellenüberblick darf kein vollständiges Upgrade- oder Installationsverfahren abgeleitet werden.

## Version

Schema Version 1 (historischer Phase-1-Plan)

## Ziel

Die SQLite-Datenbank dient als zentrale Metadatenbasis der VDR-Suite.

Mediendateien bleiben weiterhin dateibasiert.

Die Datenbank speichert strukturierte Informationen über Aufnahmen, Metadaten, Artwork und Hintergrundaufgaben.

---

## Tabellen

### recordings

Speichert VDR-Aufnahmen.

Wichtige Felder:

- title
- subtitle
- description
- channel
- recording_path
- recording_format

---

### metadata

Speichert Film-, Serien- und Episodeninformationen.

Wichtige Felder:

- media_type
- title
- original_title
- year
- genre
- source

---

### artwork

Speichert Referenzen auf Bilder.

Wichtige Felder:

- poster_path
- fanart_path
- banner_path
- thumbnail_path

---

### jobs

Speichert Hintergrundaufgaben.

Wichtige Felder:

- job_type
- status
- priority

---

## Beziehungen

recordings
    |
    +---- metadata
    |
    +---- artwork

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Project Overview](project-overview.md)