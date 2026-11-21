# Debian-Paketierung — Status und Abgrenzung

## Navigation

- [Packaging-Index](../README.md)
- [Projekt-README](../../README.md)
- [Packaging-Boundary-Guide](../../docs/development/phase-56.29-packaging-boundary-guide.md)
- [Paketkandidaten-/Install-Manifest](../../docs/development/phase-56.55-install-manifest-package-file-contract.md)
- [ADR-0037](../../docs/adr/ADR-0037-packaging-install-api-boundary.md)

## Aktueller Repository-Bestand

In **diesem Verzeichnis** liegt derzeit außer dem README **kein** `debian/control`, `debian/rules` und kein Maintainer-Skript. Aus dem leeren Verzeichnis folgt daher **kein bereits funktionierender oder veröffentlichter Debian-/Ubuntu-Paketbuild**.

Für die Paketvorbereitung existieren jedoch Quell- und Staging-Ziele im zentralen [Install-Makefile](../../mk/install.mk), und es gibt historische Paketklassen/-kandidaten im [Manifest-Vertrag](../../docs/development/phase-56.55-install-manifest-package-file-contract.md). Das ist **Packaging-Readiness**, nicht dasselbe wie ein gültiges `.deb` mit Abhängigkeiten, Upgrade-Skripten und geprüften Konfigurationsmigrationen.

## Bevor ein Paket erstellt wird

Paket-Metadaten, Abhängigkeiten, Maintainer-Skripte, persistente Eigentümerschaft, Datenbank-Upgrade, Erstadmin-Bootstrap, Konfigurationsdatei-Behandlung, Dienstreihenfolge und Rollback müssen jeweils gesondert spezifiziert und getestet werden. Ein Paket darf nicht stillschweigend bestehende yaVDR-Daten, Rechte oder Secrets ersetzen.

Das Dokument beschreibt den Repository-Stand; es ist **keine** Installationsanleitung.

## Back

- [Packaging-Index](../README.md)
- [Dokumentationsindex](../../docs/index.md)
