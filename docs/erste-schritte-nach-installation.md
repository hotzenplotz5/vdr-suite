# Erste Schritte nach der Installation – VDR-Suite (Ubuntu / PPA)

Diese Anleitung richtet sich an Betreiber, die **VDR-Suite erstmals einrichten** möchten: vom Ubuntu-Paket über das erste Administratorkonto bis zum Webfrontend, den Aufnahmen und weiteren Benutzern. Sie beschreibt den auf den GitHub-Quellständen überprüften Ablauf, **keine bereits freigegebene universelle Produktivinstallation**.

> **Wichtig (Stand 17.10.2026):** Die experimentelle Ubuntu-/Debian-
> Paketierung liegt nach [PR #494](https://github.com/hotzenplotz5/vdr-suite/pull/494)
> auf [`main`](https://github.com/hotzenplotz5/vdr-suite/tree/main).
> Das bestehende [VDR-Suite-Test-PPA](https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite)
> bietet für Noble 24.04 und Resolute 26.04 die drei VDR-Suite-Pakete und
> `vdr-plugin-web` (amd64). Alle diese Pakete bleiben **experimentell**;
> die PPA-Installation auf einem bestehenden yaVDR ist nicht allgemein
> freigegeben. Vor jeder Installation Version, Architektur, VDR-ABI und
> vorhandene Dateien/Services prüfen. Der
> [P8-Installationsschutz](https://github.com/hotzenplotz5/vdr-suite/blob/main/docs/planning/packaging-p8-install-gate.md)
> darf nicht umgangen werden.

**Nachgewiesene Pakettests:** Der echte PPA-Bezug, die Installation,
Daemon-/HTTP-Laufzeit und VDR-Plugin-ABI wurden isoliert für
[Noble (Run #39038795095)](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39038795095)
und
[Resolute (Run #39070097283)](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39070097283)
geprüft. Das sind keine Hardware- oder Produktivmigrationstests.
Die Launchpad-Daily-Veröffentlichung ist nicht eingeschaltet, weil diese
ohne verpflichtendes GitHub-CI-Gate ablaufen würde; siehe
[Packaging-Status](../packaging/README.md).

## 1. Welchen Installationsfall habe ich?

| Situation | Vorgehen |
| --- | --- |
| **Neues, entbehrliches Ubuntu-24.04-Testsystem mit bestehendem, funktionierendem VDR** | PPA prüfen, Abhängigkeiten/Dateikonflikte prüfen und die *experimentellen* Pakete erst nach ausdrücklicher Entscheidung installieren. |
| **Bestehender yaVDR mit manuell installierter VDR-Suite** | **Nicht** per `apt install` darüberinstallieren! Zuerst Besitz/Kollisionen von Binärdateien, nginx, systemd, Konfiguration und SQLite mit dem [P6/P8-Migrationskonzept](https://github.com/hotzenplotz5/vdr-suite/blob/main/docs/planning/packaging-p8-yavdr-adoption-boundary.md) klären. |
| **Ubuntu 26.04 / andere Architektur / anderer VDR** | Verfügbarkeit und passende **VDR-ABI** separat prüfen; kein pauschales Funktionsversprechen. |
| **VDR-Suite läuft bereits** | Bei [Schritt 4](#4-dienste-webadresse-und-erster-aufruf) einsteigen; nichts neu installieren oder die Datenbank zurücksetzen. |

VDR-Suite **ersetzt VDR nicht**. Tuner, Sender, EPG, Aufnahmeverzeichnis, VDR-Timer und native Plugins müssen auf der VDR-Seite funktionieren. Ein VDR-Plugin mit falscher ABI darf nicht ungeprüft installiert werden.

## 2. Ubuntu-Pakete verstehen und PPA prüfen

Die Paketierung unterscheidet drei installierbare Komponenten:

| Paket | Aufgabe |
| --- | --- |
| `vdr-suite-daemon` | Zentrale Suite mit Webfrontend, APIs, Identitäten und SQLite-Zustand |
| `vdr-plugin-suitebridge` | VDR-natives Plugin; muss exakt zur eingesetzten VDR-ABI passen |
| `vdr-suite-backend-agent` | Optionaler bzw. topologieabhängiger Agent zur Anbindung weiterer VDR-Backends; **nicht** automatisch auf jedem Einzel-VDR nötig |

**Für eine bereits bestehende Installation nur lesen, nicht installieren:**

```bash
lsb_release -ds
dpkg-query -W 'vdr*' 2>/dev/null || true
apt-cache policy vdr-suite-daemon vdr-suite-backend-agent vdr-plugin-suitebridge
systemctl status vdr.service vdr-suite-daemon.service --no-pager
```

Ein neues, isoliertes Testsystem darf nach Prüfung der Paketquelle und einer bewussten Installationsfreigabe das PPA einbinden:

```bash
sudo add-apt-repository ppa:hotzenplotz5/vdr-suite
sudo apt update
apt-cache policy vdr-suite-daemon vdr-suite-backend-agent vdr-plugin-suitebridge
```

**Hier noch nicht blind installieren.** Für `vdr-plugin-suitebridge` müssen VDR-ABI und Paket-Abhängigkeiten passen; dafür kann zusätzlich die passende [Seahawk-VDR-PPA-Quelle](https://launchpad.net/~seahawk1986-hotmail/+archive/ubuntu/vdr-2.8.3) nötig sein. Ein PPA, das Launchpad **zum Bauen** verwendet, wird nicht automatisch zum APT-Repository des Nutzers. Vor der ersten Installation müssen außerdem vorhandene Dateien, systemd-Overrides, nginx-Snippets und `/var/lib/vdr-suite` auf Konflikte geprüft sein. Die derzeitige [P8-Installationsfreigabe](https://github.com/hotzenplotz5/vdr-suite/blob/main/docs/planning/packaging-p8-install-gate.md) ist für reale Übernahmen ausdrücklich noch blockiert.

Nach einer **separat geprüften und erlaubten** Installation lässt sich der tatsächliche Paketstand ausschließlich lesend prüfen:

```bash
dpkg-query -W vdr-suite-daemon vdr-suite-backend-agent vdr-plugin-suitebridge 2>/dev/null
dpkg -L vdr-suite-daemon
```

**Nicht alle drei Dienste müssen auf einem Einzelserver aktiv sein.** Die experimentellen Pakete starten oder aktivieren daemon/Agent nicht automatisch; das ist Absicht. Der bereits installierte Backend-Agent hat nun einen separaten, geschützten **Abschluss-Schritt nach erfolgreicher Registrierung**: `vdr-suite-backend-agent-autostart --check` und anschließend, nach freigegebener Prüfung, `sudo vdr-suite-backend-agent-autostart --enable`. Der zweite Aufruf setzt den Autostart für spätere Hoststarts, **ohne** den laufenden Dienst neu zu starten oder Geräteidentitäten zu verändern. Die experimentelle Paketinstallation führt diesen Schritt niemals vor dem Enrollment aus; ein fertig integrierter Einrichtungsassistent, der ihn eigenständig nach erfolgreichem Enrollment aufruft, steht noch aus. Näheres im [Backend-Agent-Paketvertrag](../debian/README.md#after-enrollment-boot-activation-operator-opt-in-not-package-auto-start). Installation, Konfiguration und kontrollierter Dienststart sind verschiedene Schritte.

## 3. Erste Datenbank: Was wird angelegt?

VDR-Suite verwendet eine **zentrale SQLite-Datenbank** (Standardpfad: `/var/lib/vdr-suite/vdr-suite.db`). Sie hält Suite-eigene Identitäten, Berechtigungen, Metadaten und weitere persistente Zustände. Der Backend Agent ist **kein separater Datenbankserver**.

- **Frische, geprüfte Installation:** Der Daemon bzw. die zuständigen Runtime-Repositories initialisieren ihre jeweils benötigten Schemata im konfigurierten Zustand. Die Datei wird nicht durch Import einer beliebigen SQL-Datei aus dem Quellbaum „vorbereitet“.
- **Bereits vorhandene Datenbank:** **Nicht neu initialisieren, löschen, austauschen oder blind migrieren.** Sie enthält womöglich Benutzer, Sitzungen, Grants, Medienzustände und aktive SQLite-WAL-Dateien.
- **Backup/Upgrade:** Eine konsistente Sicherung muss den laufenden SQLite-Schreibbetrieb und mögliche `-wal`/`-shm`-Dateien korrekt berücksichtigen. Nicht einfach die offene `.db` kopieren; vor einem Upgrade die tatsächlich unterstützte Schema-/Rollback-Strategie prüfen.
- **Keine Demo-Daten:** `database/testdata/sample-data.sql` ist Testmaterial, keine Erstinstallation. `database/migrations/` enthält keinen universellen produktiven Migrationsrunner.

Zum **reinen Nachsehen** (ohne SQLite-Änderung) eignen sich die Dienstkonfiguration und der Dateistatus:

```bash
systemctl cat vdr-suite-daemon.service
ls -ld /var/lib/vdr-suite 2>/dev/null
ls -l /var/lib/vdr-suite/vdr-suite.db* 2>/dev/null
```

Die Quelle der konkreten DB-Konfiguration ist entscheidend; der Paket-Service setzt `VDR_SUITE_DATABASE_PATH=/var/lib/vdr-suite/vdr-suite.db`. Mehr Hintergrund: [Datenbankverträge](../database/README.md).

## 4. Dienste, Webadresse und erster Aufruf

1. Prüfen, ob **VDR** und der **Suite-Daemon** laufen. Kein Dienst-Neustart nur zum Testen der Dokumentation.
2. Die installierte Paketkonfiguration prüfen: `/etc/default/vdr-suite-daemon`, systemd-Einheit und den tatsächlich aktiven HTTPS-Reverse-Proxy. **Keine Geheimnisse aus Konfiguration oder Journal veröffentlichen.**
3. Das mitgelieferte nginx-Snippet (`/etc/nginx/snippets/vdr-suite.conf`) aktiviert sich **nicht von allein**. Die Beispielkonfiguration verwendet den Pfad `/vdr-suite/`, leitet den Browser zu `/vdr-suite/frontend/` und proxyt intern zu `127.0.0.1:18080`. Die tatsächliche URL hängt vom eingerichteten HTTPS-Host und Proxy ab.
4. Diese HTTPS-Webadresse öffnen; niemals einen erfundenen Standardhost oder ein Standardpasswort voraussetzen.

Bei der Diagnose zunächst `systemctl status vdr-suite-daemon.service --no-pager`, VDR-Status und die aktive Reverse-Proxy-Konfiguration **nur lesend** kontrollieren. Der interne API-Port ist kein Ersatz für den abgesicherten Browser-Zugang.

## 5. Ersten Administrator anlegen

**MLD-Entwicklungsversion (noch nicht nach `main` übernommen):** Sobald die
Version aus `work/mld66-platform-integration` auf dem VDR installiert ist,
öffnet der Browser bei der ersten Verbindung ohne Administratorkonto direkt
den Einrichtungsdialog. Für reguläre Benutzer ist **kein Terminal, kein
`sudo`, kein Skript, kein Bootstrap-Geheimnis** notwendig.

1. VDR-Suite über eine **vertrauenswürdige HTTPS-Adresse** aufrufen.
2. Im Dialog **„Code am TV anzeigen“** auswählen. Der Suite-Daemon sendet
   ausschließlich den einmaligen achtstelligen Code über den **lokalen**
   VDR-SVDRP-Anschluss (Standard-Port 6419, konfigurabel über
   `VDR_SUITE_FIRST_ADMIN_OSD_PORT`) an das physische VDR-OSD.
3. Die acht Ziffern vom Fernsehbild in das Webformular übernehmen,
   Benutzername und Anzeigename wählen, Passwort zweimal eingeben.
4. **„Administrator erstellen“** drücken. Bei erfolgreicher Bestätigung
   erstellt die vorhandene First-Admin-Claim-Logik das Konto und der Browser
   meldet es regulär an. Anschließend erscheint nur noch die normale Anmeldung.

Der Code gilt **fünf Minuten**. Pro Code gibt es **höchstens fünf Versuche**;
ein erneutes Anzeigen kann frühestens nach **60 Sekunden** angefordert werden.
Das Verfahren sendet **keinen Code an den Webbrowser**, sondern zeigt ihn
nur auf dem lokalen TV/OSD. Es legt keine weiteren Services/Tabellen an.
Ohne erreichbares **lokales VDR-OSD** wird der Ersteinrichtungsversuch
abgelehnt. Insbesondere ein reiner Headless-Server benötigt einen anderen
separat autorisierten Erstinstallationskanal; es erfolgt niemals eine
unbestätigte Freischaltung für den ersten Netzwerkbesucher.

Ein installiertes VDR-Suite-Paket und der laufende Daemon müssen aus diesem
Branch gebaut sein. Ein bloßes `git pull` aktualisiert noch keine
Paketinstallation. Die HTTPS-Konfiguration des jeweiligen MLD-Systems bleibt
eine Voraussetzung. **Nicht ungeprüft auf produktive yaVDR-Anlagen installieren.**

### Technischer Altweg (nicht der normale Einrichtungsdialog)

**Es gibt kein mitgeliefertes Standard-Admin-Passwort.** Die Implementierung benutzt einen bewusst getrennten zweistufigen Ablauf:

1. Auf dem **lokalen Server** gibt ein vertrauenswürdiger Root-Operator mit dem installierten Werkzeug `vdr-suite-first-admin-bootstrap` ein einmaliges Setup-Material aus. Das Werkzeug ist nur für einen *noch nicht beanspruchten* (unclaimed) Server zulässig; Standardgültigkeit **900 Sekunden** (zulässig 300–3600 Sekunden).
2. Der Operator übermittelt `bootstrapId`, `setupSecret`, gewünschten `loginName`, `password` und `displayName` an die **Claim-Sicherheitsroute** `POST /api/security/first-admin/claim`, über die tatsächlich abgesicherte HTTPS-Route der Installation. Die API verwendet genau diese fünf JSON-Felder. **Diese Aktion wird nicht als bereits fertiger grafischer Installationsassistent ausgegeben.** Der aktuelle Quellstand dokumentiert die Claim-Route; der normale Browser-Login ist ein davon getrennter Schritt.
3. Der Claim verbraucht das einmalige Secret und erzeugt in **einer Transaktion** ein Human Account, den zugehörigen Benutzer-Actor, einen `human-password`-Zugang und die anfängliche globale `role.admin@*`-Berechtigung. Die Passwortdaten werden als gesalzener Einweg-Verifier abgelegt.
4. Danach mit dem **neu vergebenen normalen Benutzerpasswort** über den Login der Weboberfläche anmelden. Das Setup-Secret ist **kein** dauerhaftes Passwort und keine Browser-Sitzung.

**Konkrete Bedienung auf einem frisch vorbereiteten, noch unclaimed Testserver:** Nach kontrolliertem Dienststart lokal am Server ausführen:

```bash
sudo vdr-suite-first-admin-bootstrap
```

Die drei ausgegebenen Werte `bootstrap_id`, `setup_secret` und `expires_at` nur für die wenige Minuten später folgende Einrichtung verwenden. Der HTTP-Claim erwartet diese JSON-Schlüssel (keine Shell-Passwortargumente):

```json
{
  "bootstrapId": "<einmalige ID>",
  "setupSecret": "<einmaliges Secret>",
  "loginName": "<gewünschter Admin-Login>",
  "password": "<neues persönliches Admin-Passwort>",
  "displayName": "<Anzeigename>"
}
```

**Für den Claim ohne Secret in der Shell-History** eignet sich z. B. dieser interaktive Python-3-Aufruf auf einem vertrauenswürdigen Rechner mit Zugriff auf den **geprüften HTTPS-Proxy**. Keine fremden HTTP-URLs und keine deaktivierte TLS-Zertifikatsprüfung verwenden:

```bash
python3 - <<'PY'
import getpass
import json
import urllib.parse
import urllib.request

base = input("HTTPS-Basisadresse vor /api (z.B. https://server/vdr-suite): ").strip().rstrip("/")
u = urllib.parse.urlsplit(base)
if u.scheme != "https" or not u.netloc or u.username or u.password or u.query or u.fragment:
    raise SystemExit("Ungültige HTTPS-Basisadresse")
password = getpass.getpass("Neues Admin-Passwort: ")
if password != getpass.getpass("Admin-Passwort wiederholen: "):
    raise SystemExit("Passwörter stimmen nicht überein")
payload = {
    "bootstrapId": input("bootstrap_id: ").strip(),
    "setupSecret": getpass.getpass("setup_secret (verdeckt): "),
    "loginName": input("Gewünschter Loginname: ").strip(),
    "password": password,
    "displayName": input("Anzeigename: ").strip(),
}
req = urllib.request.Request(
    base + "/api/security/first-admin/claim",
    data=json.dumps(payload).encode("utf-8"),
    headers={"Content-Type": "application/json"},
    method="POST",
)
with urllib.request.urlopen(req, timeout=20) as response:
    print("HTTP", response.status, "– erster Administrator beansprucht")
PY
```

Das Programm meldet bei abgelehnten, abgelaufenen oder bereits verbrauchten Ansprüchen einen HTTP-Fehler und erstellt **keinen** zweiten Administrator. Es sendet ID/Secret/Passwort nur an den vom Betreiber eingegebenen TLS-Endpunkt, ohne diese Werte als Shell-Argument oder Datei zu speichern. Ein gültiges Zertifikat und der *richtige* HTTPS-Host sind Voraussetzung.

Das lokal installierte Handbuch ist die verbindliche Bedienhilfe: `man vdr-suite-first-admin-bootstrap`. Die Ausgabe enthält den Secretwert **nur einmal**; ihn nicht in Tickets, Screenshots, Shell-History, URLs oder Logs kopieren. Der Claim kann nach Ablauf oder nach bereits erfolgter Beanspruchung nicht erneut als zweiter Admin ausgeführt werden.

**Passwort vergessen?** Es gibt den getrennten, lokalen Root-Recovery-Weg `vdr-suite-human-account-recover` für ein **bestehendes**, genau ausgewähltes Konto, mit `--account-id` und `--login`. Das Werkzeug fordert das neue Passwort ohne Kommandozeilenargument an, rotiert das Credential und widerruft betroffene Sitzungen. Es ist kein ungesicherter Web-Reset. Details: [ADR-0066](adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md) und `man vdr-suite-human-account-recover`.

## 6. Erstes Backend, Sender, EPG und Aufnahmen

Nach der Anmeldung zuerst ein tatsächlich erreichbares VDR-Backend auswählen. Die Weboberfläche zeigt Backends samt Online-/Offline- und Zugriffsstatus. Ein Backend ist **nicht automatisch** allein durch Installation eines Agents verbunden.

| Prüfung | Woran erkennt man ein sinnvolles Ergebnis? |
| --- | --- |
| **Backend-Status** | Das ausgewählte VDR-Backend ist sichtbar und meldet einen plausiblen Online-/Berechtigungsstatus. |
| **Kanäle und EPG** | Die erwarteten Sender und aktuelle/kommende Sendungen erscheinen; VDR muss selbst Sender- und EPG-Daten haben. |
| **Aufnahmen** | Vorhandene VDR-Aufnahmen erscheinen samt Details; Zugriffspfade und Dateirechte müssen zur echten VDR-Konfiguration passen. |
| **Timer** | Ein bestehender Timer kann betrachtet werden; erst danach eine neue Aufnahme gezielt und berechtigt ausprobieren. |
| **Wiedergabe** | Bild, Ton, Stoppen, ggf. Spulen/Untertitel auf dem *wirklich verwendeten* Browser/TV prüfen; ein Vorschaubild ist kein Wiedergabenachweis. |

Die lokale SuiteBridge-Anbindung und ein zusätzlich registrierter Backend Agent sind unterschiedliche Möglichkeiten. Bei zusätzlichen Geräten muss der Agent gesondert konfiguriert und sicher eingeschrieben werden. In der Paket-Agent-Vorlage ist die Control-Plane-URL absichtlich ungültig, bis sie vom Betreiber gesetzt wird. Niemals private VDR-Plugin-Ports einfach ins Internet freigeben.

## 7. Die Reiter im Webfrontend

Die tatsächlich vorhandene Navigation im Quellstand umfasst:

| Bereich | Typischer erster Schritt |
| --- | --- |
| **Home** | „Was läuft jetzt“, Now/Next, aktuelle Sender, zuletzt aufgenommene und empfohlene Inhalte entdecken. |
| **Live TV / Kanäle** | Sender wählen, Sendungsinformationen ansehen und auf unterstützten Quell-/Client-Kombinationen live abspielen. |
| **EPG / Zeitleiste** | Programm in der Zeitachse erkunden und Sendungsdetails öffnen. |
| **Aufnahmen** | Aufnahmen durchsehen, suchen, Detailseite/Artwork ansehen, passende Wiedergabe starten. |
| **Genres** | Filme und Serien nach Genres entdecken, soweit Quellen und Metadaten vorliegen. |
| **Suche** | Aufnahmen, EPG-Einträge und Personen recherchieren. |
| **Kanäle sortieren** | Reihenfolge ansehen und – nur bei vorhandener Berechtigung und funktionierender Backend-Anbindung – verändern. |
| **Timer** | Bestehende Aufträge und ihre VDR-Zuordnung prüfen, berechtigte neue Timer anlegen. |
| **SearchTimer** | Automatische Such-/Aufnahmeregeln prüfen und bei vorhandenem Backend-Support verwalten. |
| **OSDTeletext / HbbTV / Fernbedienung** | Über die jeweilige Live-TV- oder Companion-Oberfläche verwenden, **wenn** der aktuelle Sender, das Plugin, der Server und der Client die Funktion anbieten. Keine universellen eigenen Hauptreiter versprechen. |
| **Einstellungen** | Sprache, Konto/Zugriffe, Backend-Informationen und unterstützte Medien-/Darstellungsoptionen prüfen. |

Siehe die [echten Screenshots in der README](../README.md#screenshots--vdr-suite-web-frontend). Nicht alle Backend-Funktionen stehen für jeden Benutzer, Sender oder Client zur Verfügung.

## 8. Einstellungen im Webfrontend

Unter **Einstellungen** zunächst in dieser Reihenfolge vorgehen:

1. **Sprache** (Deutsch/Englisch) und die angezeigte Backend-Auswahl prüfen.
2. **Backendinfo** ansehen: Name/ID, Onlinezustand, Zugriff, Schreibrechte, Timer- und Aufnahmenberechtigungen. Anzeige einer Berechtigung ist kein Freibrief, sie zu umgehen.
3. **Kontenverwaltung** (nur mit globalen Adminrechten): Konto, Zugriff, Anmeldung und Geräte/Sitzungen – siehe unten.
4. **Medien-/Transcoding-Einstellungen** nur anpassen, wenn die dazugehörige Server-Funktion angeboten wird. Ein backendbezogener Encoderwert kann den Deployment-Standard überschreiben; die Verfügbarkeit von VAAPI und reale Performance müssen gesondert geprüft sein.
5. **Artwork/Metadaten**: Externe Anbieter und optionale Tokens nur über unterstützte Konfigurations-/Secret-Quellen konfigurieren; kein API-Token in GitHub oder ein öffentliches Frontend eintragen. Fehlende Cover bedeuten nicht automatisch einen Datenbankfehler.
6. **Frontend/EPG-Anzeige und Fernbedienung**: Anzeigeinformationen und Bedienhilfe prüfen; technische System-, nginx- oder Serviceeinstellungen sind nicht automatisch über den Browser editierbar.

## 9. Multiuser: weitere Personen, Rechte und Geräte

**Human Accounts** sind echte Konten auf dem Server, nicht bloß Profile oder voneinander getrennte Browser-Lesezeichen.

1. Als erster Administrator zu **Einstellungen → Benutzer-/Kontenverwaltung** wechseln.
2. **Benutzer hinzufügen**: Loginname, Anzeigename und initiales Passwort vergeben. Neue Konten erhalten **nicht automatisch Berechtigungen**.
3. Konto auswählen und unter **Zugriff** die vom Server angebotenen **Berechtigungen und Geltungsbereiche** zuordnen. Ein Grant kann global oder backendbezogen sein; gewöhnlichen Benutzern nur die benötigten Lese-, Wiedergabe- oder Timer-Rechte geben.
4. Unter **Anmeldung** Credentials und unter **Geräte/Sitzungen** aktive Sessions prüfen. Bei Bedarf ausdrücklich widerrufen; das kann eine sofortige Abmeldung auslösen.
5. In einem **separaten privaten Browserfenster** als neuer Benutzer anmelden: Stimmt die angezeigte Backend-Auswahl? Kann der Benutzer genau die erlaubten Dinge und keine administrativen Änderungen ausführen?
6. **Mindestens einen nutzbaren Administrator behalten.** Die Serverlogik schützt gegen das Entfernen der letzten verwendbaren Administrator-Autorität. Ein Admin-Konto nicht als normalen Fernsehzuschauer-Zugang verteilen.

Die Optionen im Zugriffsdialog kommen vom **Server**; die Oberfläche darf keine beliebigen Rechte frei erfinden. Die Plattform kennt auch **technische Device-Identitäten** für TV-/App-Clients: Bei unterstützter Kopplung folgt auf eine Pairing-Anfrage erst eine Admin-Genehmigung, dann eine einmalige Geräte-Credential-Ausgabe und **separat** zugewiesene Geräte-Berechtigungen. Ein TV ist danach **nicht** automatisch Administrator. Pairing und dauerhafter App-Login müssen auf dem echten Zielgerät geprüft werden. **Personenprofile und Familien-Personalisierung nicht mit den bereits vorhandenen Accounts/Grants gleichsetzen.**

Vertiefung: [Benutzer und Grants](../api/accounts-and-grants.md), [TV-/Gerätekopplung](../api/device-pairing.md), [Geräteadministration](../api/device-administration.md).

## 10. Erste Inbetriebnahme abhaken

- [ ] Ubuntu-Version, VDR-Version, ABI und **tatsächlich installierte** Paketversionen geklärt.
- [ ] Beim bestehenden System: keine ungeprüfte Übernahme durch APT; alle fremden Dateien und SQLite-State erhalten.
- [ ] VDR-Tuner, Sender und VDR-Aufnahmen arbeiten schon ohne Suite.
- [ ] Dienste und HTTPS-Reverse-Proxy sind kontrolliert eingerichtet; keine privaten VDR-Provider öffentlich erreichbar.
- [ ] SQLite-Datenbank wurde ohne manuellen SQL-Import im richtigen Pfad initialisiert.
- [ ] Erster Admin einmalig beansprucht; Bootstrap-Secret sicher verworfen; Login funktioniert.
- [ ] Backend online, EPG und Aufnahmen plausibel; mindestens **ein realer** Live-/Aufnahme-Abspieltest mit Bild und Ton.
- [ ] Timer-/Aufnahmenänderungen nur mit expliziten Rechten getestet.
- [ ] Weitere Benutzer mit minimalen Rechten angelegt und getrennt getestet.
- [ ] Falls genutzt: TV/App-Pairing, Credential-Aufbewahrung und Gerätegrants geprüft.
- [ ] Backup, Update-/Rollback-Plan, Speicherplatz, HTTPS und Datenschutz geregelt.

## 11. Häufige Stolperstellen

| Symptom | Zuerst prüfen |
| --- | --- |
| **Paket fehlt / Abhängigkeit nicht erfüllbar** | Paketversion/Ubuntu-Serie/Architektur per `apt-cache policy`; VDR-ABI und nötige eigene APT-Quelle. |
| **Paketinstallation verweigert fremde Dateien** | **Nicht** `dpkg --force-overwrite` nutzen. P6/P8-Kollisionsschutz befolgen und Eigentümer von Binärdatei, nginx, systemd und DB klären. |
| **Daemondienst läuft nicht** | Bei Testpaketen ist Autostart absichtlich deaktiviert; zuerst Konfiguration, Servicepfad und installierte Abhängigkeiten prüfen. |
| **Browser zeigt 404 / keine Anmeldung** | Tatsächlichen HTTPS-Pfad, nginx-Include, Proxy-Prefix und Browser-Session prüfen, nicht blind neue Ports freigeben. |
| **Erster Admin lässt sich nicht claimen** | Bereits beanspruchter Server? Kurzlebiges Secret abgelaufen oder schon verbraucht? Richtige Datenbank und Route? Setup ist keine wiederholbare Passwort-Rücksetzung. |
| **Keine Sender/EPG/Aufnahmen** | Native VDR-Daten, Plugin-/Agent-Verbindung, Backendwahl, Dateirechte und Benutzer-Grants unterscheiden. |
| **Video ohne Ton / kein Seek** | Client, Codec, MediaSession, Backend und getestetes Wiedergabeprofil prüfen; Screenshots/EPG-Daten belegen das nicht. |
| **Neuer Benutzer sieht nichts** | Konto aktiv, **Grants explizit gesetzt**, richtiger Backend-Scope, neue Anmeldung und eigener Backend-Zugriff? |
| **TV verbunden, aber ohne Inhalt** | Pairing-Genehmigung allein genügt nicht: Geräte-Credential und getrennte Gerätegrants kontrollieren. |

## Weiterführende Dokumentation

- [README und echte Web-Screenshots](../README.md)
- [Einführung / vorhandene Installation](getting-started.md)
- [Packaging-/PPA-Arbeitsstand](https://github.com/hotzenplotz5/vdr-suite/blob/main/docs/planning/packaging-launchpad-ppa.md)
- [Installations- und Migrationsgrenzen](https://github.com/hotzenplotz5/vdr-suite/blob/main/docs/planning/packaging-p8-install-gate.md)
- [Erste Admin- und Recovery-Architektur](adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md)
- [Multiuser-Kontoverwaltung](development/post-phase69-multiuser-administration-usability.md)
- [Dokumentationsstartseite](index.md)
