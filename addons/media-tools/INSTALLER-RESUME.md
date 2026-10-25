# MT.I2 – Installations-Wiederaufnahmepunkt vor MT.UX

**Festgehalten am 25.10.2026 auf Branch `work/addon-platform-registry-lifecycle`.**

## Wo wir stehen

MT.I0-Quell-/Hostinventur liegt in [INSTALLATION-BASELINE.md](INSTALLATION-BASELINE.md).
MT.I1 hat eine einzige versiegelte Suite+Media-Tools-Stage mit unabhängig
geprüftem Importhandler-SHA256 und versionierter Inventur.
MT.I2 ist **quellseitig implementiert und isoliert CI-verifiziert**:
Code-/Test-Commit `f4b9c9dac2fb8fde6dc2804bfc2d97b345ae512f`,
[GitHub Actions #40159699229](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40159699229),
**alle vier Jobs PASS**. Der Dokumentations-HEAD vor MT.UX ist
`4d8c3a50a93fa96fb52ed5d06b7ace937a69b1b0`.

Der bestehende `tools/addons/install_media_tools.sh` ist einziger CLI-Einstieg:
`prepare` (geprüften Build wiederverwenden), `check` (read-only),
`verify` (read-only), ausdrücklich bestätigte yaVDR-Erstinstallation
und Offline-`live-update`/`live-rollback` über den existierenden
`tools/install_runtime_stage.py`-Dateideployer. Das separate
`media_tools_runtime_transaction.py` ist **kein eigener Dateideployer**.
Quellenkonfiguration bleibt beim vorhandenen
`configure_media_tools_paths.sh`; Datei-Installation erteilt keine
Core-/Backend-/Nutzerrechte und startet keine Dienste. Der isolierte
Updater mit synthetischem Root bleibt für Quelltests verfügbar.

## Die absichtlich nicht erfolgte Host-Abnahme

**REAL-VDR/INSTALL/UPDATE = NOT_VERIFIED.** Am produktiven yaVDR
wurden für MT.I2 keine Installations-/Update-Kommandos ausgeführt,
keine laufenden Services neu gestartet, keine vorhandenen Dateirechte
oder Eigentümer geändert, keine Datenbank oder Aufnahmen modifiziert.
Der mögliche früher installierte Debian-/Preview-Paketbesitz und der
aktuelle Servicezustand dürfen nicht geraten werden.

Beim Wiederaufnehmen **zuerst nur lesend** feststellen:

1. Welcher aktuelle Code-/Stage-Stand und welche Hashes liegen vor?
   Gibt es einen unveränderten, versiegelten vollständigen alten Runtime-
   Stage-Stand, auf den sich eine Migration tatsächlich zurückrollen lässt?
2. Wem gehören die installierten Media-Tools- und Suite-Dateien
   (`dpkg-query -S`, keine Überlagerung paketbesessener Dateien)?
   Welche früheren Approval-/Source-/systemd-Konfigurationen sind
   vorhanden? Bestehende Config/State/SQLite **erhalten**.
3. Sind Suite-Daemon/Agent stabil und lassen sie sich nach ausdrücklicher
   Betriebsfreigabe sicher anhalten? Gibt es laufende oder ungewisse
   Agent-Operationen, die eine Wartung verhindern?
4. Passen Backend-ID, gültige Importquellen, VDR-Videoziel,
   tatsächlicher Agent-Account/Gruppe, Platzreserve, Systemd-Unit und
   Root-kontrollierte Backup-Orte zum yaVDR-Host?
5. Erst nach explizitem Freigabeentscheid über **die konkrete**
   Installation: gesonderter Backup-, Stop-/Deploy-/Verify-/Rollback-
   Durchlauf mit vorher geprüftem alten/neuen Stage-Paar und unabhängigem
   Handler-Digest. Erfolgreiche Dateiinstallation ist **keine**
   Erlaubnis zur Freigabevergabe oder Dienstaktivierung.
6. Testmedien-/Agent-/Core-/UI-Abnahme separat protokollieren.

## Offene benachbarte Installations-Slices

`MT.I3`: nachgewiesene Distributionsadapter/Hostkonfiguration;
openSUSE und MLD **ohne erfundene Defaults**.
`MT.I4`: Debian-Pakete aus exakt derselben Stage, Besitz-/Migrationstests,
keine automatische Aktivierung.
`MT.I5`: Anwenderanleitung und reale Release-/Host-Abnahme.
**Keinen Build allein wegen dieses Dokumentations- oder MT.UX-Wechsels
wiederholen.**

## Bewusster Phasenwechsel

Auf Wunsch des Maintainers wird **vor jeder weiteren Live-Installation**
MT.UX0–UX7 (Media-Import und Recording-Aktionen) bearbeitet.
Die MT.I2-Fakten oben bleiben unverändert, bis die separate
Host-Abnahme tatsächlich stattgefunden hat. Dieser Wechsel gibt
keine Erlaubnis zu `sudo`, `chmod/chown`, `systemctl`, VDR-Aufnahmeänderung
oder produktiver Migration.
