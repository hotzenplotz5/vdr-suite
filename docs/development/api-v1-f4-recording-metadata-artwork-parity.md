# API.V1.F4 — Recordings, Metadaten, Genres und Artwork: Client-Parität

**Stand:** 21.11.2026 · **Branch:** `work/api-v1-finalisierung-f2-f8` ·
**Ausgangspunkt:** `583ca30382f1864481eea1cd5ac4815bf47e1df6`.
Root-`AGENTS.md` gelesen: Blob `661a61491014416dfde169340ad716180c31b599`;
keine weiteren AGENTS in den betroffenen Verzeichnissen gefunden.

**Scope:** Vorhandene Public-v1-**Leseoperationen** im Reference-JS-Client
vervollständigen und gegenüber den bestehenden Browserfunktionen
klassifizieren. Keine neue Serverroute, kein Cache-Rewrite, kein
Metadaten-Provider, keine Aufnahmen-Mutation und kein Artwork-Schnellfix.

## F4-Implementierung

| Public-v1-Client | HTTP-Vertrag | Ergebnis |
| --- | --- | --- |
| `getRecordings` (bestehend) | `GET /api/v1/recordings` | Flache, backendgebundene Aufnahmeliste, opaker `rc1_`-Cursor |
| `getRecordingFolders` (**neu**) | `GET /api/v1/recordings?view=folders` | Root/Unterordner via opakem `fld1_`-ID; `limit/offset`, gemischte Folder/Recording-Leaves |
| `getRecordingDetail` (**neu**) | `GET /api/v1/recordings/{recordingId}` | Einzelaufnahme und explizite Metadaten-Verfügbarkeit |
| `getRecordingGenres` (**neu**) | `GET /api/v1/genres` | Backend-Genres mit `locale=de|en` und `limit/offset` |
| `getGenreRecordings` (**neu**) | `GET /api/v1/genres/recordings` | Zugehörige Aufnahmen eines `genreId`, paginiert |
| `searchRecordings` (F3) | `GET /api/v1/search/recordings` | Suche über den bestehenden gemeinsamen Suchindex |
| `getRecordingArtwork`, `getRecordingArtworkImage` (bestehend) | `GET /api/v1/recordings/{id}/artwork[/{artworkId}]` | Artwork-Referenzen und PNG/JPEG/WebP-Bytes, keine Provider-URL |
| `resolveMetadata` (bestehend) | `POST /api/v1/backends/{id}/metadata/resolve` | **Nur Kandidatenauflösung**, keine Mutation oder Zuordnung |

Alle Methoden nutzen den vorhandenen Reference-JS-Transport und dessen
caller-owned Browser-/Device-Nachweise. `rec_`- und `fld1_`-Identitäten
sind öffentlich und opak, **keine nativen Pfade**. Die neuen
`limit/offset`-Abfragen lehnen unbekannte Felder oder vermischte
Cursor-/Folder-Modi *vor* einer HTTP-Anfrage ab. Fehlende cached Artwork
liefert laut Source-Vertrag `200 items:[]`; für tatsächlich fehlende
Einzelbilder bleibt `404` ein überprüfbarer Fehler, nicht ein
stiller Fallback.

## Verbindlicher Webfrontend-vs-Public-v1-Paritätsvergleich

| Dimension | Etabliertes Webfrontend | Public v1 | Entscheidung |
| --- | --- | --- | --- |
| Suche | `/api/search` und `/api/vdr/recordings/query` mit Web-spezifischen Filtern/Komposition | `/api/v1/search/recordings`, ein Backend, `limit/offset`, `items/page.totalCount` | Gemeinsamer Suchindex nur im benannten Public-Vertrag; volle Ergebnis-/Filter-Parität nicht bewiesen |
| Flache Liste | `/api/vdr/recordings/query` mit Web-Sortierung, Gruppen und Zusatzmetadaten | `GET /api/v1/recordings` nach `recordingId` mit `rc1_`-Keyset | **Nicht austauschbar**, andere Reihenfolge/Felder |
| Ordner | `/api/vdr/recordings/folder` mit Web-Navigation | `view=folders`, `fld1_`-Identität, `limit/offset/totalCount` | Logische Browse-Projektion; keine nativen Ordnerpfade zurückgeben |
| Titel, Schnitt | Anzeige aus `recordings2-shared.js` inkl. `#XX`, `_`, führendem `%`; Websuche kann manuell angepassten Metadatentitel bevorzugen | Server-Public-Projektion derselben Titelregeln; `edited` separat | Identitäts- und Titeltreue erforderlich; manuelle Such-/Metadaten-Titel nicht überschreiben |
| Metadaten | Browser-Details/Personen/Trailer via `/api/metadata`, `/api/vdr/persons` u. a. | `getRecordingDetail` + leseorientiertes `resolveMetadata` | **Nicht gleichwertig:** keine Persons-/Trailer-/Home-Aggregation als automatisch öffentlich behandeln |
| Genres | `/api/metadata/genres` und `/api/metadata/genres/recordings` inkl. Browser-`scope/from/until` | `/api/v1/genres` und `/api/v1/genres/recordings` mit `locale/limit/offset` | Scope-/Zeitfilter fehlen im v1-Vertrag; kein blindes Mapping |
| Artwork | Browser-Caches, Proxy-/Home-Varianten und eigene Renderpriorität | Opaque `artworkId` und gesicherte Binär-GETs, keine Browser-Cache-Semantik | **Kein** generischer 404-Fix oder UI-Performance-Änderung aus dem F4-Referenzclient ableitbar |
| Grants | Browser-Session + CSRF bei Mutationen; private Browser-APIs | `recordings.view@backendId`, Backend-Identität pro Read | Caller-owned Security, keinerlei neue Berechtigung |
| Fehler | Browser kann Teilresultate und Home-Cache-Status mischen | Typisierte `400/401/403/404/409/503` ohne Legacy-Fallback | 404 kein identischer Fehler wie 200 leere Artwork; 409 nie blind neu senden |
| Mutationen | `/api/vdr/recordings/actions/validate|execute` | Bereits existierende `/api/v1/recordings/actions/validate|execute` | Reale RENAME/MOVE/DELETE-Abnahme vom 21.10. bleibt gültig; keine erneuten Schreibtests/Releaseänderung in F4 |

Diese Paritätsmatrix **belegt Unterschiede**, nicht deren Aufhebung.
Eine automatische Umstellung der etablierten Web-Frontend-/Home- und
Genre-Pfade ist deshalb **nicht** Teil dieser F4-Quellintegration.
Vor einer Web-Installation ist eine echte, automatisierte
Web-vs-v1-Antwort-Regression mit realistischer Aufnahme inklusive
Titel-/Genre-/Suchzahlen, Sortierung, Metadaten und Artwork nötig;
danach je nach Änderung eine separate yaVDR-Leseabnahme.

## Verifizierte Regressionen

`clients/reference-js/tests/test_public_v1_f4_recording_read_client.js`
prüft mit Source-Client und simuliertem HTTP-Transport:
flache Liste, Ordnerwurzel und Folgeseite, `fld1_`-Navigation,
`rec_`-Detail, `edited`/Titel-Konstanz in den erwarteten
Beispielantworten, Genres, Zugehörigkeit, Artwork-Binärdaten,
Metadaten-Kandidaten, leere Artwork-Liste, fehlendes Bild als 404,
abgelaufenen `rc1_`-Cursor als 409 sowie nicht erlaubte
Parameter ohne HTTP-Aufruf.

Fokussierte isolierte JavaScript-Ausführung:
`API_V1_F4_RECORDING_READS=PASS`; auch der vorhandene
`test_public_v1_recording_client.js` sowie F2/F3-Tests PASS.
**Noch ausstehend:** echter Node-/Make-/CI-Lauf für den finalen Commit
und reale vergleichende Web-vs-v1-Hardwareprüfung. Diese Quelltests sind
kein Nachweis für eine behobene produktive Artwork-404-Ursache.

## Vergleich mit Kodi / Plex

Vergleichsmaßstab ist verlässliche Aufnahme-/Genre-Navigation und
mediengerechte Artwork-Fehlerbehandlung. Ein reproduzierbarer
aktueller Kodi-/Plex-Bildschirm- oder API-Vergleich wurde hier
**nicht** durchgeführt; daraus wird keine Funktionsüberlegenheit
und keine neue Produkt-Anforderung abgeleitet.

**F4 Ergebnis:** Die fehlenden Read-Methoden im Public-v1-Referenzclient
sind ergänzt, testbar und dokumentiert. Die Browser-Paritäts- und
Artwork-Runtime-Abnahme bleibt ausdrücklich offen. F5–F8 sind
separate Folgeslices; kein Merge auf `main`.
