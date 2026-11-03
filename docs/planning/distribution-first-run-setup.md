# Gemeinsame VDR-Suite-Ersteinrichtung – alle unterstützten Distributionen

**Stand:** 03.11.2026 – gemeinsame Implementierung und Quell-/Paket-CI für SHA `7e5c76cd` PASS; automatische Erstaktivierung auf realem VDR und externes MLD-Rezept noch nicht abgenommen.  
**Pilot-/Arbeitsbranch:** `work/mld66-skippy-feedback-20261027`; kein neuer Distributions-Fork.  
**Historischer Ausgangspunkt:** [MLD.F2](mld66-first-run-setup.md). MLD ist ein Testfall, keine Produktgrenze.

## Verbindliches Benutzerziel

Nach Installation oder Erststart von VDR-Suite soll ein Betreiber auf **jeder unterstützten Distribution** (insbesondere MLD 6.6, Ubuntu, yaVDR und Debian-basierte Installationen) dieselbe Web-Ersteinrichtung sehen. Ein lokaler Einzel-VDR soll ohne manuelle Backend-ID, Enrollment-Datei, `systemctl`-Befehle oder technische Agent-Konfiguration benutzbar werden. Die vorhandene First-Admin-Anlage bleibt Teil des normalen Browserwegs.

Es gibt **eine gemeinsame Suite-Implementierung** für Erkennen, einmaliges Enrollment, Agent-Aktivierung, Channel-Readback, Status und Wiederaufnahme. Distributionen liefern nur ihre tatsächlichen Pfade/Dienstkonten, Paket-/Service-Auslöser und ggf. Control-Plane-/CA-Konfiguration. Keine separate MLD-Logik, kein zusätzlicher Agent, keine zweite Datenbank, kein neuer öffentlicher privilegierter HTTP-Endpunkt.

Ein Installations- oder First-Boot-Prozess führt den bestehenden lokalen `vdr-suite-backend-agent-autostart --installer-onboard` bei bereitstehender Konfiguration **automatisch** aus. Der Browser liest nur den vorhandenen Setup-/Health-Status und zeigt verständlichen Fortschritt sowie erneutes Prüfen. Bestehende konfigurierte Installationen behalten Identität, Einstellungen, Timer, Aufnahmen und Dienstzustand; Upgrades erzwingen kein neues Enrollment.

## F2.5: korrigierter distributionsübergreifender Lieferumfang

1. **Gemeinsamer Installationsvertrag:** Standardisierte Inputs für lokale VDR-Quelle, tatsächlichen Senderpfad, Dienstbenutzer, Control-Plane-HTTPS-Origin, optionales CA-Zertifikat und First-Run-Auslösung. Die vorhandenen installierten Dateien und der vorhandene Agent sind die einzige Runtime.
2. **Paketadapter:** Debian/Ubuntu/yaVDR, MLD und weitere unterstützte Paket-/Installationswege rufen denselben gemeinsamen Onboarding-Schritt auf; Packaging-spezifische Details gehören ausschließlich in den jeweiligen Adapter. Keine Kopie des Programmlogik-Codes in Rezepte oder Maintainer-Skripte.
3. **Auslösezeitpunkt:** Automatisch nach VDR-/Daemon- und Control-Plane-/Kanalkonfiguration oder beim ersten geeigneten Systemstart, mit idempotenter Wiederaufnahme nach Unterbrechung. Nicht ungeprüft in `postinst` mit Platzhalter-URL ausführen; auch keine Benutzeraufforderung zu Root-Kommandos als Ersatz.
4. **Einheitliches UX-Ergebnis:** Die bestehende Admin-Weboberfläche zeigt die gleichen Zustände `unregistered`, `agent_offline`, `health_pending`, `channels_pending`, `ready` für alle Distributionen. Kein neues manuelles Setup-Formular.
5. **Kompatibilität:** Eine vorhandene Agent-Identität wird nie durch ein Update ersetzt. Lokale Fälle mit nicht unterstützter Topologie bekommen eine verständliche Fehlermeldung, nicht stillschweigend eine neue Registrierung.
6. **Abnahme:** Zuerst isolierte Tests des gemeinsamen Programms, danach Paket-/First-Boot-Wiederholungstests auf MLD, Ubuntu/yaVDR und Debian sowie einem manuellen Installationspfad; echte Readback-Evidenz für registrierten Agenten und aktuellen Channel-Snapshot. CI, Pakettest und reale Installation getrennt berichten.

## Derzeitiger Quellstand versus offene Arbeit

Der Branch enthält den gemeinsamen `--installer-onboard`-Einstieg und die nur lesend arbeitende Web-Statusanzeige. **Gemeinsames Staging implementiert:** `tools/distribution_first_run_stage.py` ist jetzt der einzige Quellcode für das Paket-Staging; `tools/mld66_stage_first_run.py` delegiert lediglich an ihn (die bisherige MLD-Ausgabe bleibt erhalten). `make stage-distribution-first-run-config` stellt dieselben Optionen für alle Distributionen bereit, während `make stage-mld-first-run-config` weiterhin funktioniert. `tools/test_packaging_distribution_first_run_stage.py` enthält no-root Testfälle für MLD, Ubuntu/yaVDR, Wiederholung und vorhandene Konfiguration; Tests auf SHA `7e5c76cd` im CI-Paketworkflow ausgeführt: PASS; keine reale MLD-Erststartabnahme. **Ein gemeinsamer systemd-Quell-Trigger ist jetzt implementiert:** Beim Start des VDR-Suite-Daemons zieht dessen `Wants` die getrennte `vdr-suite-first-run.service` und `vdr-suite-first-run.path` heran. Das One-Shot ruft den bestehenden Agent-Helfer mit `--auto-first-run` auf; fehlende VDR-/Daemon-/Agent-/Channel-/HTTPS-Voraussetzungen werden ohne Mutation zurückgestellt. Änderungen der Agent-Konfiguration lösen eine erneute Prüfung aus. `mk/install.mk` installiert beide Units; die Debian-/Ubuntu-/yaVDR-Daemon-Paketliste enthält sie. `debian/rules` behält `dh_installsystemd --no-enable --no-start`: Die First-Run-Units werden nicht eigenständig durch das Paket aktiviert, sondern mit dem bereits laufenden Suite-Daemon angefordert. **Quellimplementierung ist nicht gleich reale Paket- und Boot-Abnahme.** Für frische lokale Einzelserver nutzt der Agent nun exakt `http://127.0.0.1:18080` (direkter Loopback zum bestehenden Daemon) und die vorhandene `channels.conf`; **externe Agent-Verbindungen bleiben HTTPS-pflichtig**. Debian/Ubuntu-Binärpakete wurden im CI gebaut und in isolierten Umgebungen ohne aktivierte Dienste installiert; **die tatsächliche automatische Erstregistrierung bleibt ungetestet**.

**Nicht als fertig markieren**, bevor sämtliche unterstützten Installationswege denselben automatischen Setup-Lifecycle verwenden und mindestens MLD, Ubuntu/yaVDR und Debian einen wiederholbaren Installations-/Upgrade-Test bestehen. Skippys MLD-F2.6-Realtest bleibt eine konkrete Abnahme innerhalb dieser universellen Funktion.

## Nächste Umsetzung: automatischer First-Run-Trigger

Der universelle Staging-Vertrag alleine richtet noch kein installiertes System ein.
**Source done, Paket-/Realtest offen:** Der gemeinsame Auto-Trigger ruft `--auto-first-run` auf. Wenn ein neuer lokaler Backend-Agent noch die Paketvorlage verwendet, wird ohne Browser-/Root-Schritte der einzige lokale HTTP-Loopback-Control-Plane-Origin gesetzt und die vorhandene `channels.conf` eingebunden. Danach wird derselbe `--installer-onboard`-Code verwendet. Ein vorhandener Agent mit Identity wird beim automatischen Bootlauf **nicht** gestartet, ersetzt oder neu registriert. Remote-Agents benötigen weiterhin eine echte HTTPS-Konfiguration.
Debian/Ubuntu/yaVDR und MLD sollen denselben Entry Point benutzen;
nur die native Paket-Anbindung unterscheidet sich. **Nicht** bei jedem Upgrade
eine neue Identity erzeugen oder einen existierenden Agent ungewollt neu starten.
Ohne nachgewiesenen Trigger bleibt F2.5 offen.

### Gemeinsamer Staging-Aufruf (Paketbau, nicht Live-System)

```sh
make stage-distribution-first-run-config \
  DESTDIR="$PACKAGE_STAGING_ROOT" \
  DISTRIBUTION_FIRST_RUN_CONTROL_PLANE_URL="$LOCAL_HTTPS_ORIGIN" \
  DISTRIBUTION_FIRST_RUN_CHANNELS_CONF_PATH="$VDR_CHANNELS_PATH" \
  DISTRIBUTION_FIRST_RUN_CA_CERTIFICATE_PATH="$TRUSTED_CA_PATH"
```

Ein lokales Paket kann die `DISTRIBUTION_FIRST_RUN_CONTROL_PLANE_URL`-Variable leer lassen: der bekannte Loopback-Daemon wird dann gewählt. Entfernte Control Planes müssen weiterhin eine verifizierte HTTPS-Origin und ggf. CA erhalten. Die manuelle Senderpfad-Angabe ist für distributionsspezifische Paketrezepturen weiterhin möglich; der frische Laufzeitpfad erkennt die vorhandene lokale VDR-Konfiguration selbst.

### Verifikation des neuen automatischen Auslösers

- `tools/test_packaging_backend_agent_autostart.py`: ohne root, konfigurierte und zurückgestellte automatische Ausführung.
- `tools/test_packaging_distribution_first_run_stage.py`: gemeinsame Paketdateien und Aktivierungskontrakte.
- `tools/check_systemd_unit_contract.py`: Daemon-`Wants`, unabhängige One-Shot-Unit und `.path`-Beobachter.
- **CI-Nachweis für `7e5c76cd`:** [VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/runs/41550527559): Packaging, Agent-Client-C++, Systemd-Verträge, Frontend, Architektur und Dokumentation erfolgreich. [Experimental SuiteBridge package](https://github.com/hotzenplotz5/vdr-suite/actions/runs/41550527758): Drei Debian-Binärpakete auf Ubuntu 24.04/26.04 gebaut; Disposable-Installations-, Upgrade-, Remove- und Purge-Tests erfolgreich, dabei **Dienststart ausdrücklich verhindert**.
- **Noch nicht belegt:** echte automatische Systemd-Start-/Enrollment-Transaktion, externer MLD-BitBake-Rezept-Einbau, MLD-/Ubuntu-/yaVDR-Erststart mit aktueller Channel-Observation sowie anschließender Reboot und Live-TV-End-to-End.

## Quellabschluss der ersten automatischen Paketaktivierung (02.11.2026)

- C++-Agenttransport akzeptiert **ausschließlich** `http://127.0.0.1:18080` zusätzlich zu bestehenden HTTPS-Origins. Keine entfernte HTTP-Adresse, keine Umleitung und kein Proxy für diese lokale Verbindung.
- `tools/vdr_suite_backend_agent_autostart.py` füllt beim **frischen** lokalen Default-Backend fehlenden Control-Plane-Origin, Senderpfad und Channel-Observation aus. Geänderte Konfiguration wird nach Prüfung atomar geschrieben; vorhandene Identities und individuelle Origins werden nicht ersetzt.
- Die neue `debian/vdr-suite-daemon.postinst` aktiviert/startet den Suite-Daemon nur bei **Erstinstallation**. `debian/vdr-suite-backend-agent.postinst` stößt bei nachgelagerter Erstinstallation des Agenten denselben vorhandenen First-Run-Service an. Beide sind Paket-Hooks, keine Duplikate der Onboarding-Logik.
- Der lokale Setup-Status ist in neuen Daemon-Paketdefaults aktiviert, um Fortschritt und eventuelle ausstehende Senderbeobachtung im bestehenden Webfrontend anzeigen zu können.
- Regressionen für exakte Loopback-Origin, unveränderte Identity, MLD-Staging und Debian-Paketintegration sind auf `7e5c76cd` im CI **PASS**. **Nicht** als Real-Hardware-PASS zu werten.
- Das externe MLD-BitBake-Rezept ist nicht Teil dieses Repositories. Es muss die gemeinsamen Units/Pakethooks im nativen MLD-Lebenszyklus installieren. **MLD-Rezeptintegration, MLD-Paketbuild, Realtest, Reboot und Live-TV-PASS offen; Ubuntu-Debian-Paketbuild im CI PASS.**

## Nächster Abnahmeschritt: F2.6 reale Erstinstallation

Die bestehenden Quelltests und isolierten Debian-Paketbuilds müssen **nicht erneut entwickelt** werden. Zu prüfen ist jetzt der tatsächliche Laufzeit-Lifecycle, nicht ein weiterer statischer Mock:

1. **MLD 6.6 BitBake:** Das externe Rezept muss die bereits gebauten Backend-Agent-/Daemon-Dateien, gemeinsamen `vdr-suite-first-run.service`/`.path`-Units, Dienstnutzer und First-Run-Auslöser installieren. Das BitBake-Rezept liegt nicht im VDR-Suite-Repository und ist hier nicht geändert.
2. **Ubuntu/yaVDR und MLD:** Frische Testinstallation mit VDR und vorhandener `channels.conf`; nach automatischem Paket-/Daemon-Start müssen ohne technische Benutzerschritte eine persistierte Agent-Identity, Online-Lease, frische Backend-Health und aktuelle Channel-Observation für den Backend `default` vorliegen.
3. **Wiederholung:** Reboot und Upgrade erhalten identische Identity/Backend-ID, Receiver-/Timer-/Aufnahmedaten und den bestehenden Agent-Dienststatus. Es darf keine zweite Registrierung oder Enrollment-Ausgabe entstehen.
4. **Browser:** Bestehenden First-Admin-Dialog, Setup-Status, Senderanzeige und Live-TV auf der realen VDR-Installation prüfen. Separat `UNIT/CONTRACT=PASS`, `PACKAGE-BUILD=PASS`, `FIRST-BOOT=OPEN` und `REAL-VDR=OPEN` dokumentieren.

**Stoppschwelle:** F2.5-Quell- und Packaging-Tests sind abgeschlossen; das End-to-End-Produktziel bleibt bis F2.6-Nachweis auf echten Systemen **offen**. Ein CI-Runner mit blockierten Service-Starts kann diesen Nachweis nicht ersetzen.
