# HbbTV Discovery und optionales VDR-Suite-Addon — Roadmap (Entwurf)

**Stand:** 23.11.2026 · **Status:** ZURÜCKGESTELLT / NICHT BEAUFTRAGT · **Typ:** historische Option, kein aktiver Workstream

**Maintainer-Entscheidung 23.11.2026:** Der Aufwand für ein separates Discovery-Plugin oder die HbbTV-Addon-Extraktion ist derzeit zu hoch. **HDE.0–HDE.4, HBA.0–HBA.4 und HBA.X sind vollständig zurückgestellt.** Stattdessen bleiben die vorhandenen Komponenten und die bestehende VDR-Suite-Architektur erhalten. Die laufende Aufgabe ist ausschließlich, unsere bestehenden HbbTV-Patches in `hotzenplotz5/vdr-plugin-web`, `cefbrowser` und `remotetranscode` auf Upstream abzustimmen, gezielt zu testen und zu paketieren. Keine neue Plugin-/Addon-Architektur, kein neuer Branch und keine Migration.

Diese Roadmap dokumentiert zwei getrennte Fragen:
1. Ausgliederung der **read-only AIT/HbbTV-Discovery** aus `vdr-plugin-web` in ein kleines eigenständiges natives VDR-Plugin.
2. Zusammenfassung der **VDR-Suite-HbbTV-Funktion** als optionales Suite-Addon einschließlich UI, Konfiguration und Debian-Paketierung.

Sie ist **kein Auftrag**, bestehende produktive Laufzeitdienste aus dem Core zu löschen, eine neue allgemeine Plugin-ABI einzuführen oder laufende VDR-Dienste umzustellen. Ein vollständiger Core-Extract wäre eine dritte, separat zu genehmigende Architekturentscheidung. Phase 67 ist historisch abgeschlossen und wird nicht neu eröffnet; Phase 70 wird nicht gestartet.

## Ausgangsbefund und Eigentümer

- Upstream `Zabrimus/vdr-plugin-web` PR #1 (`Add read-only HbbTV discovery service v1`) ist offen und konfliktbehaftet. Zabrimus schlug am 23.11.2026 ausdrücklich ein eigenes kleines Plugin als Alternative vor. Das ist ein **Vorschlag**, keine Upstream-Zusage.
- Die bisherige Discovery ermittelt AIT-Anwendungen in `vdr-plugin-web/ait.cpp`, hält den Kanalstatus in `status.cpp` und liefert den read-only Service über `web.cpp` / `service/vdrsuite_hbbtv_discovery_service.h`. Der Upstream-AIT-Empfang ist an den Browser-Client gekoppelt; ein eigenständiger Empfänger muss diese Abhängigkeit lösen.
- Die Suite besitzt bereits SuiteBridge-Resolver/Provider-Verträge, Agent-Transport, Core-/Daemon-Sessions, REST-Endpunkte und Frontendfunktionen. Diese sind nicht identisch mit der Discovery und bleiben zunächst bei ihren bisherigen Eigentümern.
- Drei zusammengehörige externe Laufzeitpakete sind `vdr-plugin-web`, `cefbrowser` und `remotetranscode`. Die bestehende Paketdokumentation und die tatsächlichen GitHub-CI-Ergebnisse sind getrennt zu verifizieren.
- **Binding:** ADR-0054 (Broadcast Companion), ADR-0068 (Optional Add-ons), ADR-0037 (Packaging), AGENTS.md und die bestehenden Core-Authentisierungs-/Berechtigungs-/API-Grenzen. ADR-0068 deckt bisher produktiv vor allem Media Tools ab; ein aktivierbares HbbTV-Addon erfordert eine ausdrücklich abgestimmte Erweiterung seines Produktumfangs, nicht bloß eine neue Manifestdatei.

## Zielarchitektur (Vorschlag, noch keine akzeptierte ADR)

```text
VDR / DVB / AIT
   |
   +--> vdr-plugin-hbbtv-discovery (Arbeitstitel)
   |       -> read-only, versionierter VDR-Discovery-Service
   |       -> SuiteBridge -> bestehender Suite-Core -> API -> Clients
   |
   +--> vdr-plugin-web
           -> HbbTV-Start / Browser-Sitzung / OSD / Medien
           -> cefbrowser + remotetranscode
           -> bestehende SuiteBridge- und Core-Session-Verträge

Optionales VDR-Suite-Addon (Paket/Produktkomposition):
   -> UI + Capability-Bindings + dokumentierte Komponentenabhängigkeiten
   -> verwendet die bereits existierende Core-Autorität und API
   -> kein neuer generischer C++-Runtime-Plugin-Loader
```

Die Discovery soll **ohne gestarteten cefbrowser** funktionieren. Das eigenständige Plugin darf die laufende Browser-Wiedergabe nicht verändern. Live-/Kanalidentität, Revision/Freshness und AIT-Daten müssen wahrheitsgetreu bleiben. Discovery-Provider-Wechsel und alte Service-IDs werden vor einem Umschalten explizit auf Kompatibilität geprüft; keine heimliche gleichzeitige Autorität zweier Anbieter.

## Track D — eigenständige Discovery

| Slice | Umsetzung | Nachweis / Abschluss |
| --- | --- | --- |
| **HDE.0 – Boundary/ABI-Inventur** | Bestehende AIT-Empfangspfade, Kanalwechsel, Service-IDs, SuiteBridge-Resolver und Upstream-Konflikte gegenüber aktuellem HEAD erfassen. Ziel-Service-Name und v1-Kompatibilitätsadapter entscheiden, ohne heute einen neuen Vertrag zu erfinden. | Eine dokumentierte Kompatibilitätsmatrix: bisherige Service-ID, neuer Anbieter, Versions-/Feldgleichheit, Umschaltpfad und Lizenzlage. |
| **HDE.1 – Native read-only Discovery** | Kleines VDR-Plugin mit eigenem AIT-/PAT-/PMT-Filter, kanalgebundener Anwendungsliste, begrenzten Feldern und Versionierung. Keine Browser-/Thrift-/Transcoder-Abhängigkeit. | Unit-Tests zu AIT, Mehrfach-Anwendung, Kanalwechsel, leerem Signal, Revision/Freshness, Neustart; Build für die tatsächliche VDR-ABI. |
| **HDE.2 – SuiteBridge-Adapter** | Discovery-Abfrage des eigenständigen Plugins über vorhandene SuiteBridge-/Core-Grenzen; dieselben für Clients sichtbaren Felder/Status. Bestehenden `vdr-plugin-web`-Provider während der Migration kompatibel halten. | Vorher-/Nachher-Parität für vorhandene API und Webfrontend; keinerlei neue Berechtigungs- oder Polling-Logik. |
| **HDE.3 – Paket & Real-VDR-Abnahme** | Separates Debian-/Ubuntu-VDR-Plugin-Paket, installier- und entfernbar ohne Browser-Komponenten; keine Installation aus Git-Checkout in produktive Pfade. | Source- und Binary-Build, ABI/Deps, Installation auf testgeeignetem yaVDR, realer Tuner/AIT, Senderwechsel, Restart, Remove/Rollback; Decoder/OSD weiter funktionsfähig. |
| **HDE.4 – Upstream-Entscheidung** | Nach erfolgreicher Abnahme Zabrimus das schmale Plugin bzw. die Schnittstelle zur Diskussion vorlegen. Den vorhandenen PR #1 nicht automatisch schließen, rebasen oder mergen. | Ein präziser Vorschlag mit Testnachweisen; Upstream-Maintainer entscheidet unabhängig. |

**Geschätzter Aufwand HDE gesamt:** etwa **1–2 Entwicklerwochen** mit bestehendem Discovery-Code als Vorlage; Empfangs-/Tuner- und Distributionstests können den Aufwand erhöhen. Ein Plugin, das nur eine Headerdatei verschiebt, erfüllt HDE.1 nicht.

## Track A — optionales HbbTV-Addon für VDR-Suite

| Slice | Umsetzung | Nachweis / Abschluss |
| --- | --- | --- |
| **HBA.0 – Produktgrenze** | Aus ADR-0054/0068 ableiten, was ein HbbTV-Addon verwaltet: opt-in Funktionsoberfläche, Komponenten, Konfiguration, Paketabhängigkeiten, Status. Core behält Auth/Grants, API, Anwendungssitzungen und Backendauswahl. | Gezielt dokumentierte Eigentümer- und Installationsmatrix; Entscheidung zur Erweiterung des Add-on-Geltungsbereichs durch Maintainer. |
| **HBA.1 – Addon-Paketkomposition** | Eigenständiges optionales `vdr-suite-addon-hbbtv` (Arbeitstitel) mit Manifest/README/AGENTS, UI-Assets bzw. UI-Bindings und überprüften Paketabhängigkeiten. Vorhandene Komponenten nicht kopieren oder umbenennen. | Reproduzierbare isolierte Stage, Paketinhalt und apt-Abhängigkeiten; bloße Paketpräsenz bewirkt keine ungeplante Runtime-Aktivierung. |
| **HBA.2 – Nutzeroberfläche und Fähigkeiten** | Bestehende HbbTV-Anzeige/Launch/Navigation/Schließen in ein optionales Feature-Bündel überführen. Vorhandene Public API und Session-Verträge weiterverwenden; kein zweiter Browser-Controller und keine neue künstliche Rechteprüfung. | Echte Browser-Journeys: Liste, Start, Farbtasten/Navigation, Video, OSD, Stop/Zurück; gleiche Semantik mit und ohne optionales Paket. |
| **HBA.3 – Drei Laufzeitkomponenten** | Versions-/Thrift-Verträge zwischen `vdr-plugin-web`, `cefbrowser` und `remotetranscode` paketiert prüfen. Legacy-`/opt/hbbtv`-Installation nur nach getrenntem Inventar und abgesichertem Umstellpfad ablösen. | Echte Noble-/Resolute-Source-/Binary-CI, korrekte VDR-ABI, Reinstall/Upgrade/Remove/Rollback, keine parallelen manuellen/paketierten Dienste. |
| **HBA.4 – Gesamtabnahme und Dokumentation** | End-to-End im echten yaVDR, plus Browser-/TV-Clients; Sender ohne AIT, Neustart/Disconnect, Wechsel des aktiven Kanals und Rückkehr zu Live-TV prüfen. Quell-/Paket-Lizenzstatus vor öffentlichem PPA-Release klären. | Verifizierte Commit-/Paket-/Host-Matrix; Benutzeranleitung aktualisiert. Kein PPA-Upload oder Live-Install ohne separate Freigabe. |

**Geschätzter Aufwand HBA:** etwa **1–3 zusätzliche Entwicklerwochen**. Das ist ein **optionales Produkt-/Paket-Addon auf dem bestehenden Core** und ausdrücklich **noch keine physische Core-Extraktion**.

## Track X — vollständige technische Extraktion (nur bei späterer Freigabe)

**HBA.X1**: neue getestete Laufzeit-Grenze zwischen Addon und Suite-Core für Discovery, Runtime, OSD/Presentation und Medien, ohne vorhandene API/Grants/Sitzungs-Eigentümer zu verdoppeln.

**HBA.X2**: Core-/Daemon-/Agent-/SuiteBridge-Implementierung auf schmale Adapter reduzieren, Addon-spezifische Umsetzungen herausziehen, Migration und Rückfallkompatibilität.

**HBA.X3**: getrennte optionale Builds, Paket-/Update-/Remove-Lifecycle und vollständige reale End-to-End-Abnahme. Kein generischer dynamischer C++-Plugin-Loader als Nebenprodukt.

**Geschätzter Zusatzaufwand:** **6–12 Entwicklerwochen**, stark abhängig von der gewünschten Laufzeitisolation und API-Stabilität. Wenn zusätzlich der komplette HbbTV-Browser-/OSD-/Video-Teil aus **`vdr-plugin-web`** in ein neues natives VDR-Plugin verlagert werden soll, kommen grob **4–8 Wochen** hinzu; diese Auslagerung ist nicht Voraussetzung für HDE/HBA.

## Verbindliche Reihenfolge und Stop/Go

1. **Jetzt:** HDE.0 durchführen; keinen vorhandenen funktionierenden Browser- oder Suite-Core-Code löschen. Auf dem jeweils tatsächlichen aktuellen Repository-HEAD arbeiten, AGENTS.md befolgen.
2. **Danach:** HDE.1–HDE.3 als eigenständigen, nachweisbaren Discovery-Vertical umsetzen und auf echtem VDR abnehmen.
3. **Nach expliziter Addon-Produktentscheidung:** HBA.0–HBA.4. Vorhandene Auth-/API-/Session-Owners beibehalten. Keine neue Phase 70 und keine unaufgeforderte Migration.
4. **Optional und separat:** HBA.X erst nach belegtem Nutzen gegenüber dem wesentlich kleineren optionalen Paketansatz und eigener Architekturfreigabe.

**Nicht als Erfolg ausgeben:** dokumentierte Planung, vorhandene Quell-Dateien, erfolgreiches Provider-Unit-Testset oder ein grüner einzelner Paketbuild sind jeweils kein Nachweis für eine installierbare komplette HbbTV-Addon-Runtime.

## Referenzen

- [ADR-0054 Broadcast Companion](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0068 Add-on-/Paketgrenze](../adr/ADR-0068-optional-addons-and-source-packaging.md)
- [Add-on-Plattform-Roadmap](addon-platform-roadmap.md)
- [HbbTV-Installation](../hbbtv-installation.md)
- [Upstream-PR #1 / Review-Kommentar](https://github.com/Zabrimus/vdr-plugin-web/pull/1)
- [VDR-Suite HbbTV Integrations-PR #15](https://github.com/hotzenplotz5/vdr-plugin-web/pull/15)
