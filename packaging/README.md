# Packaging — Installationsquellen und Deploy-Grenzen

## Navigation

- [Projekt-README](../README.md)
- [Erste Schritte nach der Installation (Ubuntu/PPA)](../docs/erste-schritte-nach-installation.md)
- [Dokumentationsindex](../docs/index.md)
- [Installations-/Deployment-Zuverlässigkeit](../docs/development/install-runtime-deployment-reliability.md)
- [Install-Layout-Vertrag](../docs/development/phase-56.51-install-layout-contract.md)
- [Packaging-Entscheidung ADR-0037](../docs/adr/ADR-0037-packaging-install-api-boundary.md)
- [Debian-Verzeichnisstatus](debian/README.md)
- [Backend-Agent: Paketinhalt, Single-Host/Multi-Backend, Enrollment und Sicherheit](../debian/README.md#backend-agent-package-roles-and-deployment)

## Was im Quellbaum vorhanden ist

- [`nginx/vdr-suite.conf`](nginx/vdr-suite.conf): Beispiel für eine Public-Origin-Reverse-Proxy-Einbindung. **Die Datei aktiviert sich nicht durch bloßes Vorhandensein.**
- [`systemd/vdr-suite-daemon.service`](systemd/vdr-suite-daemon.service): Daemon-Servicevorlage.
- [`systemd/vdr-suite-backend-agent.service`](systemd/vdr-suite-backend-agent.service) und [`systemd/backend-agent.conf`](systemd/backend-agent.conf): Agent-Service und Beispielkonfiguration.
- [`systemd/vdr-suite-daemon.default`](systemd/vdr-suite-daemon.default) sowie [`systemd/vdr-suite-live.conf`](systemd/vdr-suite-live.conf): mitgelieferte System-/Laufzeitkonfigurationsquellen.

Der ausführbare Installationsgraph und die tatsächlich installierten Dateien gehören zu [`mk/install.mk`](../mk/install.mk), nicht allein zu dieser Verzeichnisliste. Historische Paketkandidaten im [Install-Manifest-Vertrag](../docs/development/phase-56.55-install-manifest-package-file-contract.md) sind nicht automatisch veröffentlichte `.deb`-Pakete.

## Sichere Deployment-Grenze

Für repositorybasierte Runtime-Deployments ist der **versiegelte Staging-Baum** maßgeblich: [verbindlicher Deployment-Vertrag](../docs/development/install-runtime-deployment-reliability.md). Direktes Kopieren einzelner Dateien aus `web/frontend` oder `packaging` in ein laufendes System kann einen Versionsmix erzeugen. Ein Quellcode- oder CI-Erfolg beweist außerdem **nicht**, dass genau diese Version bereits auf yaVDR installiert ist.

Dieses README ist nur Navigation. Es führt **keine Installation, Migration, Rechtekorrektur oder Service-Neustarts** aus. Eine Änderung am laufenden System braucht eine getrennte Deployment-Freigabe mit Provenienz und Vorprüfung.


## Öffentliches experimentelles PPA: Noble und Resolute (17. Oktober 2026)

Die kanonische Debian-/Ubuntu-Paketierung liegt seit [PR #494](https://github.com/hotzenplotz5/vdr-suite/pull/494) auf `main` (Merge-SHA `2a81dea07455ad2801ac959d9a8fcf759dbc4994`). Das vorhandene Launchpad-Rezept `vdr-suite-packaging-experimental` verwendet `lp:vdr-suite main` und bietet Noble und Resolute als Zielserien an. Die folgenden Noble-CI-Läufe stammen aus der Abnahme vor bzw. um den Merge.
Die tatsächlich **veröffentlichten** Binärpakete werden auf einem frischen
GitHub-Actions-Runner `ubuntu-24.04` mit den PPAs
`ppa:seahawk1986-hotmail/vdr-2.8.3` und
`ppa:hotzenplotz5/vdr-suite` über APT installiert; keine lokalen
selbstgebauten `.deb` werden als Installationsersatz verwendet.

- [Noble-Paketinstallation, Run 39037557381](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39037557381):
  erfolgreich bei SHA `72282dd5877dfc36f8a5f52fedcddaff0829f471`.
- [Noble-Paket- und HTTP-Runtime-Abnahme, Run 39038464174](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39038464174):
  erfolgreich bei SHA `6d1aa3b7c99f9dbcb171c5a45ecea6cbe3bbb02d`.
- [Noble-Paketinstallation, Daemon/HTTP und VDR-Plugin-Versionserkennung, Run 39038795095](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39038795095):
  erfolgreich bei SHA `653b46ce3810876945e597cb58a7b5af559cb337`.
  VDR meldet die installierten Plugins `suitebridge (0.14.1)` und `web (0.0.1)`.
  Die wiederholbare Prüflogik liegt im [Noble-Workflow](../.github/workflows/ppa-noble-install-smoke.yml).

Der Abnahmelauf installiert und verifiziert `vdr` mit Version
`2.8.3-0yavdr0~noble`, die drei VDR-Suite-Binärpakete mit Version
`0.14.1-1~exp2~ppa2+202610170549~ubuntu24.04.1` und
`vdr-plugin-web` mit Version
`0.0.1+git20260827-111-34ded50-1yavdr1+vdrsuite1+ppa1+202610171021~ubuntu24.04.1`.
APT-Simulation, echte Installation, `dpkg --audit`, Paketinhalt,
VDR-ABI-Abhängigkeiten, ELF-Linker-Abhängigkeiten und Unit-Dateien wurden
geprüft. Der installierte Daemon startet unprivilegiert mit einer
testlokalen SQLite-Datenbank, liefert `index.html`, CSS, HbbTV-QOI,
Public-v1-Client und `app.js` über HTTP aus und lässt sich zweimal
sauber starten und per SIGTERM beenden. Ein anonymer Public-v1-API-Aufruf
liefert erwartungsgemäß HTTP 401 mit
`Content-Type: application/problem+json`. Das Frontend verwendet
hierbei **Dateien aus dem veröffentlichten Debian-Paket**.

**Abgrenzung:** Die Runtime-Prüfung verwendet keinen echten VDR-Tuner
und kein produktives yaVDR-System. Der fehlende `restfulapi`-Backend-Port
8002 im isolierten Runner ist kein Daemon-Startfehler. Weder ein
enrollter/verbundener Backend Agent noch eine produktive SuiteBridge-
Session, ein laufender HbbTV-CEF-Browser, `remotetranscode` oder eine
HbbTV-Media-Presentation mit TV-Hardware sind damit abgenommen.
Die Bibliotheken `libvdr-suitebridge.so.14` und `libvdr-web.so.14`
wurden installiert, auf fehlende dynamische Bibliotheken geprüft und
von VDRs Plugin-Versionserkennung erkannt. Das ist ein Loader-Nachweis,
aber **kein** produktiver Plugin-Start und keine HbbTV-Wiedergabe.
Upgrade/Purge und Konfigurationsschutz waren bereits Gegenstand
separater Offline-Paketlebenszyklus-Tests; ein Upgrade direkt aus dem
**veröffentlichten** PPA ist nicht durch diesen Workflow bewiesen.

**Ubuntu 26.04 Resolute – veröffentlicht und erfolgreich aus dem PPA installiert:**
Das Rezept veröffentlichte die drei VDR-Suite-Binärpakete für amd64 mit der
Version `0.14.1-1~exp2~ppa2+202610171542~ubuntu26.04.1` neben dem separat
gepatchten `vdr-plugin-web`. Der manuell ausgelöste
[Resolute-PPA-Abnahmelauf #39070097283](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39070097283)
lief auf dem Merge-Commit `2a81dea07455ad2801ac959d9a8fcf759dbc4994`
erfolgreich durch. Die tatsächlichen Schritte für APT-Installation,
VDR-Plugin-ABI, ELF-Abhängigkeiten, Start des installierten Daemons als
`vdr`, HTTP-Assets, HTTP-401-Sicherheitsvertrag, Shutdown und Neustart
wurden **ausgeführt, nicht übersprungen**. Die Logs melden
`RESOLUTE_PPA_CANDIDATES=READY`,
`RESOLUTE_PPA_APT_VDR_PLUGIN_ABI=PASS` und
`RESOLUTE_PPA_DAEMON_HTTP_RESTART=PASS`. Der frühere
[Verfügbarkeitslauf #39039206371](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39039206371)
ist nur noch ein historischer Vor-Publikationsnachweis.

**Daily-Build-Grenze:** Das bestehende Rezept steht nach den bestätigten
manuellen Builds weiterhin auf *Built on request*. Die aktuelle
Versionserzeugung `0.14.1-1~exp2~ppa2+{revtime}` erzeugt für einen neueren
Revision-Zeitstempel eine höhere Paketversion. Eine automatische
Launchpad-Daily-Veröffentlichung wartet jedoch **nicht** auf erfolgreiche
GitHub-CI-Läufe. Insbesondere ist die Packaging-CI auf bestimmte Pfade
eingeschränkt, während Launchpad den ganzen geänderten `main`-Stand
verpacken würde. Deshalb Daily nicht ohne eine ausdrücklich geprüfte
Freigabepolitik bzw. Trennung von Daily- und freigegebenen Paketen
einschalten. Die erfolgreiche PPA-Abnahme ist keine Hardware-, HbbTV-
Wiedergabe-, Backend-Agent-Verbindungs- oder Produktivmigrationsabnahme.

Weder die Noble-Abnahme noch ein PPA-Build autorisieren eine produktive
Installation oder ungeprüfte Migration einer bestehenden yaVDR-Installation.

## Back

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
