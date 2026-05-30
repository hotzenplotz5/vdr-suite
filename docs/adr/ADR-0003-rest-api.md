# ADR-0003: REST API als externe Schnittstelle

## Status

Angenommen

## Kontext

Die VDR-Suite soll langfristig mehrere Oberflächen unterstützen:

- OSD
- WebUI
- Mobile Clients
- externe Anwendungen

## Entscheidung

Eine REST API bildet die zentrale externe Schnittstelle.

## Beispiele

/api/recordings

/api/metadata

/api/jobs

/api/search

/api/system

## Ziele

- lose Kopplung
- Wiederverwendbarkeit
- externe Erweiterbarkeit
- moderne Integrationsmöglichkeiten

## Konsequenzen

Neue Funktionen sollen möglichst über Services und API nutzbar sein.
