# MT — Media Tools: Installation, Import und Aufnahmeaktionen

**Datum:** 24.10.2026 — **Status:** geplant, noch nicht implementiert oder live abgenommen.  
**Owner:** `addons/media-tools/` in `hotzenplotz5/vdr-suite`.  
**Branch:** nur `work/addon-platform-registry-lifecycle`; kein neues Repo, kein Nebenbranch, keine nummerierte VDR-Suite-Phase 70.  
**Bestehende Grenzen:** [Add-on-Plattform-Roadmap](../../docs/planning/addon-platform-roadmap.md), dort A6/A7.5 und M1.R (Import/HandBrake/Überspringen).

## Verbindliche Produktentscheidung

**Rectools-einfacher Import als Default, Explorer als Zusatz.**

- Ein Administrator hinterlegt pro Backend eine oder mehrere **zugelassene Importwurzeln**, mit lesbarem Namen und optionaler Standardquelle. Der einfache Einstieg zeigt die Standardquelle und genau zwei Wege: **„Verzeichnis scannen“** oder **„Explorer öffnen“**.
- **„Verzeichnis scannen“** liest rekursiv **alle Unterverzeichnisse** der gewählten Quelle; ein Scan löst **keinen Import** aus. Danach stehen normale, zurückgestellte, übersprungene und doppelte Dateien verständlich getrennt in der Trefferliste.
- Nutzer wählen alle geeigneten Dateien, einen Ordner oder einzelne Einträge, sehen eine **Importvorschau** und starten erst dann den bestätigten Auftrag. Optionale Neukodierung braucht eine explizite, zum Auftrag passende Zustimmung.
- Der Explorer nutzt dieselbe erlaubte Dateiliste, nur mit Ordnernavigation, Breadcrumbs, Filtern und Mehrfachauswahl. Keine unbeschränkte Dateisystemnavigation durch den Browser und keine zweite Importengine.
- Navigation: **„Media Import“ direkt rechts von „Aufnahmen“**, `Home` bleibt zuerst. Nur sichtbar mit wirksamer Backend-/Akteur-Capability.
- **„Aufnahme verkleinern (HEVC)“** gehört in **Aufnahmen → Aufnahmeaktionen**, sowohl bei den Aufnahmedetails als auch im `⋯`-Menü der Detailkarten, sofern die Aufnahme geeignet und die Funktion freigegeben ist. Kein eigener Hauptreiter.
- Technische Token (`sourceId`, Freigabe-Revision, Handler-Digest) erscheinen nur unter „Technische Details“, niemals als Hauptbeschriftung.

## Belegter Ist-Zustand – nicht mit Unterstützung verwechseln

- `vdr-rectools` Branch `develop`: `vdr-rectools import` scannt den konfigurierten `IMPORT_DIR`; `var/www/html/explorer.php` ist eine getrennte Dateiansicht. Die Bedienidee ist die Vorlage, **nicht** die alten potenziell löschenden/ungeschützten Arbeitswege.
- Bekannter **einzelner** yaVDR/Ubuntu-Host: Backend `default`, Importwurzel `/srv/video/Filme`, Videoziel `/srv/vdr/video.00`. Das sind **keine** Standardwerte für openSUSE/MLD oder jedes Ubuntu.
- Live-Read-only-`plan` erkannte eine Andor-MKV mit HEVC ohne notwendigen Re-Encode. Der bisherige Browser zeigte anfangs null gewöhnliche und sechs markierte Quellen, obwohl tiefere Serienordner vorhanden waren. Das beweist keinen fertigen produktiven Import.
- Die Source enthält bereits Scanner, C++-Importer, M1.R-Entscheidungen, Suite Core/Agent-Operationen und HEVC-Vorprüfung/Stage/Publication im Recording-Hero. Bestehende Implementierungen wiederverwenden statt neu erfinden.
- `tools/addons/install_media_tools.sh` ist derzeit ein **yaVDR-/systemd-spezifischer Adapter**. Versiegelte Stages über `stage_media_tools_runtime.py` und `install_runtime_stage.py` existieren. Ein geprüftes openSUSE- oder MLD-Installprofil **existiert hiermit noch nicht**.

## Phase MT.I — zuerst eine einfache, distributionsbewusste und paketierbare Installation

**Architekturentscheidung:** Eine gemeinsame geprüfte Datei-/Build-Staging-Basis für manuelle Installation **und** spätere Ubuntu/Debian-`.deb`-Pakete. **Nicht** zwei Installationsimplementierungen pflegen. Das Paket und der manuelle Installer verwenden dieselben Worker, geprüften Artefakte, Zielpfad-/Config-Kontrakte und denselben versiegelten Inhaltsstand; nur Deployment- und Paketmanageradapter unterscheiden sich.

| Slice | Konkrete Umsetzung | Abnahme |
| --- | --- | --- |
| **MT.I0 – Installationsvertrag prüfen** | Bereits vorhandene Stage/Installer, paketierte Dateien, C++-festkodierte Pfade, Add-on-Zulassung, Service-/Benutzer-/Config-/State- und VDR-Video-Pfade vollständig inventarisieren; Ist/gewünschte Variabilität unterscheiden. Belegtes yaVDR-Referenzprofil; openSUSE/MLD: unbekannte Werte explizit als unbekannt markieren. | Host-/Quellmatrix mit getrenntem Status für Build, manuelle Installation, Paket und echte Laufzeitprüfung. Keine Host-Mutation. |
| **MT.I1 – Gemeinsame paketierbare Stage** | Bestehendes `make stage-install-runtime` und `stage_media_tools_runtime.py` gezielt wiederverwenden/verbessern. Deklarative Datei-/Versionliste mit Hashes, Binär-/Metadaten-/Config-/State-Trennung und vertrauenswürdiger Stage-Seal-Prüfung. `DESTDIR`/`PREFIX` **nur** anbieten, wenn tatsächliche Runtime- und Handlerpfade korrekt mitziehen; sonst sichtbare feste Grenze statt vorgetäuschter Portabilität. | Derselbe geprüfte Build-/Staging-Inhalt ist Eingang sowohl für manuelles Deployment als auch für spätere Paketierung. Keine abweichenden parallelen Dateibäume. |
| **MT.I2 – Einfacher manueller Installer/Updater** | Kleine Bedienoberfläche/CLI mit `prepare`, read-only `check`, ausdrücklichem `install/update`, `verify` und dokumentiertem Rollback. Konfigurierbare, **tatsächlich unterstützte** Werte: VDR-Videoziel, freigegebene Importroots, Account/Gruppe, Dienste, Install-/Config-/State-/Web-Ziele. Bestehende manuelle/paketierte Dateien, versiegelte Builds und Versionen erkennen und wiederverwenden; keine blind wiederholten Kompilierungen. | yaVDR-First-Install und Update mit Backups/Rollback, keinem heimlichen Quell-/DB-Verlust, keiner Dienstaktivierung/Grantvergabe durch bloße Installation. |
| **MT.I3 – Distributionsadapter und manuelle Konfiguration** | yaVDR/Ubuntu als verifizierter Ausgangspunkt. openSUSE/MLD nur anhand später geprüfter Host-/Maintainerdaten konfigurieren: Dienste/Init, reale VDR-Pfade, ABI, Gruppen/Nutzer, Abhängigkeiten und Deployment-Orte. Keine behaupteten Defaults oder stilles `systemctl` auf fremden Plattformen. | Mit synthetischen Nicht-Produktions-Roots und expliziten Beispielkonfigurationen testbar; echter Support für SUSE/MLD erst nach Host-Verifikation. Fehlende Angaben stoppen vor produktiven Änderungen. |
| **MT.I4 – Ubuntu/Debian-Paketbau aus genau derselben Stage** | An vorhandenes `debian/`- und Add-on-Package-Build-System anschließen, keine zweite Installlogik. Erst paketierbare Stage + Manifest, dann deklarative Paketdateiliste, `dpkg-deb`-Build/Contents, Architektur/ABI-Abhängigkeiten, optional getrennte Pakete für Suite/Core und Media Tools, Conffiles und Upgrade-Migration. Paket-Skripte dürfen Add-on nicht automatisch freischalten, Originalmedien nicht verändern und keine Host-Verzeichnisrechte korrigieren. **Manuell installierte Dateien niemals ungeprüft mit dpkg überlagern.** | Abgleich `.deb`-Inhalt gegen dieselben Stage-Hashes; isolierter Erstinstall-/Upgrade-/Purge-Test. Später PPA/CI möglich, **nicht** als schon eingerichtet behaupten. |
| **MT.I5 – Einfache Anleitung und Release-Gates** | Eine dokumentierte Methode für manuelle Ubuntu/yaVDR-Installation und Paketerzeugung aus dem gleichen Stand, saubere Konfigurationsvorlage für alternative Systeme, klarer Rollback und Abnahme-Checkliste. | Anwender können Installpfade und Dienste nachvollziehen; kein unnötiges Neu-Build pro Doku-Commit; Live-Installation erst nach eigener Freigabe. |

**Kernformel:** `ein Build → versiegelte Stage → [manueller Adapter ODER Ubuntu-.deb]`. Benutzer-/Root-Rechte, Dienstverwaltung, Konfiguration, lokale Freigabe, Paketbesitz und Add-on-Aktivierung bleiben unabhängig. **Keine Änderungen an bereits bestehenden Eigentümern/Dateimodi**, insbesondere kein rekursives `chmod`/`chown`.

## Phase MT.UX — Bedienoberfläche nach der Installationsbasis

| Slice | Konkrete Umsetzung | Abnahme |
| --- | --- | --- |
| **MT.UX0 – Navigation/Grundzustand** | `Media Import` direkt rechts von `Aufnahmen` in `web/frontend/index.html`. Vorhandene Modul-/Session-/Backend-Capability-Prüfung nicht entfernen. | Korrekte Reihenfolge, ohne wirksame Capability weiterhin ausgeblendet; Frontend-Tests grün. |
| **MT.UX1 – Einfacher rekursiver Nur-Lese-Scan** | Standardimportroot anzeigen, gesamte Ordnerhierarchie rekursiv erfassen und klare Trefferliste zeigen. Bestehende Scannergrenzen (u. a. 4096 Einträge/256 Treffer) müssen als **„Scan unvollständig“** mit sicherem Fortsetzungs-/Paginierungskonzept statt stillem Abschluss behandelt werden. | Filme und `Serien/Star Wars-Andor` gefunden, keine Quelle verändert; große Bäume/Fehler wahrheitsgemäß gemeldet. |
| **MT.UX2 – Administrierbare Quellen** | Anzeigenamen, Standardquelle und zusätzliche Roots pro Backend durch bestehenden Admin-/CSRF-/Audit-Vertrag, aus vorhandener root-kontrollierter Source-Inventory-Grenze; keine Root-Dateien direkt aus JS bearbeiten. | Backend-Trennung, Rechte, falsche Root/Symlink- und Rekonfigurationsfälle getestet. |
| **MT.UX3 – Explorer als Erweiterung** | Breadcrumbs, Ordner-/Dateiliste, Suche, Statusfilter, ausgewählte Dateien/Ordner und Mehrfachauswahl. Gleiche Source-Identität und Revalidierung wie Standardsuche, keine browsergelieferten absoluten Pfade. | Gezieltes Anwählen verschachtelter Ordner ohne Umgehung der erlaubten Wurzeln. |
| **MT.UX4 – Importvorschau/Entscheidungen** | Klarer Titel/Codec/Größe/Ziel/Kapazität/Dublette/Remux- oder Encode-Bedarf; `Importieren`, `Für HandBrake zurückstellen`, `Überspringen`, gesondert `Markierung aufheben`. Bereits bestehende M1.R-Operationen und Marker-Schutz verwenden. | Scan und Vorschau read-only; keine implizite Re-Encode-Freigabe, keine Auto-Freigabe von Markern/Originalverlust. |
| **MT.UX5 – Stapel- und Auftragsansicht** | `Alle geeigneten auswählen` mit Anzeige der Anzahl, bestätigte sequentielle Core-Aufträge, Fortschritt, Fehler/ungewisser Ausgang und sicheres Anhalten. Optionaler Nachtlauf nur mit späterer gesonderter expliziter Aktivierung. | Kein Duplikat bei Doppelklick/Netzfehler; Erfolg erst nach VDR-Recording-Readback; bestehende Actor-/Lease-/Idempotenzregeln. |
| **MT.UX6 – Shrink in Aufnahmeaktionen** | Vorhandenen HEVC-Vorprüf-/Stage-/Publish-Flow aus `recordings2-hero-detail.js` in `recordings2-actions.js` **wiederverwenden**, nicht kopieren. Aufnahmedetails: `Aufnahme verkleinern (HEVC)`; Detailkarte: `⋯`-Aktion bei zulässiger Aufnahme. Status/Auftrag am Aufnahmekontext anzeigen. | Geeignete und freigegebene Aufnahme zeigt Shrink, ungeeignete verständlich nicht verfügbar; Original bleibt erhalten, Publication separat bestätigt. |
| **MT.UX7 – Einfachheit/Abnahme** | Technische Dropdowns erst nach nachgewiesener Parität entfernen, responsive und tastaturbedienbar, verständliche Fehler, klare leere/teilweise Scans. Isolierte UI/Agent/Core-Kontrakttests und separate read-only-Hostabnahme. | Alltag ohne Tokenkenntnis: `Quelle → Scan → Auswahl → Vorschau → bestätigen`. Keine Rechte-, VDR- oder Aufnahmeänderung durch Leseaktionen. |

## Umsetzungsvorgaben

- **Beginnen bei MT.I0**, nicht mit einem neuen generischen Installationsframework oder einem unüberprüften `make install`.
- Je Slice vorhandene produktive Owner und Tests prüfen; nur die kleinste fehlende Funktion ergänzen. Kein zweiter C++-Importer, keine neue Rechteverwaltung/Operationstabelle.
- Keine `AGENTS.md`-Datei ohne ausdrücklichen Nutzerauftrag ändern.
- Build, `.deb`, Quellcommit und realer yaVDR-Test sind **verschiedene** Nachweise. Benutzer- und Systemdaten, vorhandene Eigentümer/Rechte, Aufnahmen, DB, Dienste und andere Add-ons bleiben bei Doku/Offline-Tests unverändert.
- Die Roadmap ist **keine Autorisierung**, Pakete zu installieren, VDR neu zu starten, Dateien zu importieren oder Aufnahmen zu verkleinern.
