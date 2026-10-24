# Media Tools: Installation und Backend-Agent-Voraussetzungen (yaVDR)

Diese Anleitung bezieht sich auf das **erste** lokale Installieren des Media-Tools-Addons aus dem VDR-Suite-Zweig `work/addon-platform-registry-lifecycle`, insbesondere auf [`tools/addons/install_media_tools.sh`](../tools/addons/install_media_tools.sh). Sie ist **keine** allgemeine Update-, Migrations- oder Deinstallationsanleitung. Der Installer verwendet jetzt `/srv/video/Filme` als bestätigten yaVDR/RecTools-Importstandard und `/srv/vdr/video.00` als vorgeschlagenes Videoziel. Beide Verzeichnisargumente sind **optional**.

## Warum benötigt Media Tools einen Backend-Agenten?

Der Media-Tools-Installer integriert Import- und Shrink-Funktionen in den **bestehenden** VDR-Suite-Backend-Agenten. Dabei wird nach der Paket- und Runtime-Installation ein Agent-systemd-Override mit den expliziten Optionen `--enable-media-import` und `--enable-media-shrink` erzeugt. Anschließend werden Suite-Daemon und Backend-Agent gestartet. Mit der bewusst durchgeführten Root-Administratorinstallation ist die Aktivierungsentscheidung bereits getroffen. Nach erfolgreicher technischer Agent- und Core-Verifikation werden die Funktionen ohne zweite Admin-Freigabe aktiv. Der Installer meldet `MEDIA_TOOLS_ACTIVATION=AUTOMATIC_WHEN_VERIFIED`; andere Benutzer erhalten weiterhin ihre eigenen Rechte.

Der produktive Installationsweg setzt deshalb voraus:

- `vdr-suite-daemon.service` ist installiert und **active/running**.
- `vdr-suite-backend-agent.service` ist installiert, für den Backend-Betrieb konfiguriert und als systemd-Einheit **loaded**. Der aktuelle Installer erlaubt `ActiveState=active` oder bei sauberem Ergebnis (`Result=success`) `inactive`; er startet den Agenten nach der Installation.
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
- Der Checkout muss exakt zum bei der Installation freigegebenen **Commit-PIN** passen. Der Operator-Block und der Installer prüfen zusätzlich den **aktuellen Remote-Head** des Addon-Zweigs. **Jeder Commit auf diesem Zweig, auch reine Dokumentation, macht einen zuvor kopierten PIN-Installationsblock ungültig.** Vor erneuter Verwendung PIN, Checkout und Review-Freigabe anhand des aktuellen Quellstands neu prüfen.
- Der Quell-Build erfolgt unprivilegiert; die Installationsphase benötigt Root und eine **unabhängig geprüfte** Handler-SHA256. Die vom gleichen Build angezeigte SHA256 einfach zurückzukopieren ist keine unabhängige Freigabe.
- Während der eigentlichen Installation stoppt das Skript **Suite-Daemon und Backend-Agent**, erstellt ein SQLite-Backup, installiert Paket und staged Suite-Runtime und startet die beiden Dienste wieder. Es enthält keinen expliziten Neustart von `vdr.service`. Das ist kein risikofreier, unterbrechungsfreier Vorgang.
- Nach `MEDIA_TOOLS_INSTALL=PASS` ist **keine zweite Administratoraktivierung** erforderlich. Der aktuell geprüfte Handler und Agent müssen jedoch weiterhin technisch zugelassen sein; für andere Benutzer werden Rechte separat vergeben. Ein installiertes Paket beweist keine bereits im Webfrontend funktionsgeprüfte Import-/Shrink-Aktion.

## Nach dem Installationsversuch

```bash
systemctl show vdr-suite-daemon vdr-suite-backend-agent \
  -p Id -p ActiveState -p SubState --no-pager
dpkg-query -W -f='${Package} ${Status} ${Version}\n' \
  vdr-suite-addon-media-tools 2>/dev/null || true
journalctl -u vdr-suite-backend-agent --since '10 minutes ago' -n 30 --no-pager -l
```

Bei Fehlschlag den konkreten `STOP=`-Code und das betroffene Installationslog prüfen; keine automatische Wiederholung bei unbekanntem Teilzustand. VDR-Aufnahmen, produktive Importaufträge und Shrink-Operationen benötigen vor jeder echten Mutation eine separate bewusste Freigabe.
