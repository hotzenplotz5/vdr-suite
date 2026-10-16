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

## Back

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
