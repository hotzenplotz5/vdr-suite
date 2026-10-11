# Recording Services — historische Entwürfe und aktuelle Grenzen

## Navigation

- [Service-Index](../README.md)
- [Aktuelle Architektur](../../docs/development/current-architecture-state.md)
- [Core-Domänenübersicht](../../core/README.md)
- [Recording-/Genre-/EPG-Public-v1-Vertrag](../../api/recording-browse-epg.md)

## Dokumente

- [RecordingService](recording-service.md) – frühe Service-Methoden und geplante Aufnahmeoperationen.
- [RecordingRepository](recording-repository.md) – frühe Trennung zwischen Datenbankzugriff und Fachlogik.
- [ActionService](action-service.md) – früher Entwurf für validierbare Aufnahmeaktionen und Jobs.

Diese Markdown-Dateien sind **historische Phase-1-Entwürfe**. Beispielsweise bedeutet „Implementierung folgt später“ im Originaldokument nicht, dass heutige native Schnittmarken, Recording-Read-Modelle oder Aufnahmeaktionen fehlen. Die aktuelle Implementierung befindet sich in `core/recordings/`, `core/operations/`, den zuständigen Runtime-/Agent-Grenzen und Tests. Der [aktuelle Architekturstand](../../docs/development/current-architecture-state.md) und die [Public-v1-API](../../api/README.md) sind für neue Clients verbindlicher.

Das reine Auflisten oder Browsen von Aufnahmen ist keine Wiedergabe-Berechtigung: Device-Recording-Playback und Medien-Tracks haben eigene Sicherheits-/Runtime-Verträge. Siehe [Recording Playback](../../docs/development/public-v1-device-recording-playback.md).

## Back

- [Service-Index](../README.md)
- [Dokumentationsindex](../../docs/index.md)
