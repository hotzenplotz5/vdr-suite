# ADR-0002: SQLite als zentrale Metadaten-Datenbank

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Angenommen

## Kontext

Aktuelle VDR-Installationen speichern viele Informationen verteilt:

- info-Dateien
- resume-Dateien
- marks-Dateien
- Plugin-Konfigurationen

Dadurch entstehen doppelte Datenhaltungen und schwer nutzbare Informationen.

## Entscheidung

VDR-Suite verwendet SQLite als zentrale Metadaten-Datenbank.

## Gründe

- leichtgewichtig
- keine Server-Komponente
- bewährt
- einfach zu sichern
- sehr gute Performance
- ideal für Embedded-Systeme

## Verwendung

SQLite speichert:

- Metadaten
- Artwork-Referenzen
- Jobs
- Suchindex
- Service-Daten

## Nicht in SQLite

Medien bleiben dateibasiert:

- ts
- mkv
- jpg
- png
- srt

## Konsequenzen

Alle Suite-Komponenten greifen langfristig auf dieselbe Metadatenbasis zu.
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
