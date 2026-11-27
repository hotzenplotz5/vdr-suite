# Image Gallery — Roadmap und IG0-Bestandsaufnahme

Stand: 2026-11-27. Arbeitsbranch `work/image-gallery-addon`, Basis `266eb85691fee5140197be0b7c45d136e5a8dbe7`.
**Status: lokale Laufzeit implementiert; vollständige CI, Paket- und Geräteabnahme noch ausstehend.** Kein Merge ohne Maintainer-Freigabe.

## Verifizierte Ausgangslage

- [Images-Metadaten](addon.json), [Images-README](README.md) und [Core-Katalog](../../core/addons/include/AddonCatalog.h) führen bereits die ID `image` und das Paket `vdr-suite-addon-image`. Das vorhandene Modul ist ein inaktives Scaffold. Architekturvorschlag: Produktname **Image Gallery**, bestehende technische Identitäten beibehalten, kein zweites Images-Modul.
- [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md) und [Registry-Vertrag](../../docs/development/addon-installed-registry-contract.md) trennen Paketmetadaten, Aktivierung und tatsächliche Laufzeit. Keine neue Aktivierungs- oder Berechtigungsarchitektur einführen; bestehende Schutzvorgaben und die neuere Maintainer-Vorgabe gegen zusätzliche Sperren beachten.
- [Media Tools](../../addons/media-tools/README.md) dokumentiert Core-/Agent-/Worker-Verteilung. Konkrete bestehende Anknüpfungspunkte: [Settings](../../web/frontend/settings-addons.js) mit `/api/v1/addons/rectools/activation` und `/api/v1/addons/rectools/sources`, [lokale Quellenverwaltung](../../core/daemon/include/LocalMediaImportSourceAdmin.h), [Agent-Runtime](../../core/agent/include/BackendAgentMediaImportRuntime.h) und [Agent-Katalog](../../core/agent/include/BackendAgentMediaImportCatalog.h). Dies sind Media-Tools-spezifische Owner, keine bereits verfügbaren Images-Endpunkte. Lesende Fotooperationen nicht als Import-/Transcode-Mutation modellieren.
- [API-Übersicht](../../api/README.md) und [REST-Owner](../../api/rest/README.md): tatsächliche Public-v1-Routen, Router, Security und Produktionstests prüfen; keine parallele private API. Laufenden API-v1-Arbeitsbranch unverändert lassen.
- [SuiteOSD-Referenz](https://github.com/hotzenplotz5/vdr-plugin-suiteosd/blob/023a179948f45e122b33848213048980dd768850/docs/image-next-integration.md) auf `023a179948f45e122b33848213048980dd768850` und [Image Next Service ABI](https://github.com/hotzenplotz5/vdr-plugin-image-nex/blob/96cdf0b82351db290562fb30f8706593d9a6308d/image-next-service-v1.h) auf `96cdf0b82351db290562fb30f8706593d9a6308d` bestätigen native VDR-in-process-Aufrufe: Browse liefert Quellindex und relative Pfade, Thumbnail liefert ARGB-Pixel und asynchrones ready, Play startet die native Wiedergabe. Diese ABI ist kein HTTP-Protokoll. Browser-Vollbild darf nicht die TV-Wiedergabe starten.

## Maintainer-Entscheidung (27.11.2026)

**Local first.** Der lokale Bildprovider benötigt weder Backend-Agent noch Image Next oder einen laufenden VDR. Lokale und eingehängte NAS-/Festplattenverzeichnisse werden unmittelbar über den bestehenden Core/API-Kontext bereitgestellt, soweit der Core darauf zugreifen kann. Die Addon-Einstellungen sind die Konfigurationsoberfläche für mehrere benannte Bildquellen (hinzufügen, bearbeiten, entfernen); für yaVDR/Ubuntu ist `/srv/picture` die vereinbarte Voreinstellung. Das genaue Seahawk-Paketdefault ist noch nicht separat belegt. Kein globaler distributionsübergreifender Pflichtpfad. Die Voreinstellung darf keine vorhandene Konfiguration überschreiben. Native Image-Next-Einstellungen bleiben unabhängig und unverändert. Keine Agent-Pflicht für lokalen Betrieb, kein neuer Dienst, möglichst keine eigene Datenbank. Entfernte Backend-Anbindung über den vorhandenen Agent-Transport als späterer Integrationsschritt; keine Remote-Funktion als fertig melden, bevor deren tatsächlicher Byte-Transport und Autorisierung nachgewiesen sind. Die lokale Galerie wird zuerst vollständig umgesetzt. Kein Status-Polling.

## Architekturziel / noch zu prüfende Entscheidungen

Eine Bilddomäne, ein zuständiger Quellowner pro Backend, ein Preview-Pfad pro Provider, eine Public-v1-Integration. Core bleibt Owner der Authentifizierung, vorhandenen Berechtigungen und Backend-Zuordnung. Das Backend mit den tatsächlichen Bildern besitzt Zugriff, Verzeichnisnavigation und Verarbeitung. Webfrontend rendert Raster, Carousel und Vollbild, nicht einen zweiten Bilddecoder.

Image Next bleibt unabhängig; eine optionale Übernahme vorhandener Quellen darf keine zweite laufende Konfigurationsautorität oder Plugin-Pflicht schaffen. Die Addon-Einstellungen sind der vereinbarte Owner der Web-Bildquellen. Der bekannte yaVDR-Pfad ist kein globaler Default. Native Service-Wiederverwendung erfordert eine nachgewiesene vorhandene VDR-Bridge; ARGB-Cache nicht ungeprüft als Browserbildtransport verwenden. Alternative vorhandene Bildbibliotheken, EXIF, HTTP-Binärtransport, Cache-/Invalidierungsowner und sicherer Zugriff auf entfernte Backends noch anhand der Implementierung prüfen. Keine zweite Konfiguration, keine beliebigen Client-Dateipfade, keine Originaldatei-Mutationen, keine unnötige rekursive Enumeration.

API und DTOs werden erst nach Prüfung der tatsächlichen Public-v1- und Agent-Owner festgelegt. Vorhandene Quelle-/Backend-Autorisierung wiederverwenden. Bedarfsgerechte GETs und lokale Diashow-Timer sind keine Statusabfragen. Für asynchrone Statusänderungen bestehende SSE nutzen, kein neues Status-Polling. Keine zusätzlichen Sperren, keine Rechteänderung, keine neue persistente Infrastruktur ohne belegten Bedarf und erforderliche Maintainer-Entscheidung.

## Roadmap

| Phase | Ziel / Abnahme | Status |
| --- | --- | --- |
| IG0 | Bestandsanalyse, Owner-/Datenflussvertrag, Akzeptanzmatrix, Wettbewerbsvergleich, genaue Wiederverwendungsentscheidung | Owner und lokale Architektur geklärt; Remote später |
| IG1 | Bestehendes Addon registrieren, aktivieren/deaktivieren, Navigation, Einstellungen, DE/EN | implementiert; Integrationstest offen |
| IG2 | Bildquellen, Unterordner, Ordnercover, natürliche Sortierung, Unicode, Fehler pro Quelle | implementiert; große Ordner zu prüfen |
| IG3 | Bedarfsgerechte Vorschauen, EXIF, begrenzter Cache, Original auf Bedarf, keine UI-Blockade | Previewprofil implementiert; EXIF/Lasttest offen |
| IG4 | Public-v1-Quellen, Ordner, Bild-/Preview-Bytes, Metadaten, erforderliche Settings; Produktionsroute und Vertragstests | lokale Routen integriert; HTTP-End-to-end offen |
| IG5 | Responsives dunkles Raster, TV vier Spalten, Cyan-Fokus, kurze letzte Reihen, Lazy Loading, D-Pad | implementiert; visuelle Abnahme offen |
| IG6 | Cover Flow mit mittigem Bild, zwei Nachbarn je Seite, nach innen gerichteter Perspektive, Spiegelung, flüssigen CSS-Übergängen; Ansicht speichern | implementiert; visuelle Abnahme offen |
| IG7 | Vollbild im Originalseitenverhältnis, vor/zurück, Zoom, Drehung, optionale Infos, Rückkehr zur Auswahl | implementiert; Geräteabnahme offen |
| IG8 | Diashow ab Auswahl, aktueller Ordner, Intervall, Pause, Wiederholung, Vorladen, Ende | implementiert; Ablauf-/Geräteabnahme offen |
| IG9 | Desktop, Touch, Tablet, CEF, Fernbedienung (Taste 1 sofern kompatibel), große Fotos/Sammlungen, langsame Backends, Speicher, Back-Navigation | begonnen; reale Geräte und Last offen |
| IG10 | Paket, Upgrade/Deinstallation, yaVDR/MLD, Dokumentation, exakte CI-Nachweise und reale Browser-/CEF-Abnahme | separates Debian-Staging implementiert; Paketlauf offen |

IG5–IG8: Fotos stehen im Mittelpunkt, kein permanentes Metadatenfeld oder großes Seitenmenü, keine Dateinamen unter jeder Kachel, keine blauen Füllflächen. Ordner klar beschriften. Fotoauswahl beim Wechsel zwischen Ansichten und Rückkehr erhalten. Mouse/Touch/Keyboard/TV-Remote unterstützen. Keine Migration fremder Clients in dieser Version.

## Verifizierte Zwischenstände (keine Geräteabnahme)

- `d8dc8722cd34b040bedeac2d8ad0bc0099ec854b`: [Haupt-CI 45984094188](https://github.com/hotzenplotz5/vdr-suite/actions/runs/45984094188) vollständig erfolgreich, einschließlich Provider-, API- und Frontend-Tests. Beide Ubuntu-Binärpaketjobs in [45984094264](https://github.com/hotzenplotz5/vdr-suite/actions/runs/45984094264) meldeten tatsächlich `IMAGE_GALLERY_BINARY_PAYLOAD=PASS`; der gesamte Workflow wurde durch einen Folgecommit abgelöst und ist nicht als vollständig bestanden zu werten.
- `4626c8f22f460ef4d8fb6e1e50fc80e2afdfbc90`: [Frontendjob 140467400411](https://github.com/hotzenplotz5/vdr-suite/actions/runs/45987656695/job/140467400411) meldete `IMAGE_GALLERY_MOUNT_LARGE_COLLECTION=PASS`. DOM-/Zeitgeber-Fixture, keine visuelle Browser- oder CEF-Abnahme.
- Das Raster hält maximal 80 Kacheln pro Seite; Cover Flow hält höchstens sieben Elemente. Neue Tests für echte APT-Lebenszyklen mit installiertem Gallery-Paket, EXIF-Orientierung und 1000 Dateien im lokalen Provider sind ergänzt; ihre Ausführung und Ergebnisse müssen noch separat verifiziert werden.

## Akzeptanzmatrix (Plan, nicht Testnachweis)

| Szenario | Erwartetes sichtbares Ergebnis |
| --- | --- |
| Mehrere konfigurierte Quellen, zunächst lokal; entfernte Backends in späterem Integrationsschritt | Eindeutige Zuordnung, keine Vermischung von Quell-IDs oder Daten |
| Unterordner, Unicode und natürliche Sortierung | Nur aktueller Ordner geladen; stabile Sortierung, korrektes Cover und Rückkehrposition |
| Leere/unlesbare Quelle oder Agent-Ausfall | Verständliche lokale Meldung; unabhängige Quellen erreichbar; letzte erlaubte Daten bei temporärem Fehler erhalten |
| Bestehende Autorisierung, Abmeldung, fremde Quellen-ID | Vorhandene Core-Sicherheitsentscheidung gilt auch für Vorschau/Original; keine absoluten Pfade preisgeben |
| Hoch-/Querformat und EXIF-Rotation | Kachel darf zuschneiden, Vollbild zeigt korrekt orientiertes Originalseitenverhältnis |
| Große Bilddateien und Sammlung | Raster lädt kleine Previews; Navigation bleibt bedienbar; keine vollständigen Originale je Kachel |
| Raster/Carousel, Taste 1, Neustart | Ansicht wechselt und bleibt gespeichert; Auswahl und Fokus bleiben erhalten |
| OK, Info, Back, Zoom/Drehung | Vollbild und optionales Info-Overlay; Rückkehr zur Auswahl; Original unverändert |
| Diashow | Start ab Auswahl, Pause/Fortsetzen, Intervall, Wiederholung und Ende korrekt |
| Deaktivierung/Installation/Update | Vorhandene Addon-Verträge; keine Auswirkungen auf native Image Next-/SuiteOSD-Funktion |
| Konkurrenzvergleich | Kodi/Plex/Apple Fotos als UX-Referenz prüfen; keine unbestätigten Funktionsbehauptungen |
| Produktionspfad | Browser -> Public v1 -> bestehende Auth-/Backend-Owner -> Bildprovider -> Bildantwort; Unit/Contract, Integration, CI, Paket, reale Hardware separat |

## CI-Baseline (GitHub Workflow-Status, nicht Job-Log-Audit)

- [VDR-Suite CI #13354](https://github.com/hotzenplotz5/vdr-suite/actions/runs/45948469072) und [Add-on platform #1561](https://github.com/hotzenplotz5/vdr-suite/actions/runs/45948469252), Basis-SHA oben: completed/success.
- [SuiteOSD CI #258](https://github.com/hotzenplotz5/vdr-plugin-suiteosd/actions/runs/45957554750), Referenz-SHA oben: completed/success.
- [Image Next CI #99](https://github.com/hotzenplotz5/vdr-plugin-image-nex/actions/runs/45956632175), Referenz-SHA oben: completed/success.

Die Baseline oben ist historisch. Die Branch-Implementierung enthält lokale Provider-, API-, Frontend- und Paketierungstests. Der vollständige aktuelle Branch-CI-Nachweis und reale Installation/CEF-/TV-Abnahme stehen noch aus. Remote-Agent-Transport ist absichtlich nicht Bestandteil der lokalen Abnahme.
