# Agent-Operator-Workflows: direkt vom Fix zum Update oder zur Diagnose

Stand: 15.11.2026. **Arbeitsleitfaden, keine neue Installationsfreigabe,
kein CI-Gate und kein paralleler Installer.** Die bindenden Vorgaben stehen in
[AGENTS.md](../../AGENTS.md), besonders AG-003, AG-005, AG-006, AG-007,
AG-008 und AG-011 aus dem [Regelregister](agent-rule-register.md).
Vor Nutzung die aktuelle Quellimplementierung und den konkreten Host
prüfen. Dieser Text ist eine Vorlage für **konkrete** Operator-Blöcke,
keine Behauptung, dass eine bestimmte Installation bereits erfolgt ist.

## 1. Welcher Weg ist der richtige?

| Änderung / Situation | Vorgehen | Nicht als Routine |
| --- | --- | --- |
| Nur bekannte statische Frontend-Dateien, etwa `app.js` und `modules/media-import.js` | Genau die Dateien aus der nachgewiesenen Source-SHA aktualisieren, Bytes und Browserwirkung prüfen | Runtime-Komplettstage, Daemon-Rebuild, Service-Neustart, CI-Warten, Branch-Namenssperre |
| Zusammengesetztes Frontend oder vollständiger Runtime-Wechsel | [Canonical Runtime-Deployment](install-runtime-deployment-reliability.md), ggf. dessen `deploy-frontend`/`check-frontend` für Frontend-only | Rohes `web/frontend` mit kompletter Installationskomposition gleichsetzen |
| Daemon-Fix ohne Paketmigration | Vorhandenen `tools/install_daemon.py` für `check`, Build-Wiederverwendung, ggf. einmaliges `build` und separat genehmigtes `apply` verwenden | Doppelter Build, unbewiesene Schema-Freigabe, Root-Shell verlassen |
| Debian-/Ubuntu-Pakete, einschließlich Media Tools | Source-/Paket-/ABI-Provenienz und isolierte Paketabnahme von einem echten Host-Upgrade trennen | `apt install`/`dpkg -i` auf einem vorhandenen yaVDR allein aufgrund erfolgreicher Paket-CI |
| Fehlerursache noch unbekannt | Sofort den kleinsten lesenden `## Diagnoseblock` liefern | Spekulative Reparatur, `chmod`/`chown`, Rebuild, Backup oder automatischer Neustart |
| Reine Doku-Änderung | Kleiner lesender `## Testblock` zur Dateiprüfung | Runtime-Abnahme oder CI-Wartezeit erneut auslösen |

**Scope-Unterschied:** Beauftragte yaVDR-Installationen laufen in der
bestehenden Root-Shell. **Isolierte** Debian-Paketbuilds unterliegen
weiterhin [debian/AGENTS.md](../../debian/AGENTS.md), einschließlich
des dort vorgeschriebenen gewöhnlichen Build-Benutzers.

## 2. Was in der Abschlussantwort stehen muss

1. **Exakte Identität:** Repo, tatsächlich geprüfter bestehender Branch,
   volle Kandidaten-SHA, betroffene Dateien und tatsächlicher Teststatus.
   Produktcode-SHA, neue Dokumentations-SHA, Paketversion und laufende
   Host-Binary-Version nicht vermischen.
2. **Nächste Aktion direkt:** Bei fertigem Fix **in derselben Antwort**
   `## Testblock` mit dem nachgewiesenen kleinsten Update/Installationsweg
   und direktem Readback; bei ungeklärter Ursache **in derselben Antwort**
   `## Diagnoseblock`. Nicht nach einem weiteren Chat zum Nachreichen
   des Blocks fragen.
3. **Sofort kopierbar:** Der Agent ersetzt alle Musterwerte durch
   **geprüfte reale Werte** und prüft Befehle, Pfade und Make-Ziele gegen
   aktuelle Quellen. Ein bloßer Commit-Link oder ein Platzhalter wie
   `FULL40` ist **kein** fertiger Operator-Block.
4. **Genau ein Schlussblock:** Nur `## Testblock` oder `## Diagnoseblock`,
   unmittelbar gefolgt von **einem** Bash-Codeblock; keine Prosa danach.
   Zu Beginn `set +e`, `set +u`, `set +o pipefail`. Kein
   `set -euo pipefail`, kein ungeschütztes `exit`, kein Wechsel des
   Unix-Benutzers für eine yaVDR-Installation.
5. **Keine falsche Freigabe:** Das Bereitstellen einer möglichen
   Installation autorisiert keine unangekündigte Ausführung. Bei
   fehlender geprüfter Installationsmöglichkeit sofort lesende
   Diagnose statt eines erfundenen mutierenden Befehls ausgeben.
   Unabhängige bereits freigegebene Arbeit fortsetzen.
6. **Nachweis getrennt:** `UNIT/CONTRACT`, `PRODUCTION-INTEGRATION`,
   `CI` und `REAL-VDR` unterscheiden. Ein Bytevergleich, ein grüner
   Build oder ein erfolgreicher Git-Commit sind keine echte TV-Abnahme.

## 3. Frontend-Fix: kleine Dateiänderung

Die von `mk/install.mk` belegte Zuordnung umfasst:
- `web/frontend/app.js` →
  `/usr/share/vdr-suite/web/frontend/app.js`;
- `web/frontend/modules/media-import.js` →
  `/usr/share/vdr-suite/web/frontend/modules/media-import.js`.

Die zentrale `AGENTS.md` erlaubt bei einem **gezielten Zwei-Dateien-Fix**
diese beiden Dateien direkt aus einem **nachgewiesenen gepinnten Commit**
zu ersetzen. Die lokale Branch-Wahl ist dabei kein künstlicher Stopper.
Vor dem Ersetzen sollen der Agent und sein Fix-Test die exakten
Quelldateien geprüft haben. Danach beide installierten Dateien bytegenau
mit dem Kandidaten vergleichen und den **tatsächlichen Browserpfad**
frisch laden/testen. Keinen Daemon oder Backend-Agent neu starten,
keine neue Stage oder einen Komplettbuild erzeugen.

**Lesendes Diagnosemuster** (kein Update und keine Behauptung, dass
der sichtbare Browser bereits diese Bytes geladen hat):

~~~bash
set +e
set +u
set +o pipefail
if cd /home/yavdr/vdr-suite; then
  git -c safe.directory=/home/yavdr/vdr-suite rev-parse --show-toplevel
  git -c safe.directory=/home/yavdr/vdr-suite rev-parse HEAD
  git -c safe.directory=/home/yavdr/vdr-suite status --short
  ls -l /usr/share/vdr-suite/web/frontend/app.js \
    /usr/share/vdr-suite/web/frontend/modules/media-import.js
fi
~~~

**Bei einem tatsächlich fertigen Fix** darf der Agent nicht bei diesem
Diagnosemuster stehenbleiben: er liefert die auf die real geprüfte
Kandidaten-SHA gepinnten Dateiübertragungsbefehle, die Byte-Prüfung
und die Browser-Readback-Anweisung **gleich mit**. Die Liste der
betroffenen Dateien kann bei einem anderen Fix abweichen.

Wenn stattdessen das *komponierte* Frontend oder ein kompletter
Runtime-Baum betroffen ist, gilt die [Stage-Anleitung](install-runtime-deployment-reliability.md).
Die dokumentierten Make-Ziele `stage-install-runtime`,
`deploy-install-runtime` und `check-install-runtime-deployment`
gehören nur zu einem entsprechend freigegebenen **vollständigen**
Runtime-Deploy; dessen Ergebnis `RUNTIME_DEPLOYMENT_MATCH=YES`
muss echt vorliegen. Der Frontend-only-Stagingpfad hat den
eigenen Readback `FRONTEND_DEPLOYMENT_MATCH=YES`. Die zuvor
installierte *aktuelle* Stage darf nicht mit einem älteren Snapshot
verwechselt werden.

## 4. Daemon-Fix: bestehendes Artefakt wiederverwenden

Autoritative Umsetzung: [Daemon-only-Installer](daemon-only-installer.md)
und `tools/install_daemon.py`. `check` ist lesend; `build` kann einen
vorhandenen versiegelten Kandidaten wiederverwenden oder einen fertig
gelinkten Kandidaten ohne Neukompilierung nachversiegeln. `apply`
kompiliert **nie**, braucht Root und `--confirm-apply`.

Der Agent bereitet in einem **zur Produktcode-SHA passenden sauberen
Checkout** diese Entscheidungen vor:

1. Tatsächlich installierten/running Daemon und frühere bekannte
   geprüfte Kandidaten ermitteln. Dokumentations-HEAD von der letzten
   gültigen Produktcode-SHA unterscheiden.
2. Nachgewiesene volle Code-SHA in
   `python3 tools/install_daemon.py check --sha FULL40` einsetzen.
   Ausgabe `INSTALLED_SOURCE_SHA`, `TARGET_SOURCE_SHA`,
   `BUILD=REUSABLE`/`BUILD=NOT_SEALED`, `DB_BACKUP_DECISION`
   und `DB_REVIEW_PATH` **wirklich auswerten**.
3. Ist der Zielkandidat bereits installiert: **nichts tun**.
   Ist er versiegelt: **nicht builden**. Fehlt der Seal, bestehendes
   überprüfbares Binary gegebenenfalls *ohne* Kompilierung
   wiederherstellen. Nur bei belegtem Bedarf **einmal** bauen;
   Cache und schon gebaute Kandidaten bewahren.
4. Separat autorisiertes `apply --sha FULL40 --confirm-apply`
   nur bei nachgewiesenem DB-/Schema-Risikoentscheid anbieten.
   Keine willkürlichen `--db-review`-Argumente, automatischen
   DB-Backups oder Dienstneustarts. Nach genehmigtem Apply
   echten Service-/Binary-Readback verlangen.
5. **Kein** `sleep`, keine GitHub-CI-Abfrage und keine CI-Schleife
   in den Installationsblock einbauen. Erforderliche Final-Head-CI
   vor Merge ist ein **anderer** Prozess.

**Direkt nutzbare lesende Hostdiagnose:**

~~~bash
set +e
set +u
set +o pipefail
if cd /home/yavdr/vdr-suite; then
  git -c safe.directory=/home/yavdr/vdr-suite rev-parse --show-toplevel
  git -c safe.directory=/home/yavdr/vdr-suite rev-parse HEAD
  systemctl show vdr-suite-daemon.service \
    -p LoadState -p ActiveState -p SubState -p MainPID -p ExecStart \
    --no-pager
  if [ -f /usr/sbin/vdr-suite-daemon ]; then
    sha256sum /usr/sbin/vdr-suite-daemon
  fi
fi
~~~

**Achtung:** Nicht einfach `HEAD` als Produktcode-Pin ausgeben,
wenn der neue Commit lediglich Dokumentation enthält.

## 5. Debian-/Ubuntu-Paketweg einschließlich Media Tools

Die gegenwärtigen [Debian-Paketquellen](../../debian/control)
definieren **vier** Binärpakete:
`vdr-suite-daemon`, `vdr-suite-backend-agent`,
`vdr-plugin-suitebridge` und das **inaktive**
`vdr-suite-addon-media-tools`. `debian/rules` baut die reguläre
Quell-Paketvariante; für nur das VDR-Plugin gibt es
`dpkg-buildpackage -B -Pplugin-only -us -uc`. Der
normale `dpkg-buildpackage -B -us -uc`-Build gehört in einen
**geeigneten isolierten Build-Checkout**, niemals als
provisorische Live-Installationsmaßnahme auf yaVDR.

**Der aktuelle Verifizierungsstand ist begrenzt:** Die
[debian/README.md](../../debian/README.md) dokumentiert
MT.I4.1/MT.I4.2 und isolierte Paket-Lifecycle-Prüfungen;
die tatsächliche vollständige Ubuntu-APT-Abnahme des aktuellen
Vier-Paket-Standes und eine **produktive** yaVDR-Paketübernahme
sind dort nicht als bestanden ausgewiesen. Die aktuell implementierte
`tools/packaging_p8_installer.py --install`-Aktion verweigert
eine Host-Übernahme ausdrücklich; die [P8-Inventur](../planning/packaging-p8-yavdr-adoption-boundary.md)
hält bestehende Dateikollisionen und unvollständige Migration fest.
Daher **keinen generischen `apt install`-/`dpkg -i`-Block**
für das bereits manuell verwaltete Hostsystem erfinden.

Für eine *später tatsächlich freigegebene* Paketübernahme nennt
die Fix-Antwort die verifizierte Paketquelle, exakten Paketnamen
und Versionen, passende Distribution/VDR-ABI, genehmigten
Migrations-/Upgradeweg und danach Paket-, Dienst- und API-Readback.
Ein neues Paket im Manifest aktiviert nicht automatisch Media Tools,
Benutzerrechte, Agent-Dienste oder Import-Aufträge.

**Sofort nutzbarer lesender Paketstatus:**

~~~bash
set +e
set +u
set +o pipefail
dpkg-query -W vdr-suite-daemon vdr-suite-backend-agent \
  vdr-plugin-suitebridge vdr-suite-addon-media-tools
apt-cache policy vdr-suite-daemon vdr-suite-backend-agent \
  vdr-plugin-suitebridge vdr-suite-addon-media-tools
dpkg --audit
~~~

Nicht installierte Pakete bzw. fehlende APT-Quelle sind **Information**,
kein Grund, eigenmächtig einen fremden Paketstand zu installieren.

## 6. Fehlersuche: ohne Rückfrage unmittelbar diagnostizieren

**Beispiel eines host-only lesenden Diagnoseblocks** bei einem
Daemon-/Agent-Dienstproblem:

~~~bash
set +e
set +u
set +o pipefail
systemctl show vdr-suite-daemon.service vdr-suite-backend-agent.service \
  -p Id -p LoadState -p ActiveState -p SubState -p MainPID \
  --no-pager
journalctl -u vdr-suite-daemon.service -n 40 --no-pager
~~~

Bei einem Frontend-, Git-, Paket- oder Media-Tools-Fehler
**nicht** diesen allgemeinen Block erzwingen, sondern aus den
obigen geprüften Pfaden **nur den relevanten** Diagnoseweg
ausgeben. Journal-/Konfigurationsinhalte können vertraulich sein:
beim Teilen der Ausgabe Zugangsdaten und personenbezogene Werte
nicht weitergeben. Für Media Tools niemals zur Diagnose einen
Import, Reencode, Delete, Service-Start oder eine Datenbank-Mutation
automatisch auslösen. Die Ausführung des blocks bleibt beim Operator.

## 7. Reine Dokumentationsänderung

Kurz den **wirklichen** veröffentlichten Commit und die geänderten
Dateien prüfen. Keine Neuerstellung vorhandener Daemon-Artefakte,
kein Runtime-/Hardwaretest und kein automatisches CI-Warten.

~~~bash
set +e
set +u
set +o pipefail
if cd /home/yavdr/vdr-suite; then
  git -c safe.directory=/home/yavdr/vdr-suite rev-parse --show-toplevel
  git -c safe.directory=/home/yavdr/vdr-suite rev-parse HEAD
  git -c safe.directory=/home/yavdr/vdr-suite status --short
fi
~~~

## Bezug zu den zehn Agenten-Regressionsfällen

Die [konkreten Evaluationsaufgaben mit PASS-/FAIL-Kriterien](agent-regression-evaluation.md)
prüfen, ob der Agent diese Abläufe in einer simulierten Situation tatsächlich
anwendet; sie ersetzen keinen Produkt- oder Live-VDR-Test.

- [ARS-01 und ARS-02](agent-regression-scenarios.md):
  Fix + sofortiger Updateblock; Problem + sofortiger Diagnoseblock.
- ARS-03/04/05: vorhandene Root-Shell, Build-Wiederverwendung, kein
  unnötiger CI-Stopp.
- ARS-06/07/09: sichtbare Frontend-/SSE-/TV-Wirkung statt bloßer
  Dateikopie oder bestandener Mock-Test.
- ARS-08/10: kein künstliches Branch-Gate, keine Verwechslung
  von Produktcode-, Paket-, CI- und Real-VDR-Nachweisen.

**Grenze:** Die Muster diagnostizieren nur. Erst ein **konkreter Fix
mit geprüfter SHA und nachgewiesenem Hostpfad** erlaubt einen
entsprechend konkreten, unmittelbar ausführbaren Updateblock.
Kein neuer freigabepflichtiger Schritt und kein zusätzlicher CI-Gate
wird durch dieses Dokument geschaffen.
