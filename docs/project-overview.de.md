# VDR-Suite Projektübersicht

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)

---

VDR-Suite – Moderne Plattform für den VDR

Was ist VDR-Suite?

VDR-Suite ist eine moderne Open-Source-Plattform rund um den Video Disk Recorder (VDR).

Das Projekt verfolgt nicht das Ziel, den VDR zu ersetzen. Stattdessen soll eine moderne, langfristig wartbare Plattform entstehen, die den VDR um zeitgemäße Schnittstellen, Dienste und Frontends ergänzt.

Der VDR bleibt weiterhin für Empfang, Aufnahmen, Timer und Live-TV verantwortlich.

VDR-Suite stellt darüber eine zusätzliche Schicht bereit, die Daten zusammenführt, verarbeitet und modernen Anwendungen zugänglich macht.

Warum entsteht VDR-Suite?

Viele bestehende VDR-Komponenten stammen aus einer Zeit, in der:

- Webanwendungen noch selten waren
- mobile Geräte keine Rolle spielten
- mehrere VDR-Systeme kaum genutzt wurden
- Echtzeit-APIs ungewöhnlich waren

VDR-Suite soll diese Lücke schließen.

Ziele

- Moderne Backend-Architektur
- Klare Schnittstellen
- Mehrere Frontends
- Mehrere VDR-Systeme
- Echtzeitdaten
- Hohe Erweiterbarkeit
- Langfristige Wartbarkeit

Mehrere VDR-Systeme

VDR-Suite wird von Anfang an für den Betrieb mit mehreren VDR-Instanzen vorbereitet.

Beispiele:

- Hauptsystem im Wohnzimmer
- Zweitsystem im Keller
- Zweiter Standort
- Ferienhaus
- Testsystem

Langfristig sollen alle Systeme zentral verwaltet werden können.

Moderne Frontends

VDR-Suite trennt Backend und Frontend.

Dadurch können unterschiedliche Clients entstehen:

- Windows-Anwendungen
- Web-Frontends
- Android-Apps
- iOS-Apps
- Smart-TV-Anwendungen
- zukünftige OSD-Erweiterungen

Alle greifen auf dieselben Daten und Dienste zu.

Echtzeit statt Polling

VDR-Suite unterstützt Ereignisse und Statusänderungen in Echtzeit.

Dadurch können Frontends unmittelbar auf Änderungen reagieren, ohne ständig komplette Datenbestände neu laden zu müssen.

Effizienz

Ein wichtiges Entwicklungsziel ist die Vermeidung unnötiger Vollabfragen.

Aktuelle Entwicklungsphasen konzentrieren sich auf:

- selektive EPG-Aktualisierung
- gezielte Event-Abfragen
- reduzierte Netzwerklast
- geringeren Ressourcenverbrauch

Dies wird insbesondere bei mehreren VDR-Systemen wichtig.

Kein Plex- oder Jellyfin-Klon

VDR-Suite soll kein Ersatz für Plex, Jellyfin oder Kodi werden.

Der Fokus bleibt bewusst auf den klassischen Stärken des VDR:

- DVB
- Live-TV
- Aufnahmen
- Timer
- EPG

Ziel ist eine moderne Plattform rund um den VDR, nicht die Nachbildung anderer Mediensysteme.

Aktueller Entwicklungsstand

Bereits umgesetzt wurden unter anderem:

- Backend-Grundarchitektur
- REST-Schnittstellen
- VDR-Adapter-Schicht
- Snapshot-System
- Cache-System
- Änderungsverfolgung
- Live-Updates
- Multi-Backend-Vorbereitungen
- Selektive Aktualisierungsstrategien
- Umfangreiche Testinfrastruktur

Langfristige Vision

VDR-Suite soll langfristig eine moderne Plattform bilden, auf der verschiedene Anwendungen und Frontends rund um den VDR aufsetzen können.

Der VDR bleibt das Herzstück.

VDR-Suite soll ihn für moderne Anforderungen zugänglich machen.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Project Overview](project-overview.md)
