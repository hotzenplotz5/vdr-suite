# Core — Implementierte Domänen und Laufzeitgrenzen

## Navigation

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
- [Aktueller Architekturstand](../docs/development/current-architecture-state.md)
- [Zielarchitektur](../docs/architecture/target-platform-architecture.md)
- [Entwickler-Einstieg](../docs/development/developer-onboarding.md)

## Zweck

`core/` enthält die internen C++-Domänen-, Sicherheits- und Laufzeitbausteine der VDR-Suite. Die konkreten Eigentümer liegen in den jeweiligen `include/`, `src/` und `tests/`-Teilbäumen; das ist **keine öffentliche, stabile C++-SDK- oder ABI-Zusage**.

VDR bleibt die maßgebliche Quelle für native Geräte, Timer und Aufnahmen. VDR-Suite ergänzt kontrollierte Identität, Read-Modelle, Operationen und Client-Schnittstellen. Die HTTP-Verträge werden unter [`api/rest/`](../api/rest/README.md) und [`api/`](../api/README.md) dokumentiert, nicht als direkte Core-Einstiegspunkte veröffentlicht.

## Bereiche im aktuellen Quellbaum

| Verzeichnis | Verantwortung |
| --- | --- |
| `agent/` | Backend-Agent, Enrollment, Observationen und gesicherte Command-/Result-Übertragung |
| `daemon/` | Zusammensetzung und Lebenszyklus des Daemons; HTTP-Runtime einschließlich Device-Medienrouten |
| `http/` | HTTP-Grundlagen, Tests und Protokoll-Hilfen |
| `media/` | Medien- und Playback-Domänen |
| `metadata/` | Suite-Metadatenidentität und fachliche Metadatenservices |
| `operations/` | Dauerhafte Operationen und Zustandsverwaltung |
| `recordings/` | Aufnahme-Domäne, Schnittmarken, native Aufnahmeaktionen und Read-Modelle |
| `runtime/` | Laufzeitdiagnostik und Runtime-Bausteine |
| `security/` | Authentifizierung, Autorisierung, Berechtigungen und Audit |
| `sqlite/` | Interne SQLite-Basis; [Teilbereich](sqlite/README.md) |
| `timers/` | TimerAssignments und orchestrierte native Timer-Operationen |
| `vdr/` | VDR-Adapter, Snapshot- und Backend-Fundamente |

Das ist ein Navigationsverzeichnis nach dem vorhandenen Quellbaum, **keine** Zusicherung eigenständig installierbarer Core-Pakete.

## Änderungen und Tests

Der oberste [Makefile-Build](../Makefile) bindet die Quellgruppen über [`mk/`](../mk/common.mk) ein. Vor Änderungen den exakten Source-Owner und die zugehörigen Tests im gleichen Teilbaum ermitteln; nicht einfach historische `services/`-Skizzen als aktuelle Implementierung lesen. Sicherheits- und HTTP-Änderungen erfordern insbesondere Abgleich mit [SecurityHttpGate](security/include/SecurityHttpGate.h), [PublicApiRuntime](../api/rest/src/PublicApiRuntime.cpp) und [ADR-0037](../docs/adr/ADR-0037-packaging-install-api-boundary.md).

Aktuelle und historische Akzeptanzstände stehen im [CURRENT](../docs/CURRENT.md) beziehungsweise in den abgeschlossenen Phasennachweisen. Eine Quelldatei allein belegt keinen auf yaVDR installierten Runtime-Stand.

## Back

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
