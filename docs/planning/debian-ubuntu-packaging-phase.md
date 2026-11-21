# Packaging-Phase: Debian/Ubuntu und manuelle Installation (Plan)

Status: PLANNED — noch keine produktiven Pakete freigegeben.
Branch: `work/packaging-debian-ubuntu-install`
Basis: `main`

## Ziel und Scope

Release-fähige, versionierte Installation von VDR Suite als **Server-/Daemon-Runtime** samt notwendigen Web-Assets, Admin-/Bootstrap-Hilfsprogrammen und **VDR SuiteBridge Plugin**. Der Backend-Agent wird nur insoweit mitgeliefert, wie er für den unterstützten Serverbetrieb benötigt wird; ansonsten als explizit optionales Unterpaket. Keine TV-Apps, Output-Clients, fremden VDR-Plugins oder Distribution-spezifische VDR-Komplettinstallation.

Drei Wege teilen eine Build- und Installationsquelle: native `.deb`-Pakete; automatisiertes Quellinstallationsskript; dokumentierter manueller Bau mit `make` und `DESTDIR`. Keine Installationsroute darf heimlich eine abweichende Dateiliste pflegen.

## Vorgeschlagene binäre Pakete

- `vdr-suite-daemon`: Server-Runtime inklusive benötigter Web-Assets und administrativer Bootstraphilfen; systemd.
- `vdr-plugin-suitebridge`: VDR-Plugin, getrennt wegen VDR-ABI-/Distribution-Bindung.
- `vdr-suite-backend-agent`: optional bzw. nur dann zwingend, wenn die zur Veröffentlichung ausgewählte Server-Betriebsart dies erfordert.
- `vdr-suite`: optionales Metapaket für die lokale Komplettinstallation; seine Abhängigkeiten werden erst nach Test der unterstützten Betriebsart fixiert.

Kein öffentliches C++ `-dev`-Paket; v1 bewusst ohne allgemeines Binär-Repository.

## Slice-Plan (P0–P12)

| Slice | Gegenstand | Akzeptanz |
| --- | --- | --- |
| P0 | Baseline von main, Scope, vorhandene Artefakte, Install-Manifest, Versionen und VDR-ABI erfassen | kein Live-Host verändert; dokumentierte Inventur und Risiken |
| P1 | Distributionen und VDR-Baselines, Support-Matrix (Debian/Ubuntu/yaVDR), Paketnamen und Layout | eindeutige Paketgrenzen und Testmatrix; keine ungetestete Support-Zusage |
| P2 | einheitliche Versionierung mit SemVer-orientierter Projektversion, Debian-Revisionssuffix, Git-Provenienz | Ausgabe in Daemon/Plugin/Manpages/Paketmetadaten konsistent; dirty builds gekennzeichnet |
| P3 | reproduzierbarer Quell-/Staging-Build aus bestehenden Make-Zielen plus separatem SuiteBridge-Build | `DESTDIR` sauber, kein root beim Build, kein Host-Schreibzugriff |
| P4 | Build-Depends/Depends/Recommends/Suggests und VDR-Plugin-ABI-/Architekturprüfungen | Dependency-Audit mit `dpkg-shlibdeps`, `readelf`, `pkg-config`; Fail-closed bei inkompatibler ABI |
| P5 | Debian Source-Package und binary `.deb`-Regeln (`debian/control`, `rules`, `changelog`, `copyright`, `*.install`) | `dpkg-buildpackage` produziert validierbare Pakete; `lintian` geprüft |
| P6 | systemd, Konfigurations-/Daten-/Cache-Verzeichnisse, Besitzrechte, Conffiles und Secret-Policy | install/remove/purge korrekt; kein Überschreiben alter Secrets/Daten |
| P7 | atomare Upgrade-, DB-Migrations-, Downgrade-/Recovery- und Paketlebenszyklusregeln | geprüfte Upgrade-Szenarien mit Backup/Restore-Anleitung; keine stille destruktive Migration |
| P8 | Installationsskript mit `--check`, `--build`, `--install`, `--uninstall` (Optionsvertrag noch zu testen) | OS-/Architektur-/Abhängigkeitschecks, keine ungefragten Dienst-Neustarts; idempotent |
| P9 | manueller Build, Installation und Deinstallation aus `DESTDIR`, konsistent mit Paketdateiliste | nachvollziehbarer Walkthrough ohne Wrapper auf frischer Testumgebung |
| P10 | Multi-Distro-Dokumentation: Debian, Ubuntu, yaVDR, Abweichungen und Troubleshooting | klare geprüfte Kommandos, Voraussetzungen, VDR-/Systemd-Versionen und Recovery |
| P11 | isolierte CI/VM/Container-Matrix: Build, Install, Upgrade, Remove, Purge und Konfliktprüfung | reproduzierbare Build-Logs und Artefakte; realer yaVDR-Host nur nach Freigabe |
| P12 | Release-Check: Checksums, signierbare Artefakte, SBOM/Lizenzen, Support-Matrix, Abschluss-/Roadmap-Doku | dokumentierter Release-Candidate und explizite Go/No-Go-Kriterien |

## Unverhandelbare Regeln

1. `main` und laufende yaVDR-Installation sind ohne getrennte Freigabe tabu.
2. Die `.deb`-Erstellung verwendet dieselben Install-Ziele/Manifestquellen wie die Quellinstallation.
3. Beim Paketbau keine `apt install`-Seiteneffekte auf dem Build-Host und kein `sudo make install`.
4. `/etc/vdr-suite` bleibt ein Conffile-/Admin-Bereich; Passwörter/Secrets weder als Plaintext im Paket noch automatisch überschreiben.
5. `/var/lib/vdr-suite` und Datenbanken bleiben bei remove erhalten; purge löscht nur nach ausdrücklich dokumentierter Paketsemantik, keine unkontrollierte Datenvernichtung.
6. VDR-Plugin nur bei kompatibler VDR-ABI installieren; Version und Installationspfad müssen pro Distribution geprüft werden.
7. Source-Install und dpkg-Install nicht unbemerkt überlagern: Konflikt-Erkennung und explizite Migrationsanleitung.
8. Plugin-Restart/VDR-Neustart sowie Daemon-Enable/Start werden vorab spezifiziert und getestet.
9. Release-Build ist nachvollziehbar (Quell-Commit, Version, Toolchain, Build-Log), möglichst reproduzierbar.
10. Installationsdokumentation unterscheidet **getestet** von **experimentell**.

## Entscheidungen vor der Implementierung

- Unterstützte erste Distributionen und konkrete Releases nach Build-Matrix festlegen.
- Plugin-Paketname und tatsächlicher VDR-ABI-Paket-Mechanismus der Ziel-Distro auswerten.
- Ob der Backend-Agent für den lokalen Minimalserver verpflichtend ist, durch Source-/Runtime-Audit entscheiden.
- Installation zunächst nur über lokale Pakete; signiertes APT-Repository ist ein separater zukünftiger Meilenstein.
- Paketversion z.B. `0.x.y-1`; keine endgültige numerische Version ohne Release-Entscheidung.

## Bereits vorhandene Vorarbeiten

- `mk/install.mk`: Build-/Install-/Staging-Ziele.
- `packaging/systemd/`: systemd und tmpfiles.
- `packaging/debian/README.md`: explizit noch keine echten Debian Metadaten.
- `docs/adr/ADR-0037-packaging-install-api-boundary.md`.
- `docs/planning/platform-productization-roadmap.md`.

Umsetzung beginnt mit P0/P1, anschließend P2/P3; jeder Slice erhält separate Tests und Dokumentation.
