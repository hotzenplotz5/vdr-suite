# VDR-Suite Projektprinzipien

## Grundsätze

1. VDR bleibt das Basissystem.
2. Die Suite ergänzt VDR und ersetzt ihn nicht.
3. Rückwärtskompatibilität hat hohen Stellenwert.
4. OSD ist ein First-Class Citizen.
5. Service Layer vor UI.
6. API vor WebUI.
7. SQLite ist zentrale Metadatenquelle.
8. Direkte Modul-zu-Modul-Kommunikation ist zu vermeiden.
9. Klare Trennung von Daten, Services und Darstellung.
10. Moderne Media-Center-Bedienung statt klassischer Receiver-Menüs.
11. Bestehende VDR-Plugins sollen möglichst integriert statt ersetzt werden.
12. Rectools bleibt zunächst ein eigenständiges Projekt.
13. TVScraper bleibt zunächst ein eigenständiges Projekt.
14. Architekturentscheidungen werden als ADR dokumentiert.
15. Langfristige Wartbarkeit ist wichtiger als kurzfristige Feature-Gewinne.

## SVDRP und Remote-Control

SVDRP ist fuer VDR-Suite ein wichtiger Kompatibilitaets- und Steuerkanal, aber nicht die primaere interne API der Suite.

Grundsaetze:

1. SVDRP wird hinter Adapter-Grenzen gekapselt.
2. VDR-Suite darf nicht direkt von VDR-internen Klassen abhaengen.
3. Schreibende oder destruktive SVDRP-Kommandos werden nicht direkt aus der UI ausgefuehrt.
4. Schreibende Aktionen laufen ueber Services, Validierung und eine Action- oder Queue-Schicht.
5. HITK wird nur als Legacy-, OSD- oder Fallback-Steuerung verwendet.
6. HITK darf nicht Grundlage fuer moderne UI-Zustandslogik werden.
7. SVDRP-Hostfreigaben ersetzen keine Benutzer-, Rollen- oder Rechteverwaltung.
8. Authentifizierung, Autorisierung und Berechtigungen liegen in VDR-Suite.
9. RESTfulAPI, SVDRP, Plugin-Bridges und Dateisystem-Snapshots sind Transport- oder Backend-Quellen, aber keine Domänenmodelle.
10. Backend-Faehigkeiten muessen als Capabilities modelliert werden, ohne Transportdetails in UI oder Domäne offenzulegen.
