# Services — frühe Service-Vertragsdokumentation

## Navigation

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
- [Aktuelle Architektur](../docs/development/current-architecture-state.md)
- [Service-/REST-Entwicklergrenzen](../docs/development/phase-56.27-service-api-developer-boundary-guide.md)

## Status

`services/` enthält im aktuellen Repository **Dokumentationsskizzen**, nicht die aktuellen C++-Implementierungen der Suite. Einige Unterseiten stammen aus **Phase 1** und sprechen bewusst von „geplanten Methoden“ oder späterer Implementierung. Diese historischen Aussagen dürfen nicht als aktueller Implementierungsstand zitiert werden.

Die tatsächlichen Domänenservices, Repositories, Persistenz und Tests liegen in [`core/`](../core/README.md), der Daemon-Komposition und den weiteren produktiven Quellbereichen. Externe öffentliche HTTP-Verträge stehen unter [`api/`](../api/README.md). Für aktuelle Produktfunktionalität gelten [CURRENT](../docs/CURRENT.md) und der [Architekturstand](../docs/development/current-architecture-state.md).

## Dokumente hier

- [Metadaten-Servicenavigation](metadata/README.md) und [historischer MetadataService-Entwurf](metadata/metadata-service.md)
- [Aufnahme-Servicenavigation](recordings/README.md) mit [RecordingService](recordings/recording-service.md), [RecordingRepository](recordings/recording-repository.md) und [ActionService](recordings/action-service.md)
- `jobs/` enthält derzeit nur einen leeren README-Platzhalter. Er ist **kein** Beleg für ein ausführbares Jobs-Plugin.

## Back

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
