# Gemeinsame VDR-Suite-Ersteinrichtung – alle unterstützten Distributionen

**Stand:** 02.11.2026 – verbindliches Ziel, Implementierung und Realtests noch offen.  
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

Der Branch enthält den gemeinsamen `--installer-onboard`-Einstieg und die nur lesend arbeitende Web-Statusanzeige. **Gemeinsames Staging implementiert:** `tools/distribution_first_run_stage.py` ist jetzt der einzige Quellcode für das Paket-Staging; `tools/mld66_stage_first_run.py` delegiert lediglich an ihn (die bisherige MLD-Ausgabe bleibt erhalten). `make stage-distribution-first-run-config` stellt dieselben Optionen für alle Distributionen bereit, während `make stage-mld-first-run-config` weiterhin funktioniert. `tools/test_packaging_distribution_first_run_stage.py` enthält no-root Testfälle für MLD, Ubuntu/yaVDR, Wiederholung und vorhandene Konfiguration; Testergebnisse sind noch nicht ausgeführt und keine reale Paketabnahme. **Ein gemeinsamer systemd-Quell-Trigger ist jetzt implementiert:** Beim Start des VDR-Suite-Daemons zieht dessen `Wants` die getrennte `vdr-suite-first-run.service` und `vdr-suite-first-run.path` heran. Das One-Shot ruft den bestehenden Agent-Helfer mit `--auto-first-run` auf; fehlende VDR-/Daemon-/Agent-/Channel-/HTTPS-Voraussetzungen werden ohne Mutation zurückgestellt. Änderungen der Agent-Konfiguration lösen eine erneute Prüfung aus. `mk/install.mk` installiert beide Units; die Debian-/Ubuntu-/yaVDR-Daemon-Paketliste enthält sie. `debian/rules` behält `dh_installsystemd --no-enable --no-start`: Die First-Run-Units werden nicht eigenständig durch das Paket aktiviert, sondern mit dem bereits laufenden Suite-Daemon angefordert. **Quellimplementierung ist nicht gleich reale Paket- und Boot-Abnahme.** Eine vorhandene lokale HTTPS-/Channel-Konfiguration ist weiterhin Voraussetzung.

**Nicht als fertig markieren**, bevor sämtliche unterstützten Installationswege denselben automatischen Setup-Lifecycle verwenden und mindestens MLD, Ubuntu/yaVDR und Debian einen wiederholbaren Installations-/Upgrade-Test bestehen. Skippys MLD-F2.6-Realtest bleibt eine konkrete Abnahme innerhalb dieser universellen Funktion.

## Nächste Umsetzung: automatischer First-Run-Trigger

Der universelle Staging-Vertrag alleine richtet noch kein installiertes System ein.
**Source done, Paket-/Realtest offen:** Der gemeinsame Auto-Trigger ruft `--auto-first-run` auf, das bei Bereitschaft den identischen vorhandenen `--installer-onboard`-Code verwendet. Jetzt müssen die tatsächlichen Paketabläufe eine gültige lokale HTTPS-Origin, den Senderpfad und Dienstkonto-Rechte einrichten sowie auf echten Systemen geprüft werden.
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

Ein reales Paket darf diese Werte nicht blind mit Beispiel-URLs befüllen.
Die Package-Inputs bleiben zunächst explizit; automatische Erkennung der
tatsächlichen lokalen HTTPS-/Kanal-Konfiguration ist ein offener Schritt.

### Verifikation des neuen automatischen Auslösers

- `tools/test_packaging_backend_agent_autostart.py`: ohne root, konfigurierte und zurückgestellte automatische Ausführung.
- `tools/test_packaging_distribution_first_run_stage.py`: gemeinsame Paketdateien und Aktivierungskontrakte.
- `tools/check_systemd_unit_contract.py`: Daemon-`Wants`, unabhängige One-Shot-Unit und `.path`-Beobachter.
- **Noch nicht belegt:** ausgeführte Tests und echte installierte Systemd-Transaktion; produktive HTTPS-Konfiguration des lokalen Control Plane; MLD-externes Rezept; Web- und Live-TV-End-to-End.
