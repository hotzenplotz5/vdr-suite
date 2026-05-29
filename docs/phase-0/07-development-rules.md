# Entwicklungsregeln

## Architektur

- Service Layer vor UI
- API vor WebUI
- Keine direkte Modul-zu-Modul-Kommunikation
- Datenzugriffe über definierte Services

## Datenhaltung

- SQLite ist die primäre Metadaten-Datenbank
- Medien bleiben dateibasiert
- UTF-8 ausschließlich

## Code

- C++17 als Mindeststandard
- Klare Modulgrenzen
- Dokumentation vor Implementierung
- ADR für grundlegende Architekturentscheidungen

## Integration

- VDR bleibt Kernsystem
- Rectools zunächst extern
- TVScraper zunächst extern

