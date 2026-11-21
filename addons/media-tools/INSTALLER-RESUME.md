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


## MT.I3–I5: Installations- und Paketierungsvertrag (25.10.2026)

**Gültigkeit:** Der folgende Ablauf beschreibt einen **Offline-Kandidaten** aus
der bereits versiegelten Media-Tools-Stage. Er ist keine Autorisierung zur
Installation auf dem produktiven VDR. Das bestehende yaVDR-Installationsmodell
wurde nicht durch einen zweiten Installationsmechanismus ersetzt.

### I3 – Host- und Distributionsprüfung

`bash tools/addons/install_media_tools.sh profile /` zeigt die
**read-only** ermittelten `/etc/os-release`-Fakten und kennzeichnet ABI,
Agent-Account, Servicezustand, Import-/Video-Pfade sowie Paketbesitz weiterhin
als **NOT_VERIFIED**. Die existierende manuelle Erstinstallation verweigert
vor Build, Paketänderung oder Dienstoperationen Hosts ohne bestätigte
Ubuntu-ID. Das bedeutet **nicht**, dass jeder Ubuntu-Host fertig konfiguriert
ist. Der tatsächliche Backend-Account wird bereits separat aus der
aktiven systemd-Unit abgefragt und darf nie erraten oder pauschal gechownt
werden. `configure_media_tools_paths.sh` bleibt Ubuntu-/systemd-spezifisch.

Für **openSUSE/MLD** existiert kein freigegebener Deploy-Adapter. Bevor
ein solcher angelegt werden kann, müssen auf dem echten Zielsystem zumindest
Init-Manager/Servicebezeichner, VDR-ABI und -Indexgenerator, FFmpeg-Pfade,
Daemon/Agent-Paketbesitz, tatsächlicher Dienst-User/Gruppe, Root-Verzeichnisse,
Import-/Video-Roots und verfügbare Storage-Reserve lesend nachgewiesen werden.
Die gemeinsame Stage darf dort offline analysiert, aber **nicht** mit dem
yaVDR-Installationsskript ausgerollt werden.

### I4 – Dieselben acht Dateien: manuelle Stage oder optionales .deb

**Voraussetzungen:** Eine unabhängig begutachtete, bereits komponierte
`stage-with-media-tools` aus MT.I1/I2, ihr SHA-256 des Import-Workers und
ein neues, leeres Ausgabeverzeichnis auf einem ausreichend großen
Dateisystem. Das `.deb` wird **nicht** aus neu kompilierten Workern,
`modules/rectools`-Quellkopien oder einem parallelen Dateibaum erzeugt.

Der vorhandene Paketbuilder besitzt jetzt zusätzlich den expliziten
`--sealed-stage`-Modus. Sein entsprechendes bestehendes Make-Frontend heißt
`build-media-tools-stage-deb`. Die Eingaben müssen **vollständige absolute**
Pfade und der **unabhängig** überprüfte Handler-SHA-256 sein:

```bash
# Nur in einem unveränderten, nachweislich richtigen Checkout auf
# einem disponiblen Build-System; NICHT als root oder Installationslauf.
# Die drei Werte müssen zuvor real ermittelt und kontrolliert werden.
STAGE=/ABSOLUTER_GEPRUEFTER_PFAD/stage-with-media-tools
OUTPUT=/ABSOLUTER_GEPRUEFTER_PFAD/leeres-paket-verzeichnis
APPROVED_SHA256=64_stellige_unabhaengig_gepruefte_sha256

python3 tools/addons/stage_media_tools_runtime.py \
  --verify-stage "$STAGE" --approved-handler-sha256 "$APPROVED_SHA256"

make build-media-tools-stage-deb \
  RUNTIME_STAGE="$STAGE" \
  APPROVED_HANDLER_SHA256="$APPROVED_SHA256" \
  OUTPUT_DIR="$OUTPUT" \
  ADDON_MAINTAINER='VDR-Suite contributors <noreply@github.com>'
```

Der Builder verweigert ein bestehendes Zielpaket, prüft vor dem Kopieren
Platzreserve, verifiziert Stage-Siegel + Add-on-Inventar gegen den **separat**
genannten SHA-256, übernimmt **genau acht Add-on-Dateien** (drei Worker,
Agent-Manifest, öffentliches Manifest, README, AGENTS und versiegeltes
Installinventar) und vergleicht nach `dpkg-deb --extract` jeden Byte-Digest
und Dateimodus sowie den **gesamten** ausgepackten Dateisatz. Das Paket heißt
`vdr-suite-addon-media-tools`; es ist absichtlich **inaktiv**. Kein
`postinst`, keine automatische Zulassung/Aktivierung, keine Services,
Konfigurationsdateien, Freigaben, privaten Datenbanken oder Quellfilme im
Paket. Die getrennte Code-Freigabe durch die bestehende Core-/Agent-Policy
bleibt erforderlich.

Ein vor dem Entpacken ausgeführter minimaler `preinst`-Guard verweigert
**vorhandene unpaketierte Dateien, fremden dpkg-Besitz und symlinked oder
nichtverzeichnisförmige Zielvorfahren**. Bereits manuell installierte
Media-Tools dürfen **nicht** direkt mit `dpkg -i` überlagert werden.
Dies ist eine Schutzgrenze, **keine automatische Migration** und kein
Nachweis für sicheres Upgrade, Remove/Purge oder Kopplung mit dem
experimentellen `debian/`-Quellpaket/PPA. Der ältere
`build-media-tools-cpp-deb`-Weg bleibt als getrennte, ausdrücklich
als `~cpppreview1` deklarierte historische Offline-Vorschau bestehen.

### I5 – Freigabe- und Rollback-Checkliste

1. **Provenienz:** Git-Branch, exakter 40-stelliger Code-SHA, clean Checkout,
   Stage-Manifest, Import-Handler-SHA-256 und Dateidigest-Abgleich des Pakets.
2. **Offline-Typprüfungen:** `make test-addon-media-tools-package`,
   `python3 -m unittest discover -s tools/addons -p
   'test_stage_media_tools_runtime.py'`, versiegelter Stage-Check,
   isolierter `dpkg-deb --contents`/Extract-Vergleich. Exakte CI-Revision
   separat als `CI: PASS/FAILED/NOT_VERIFIED` notieren.
3. **Eigentums- und Migrationsprüfung vor jeder Live-Änderung:** Alle
   installierten Paketbesitzer/Dateien, bestehende manuelle Version,
   SQLite-Konfigurations-/Zustandsdateien, Approvals und Importquellen,
   Servicezustand, laufende/ungewisse Operationen, Backup-/Rollback-Stage
   und tatsächlichen freien Platz **lesend** erheben. Fremde/unbekannte
   Ownership und ungesicherte Altdaten sind ein Stop-Grund, kein
   `chown`-/`chmod`-Reparaturauftrag.
4. **Wegentscheidung:** Für eine explizit erlaubte manuelle yaVDR-Installation
   ausschließlich den vorhandenen `install_media_tools.sh` mit der
   geprüften Stage verwenden. Für ein Debian-Paket ausschließlich den
   Stage-Payload-Build; **kein** automatischer Wechsel von manuell nach dpkg.
   Beide Wege in denselben Pfad zu installieren ist nicht erlaubt.
5. **Wartungsfenster und separate Zustimmung:** Ein realer
   `live-update`/`live-rollback` braucht geprüfte alte und neue versiegelte
   Runtime-Stages, unveränderte Daten/Dateirechte, exakte Hashes und
   vom Betreiber ausdrücklich freigegebenes kontrolliertes
   Stop-/Backup-/Deploy-/Verify-/Rollback-Verfahren. Die Dienste starten
   nicht aufgrund eines bloßen Paketbaus, und ein Dateideploy verleiht
   keine Benutzer-/Agent-Rechte.
6. **Getrennte Abnahmen:** `SOURCE/UNIT`, `PACKAGE/STAGING`, `CI`
   und **`REAL-VDR`** jeweils separat dokumentieren. Ein Paketbuild
   beweist weder eine installierte Runtime noch einen erfolgreichen Filmimport
   samt VDR-Index und autoritativem Aufnahme-Readback.

**Der erreichte Code-Stand allein berechtigt keine Hostmutation.** Die
Endnutzeranleitung für eine produktive Migration und weitere Plattformen
kann erst nach den oben genannten Host-/ABI-/Upgrade-Nachweisen final
freigegeben werden.
