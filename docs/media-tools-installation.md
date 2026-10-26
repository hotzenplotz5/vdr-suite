# Media Tools: Installation und Backend-Agent-Voraussetzungen (yaVDR)

Diese Anleitung bezieht sich auf das **erste** lokale Installieren des Media-Tools-Addons aus dem VDR-Suite-Zweig `work/addon-platform-registry-lifecycle`, insbesondere auf [`tools/addons/install_media_tools.sh`](../tools/addons/install_media_tools.sh). Sie ist **keine** allgemeine Update-, Migrations- oder Deinstallationsanleitung. Der Installer verwendet jetzt `/srv/video/Filme` als bestätigten yaVDR/RecTools-Importstandard und `/srv/vdr/video.00` als vorgeschlagenes Videoziel. Beide Verzeichnisargumente sind **optional**.

### Installiertes Manifest ohne Änderungen an vorhandenen Rechten

Der native Backend-Agent prüft eine root-kontrollierte Kopie von `addon.json` unter
`/usr/libexec/vdr-suite/addons/rectools/addon.json`, direkt neben dem genehmigten
Import-Handler. Die identische Kopie unter
`/usr/share/vdr-suite/addons/rectools/addon.json` bleibt als rein beschreibendes
Inventar erhalten. Das Stage-Verfahren prüft, dass beide Kopien übereinstimmen.
Dadurch bleibt der Agent auch dann startfähig, wenn das **bereits vorhandene**
`/usr/share/vdr-suite` für ihn nicht durchquerbar ist (`root:root 0700`).
Es werden **keine bestehenden Verzeichnisrechte oder Eigentümer geändert**.
Die C++-Prüfung verlangt weiterhin root-eigene, nicht gruppen-/weltbeschreibbare
Verzeichnisse und Manifestdateien sowie die gepinnte Handler-SHA256.

## Warum benötigt Media Tools einen Backend-Agenten?

Der bestehende manuelle Erstinstallationsadapter bereitet die Media-Tools-Integration in den **bestehenden** VDR-Suite-Backend-Agenten vor. Er schreibt bei ausdrücklich geprüfter Erstinstallation einen systemd-Override mit `--enable-media-import` und `--enable-media-shrink`, **startet aber weder Suite-Daemon noch Agenten** und erteilt **keine** Handler-, Core- oder Benutzerfreigabe. Der tatsächliche Erfolg meldet `IMPORT_AND_SHRINK_AGENT_OPT_IN=CONFIGURED_BUT_INACTIVE`, `MEDIA_TOOLS_HANDLER_APPROVAL=NOT_ISSUED` und `MEDIA_TOOLS_CORE_APPROVAL=NOT_ISSUED`. Der optionale Stage-basierte Debian-Paketweg installiert nur inaktiven Payload. Installation, Aktivierung und Rechte sind getrennte Entscheidungen; Installation und Deinstallation direkt über das Webfrontend sind derzeit nicht implementiert.

Der produktive Installationsweg setzt deshalb voraus:

- `vdr-suite-daemon.service` ist installiert und **active/running**.
- `vdr-suite-backend-agent.service` ist installiert, für den Backend-Betrieb konfiguriert und als systemd-Einheit **loaded**. Der aktuelle Installer erlaubt `ActiveState=active` oder bei sauberem Ergebnis (`Result=success`) `inactive`; nach einem tatsächlichen Deploy verbleiben Agent und Daemon bis zur gesondert genehmigten Inbetriebnahme **gestoppt**.
- `/etc/vdr-suite/backend-agent.conf` enthält die tatsächliche `BACKEND_ID` und eine gültig konfigurierte `CONTROL_PLANE_URL=https://...`, nicht `example.invalid`. Ein HTTPS-Eintrag allein beweist **keine** funktionierende Verbindung.
- Die aktive Suite-Datenbank liegt nachweislich am vom Installer erwarteten Pfad `/var/lib/vdr-suite/vdr-suite.db`. Andere Installationen dürfen nicht durch bloßes Anlegen einer Datei passend gemacht werden.
- Der Installationsbenutzer und `vdr` haben die erforderlichen Rechte auf den existierenden Medienverzeichnissen; weitere Ressourcen-, Integritäts- und Sicherheitsprüfungen stehen **im Skript**.

`systemctl is-enabled ...` darf `disabled` melden, obwohl der Dienst für die laufende Sitzung `active` ist. Die Entscheidung über einen dauerhaften Autostart ist **separat** zu treffen. Auf keinen Fall den Agenten ohne Prüfung des beabsichtigten Backend-Betriebs automatisch aktivieren.

## Vorprüfung (ändert den Dienstzustand nicht)

Auf dem VDR als Root:

```bash
echo '=== Dienste ==='
systemctl show vdr-suite-daemon vdr-suite-backend-agent \
  -p Id -p LoadState -p ActiveState -p SubState -p UnitFileState \
  --no-pager

echo '=== Konfiguration (ohne URL oder Zugangsdaten auszugeben) ==='
conf=/etc/vdr-suite/backend-agent.conf
if [ -f "$conf" ] &&
   grep -Eq '^CONTROL_PLANE_URL=https://[^[:space:]]+$' "$conf" &&
   ! grep -q 'example.invalid' "$conf"; then
  echo 'CONTROL_PLANE=CONFIGURED'
else
  echo 'CONTROL_PLANE=NOT_CONFIGURED'
fi
grep '^BACKEND_ID=' "$conf" 2>/dev/null || true
for f in identity enrollment; do
  if [ -e "/var/lib/vdr-suite/backend-agent/$f" ]; then
    echo "$f=PRESENT"
  else
    echo "$f=MISSING"
  fi
done

echo '=== Aktuelle Agent-Meldungen ==='
journalctl -u vdr-suite-backend-agent --since '10 minutes ago' -n 30 --no-pager -l
```

Vor dem Teilen von Logausgaben Zugangsdaten, Tokens, URLs mit eingebetteten Secrets und andere sensible Werte entfernen. Die bloße Existenz von `identity` belegt keine aktuell erfolgreiche Verbindung. Eine fehlende `enrollment`-Datei kann **nach** erfolgreicher Registrierung normal sein.

Wenn der Agent absichtlich deaktiviert war, darf ihn der Betreiber **nach Prüfung seiner Konfiguration und der möglicherweise bereits freigegebenen Agent-Aufträge** für die laufende Sitzung starten:

```bash
systemctl start vdr-suite-backend-agent
systemctl show vdr-suite-backend-agent \
  -p ActiveState -p SubState -p Result -p ExecMainStatus
journalctl -u vdr-suite-backend-agent --since '5 minutes ago' -n 30 --no-pager -l
```

`start` ist **nicht** `enable`: Es ändert die dauerhafte Autostart-Einstellung nicht. Der Agent baut ausgehende Verbindungen auf und kann autorisierte Backend-Aufträge abarbeiten; der Start ist deshalb nicht als wirkungsloser Diagnoseschritt zu behandeln. Ein `active/running` ist notwendig, aber kein alleiniger End-to-End-Funktionstest.

## Bekannte Fehlermeldungen

| Ausgabe | Bedeutung und nächster Schritt |
| --- | --- |
| `STOP=SUITE_OR_BACKEND_AGENT_INACTIVE` | Kommt aus dem vorgeschalteten Operator-Installationsblock: mindestens einer der zwei Dienste war bei der Prüfung inaktiv. Mit `systemctl show` die beiden getrennt untersuchen. Der darunterliegende Installer hat eigene spezifische Prüfungen. |
| `Backend Agent unit missing or failed` / `Backend Agent unsafe state before install` | Der aktuelle Installer verweigert ungeladene oder fehlgeschlagene Agent-Einheiten. Ein sauber inaktiver Agent ist vor der Installation erlaubt; `systemctl enable` nicht erzwingen. |
| `CONTROL_PLANE=NOT_CONFIGURED` oder `Agent is still placeholder` | Die Konfiguration ist nicht für eine reale Control Plane eingerichtet. Zuerst die reguläre Backend-Einrichtung abschließen; keine erfundene HTTPS-Adresse einsetzen. |
| `protected_transport_failed` | Agent-Kommunikation ist fehlgeschlagen; aktuelle Logs sowie TLS-/Netzwerk-/Gegenstellenstatus untersuchen. Nicht automatisch als Medien-Addon- oder Installationsfehler interpretieren. |
| `command_database_unavailable` | Ein Agent-Command-Poll meldet einen nicht verfügbaren Command-Datenbankpfad; aktuellen Zustand des Control Plane/Backends prüfen, bevor man produktive Aufträge erwartet. |
| `Backend Agent synchronized` | Positive Einzelmeldung, aber allein kein umfassender Import-/Shrink-/Berechtigungsnachweis. |

**Zeitstempel beachten:** In einem dokumentierten yaVDR-Fall enthielt `journalctl -n 20` am 24.10.2026 ältere Meldungen vom 22.10.2026 (`protected_transport_failed`, `command_database_unavailable`). Nach dem erneuten Start war `ActiveState=active` / `SubState=running` zu sehen. Die alten Meldungen sind **kein Beweis**, dass die Störung nach dem Neustart noch besteht; der Start allein beweist aber auch keine aktuelle Synchronisierung. Deshalb neue Meldungen mit `--since` lesen, nicht nur die letzten Zeilen unterschiedlicher Boot-Sitzungen.

## Weitere Installationsgrenzen

- Quelle und Ziel müssen bei späterer **Aktivierung** existieren, kanonische absolute Pfade sein und dürfen nicht identisch sein. Die Erstinstallation darf auch bei fehlendem Standardverzeichnis weiterlaufen: dann wird **kein** Quellinventar angelegt und Media Import bleibt bis zur Konfiguration nicht verfügbar. Existierende Verzeichnisse müssen für `vdr` zugänglich sein. Quellpfade unter `/home`, `/root` und `/run/user` sind wegen der Agent-Sandbox nicht zulässig. Andere Pfade lassen sich alternativ als beide expliziten CLI-Argumente angeben. Ein vollwertiger Browser-Einstellungsdialog mit sicherer privilegierter Backend-Konfiguration ist noch **nicht implementiert**; bis dahin ist die Konfiguration eine kontrollierte Root-Administrationsaufgabe, kein direktes Schreiben aus dem Browser.
- Der Installer prüft ausreichend freien Speicher mit Reserven pro Dateisystem; diese Prüfung nicht umgehen.
- Der Checkout muss exakt zum bei der Installation freigegebenen **Commit-PIN** passen. Der Operator-Block und der Installer prüfen zusätzlich den **aktuellen Remote-Head** des Addon-Zweigs. Ein versiegelter, bereits unabhängig geprüfter Build-PIN darf ein Vorfahre des aktuellen offiziellen Branch-HEADs sein, sofern der Installer die exakte Herkunft, den vorbereiteten Stage-Inhalt und die unabhängige Handler-SHA256 erfolgreich prüft. Ein reiner späterer Dokumentationscommit erzwingt keinen Neubuild. Vor einem Installationsversuch muss die konkrete Source-/Stage-Provenienz erneut überprüft werden.
- Der Quell-Build erfolgt unprivilegiert; die Installationsphase benötigt Root und eine **unabhängig geprüfte** Handler-SHA256. Die vom gleichen Build angezeigte SHA256 einfach zurückzukopieren ist keine unabhängige Freigabe.
- Während der ausdrücklich bestätigten **manuellen Erstinstallation** stoppt das Skript Suite-Daemon und Backend-Agent, erstellt ein SQLite-Backup, deployt die versiegelte Suite+Add-on-Stage und hinterlegt den zunächst inaktiven Agent-Opt-in. **Beide Dienste bleiben danach gestoppt und benötigen eine separate Freigabe zum Start.** Es enthält keinen VDR-Neustart. Dies ist kein risikofreier oder unterbrechungsfreier Vorgang.
- Nach `MEDIA_TOOLS_INSTALL=PASS` sind Handler- und Core-Freigabe ausdrücklich **noch nicht ausgestellt**, die Services bleiben gestoppt und die Medienaktionen inaktiv. Freigaben, Dienststart und separate Benutzerrechte benötigen ihre jeweils bestehenden geprüften Betreiberverfahren. Ein installiertes Paket beweist weder effektive Aktivierung noch eine funktionsgeprüfte Import-/Shrink-Aktion.

## Nach dem Installationsversuch

```bash
systemctl show vdr-suite-daemon vdr-suite-backend-agent \
  -p Id -p ActiveState -p SubState --no-pager
dpkg-query -W -f='${Package} ${Status} ${Version}\n' \
  vdr-suite-addon-media-tools 2>/dev/null || true
journalctl -u vdr-suite-backend-agent --since '10 minutes ago' -n 30 --no-pager -l
```

Bei Fehlschlag den konkreten `STOP=`-Code und das betroffene Installationslog prüfen; keine automatische Wiederholung bei unbekanntem Teilzustand. VDR-Aufnahmen, produktive Importaufträge und Shrink-Operationen benötigen vor jeder echten Mutation eine separate bewusste Freigabe.
