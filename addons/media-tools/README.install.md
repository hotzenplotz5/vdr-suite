# Media Tools – Installation nach Distribution

**Stand: 11. November 2026.** Diese Übersicht führt zu den bereits vorhandenen Installations- und Paketdokumenten. Sie ist **kein universelles Installationsskript** und ersetzt weder die tatsächliche Host-Prüfung noch die Freigabe produktiver Änderungen.

[Zur Media-Tools-README](README.md) · [Aktueller Funktions- und Abnahmestand](ROADMAP.md) · [VDR-Suite Hauptprojekt](../../README.md)

## Einstieg nach System

| System | Nachgewiesener Stand | Passender Einstieg |
| --- | --- | --- |
| **yaVDR / Ubuntu (Referenzsystem)** | Media Import und die H.264-/HEVC-Produktpfade wurden am realen yaVDR in abgegrenzten Fällen abgenommen. **Das belegt keine generelle Erstinstallation oder ein gefahrloses In-place-Upgrade.** | [yaVDR-Installationsvoraussetzungen](../../docs/media-tools-installation.md), [Bestandsübernahme und Updates](INSTALLER-RESUME.md) |
| **Debian / Ubuntu-Pakete** | Die Suite besitzt experimentelle Debian-/PPA-Pakete. Ein getrenntes, inaktives Media-Tools-Payload-Paket kann aus der geprüften Stage gebaut werden. **Die vollständige Integration dieses Add-ons in die reguläre Quellpaket-/PPA- und Migrationskette ist nicht allgemein abgenommen.** | [Debian-Paketierung](../../debian/README.md), [Pakete und Release-Status](../../packaging/README.md), [Stage-Installationsweg](../../docs/media-tools-portable-install.md) |
| **MLD 6.6** | Gemeinsame Quellcode- und synthetische Portabilitätstests existieren. Die MLD-First-Run-Integration der *VDR-Suite* ist separat dokumentiert; sie ist **kein Live-Test von Media Tools auf MLD**. | [MLD-Paket-/First-Run-Vertrag](../../packaging/mld66/README-first-run.md), [MLD-Pfadvertrag](../../docs/development/mld66-distribution-path-contract-20261023.md) |
| **openSUSE** | Ein synthetischer Hostprofil-Test ist vorhanden. **Kein verifizierter openSUSE-Installationsadapter, keine reale Paket-/Dienst-/ABI- oder Media-Tools-Hardwareabnahme.** | [Distributionsvertrag und Restaufgaben](ROADMAP.md), [Portabler Staging-Weg](../../docs/media-tools-portable-install.md) |
| **Andere Linux-Distributionen** | Der Worker-Payload kann unabhängig von systemd und dpkg vorbereitet werden. Daraus folgt **keine** bestätigte Laufzeitkompatibilität. | [Portable Staging- und Bestandsübernahme](../../docs/media-tools-portable-install.md) |

## Was ist überall gleich?

Media Tools ist ein Add-on der bestehenden VDR-Suite – **kein separater Server**. Der zentrale Daemon führt Benutzerrechte, dauerhafte Operationen und den endgültigen VDR-Aufnahmeabgleich; der bereits vorhandene Backend-Agent führt genehmigte Medienaufträge aus. Die Worker für Import, Nur-Lese-Prüfung und HEVC-Verkleinerung teilen sich die geprüfte Add-on-Stage. Ein zweiter Importer, ein neuer Hintergrunddienst oder eine distributionsspezifische Abzweigung der Produktlogik ist nicht erforderlich.

Die gemeinsame technische Grundlage umfasst:

- Eine geprüfte, wiederverwendbare Suite-/Add-on-Stage mit Datei- und Hashinventar; ein Paket und eine manuelle Installation sollen denselben geprüften Payload verwenden.
- Backendbezogene FFmpeg-, FFprobe- und VDR-Werkzeugpfade über die bestehende Agent-Konfiguration.
- Optional die vorhandenen H.264-Import- und H.265-Shrink-Parameter in `backend-agent.conf`, ohne einen zweiten Settings-Speicher.
- Separate Entscheidungen für **Installieren**, **Aktivieren**, **Benutzerrechte** und **einen konkreten Medienauftrag**. Die Installation startet keinen Import von selbst.

## Vor einer Installation oder einem Update

1. **Bestand erkennen:** Gibt es bereits VDR-Suite, Backend-Agent, Media-Tools-Worker, ein manuelles Layout oder paketverwaltete Dateien? Bei vorhandener Installation ist ein Erstinstallationsskript nicht automatisch der richtige Weg.
2. **Reale Hostdaten verwenden:** Distribution, VDR-ABI, Service-/Paketmanager, Dienstnutzer, Videoverzeichnis, Quellwurzeln und Werkzeugpfade müssen zum tatsächlichen Zielsystem passen. yaVDR-Pfade dürfen nicht als MLD-/openSUSE-Defaults übernommen werden.
3. **Installationsart wählen:** Für eine neue, geprüfte yaVDR-Installation gilt die [yaVDR-Anleitung](../../docs/media-tools-installation.md); für bereits vorhandene Installationen die [Resume-/Upgrade-Dokumentation](INSTALLER-RESUME.md); für einen ausschließlich geprüften Add-on-Payload die [portable Staging-Anleitung](../../docs/media-tools-portable-install.md).
4. **Abnahme getrennt dokumentieren:** Quellcode, isolierte Tests, Paketbau, installierte Runtime und tatsächlicher VDR-Readback sind unterschiedliche Nachweise. Ein installierter Worker ist noch keine erfolgreich durchgelaufene Aufnahmeoperation.

**Keine generischen Copy-and-paste-Installationsbefehle für MLD oder openSUSE:** Hier fehlen bisher bestätigte Distributionseigenschaften und Media-Tools-Hostabnahmen. Ergänzt werden sollen ausschließlich reale Paket-/Service-Daten der entsprechenden Maintainer, nicht aus Ubuntu geratene Pfade.

## Nach der Installation prüfen

Für eine echte Media-Tools-Abnahme sind – abhängig vom freigegebenen Funktionsumfang – die folgenden Beobachtungen relevant: Das Modul ist im Webfrontend vorhanden; eine erlaubte Importquelle wird angezeigt und lesend geprüft; eine ausdrücklich ausgewählte geeignete Datei läuft über Public API v1, Core und Backend-Agent; der fertige VDR-Aufnahmeordner und der native Aufnahmeindex werden bestätigt; ein fehlgeschlagener/ungewisser Auftrag wird nicht als Erfolg ausgegeben; die Weboberfläche zeigt den Core-Status und kann ihn nach einem erneuten Öffnen wiederfinden.

Tests, die einen Import, eine Veröffentlichung, Dateibereinigung oder einen Dienstneustart auslösen, gehören **nicht** zu einem allgemeinen Nur-Lese-Installationscheck.

## Statuspflege

Die [Media-Tools-Roadmap](ROADMAP.md) trennt implementierten Quellcode, Tests, vom Maintainer gemeldete Funktionsabnahmen und noch nicht verifizierte Distributionen. Detaillierte historische Implementierungsnotizen stehen in [IMPLEMENTATION-NOTES.md](IMPLEMENTATION-NOTES.md). Den aktuellen übergreifenden VDR-Suite-Status führt [CURRENT.md](../../docs/CURRENT.md).

Die tatsächliche installierte Version eines bestimmten Rechners muss unabhängig von diesen Dokumenten nachgewiesen werden.
