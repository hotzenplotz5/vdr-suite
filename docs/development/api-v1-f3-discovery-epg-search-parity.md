# API.V1.F3 — Discovery, Channels, EPG und Suche: Paritätsbefund

**Stand:** 21.11.2026, Branch `work/api-v1-finalisierung-f2-f8` (F2 als Basis).
**Geltende Regeln:** Root-`AGENTS.md` Blob `661a61491014416dfde169340ad716180c31b599`.
**Abnahmeumfang:** gemeinsame öffentliche Reference-JS-GET-Verträge; **keine**
Freigabe für eine ungeprüfte Webfrontend-/TV-Migration und **kein** API-v1-Release.

## Konkretes F3-Ergebnis

Die bereits vorhandenen Public-v1-Serverrouten und das bisherige
Reference-JS-Discovery-/Channel-API werden wiederverwendet.
`clients/reference-js/public-v1-client.js` bietet jetzt ergänzend:

- `getAdjacentChannel({query:{backendId,relativeTo,direction}})` –
  Nachbarsender per VDR-Kanalnummer, nur mit einem Backend und `limit=1`;
- `getEpgWindow({query:{backendId,channelId,fromTime,untilTime,limit,cursor}})` –
  vollständige mehrtägige EPG-Fenster mit opakem `ew1_`-Cursor;
- `getEpgNowNext({query:{backendId,channelId,fromTime,limit}})` –
  maximal zwei Ereignisse aus dem vorhandenen Cache;
- `searchEpg({query:{backendId,query,limit,offset}})` –
  vorhandene Public-v1-EPG-Indexsuche;
- `searchRecordings({query:{backendId,query,limit,offset}})` –
  vorhandene Public-v1-Aufnahmensuche mit opaken `rec_`-Identitäten.

Bereits vorhanden: `getApiRoot`, `getCapabilities`, `getBackends` und
`getChannels` inklusive `sort=backendId,channelNumber,channelId`
und stabiler `ch2_`-Seitenfortsetzung. Die neuen Aufrufe verwenden
den bestehenden GET-`request()` und reichen Header/Credentials, Cache,
Signal und strukturierte HTTP-Fehler unverändert durch. Kein eigener Index,
keine API-Proxies, keine automatischen Retries, kein Legacy-Fallback.

## Webfrontend vs Public v1 — verbindliche Migrationsmatrix

| Funktion | Bestehender Webpfad | Public-v1-Vertrag | Beweisbarer Unterschied / Entscheidung |
| --- | --- | --- | --- |
| Backend-Discovery | `/api/backends` | `GET /api/v1/backends` | Public nur actor-gefilterte Backendliste mit Keyset; Web braucht weitere Runtime-/Selector-Daten. **Keine pauschale Umstellung** |
| Default Backend | `/api/backends/default` | kein gleichwertiger Public-v1-Default | Kein impliziter Public-Default; **Browser-Hilfe bleibt** |
| Backend-Snapshot | `/api/backends/{id}/snapshot` + Recording-Cache | keine identische v1-Snapshot-Ressource | Status-/Zählungsaggregation bleibt Web-privat |
| Senderliste | `/api/vdr/channels` | `GET /api/v1/channels` | Public erfordert explizite Quelle, liefert Teilquellen-/Cursor-Metadaten; VDR-Kanalnummer-Sortierung ist opt-in; **nicht 1:1 gleichwertig** |
| Sender verschieben | `/api/vdr/channels/move` | kein gleichwertiges Public-v1-Write | Keine neue Mutation oder stille Funktionsentfernung |
| Sender-Nachbar | Live-/Remote-Browserpfade | `GET /api/v1/channels?relativeTo=...` | Public read-only Neighbor-Query; kein Ersatz für Fernbedienungs-Mutation |
| Live-EPG / Zeitfenster | `/api/vdr/events/live`, `/api/epg/time-window`, `/api/epg/channel-window` | `GET /api/v1/epg/window` | Public verlangt ein Backend, einen Sender, Unix-Sekunden und Cursor statt Web-Ansicht; kein direkter Austausch |
| Now/Next | `/api/epg/now-next`, `/api/epg/cache/now-next` | `GET /api/v1/epg/now-next` | Public ist je Sender auf max. zwei Cache-Ereignisse begrenzt; kein vollständiges Grid |
| EPG-Suche | `/api/epg/search` | `GET /api/v1/search/epg` | Gemeinsamer Suchindex laut Public-Quellvertrag; Public scoped, `items/page` und max. 30 pro Anfrage |
| Globale Suche | `/api/search` | separate `/api/v1/search/epg` und `/api/v1/search/recordings` | Kombinierte Webdarstellung und Public-Einzelkollektionen sind **nicht derselbe Response** |

**Ergebnis:** In F3 werden **keine bestehenden Web-Pfade umgeleitet**,
weil Feldvollständigkeit, Ergebniszahlen, Sortier-/Suchlogik, Fehlerfälle,
Grant-Kontext und Ergebnisform nicht als gleichwertig abgenommen sind.
Vor einer späteren Web-Migration sind Web-vs-Public-v1-Regressionen mit
realistischen Aufnahme-/EPG-Beispielen sowie echte yaVDR-Lesetests
erforderlich. Die Weboberfläche bleibt der funktionale Mindeststandard;
ein fehlender Gleichwertigkeitsnachweis ist **kein** Grund, funktionierende
Webrouten zu entfernen.

## Test und Nachweise

`clients/reference-js/tests/test_public_v1_f3_epg_search_client.js`
prüft mit realistischen, aber **simulierten** Antwortdaten: Backendliste,
numerische Senderreihenfolge und `ch2_`-Cursor, Nachbarsender,
Multi-Day-EPG mit opaker `ew1_`-Fortsetzung, Now/Next, Unicode-Suche
(`München`, `Café & Urlaub`), paginierte `totalCount`-Ergebnisse,
stabile Recording-Identitäten, caller-owned Device-Authorization,
ungültige Eingaben ohne Netzwerkrequest und
`503 service_unavailable` ohne Legacy-Retry.

Der Test validiert den echten Reference-JS-Quelltext mit Mock-Transport;
**kein** echter yaVDR-End-to-End-Nachweis und **kein** Web-vs-v1-Server-
Regressionstest. Bestehende Server-/Security-/Cache-Implementierungen
wurden nicht geändert. F1/F2-Verträge bleiben bestehen.

## Relevanter Produktvergleich

Kodi/Plex dienen hier nur als Orientierung für verlässliche Sendernavigation,
EPG-Darstellung und medienübergreifende Suche. Ein eigener reproduzierbarer
Vergleich mit deren aktuellen Oberflächen wurde **nicht** durchgeführt;
ihre UX begründet weder neue F3-Funktionen noch Änderungen der bestehenden
VDR-Suite-Webdarstellung.

## Status und Weiterarbeit

- **F3 Reference-JS Read-Verträge:** implementiert; fokussierter Test PASS
  (isolierte V8-Ausführung auf den Source-Dateien).
- **F3 Browser-Migration:** bewusst **nicht freigegeben**, da
  Antwortparität nicht bewiesen und einige Web-Funktionen echte
  private Zusatzfunktionen sind.
- **F4–F8:** weiterhin offen; `main`, PWA.15-Freeze und bestehende
  Hardware-Abnahmematrix bleiben unberührt.
