# Externe Modulstände

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)

---

## VDR-Rectools

Stand: Ende Phase Explorer

### Fertiggestellt

- VDR-Aufnahmen statt reiner PES-zu-TS-Ansicht
- Ordnernavigation mit Breadcrumbs
- natürliche Serien-Sortierung
- Titel aus info-Datei
- Untertitel aus info-Datei
- Aufnahmedatum
- TS/PES-Erkennung
- Check
- Repair
- Shrink
- Cut
- Umbenennen
- Papierkorb statt Löschen
- Queue-System-Integration
- .trash ausgeblendet
- Config-Fallback:
  - zuerst /etc/vdr/vdr-rectools.conf
  - danach Fallback-Konfiguration

### Status

Der Explorer für Rectools ist vorerst ausreichend fertig.

Rectools bleibt zunächst ein eigenständiges Repository.

Für VDR-Suite Phase 0 gilt:

- keine Migration von Rectools in das Suite-Repository
- keine direkte technische Abhängigkeit
- spätere Integration über Adapter
- nur noch Bugfixes im bestehenden Rectools-Explorer

### Bedeutung für VDR-Suite

Rectools liefert bereits reale Anforderungen für spätere Suite-Komponenten:

- RecordingService
- JobService
- Queue-Integration
- Recording-Aktionen
- Format-Erkennung
- Dateisystem-Adapter
- Papierkorb-/Rollback-Konzept
---

## Back

- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
