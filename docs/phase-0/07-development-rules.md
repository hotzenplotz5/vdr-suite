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

## Documentation Patch Rule

For documentation changes, prefer generated patches or scripted replacements over hand-written inline patches.

Rules:

* Do not hand-write large unified diffs in chat.
* Do not mix manual edit instructions and patch instructions.
* If a patch is needed, it must be generated from an actual repository diff.
* For large documentation files, prefer scripted replacements using perl or python.
* Always verify documentation changes with git diff before committing.
