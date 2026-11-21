# API.V1.F1 — Quellinventur und Client-Paritätsmatrix (21.11.2026)

**Stand:** F1-Quellinventur fertig, **keine** neue API-v1-Freigabe oder Hardware-Abnahme.
Geprüfter `main`-Ausgangsstand: `d306c38b88b0d997add7ee03120239304e9396aa`. Der Dokumentationsstand wird
mit dem F1-Commit fortgeschrieben. Die geltende Root-`AGENTS.md` wurde am
Ausgangsstand als Blob `661a61491014416dfde169340ad716180c31b599`
gelesen; keine untergeordneten AGENTS für die betroffenen Pfade gefunden.

## Maschinenlesbare Quelle und Reproduktion

- [F1-Klassifikation](api-v1-f1-parity-matrix.json) ordnet die fachlichen Domänen,
  die historischen Browser-Gruppen, Legacy-Beispiele und den nächsten Slice zu.
- [Lesendes Prüfskript](../../tools/report_public_v1_f1_parity.js) erfasst alle
  Methoden-/Pfadpaare der **aktuellen** `api/openapi.json`, die benannten
  Browser-Client-Operationen der drei bekannten Phase-69.F-Module und deren
  historische Klassifikation; es meldet nicht zugeordnete oder doppelte
  Einträge. `--json` liefert denselben Bericht maschinenlesbar.
- Quellen sind `api/openapi.json`,
  `docs/development/phase-69f-client-contract-matrix.json`,
  `web/frontend/api/client-api.js`,
  `web/frontend/api/genre-client-api.js` und
  `web/frontend/api/live-remote-client-api.js`. Zusätzlich werden
  String-Literale aus `api/rest/src/*.cpp` als **Hinweise** erfasst.

Das alte Phase-69.F-Inventar bleibt historisch unverändert: Es klassifiziert
**72 benannte Browser-Client-Operationen** gegen den damaligen stabilisierten
Vertrag, nicht alle inzwischen implementierten Public-v1-Routen. Das aktuelle
OpenAPI-Inventar enthält am genannten Ausgangsstand **60 URL-Pfadvorlagen mit
75 HTTP-Methoden-/Pfadpaaren**. Die vollständige Menge ist **nicht**
gleichbedeutend mit 75 freigegebenen, produktiv abgenommenen Funktionen.

## Fachlicher Befund und Folgeslices

| F1-Domäne | Ergebnis | Konsequenz |
| --- | --- | --- |
| Discovery | Öffentlicher Vertrag vorhanden | **F2:** OpenAPI/Reference-JS/Auth-Parität |
| Backends | Public Discovery ≠ interner Browser-Selector/Snapshot | **F3:** Semantik vergleichen, keine blinde Ersetzung |
| Channels | Public Federation/Pagination ≠ Legacy-Channel-Move | **F3:** Reihenfolge/Cursor/Grants abgleichen |
| EPG und Suche | Öffentliche Reads neben Cache-/Global-Search-Hilfen | **F3:** Response- und Funktionsparität |
| Recordings / Genres | Öffentliche `rec_`-Routen neben älterem Browser-Query | **F4:** Browse/Action/Felder vergleichen |
| Metadaten / Artwork | Öffentliche Bilder/Metadaten neben privaten Personen-/Home-Hilfen | **F4:** klare Domänengrenze, keine Performance-Änderung |
| Timer und Operations | NativeTimer ≠ TimerIntent ≠ TimerAssignment | **F5:** sichere gemeinsame Client-Verträge |
| Suchtimer | Web nutzt Public v1 im bestätigten Umfang, Legacy-Hilfen verbleiben | **F5:** Feld-/Device-/MLD-Parität |
| Accounts / Devices | Öffentliche Security-/Lifecycle-Routen vorhanden | **F2:** exakte Auth/OpenAPI/JS-Darstellung |
| Playback / Resume | Öffentliche MediaSession-/Fortschrittsverträge als Quellstand | **F7:** echte Hardware-/Cross-Device-Abnahme |
| Marks / Cut | Eigenständiger Public-v1-Schreibvertrag nicht nachgewiesen | **F6:** Scope-Entscheidung; keine implizite Zusage |
| Broadcast Companion | Eigenständige Domänenebene | **Keine** automatische Migration nach v1 |
| Browser-Cache / Remote / Status | Private oder Übergangs-Oberflächen | **Keine** pauschale v1-Veröffentlichung |

### Wichtige Evidenzgrenzen

1. Die 72 Funktionen entstammen den **drei historischen Phase-69.F-Modulen**.
   Andere Frontend-Module, TV-Repositories und dynamisch erzeugte URLs sind
   damit **nicht vollständig** inventarisiert. Eine String-Fundstelle zeigt
   nur einen Quelltext-Aufruf, nicht die Laufzeitnutzung.
2. `api/openapi.json` bezeichnet sich ausdrücklich als **partial**.
   Der F1-Report zählt dort dokumentierte Methoden, gleicht aber weder
   methodengenaue Registrierung noch Body-Schemas oder `SecurityHttpGate`
   automatisch vollständig ab. Das ist eine klar abgegrenzte F2-Aufgabe;
   fehlende Source-Literale werden **nicht** als Route-Ausfall interpretiert.
3. Bestehende Browser- und Device-Verträge dürfen nicht anhand gleicher
   Endpunktnamen als äquivalent bezeichnet werden. Negative oder fehlende
   Evidenz erzeugt **keine** zusätzlichen Sicherheits-/Freigabesperren.
4. [Zentrale kumulative API-v1-Hardware-Abnahme](public-v1-final-hardware-acceptance.md)
   bleibt der **einzige** Ort für REAL-VDR-PASS und offene Release-Gates.
   Inaktive Suchtimer-Browser-CRUD-Abnahme vom 17.11. bleibt erhalten;
   weder Marks/Cut noch TV-Playback wird dadurch nachträglich abgenommen.
5. Der PWA.15-Performance-Freeze bleibt unverändert. F1 ändert keine
   produktive Runtime, Web-UI, API-Routen, CI-Gates oder Installationen.

**F1-Abnahmekriterium:** Domänen sind explizit klassifiziert; OpenAPI-
Methoden-/Pfade werden vollständig im lesenden Bericht erfasst; alle
historisch benannten Browser-Operationen haben eine eindeutige
Domänenzuordnung. Der Report macht zukünftige Abweichungen sichtbar,
entscheidet sie aber nicht automatisch oder führt Sperren ein.
