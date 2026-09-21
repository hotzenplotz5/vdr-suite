# Database Design

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)

---

## Version

Schema Version 1

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