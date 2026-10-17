# Packaging — Installationsquellen und Deploy-Grenzen

## Navigation

- [Projekt-README](../README.md)
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


## Öffentliches PPA: verifizierte Noble-Abnahme vom 17. Oktober 2026

Quellstand des separaten, nicht gemergten Testbranches:
`work/ppa-noble-install-smoke-20261017`.
Die tatsächlich **veröffentlichten** Binärpakete werden auf einem frischen
GitHub-Actions-Runner `ubuntu-24.04` mit den PPAs
`ppa:seahawk1986-hotmail/vdr-2.8.3` und
`ppa:hotzenplotz5/vdr-suite` über APT installiert; keine lokalen
selbstgebauten `.deb` werden als Installationsersatz verwendet.

- [Noble-Paketinstallation, Run 39037557381](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39037557381):
  erfolgreich bei SHA `72282dd5877dfc36f8a5f52fedcddaff0829f471`.
- [Noble-Paket- und HTTP-Runtime-Abnahme, Run 39038464174](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39038464174):
  erfolgreich bei SHA `6d1aa3b7c99f9dbcb171c5a45ecea6cbe3bbb02d`.
  Dauerhafte Prüflogik: [Workflow](../.github/workflows/ppa-noble-install-smoke.yml).

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
wurden installiert und auf fehlende dynamische Bibliotheken geprüft;
diese statische/linkerbezogene Prüfung ersetzt keinen Plugin-Start.
Upgrade/Purge und Konfigurationsschutz waren bereits Gegenstand
separater Offline-Paketlebenszyklus-Tests; ein Upgrade direkt aus dem
**veröffentlichten** PPA ist nicht durch diesen Workflow bewiesen.

Resolute publication availability is checked by the [disposable 26.04 readiness workflow](../.github/workflows/ppa-resolute-install-smoke.yml). A green *readiness* job with `RESOLUTE_RELEASE_GATE=NOT_READY` is **not** an installation or runtime PASS: downstream install and daemon steps are explicitly skipped until the archive publishes every package.\n\nDie [korrigierte Resolute-PPA-Vorprüfung, Run 39039206371](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39039206371) (Commit `3d84b29ab6c5526683beb6266cc20dacacc79a13`) bestätigte auf einem Ubuntu-26.04-Container: Seahawk-VDR `2.8.3-0yavdr0~resolute` und das gepatchte `vdr-plugin-web` sind im Archiv vorhanden. `vdr-suite-daemon`, `vdr-suite-backend-agent` und `vdr-plugin-suitebridge` sind **nicht** als veröffentlichte Resolute-Kandidaten verfügbar. Das Gate meldet `RESOLUTE_RELEASE_GATE=NOT_READY`; die nachfolgenden APT-Installations- und Daemon-Laufzeitschritte wurden ausdrücklich übersprungen. Der erfolgreiche *Check-Workflow* ist daher **kein erfolgreicher Resolute-Laufzeittest**.

**Ubuntu 26.04 Resolute:** Für die drei VDR-Suite-Binärpakete liegt in
diesem Noble-Testlauf noch **kein** veröffentlichter APT-Installations-
oder HTTP-Laufzeitnachweis vor. Die Launchpad-Builds und der anschließende
analoge Resolute-Installationslauf sind getrennte offene Freigabegates.
Ein erfolgreicher PPA-Build oder diese Noble-Abnahme ist keine
Freigabe für produktive Installation oder Migration.

## Back

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
