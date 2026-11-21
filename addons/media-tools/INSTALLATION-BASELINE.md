# MT.I0 — belegter Installationsvertrag (Stand 24.10.2026)

**Quellbasis:** `work/addon-platform-registry-lifecycle`, geprüft bei `4a23b56ab9aac3c4039b1016b87fe4eb68f7766f`. Dieses Dokument inventarisiert Repository-Quellen, **keine neue Host-Abnahme**. Auf dem produktiven yaVDR wurde im Rahmen von MT.I0 nichts ausgeführt.

## Vorhandene Wege und gemeinsame Build-Eingänge

| Weg | Beleg im Repository | Reale Grenze |
| --- | --- | --- |
| Suite-Runtime-Staging | `mk/install.mk:stage-install-runtime` baut Runtime-Binaries, verwendet `install-runtime` mit `DESTDIR` und `PREFIX=/usr`, versiegelt durch `tools/install_runtime_stage.py`. | Enthält Daemon, Agent, Frontend, Konfigurationsvorlage und leere private Verzeichnisstruktur; **noch keine** Media-Tools-Worker. Die Stage ist nicht der rohe Source-Tree. |
| Add-on-Komposition | `tools/addons/stage_media_tools_runtime.py`: überprüft eine vorhandene Suite-Stage, `candidate.txt` mit 40-stelliger Source-SHA und unabhängig geprüftem Import-Handler-SHA256 sowie drei bereits gebaute ELFs. Schreibt eine **neue** versiegelte `stage-with-media-tools`, lässt Eingangs-Stage unverändert. | Add-on-Dateien liegen unter festen Pfaden `/usr/libexec/vdr-suite/addons/rectools` und `/usr/share/vdr-suite/addons/rectools`. Die Stage allein berechtigt keine Ausführung. |
| Manuelle yaVDR-Installation | `tools/addons/install_media_tools.sh`: `--prepare` baut und stagiert; Installationspfad prüft Stage-Siegel, freigegebenen Handler, bestehenden Paketbesitz und Hostzustand. | **Nur yaVDR/systemd**, kein allgemeiner Distributionsinstaller. Der Live-Pfad stoppt/startet Suite-Dienste, sichert SQLite, schreibt Freigaben, Source-Ledger und systemd-Drop-in; daher ohne separate Freigabe **nicht** ausführen. Der aktuelle Installer ist auch **kein** neutraler, rein dateibasierter Paketadapter. |
| Konfiguration nach Installation | `tools/addons/configure_media_tools_paths.sh` | Nur root-/systemd-spezifisch; ändert Source-Inventar und Drop-in auf dem Host. Kein universeller Adapter. |
| Generische Add-on-Pakete | `tools/addons/addon_contract.py`, `build_deb.py` und `make build-addon-deb` | Reine, deaktivierte Manifest-/README-/AGENTS-Metadaten; **keine** ausführbaren Media-Tools-Worker. |
| C++-Preview-Paket | `tools/addons/build_media_tools_cpp_deb.py`, `make build-media-tools-cpp-deb` | Baut aus separaten Binärpfaden einen eigenen Dateibaum via `addon_contract.stage()`, nicht aus der gemeinsamen versiegelten Suite+Media-Tools-Stage. Nur `~cpppreview1`, keine produktive Aktivierung. Diese Duplikation ist die konkrete Lücke für MT.I1/MT.I4. |
| Debian-Quellpaket | `debian/rules`, `debian/control`, `debian/*.install` | Pakete für Daemon, Backend Agent und SuiteBridge. `plugin-only` überspringt Daemon/Agent. Media Tools ist dort **noch kein** aus der gemeinsamen Stage gebautes Debian-Paket. `dh_installsystemd --no-enable --no-start`. |

## Quellbelegte Installationsdateien und Runtime-Pfade

| Klasse | Festgestellte Quelle / Pfade | Variabilität und Grenze |
| --- | --- | --- |
| Installierte Worker/Trust-Reader | `/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import`, `vdr-suite-media-check`, `vdr-suite-media-shrink`, begleitende `addon.json`; `core/addons/src/AddonPinnedHandler.cpp`, `tools/addons/manual_handler_evidence.py`. | Der **Import-Handler-Pfad ist im Sicherheits-/Pinned-Reader fest**. Ein anderes `PREFIX` in `make` alleine verschiebt diese Vertrauensgrenze **nicht**. |
| Öffentliche Add-on-Metadaten | `/usr/share/vdr-suite/addons/rectools/{addon.json,README.md,AGENTS.md}`, installiert von der Add-on-Komposition. | Staging-Dateien sind nicht gleich Ausführungsfreigabe; das Manifest bleibt `state=scaffold`, ohne Capabilities/Entrypoints. |
| Root-kontrollierte Zulassung | `/etc/vdr-suite/addon-approvals/rectools.json`, `rectools.<backend>.json`, `rectools.import-running.lock`; `core/addons/src/AddonRootApproval.cpp` und Approval-Issuer. | Trust-Pfade sind fest und als Sicherheitsgrenze zu behandeln; nicht nach Belieben umlegen. Keine Pakete mit aktivierten Freigaben. |
| Quellenkonfiguration | `/etc/vdr-suite/addon-sources/media-import.tsv`; `addons/media-tools/src/LocalMediaImportInventory.cpp`. | Root-kontrolliertes Backend-/Quelleninventar, kein Browser-Dateisystempfad. Importroot und VDR-Videoziel sind **Datenwerte**, nicht automatisch distributionsweit gültige Defaults. |
| Suite/Agent-Dienste | `packaging/systemd/vdr-suite-daemon.service` und `vdr-suite-backend-agent.service`, `/usr/sbin/vdr-suite-{daemon,backend-agent}`, `/etc/vdr-suite/backend-agent.conf`, `/etc/default/vdr-suite-daemon`. | Referenz-Unit nutzt `User=vdr`, `Group=vdr`, systemd; der Installer liest den effektiven Agent-User. Für MLD/openSUSE kein aktueller Dienst-/Account-Nachweis. |
| Persistenz | `/var/lib/vdr-suite/vdr-suite.db`, `/var/lib/vdr-suite/backend-agent`, `/var/lib/vdr-suite/secrets`, optional `backend-agent/media-shrink`. | State/SQLite/Credentials sind **keine Paket-Payloads**. Private Verzeichnisse im Staging `0700`; existierende Host-Verzeichnisse **niemals** auf einen Stage-Modus umstellen. |
| Web/Cache | `/usr/share/vdr-suite/web/frontend`; `/var/cache/vdr-suite/channel-logos` manuell; Debian-Override `/usr/share/vdr-suite/channel-logos`. | Frontend wird aus install-runtime-Stage komponiert. Getrennten Runtime-/Paket-Vertrag erhalten; kein unbewiesenes frei konfigurierbares `DATADIR` versprechen. |
| Medienziele | In bestehenden yaVDR-Hinweisen `/srv/video/Filme` (Importquelle) und `/srv/vdr/video.00` (VDR-Aufnahmen). | **Beobachtung eines Referenzhosts**, keine allgemeine Ubuntu-/SUSE-/MLD-Pfadkonvention. Für Zielpfade SuiteBridge-Discovery berücksichtigen und reale Hostwerte read-only prüfen. |
| Externe Werkzeuge | C++17, `g++`, `make`, Python 3, `pkg-config`, `sqlite3`, `libcurl`, OpenSSL; Media-CLI nutzt FFmpeg/FFprobe und VDR-Aufnahmeformat. Debian benutzt `dpkg-deb`, `dh` und Zielsystem-VDR-ABI für SuiteBridge. | Konkrete Binärorte und Paketnamen auf MLD/openSUSE **nicht ermittelt**. Abhängigkeiten je Plattform erst prüfen, nicht erfinden. |

**Wichtige harte Grenzen:** `mk/install.mk` bietet zwar `DESTDIR`, `PREFIX`, `DATADIR`, `LOCALSTATEDIR` und `SYSTEMDUNITDIR` als Make-Variablen, aber es gibt fest codierte Runtime-, Approval- und Handler-Reader. Vor einer verifizierten Compile-/Runtime-Umstellung **nur** das belegte Layout `/usr` verwenden. `tools/install_runtime_stage.py` versieht jede Datei mit SHA256, Größe und Modus, alle Verzeichnisse mit Modus, verweigert Stage-Drift und erhält vorhandene `/etc`-Konfiguration beim Deployment. Das Manifest ist eine **Integritätsbeschreibung**, keine vertrauenswürdige Signatur und keine Root-Freigabe.

## Systemmatrix und Abnahmenachweise

| Umgebung | Source/Layout | Offline-Build/Tests | Install-/Paketpfad | Laufende Dienste, tatsächliche Rechte, reale VDR-Abnahme |
| --- | --- | --- | --- | --- |
| yaVDR/Ubuntu Referenzhost | **Dokumentiert**: systemd-Adapter, Import-/Video-Pfade oben | Existierende Targets/CI-Fixtures belegt; **nicht erneut ausgeführt** | Manuelle Source-Installation als Hostadapter vorhanden; Debian-Preview und Suite-debian/ unabhängig | **MT.I0 aktuell nicht neu geprüft**. Zuvor dokumentierter Agent-`inactive/dead`-Zustand ist kein Beleg für gegenwärtige Betriebsbereitschaft. Paketbesitz/Dateirechte/Storage/Agent-Unit vor Live-Einsatz read-only prüfen. |
| Ubuntu/Debian Paketbau | `debian/` und getrennte Preview belegt | Infrastrukturskripte vorhanden, **kein Paketlauf in MT.I0** | Gemeinsamer Stage-Input für Media-Tools-`.deb` **fehlt** | Keine produktive Paketmigration freigegeben. |
| openSUSE | **Nicht verifiziert**: Dienstname, Init, User/Gruppe, Verzeichnisse, VDR-ABI, Pakete | **Nicht verifiziert** | Kein belegter Adapter | **Nicht unterstützt/abgenommen**. Nur explizit zu liefernde Hostkonfigurationen zulässig. |
| MLD | **Nicht verifiziert**: Dienstname, Init, User/Gruppe, Verzeichnisse, VDR-ABI, Pakete | **Nicht verifiziert** | Kein belegter Adapter | **Nicht unterstützt/abgenommen**. Nur explizit zu liefernde Hostkonfigurationen zulässig. |

## MT.I1: engster sinnvoller Anschluss

Bestehende `stage_media_tools_runtime.py` **nicht ersetzen**. Ihr versiegelter Gesamtbaum bleibt die Wahrheit für manuelles Deployment; darin ein kleines versions-/rollenbezogenes Media-Tools-Dateiinventar verankern und read-only gegen vorhandenes Suite-Stage-Siegel, gepinnten Handler-SHA256 und konkrete Payload-Dateien validieren. Dieser eine Input soll später `build_media_tools_cpp_deb.py` bzw. die Debian-Integration speisen. Erst beim separat zu testenden MT.I4 wird der bisherige eigene Paket-Staging-Pfad abgelöst. Keine neuen Services, Agenten, Protokolle, Imports, Produktiv-Dateirechte oder automatischen Freigaben.
