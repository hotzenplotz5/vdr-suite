# HbbTV installieren

VDR-Suite kann HbbTV über drei zusammengehörige Komponenten nutzen:

- `vdr-plugin-web` – VDR-Plugin und HbbTV-/Thrift-Anbindung
- `cefbrowser` – Chromium Embedded Framework (CEF) Browser
- `remotetranscode` – Medien-/DASH-/MSE-Transcoder

Alle drei Komponenten müssen dieselbe Socket-Konfiguration verwenden. Das
Ubuntu-/yaVDR-Paket `vdr-plugin-web` installiert dafür
`/etc/vdr/sockets.ini`.

## Aktueller Paketstatus

`vdr-plugin-web` ist bereits als Debian/Ubuntu-Paket verfügbar.

Für `cefbrowser` und `remotetranscode` werden derzeit echte Debian/Ubuntu-
Pakete in den VDR-Suite-Forks vorbereitet:

- <https://github.com/hotzenplotz5/cefbrowser>
- <https://github.com/hotzenplotz5/remotetranscode>

Die Paketierungsbranches basieren auf den erweiterten `rellla/fixes`-
Ständen, die zu den erweiterten Thrift-Schnittstellen von
`vdr-plugin-web` passen.

**Noch keine PPA-Veröffentlichung:** Die ursprünglichen Zabrimus-Repositories
enthalten derzeit keine eindeutige projektweite Redistribution-Lizenz.
Deshalb werden die Pakete gebaut und getestet, aber noch nicht öffentlich
über Launchpad verteilt. Eine öffentliche PPA-Veröffentlichung erfolgt erst
nach Klärung dieser Lizenzfrage.

## Gemeinsame Socket-Konfiguration

Die Standardkonfiguration ist:

```ini
[vdr]
http_ip = 127.0.0.1
http_port = 50000

[browser]
http_ip = 127.0.0.1
http_port = 50001

[transcoder]
http_ip = 127.0.0.1
http_port = 50002
```

Der Paketpfad ist:

```text
/etc/vdr/sockets.ini
```

`vdr-plugin-web`, `cefbrowser` und `remotetranscode` müssen dieselben
Adressen und Ports verwenden.

## Installation mit lokal gebauten Paketen

Solange die beiden neuen Pakete noch nicht im PPA veröffentlicht sind, können
sie aus den Forks als normale Debian-Pakete gebaut werden.

### 1. vdr-plugin-web installieren

Auf einem yaVDR-/Ubuntu-System mit passender VDR-Paketquelle:

```bash
apt update
apt install vdr-plugin-web
```

Danach muss `/etc/vdr/sockets.ini` vorhanden sein.

### 2. cefbrowser-Paket bauen

```bash
git clone --branch packaging/rellla-fixes \
  https://github.com/hotzenplotz5/cefbrowser.git
cd cefbrowser

apt install \
  build-essential devscripts debhelper dpkg-dev \
  cmake meson ninja-build pkg-config curl xz-utils \
  libboost-dev libssl-dev libcurl4-openssl-dev libidn2-dev zlib1g-dev \
  libglib2.0-dev libnss3-dev libatk1.0-dev libatk-bridge2.0-dev \
  libcups2-dev libx11-dev libxcomposite-dev libxdamage-dev \
  libxrandr-dev libgbm-dev libxkbcommon-dev libpango1.0-dev \
  libasound2-dev

./packaging/prepare-orig.sh
cd dist/cefbrowser-0.1+git20261117.2931cc5
dpkg-buildpackage -us -uc -b
```

Das fertige Paket liegt anschließend im übergeordneten `dist/`-Verzeichnis.

### 3. remotetranscode-Paket bauen

```bash
cd ../..
git clone --branch packaging/rellla-fixes \
  https://github.com/hotzenplotz5/remotetranscode.git
cd remotetranscode

apt install \
  build-essential devscripts debhelper dpkg-dev \
  cmake meson ninja-build pkg-config xz-utils \
  libboost-dev libssl-dev libcurl4-openssl-dev \
  libavutil-dev libavformat-dev libavfilter-dev libavcodec-dev

./packaging/prepare-orig.sh
cd dist/remotetranscode-0.1+git20261117.49240d0
dpkg-buildpackage -us -uc -b
```

### 4. Pakete installieren

Die konkreten Dateinamen enthalten die jeweilige Paketversion. Aus den
`dist/`-Verzeichnissen können sie anschließend mit APT installiert werden:

```bash
apt install ./cefbrowser_*_amd64.deb
apt install ./remotetranscode_*_amd64.deb
```

APT löst dabei die normalen Paketabhängigkeiten auf.

## Vorhandene manuelle /opt/hbbtv-Installation ablösen

Ältere Testinstallationen können `cefbrowser` und `remotetranscode`
manuell unter `/opt/hbbtv` installiert haben, zum Beispiel:

```text
/opt/hbbtv/cefbrowser/
/opt/hbbtv/remotetranscode/
/etc/systemd/system/cefbrowser.service
/etc/systemd/system/remotetranscode.service
/etc/hbbtv/
```

Ein Service unter `/etc/systemd/system/` hat Vorrang vor einer vom Debian-
Paket unter `/usr/lib/systemd/system/` installierten Unit. Deshalb müssen
alte manuelle Units vor dem Wechsel entfernt werden.

Zuerst die alte Runtime stoppen:

```bash
systemctl disable --now cefbrowser.service remotetranscode.service
rm -f /etc/systemd/system/cefbrowser.service
rm -f /etc/systemd/system/remotetranscode.service
systemctl daemon-reload
```

Danach die neuen `.deb`-Pakete installieren.

Die paketierten Pfade sind anschließend:

```text
/usr/bin/cefbrowser
/usr/lib/cefbrowser/
/usr/bin/remotrans
/usr/share/remotetranscode/
/etc/remotetranscode/codecs.ini
/usr/lib/systemd/system/cefbrowser.service
/usr/lib/systemd/system/remotetranscode.service
/var/lib/cefbrowser/
/var/lib/remotetranscode/
```

Die alten Verzeichnisse unter `/opt/hbbtv` erst entfernen, nachdem die neue
Paketinstallation erfolgreich geprüft wurde.

## Dienste prüfen

```bash
systemctl status cefbrowser.service remotetranscode.service --no-pager
systemctl show cefbrowser.service -p ExecStart --no-pager
systemctl show remotetranscode.service -p ExecStart --no-pager

pgrep -a cefbrowser
pgrep -a -f 'remotrans|remotetranscode'
```

Bei einer Paketinstallation dürfen die laufenden Prozesse anschließend nicht
mehr aus `/opt/hbbtv` gestartet werden.

Erwartet werden Pfade unter `/usr/lib/cefbrowser` beziehungsweise
`/usr/bin/remotrans`.

## VDR-Plugin prüfen

```bash
systemctl restart vdr
journalctl -u vdr --since "-2 min" --no-pager \
  | grep -Ei 'vdrweb|hbbtv|thrift|browser|transcode|error'
```

Typische erfolgreiche Startmeldungen sind unter anderem:

```text
[vdrweb] Start hbbtv url collector
[vdrweb] Start Thrift Server on 127.0.0.1:50000
```

Die drei Standardports sind:

| Komponente | Port |
| --- | ---: |
| vdr-plugin-web | 50000 |
| cefbrowser | 50001 |
| remotetranscode | 50002 |

## HbbTV in VDR-Suite testen

Nach erfolgreichem Start der drei Komponenten:

1. einen Sender mit HbbTV/AIT auswählen,
2. Live-TV im VDR-Suite-Webfrontend öffnen,
3. HbbTV starten,
4. Red-/Farbtasten und Navigation prüfen,
5. eine Mediathek oder einen Livestream öffnen.

Für die aktuelle VDR-Suite-Integration wurden bereits normale HbbTV-
Darstellung, Navigation, ARTE-Streaming sowie ZDF-Mediathek und
ZDF-Livestream erfolgreich erprobt. Ein Test auf einem konkreten System
bleibt trotzdem erforderlich, weil Browser-, Sender- und Codec-Verhalten
variieren können.

## Später: Installation aus dem PPA

Nach geklärter Upstream-Lizenz und erfolgreicher Paketabnahme soll die
Installation ohne manuellen Build möglich sein:

```bash
apt update
apt install vdr-plugin-web cefbrowser remotetranscode
```

Dieser Abschnitt beschreibt den **Zielzustand**. Solange die Lizenzfrage nicht
geklärt und die Pakete nicht tatsächlich im PPA veröffentlicht sind, ist
dieser Installationsweg noch nicht verfügbar.

## Quellen

- VDR-Plugin: <https://github.com/hotzenplotz5/vdr-plugin-web>
- cefbrowser-Fork: <https://github.com/hotzenplotz5/cefbrowser>
- remotetranscode-Fork: <https://github.com/hotzenplotz5/remotetranscode>
- ursprüngliches cefbrowser-Projekt: <https://github.com/Zabrimus/cefbrowser>
- ursprüngliches remotetranscode-Projekt: <https://github.com/Zabrimus/remotetranscode>
