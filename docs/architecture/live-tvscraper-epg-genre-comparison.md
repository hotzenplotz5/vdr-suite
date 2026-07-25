# Live 3.5.5 / TVScraper 1.2.15 – EPG-Genrevergleich

Status: Phase-61-Diagnosevertrag auf `feature/phase61-metadata-genre-browser`

## Zweck

Dieses Dokument hält die vollständige fachlich relevante Live-/TVScraper-
Analyse für EPG-Filmgenres fest und definiert den reproduzierbaren Vergleich
gegen SuiteBridge.

Die Analyse betrifft ausschließlich den read-only EPG-Metadatenpfad. Sie
ändert weder die EPG-Timeline noch Recordings 2 noch den LiveRemote-/
Overlay-Pfad.

## Verbindliche Versionsstände

Für die Produktionskombination sind folgende Stände maßgeblich:

- VDR: 2.7.9
- Live: Tag `v3.5.5`, Commit
  `e9de9ba07335f9ea4aa451fc3f7a765aefcbd6cb`
- TVScraper: Tag `v1.2.15`, Commit
  `dbfd299678977363e8041e54e3d134c05287b73d`
- SuiteBridge-Produktionsstand vor der Diagnose: 0.13.0
- SuiteBridge-Diagnosestand mit `MCOMPARE`: 0.13.1
- VDR-Suite-Ausgangsstand dieser Analyse:
  `bbde90681642d916d752683e4785cabf057ebdba`

Authoritative Upstreams:

- `MarkusEh/vdr-plugin-live`
- `MarkusEh/vdr-plugin-tvscraper`

Die in Live eingebaute lokale Kopie von `services.h` entspricht dem neuen
TVScraper-Servicevertrag, den TVScraper 1.2.15 implementiert.

## Vollständig erfasster relevanter Live-Sourcebaum

Die Analyse umfasst nicht nur Suchtreffer auf einzelne Methoden, sondern die
zusammenhängenden Aufrufer-, Anzeige-, Cache- und Lebensdauerpfade:

- `services.h`
  - `cGetScraperVideo`
  - `cScraperVideo`
  - `getMovieOrTv`
  - `getEpisode`
  - Personen-, Bild-, Poster- und Update-Time-Strukturen
- `epg_events.h`
- `epg_events.cpp`
  - Event-ID-Auflösung
  - `EpgInfo`
  - `InitializeScraperVideo`
  - EPG-Listen- und Detailpfad
  - Recording-zu-EPG-Hilfspfade
- `pages/epginfo.ecpp`
  - Event- und Recording-Detailseite
  - Locking bis zur Übergabe an die Seitenelemente
- `pages/pageelems.ecpp`
  - Scraper-Block
  - Movie-/Series-Ausgabe
  - Genres
  - Episode
  - Personen
  - Bilder
  - Collection-/Network-/Country-Felder
- `recman.h`
- `recman.cpp`
  - Recording-Lock und Recording-Lebensdauer
  - `cGetScraperUpdateTimes`
  - Recording-Scraper-Cache
  - Movie-/Series-/Episode-/Collection-Einsortierung
- `tools.h`
- `tools.cpp`
  - TVScraper-Pluginauflösung
  - `ScraperCallService`
  - Bildpfadübersetzung
- `pages/recordings.ecpp`
  - Recording-Detail- und Listenübergabe
- EPG-Listen-, Such- und Tooltip-Seiten, die über `epg_events.cpp` und
  `pageelems.ecpp` denselben Resolver benutzen
- Content-/Bildauslieferung für bereits vom Resolver gelieferte Pfade

Alle Aufrufer des neuen `cGetScraperVideo`-Vertrags liegen in Live 3.5.5 in:

- `epg_events.cpp`
- `recman.cpp`
- `pages/pageelems.ecpp`

`GetScraperUpdateTimes` wird nur für die Invalidierung des persistenten
Recording-Baums benutzt. Vor einer einzelnen EPG-Metadatenauflösung ruft Live
keinen Scraper-Refresh, keinen Worker und keinen zusätzlichen Cache-Service
auf.

## Exakter Live-EPG-Datenfluss

### Ereignisauflösung und Lebensdauer

Der Detailpfad beginnt in `CreateEpgInfo` beziehungsweise den
EPG-Listenhelfern.

Live:

1. hält die erforderlichen VDR-Read-Locks,
2. löst Channel und Schedule auf,
3. sucht das reale, schedule-eigene `cEvent` nach Event-ID,
4. erzeugt `cGetScraperVideo(event, nullptr)`,
5. ruft den Service `GetScraperVideo`,
6. bewahrt das zurückgegebene `cScraperVideo` im request-lokalen `EpgInfo`,
7. liest die Scraper-Felder, solange die zugehörigen VDR-Objekte im
   dokumentierten Aufrufkontext gültig sind.

Live verwendet für den relevanten EPG-Pfad keinen selbst konstruierten
Event-Datenträger und keine Kopie. Der TVScraper-Aufruf erhält den echten
`cEvent*` aus dem `cSchedule`.

### Film-/Serien-Erkennung

Live liest:

- `getVideoType()`
- `getDbId()`
- `getSeasonNumber()`
- `getEpisodeNumber()`

`tMovie` und `tSeries` werden vom selben `cScraperVideo` unterschieden. Es
gibt keinen zweiten Genre-Resolver.

### Movie-/Series-Metadaten und Genres

Für Movie und Series ruft `pageelems.ecpp` genau einmal
`getMovieOrTv(...)` auf. Dabei wird ein `vector<string> genres` übergeben.

Die ausgegebenen Genres stammen direkt aus diesem Vektor. Live:

- kombiniert keine Episode-Genres,
- leitet Genres nicht aus Personen oder Bildern ab,
- liest keine Collection-Genres,
- ruft keinen separaten Genre-Service auf,
- normalisiert die gelieferten Strings nicht in eine eigene Taxonomie.

Collection-Name und Collection-ID sind nur zusätzliche Movie-Felder aus
demselben `getMovieOrTv`-Aufruf.

### Episode

Für Series ruft Live anschließend `getEpisode(...)` auf. Die Episode liefert:

- Episodentitel
- Episodenübersicht
- absolute Episodennummer
- Erstausstrahlung
- Laufzeit
- Bewertung
- IMDb-ID

Die Episode besitzt in diesem Servicevertrag kein Genre-Feld.

### Personen und Bilder

Personen und Bilder werden aus demselben `cScraperVideo` gelesen:

- `getCharacters(...)`
- `getPoster()`
- `getBanners()`
- `getMedia()`

Diese Aufrufe beeinflussen die Genre-Liste nicht. Sie bestätigen jedoch, dass
Live keine zweite Movie-/Series-Entität auflöst, sondern alle Details aus
dem bereits an das reale Event gebundenen Resolverobjekt bezieht.

## Exakter Live-Recording-Datenfluss

Der Recording-Pfad verwendet ein echtes `cRecording*` unter
`LOCK_RECORDINGS_READ`.

`RecordingsItemRec` ruft den gleichen `GetScraperVideo`-Service mit
`recording` statt `event` auf. Der Recording-Baum cached daraus unter anderem:

- Video-Type
- Provider-ID
- Staffel/Episode
- Scraper-Titel
- Episodentitel
- Laufzeit
- Veröffentlichungsdatum
- IMDb-ID
- Poster
- Collection

`cGetScraperUpdateTimes` invalidiert diesen Recording-Cache, wenn TVScraper
neue Recording-Daten meldet. Dieser Mechanismus ist nicht Teil der
EPG-Einzelauflösung und erklärt daher keine zusätzlichen Live-EPG-Genres.

Recordings 2 in VDR-Suite besitzt einen eigenen RMETA-/SQLite-Pfad und wird
durch die Phase-61-Diagnose nicht verändert.

## TVScraper-1.2.15-Service intern

### Event-Identität

`cScraperVideoImp` erzeugt zunächst `cMovieOrTv`. Für ein EPG-Ereignis löst
TVScraper die Zuordnung über die Tabelle `event` auf.

Der relevante Schlüssel ist:

- `event_id`
- `channel_id`

Die Channel-ID wird aus `event->ChannelID()` gebildet. TVScraper verwendet
für diese Abfrage weder Titel noch Startzeit.

### Movie-/Series-Datensätze

Der Wert `season_number == -100` kennzeichnet Movie. Andere gemappte Einträge
werden als Series behandelt.

Movie-Metadaten kommen aus:

- Tabelle `movies3`
- Schlüssel `movie_id`
- Genrefeld `movie_genres`

Series-Metadaten kommen aus:

- Tabelle `tv2`
- Schlüssel `tv_id`
- Genrefeld `tv_genres`

### Genreformat

`movie_genres` und `tv_genres` sind `|`-getrennte Strings.

`stringToVector`:

- trennt ausschließlich am Zeichen `|`,
- akzeptiert optionale Trenner am Anfang und Ende,
- verwirft leere Werte,
- verwirft doppelte Werte,
- bewahrt die übrigen Werte unverändert.

Damit ist die Live-Ausgabe für Genres vollständig durch diese beiden
Datenbankfelder bestimmt.

## Exakter SuiteBridge-Pfad

### ETYPES

ETYPES:

1. baut ein stabiles 48-Stunden-Snapshot der Ereignisidentitäten,
2. löst pro Eintrag erneut Channel und Schedule auf,
3. prüft Event-ID, Start- und Endzeit,
4. ruft TVScraper mit dem echten schedule-eigenen `cEvent` unter
   Channel- und Schedule-Read-Lock auf,
5. speichert nur Movie oder Series als
   `tvscraper-media-type` / `scraper-media-type`.

Dieser Aufrufkontext entspricht Live.

### META vor dem Diagnosepatch

META:

1. löst das reale Event unter Schedule-Lock auf,
2. kopiert dessen Felder in einen neu konstruierten `cEvent`,
3. hängt die Kopie an einen privaten `cSchedule`,
4. gibt den VDR-Schedule-Lock frei,
5. ruft TVScraper erst danach mit der Event-Kopie auf.

Die Kopie erhält dieselbe Event-ID und dieselbe Channel-ID über den privaten
Schedule. Weil TVScraper 1.2.15 anhand `event_id + channel_id` auflöst, ist
dieser Pfad statisch plausibel. Er verletzt aber den von Live verwendeten und
im Servicevertrag beschriebenen Lebensdauer-/Objektkontext. Nur ein realer
Vergleich kann beweisen, ob TVScraper in der Produktionskombination beide
Objekte identisch behandelt.

### META-Extraktion

Wenn TVScraper ein Resolverobjekt liefert, ruft SuiteBridge wie Live
`getMovieOrTv(...)` mit einem Genre-Vektor auf. Die Werte werden ohne
zusätzlichen Provideraufruf in `metadata.genres` übernommen.

Movie-/Series-/Episode-/Personen- und Bildfelder stammen vom selben
Resolverobjekt.

### Serialisierung und Transport

Der META-JSON-Vertrag enthält `genres` als Stringarray.

Der SVDRP-Transport:

- sendet `PLUG suitebridge META <channel-id> <event-id>`,
- verlangt Antwortcode 250,
- bewahrt den Payload unverändert bis zum Resolverparser.

Der SuiteBridge-Adapter begrenzt den öffentlichen META-Vertrag auf zwölf
Genrewerte. Der Serializer und der Agentparser ersetzen oder filtern die
übernommenen Werte nicht. `MCOMPARE` liest den Live-Vektor zusätzlich
unbegrenzt, damit auch diese Vertragsgrenze im Vergleich sichtbar wäre.

### Evidenz und Browse-Reconciliation

Die vollständige META-Evidenz wird gespeichert als:

- Provider: `tvscraper`
- Source Kind: `scraper-metadata`

Die Media-Type-Evidenz bleibt getrennt:

- Provider: `tvscraper-media-type`
- Source Kind: `scraper-media-type`

Leere Genrelisten werden als `missing` gespeichert. Nicht erfolgreiche
Transportversuche werden `stale`; ein expliziter, erfolgreich aufgelöster
Nichttreffer ist sechs Stunden frisch.

`replaceEvidence` speichert:

- Originalwert
- kanonische Genre-ID
- Assignment State
- observed_at
- Provider
- Source Kind

Die Film-Untergenreabfrage verwendet ausschließlich:

- Browse-Klasse `movie`
- Provider `tvscraper`
- Source Kind `scraper-metadata`
- State `active`, `unknown` oder `conflict`
- bekannte kanonische Filmgenre-IDs

`conflict` wird also nicht aus der Filmhierarchie ausgeschlossen.
`stale` und `missing` werden bewusst ausgeschlossen.

## Statisch ausgeschlossene Ursachen

Aus dem Code sind folgende Möglichkeiten ausgeschlossen:

- META besitzt gar keinen Genreaufruf.
- Genres liegen an der Episode.
- Genres liegen nur an einer Collection.
- Live kombiniert einen zweiten Genre-Service.
- Der META-Serializer lässt das Feld weg.
- Der Agentparser verwirft eine gültige Genre-Liste.
- Konflikte werden grundsätzlich aus der Filmhierarchie entfernt.
- ETYPES- und META-Frische verwenden noch denselben Provider.
- Das 64er Kandidatenlimit verhindert dauerhaft alle weiteren Kandidaten.
- Live löst vor jedem EPG-Aufruf einen TVScraper-Refresh aus.

## Noch real zu beweisende Möglichkeiten

Vor der Produktionsausführung bleiben insbesondere offen:

1. Das reale `cEvent*` liefert mehr als die Event-Kopie.
2. Beide Aufrufkontexte sind identisch und TVScraper besitzt für die
   meisten Movie-Mappings leere `movie_genres`.
3. Live/META liefern Genres, aber die bereits persistierte Evidenz stammt
   noch aus einem früheren leeren oder fehlgeschlagenen Lauf.
4. Ein Ereignis ist in ETYPES Movie, besitzt bei der späteren META-Auflösung
   aber kein aktuelles TVScraper-Eventmapping mehr.
5. Die vorhandenen Genrewerte sind nicht Teil der kanonischen
   Filmgenre-Taxonomie.
6. Ein Provider-/Source-Kind-/State-Unterschied liegt in den realen
   Persistenzzeilen vor.

## Read-only Vergleichsvertrag

### SVDRP `MCOMPARE`

`MCOMPARE <channel-id> <event-id>` ist ein begrenzter read-only
Diagnosebefehl.

Er führt für dasselbe Ereignis aus:

- `live`
  - echtes schedule-eigenes Event
  - Channel- und Schedule-Read-Lock
  - direkter `cGetScraperVideo`-/`getMovieOrTv`-Aufruf wie Live, ohne
    META-Vektorgrenze
- `detached`
  - dieselbe Eventkopie wie der bestehende META-Pfad
  - Aufruf nach Freigabe der VDR-Locks
  - derselbe direkte, ungekürzte Genreaufruf

Der Payload enthält nur:

- Eventidentität
- Titel und Zeitgrenzen
- Found
- Provider
- Provider-ID
- Media-Type
- Genres

Es werden weder Personen noch Bilder dupliziert. Der Befehl verändert weder
VDR noch TVScraper noch die VDR-Suite-Datenbank.

### Repository-Werkzeug

`tools/compare_phase61_live_tvscraper.py` vergleicht je Ereignis:

- Backend
- Channel-ID
- Event-ID
- Titel
- Start- und Endzeit
- ETYPES
- `MCOMPARE.live`
- `MCOMPARE.detached`
- direktes META
- rohe TVScraper-1.2.15-Daten aus `event`, `movies3` und `tv2`
- vollständige persistierte Genre-Evidenz
- persistierte Browse-Klasse

Das Werkzeug:

- bevorzugt Filmereignisse ohne kanonisches Untergenre,
- unterstützt begrenzte Stichproben,
- unterstützt `--limit 0` für alle Filme im Fenster,
- schreibt strukturiertes JSON,
- schreibt nach jedem Ereignis atomar,
- kann mit `--resume` fortsetzen,
- verwendet SQLite ausschließlich read-only,
- verwendet ausschließlich lokale SVDRP-Kommandos,
- besitzt keinen HTTP- oder Browserzugriff auf TVScraper.

### Diagnoseergebnisse

`PASS` bedeutet pro geprüftem Ereignis:

- ETYPES meldet Movie,
- reales Live-äquivalentes Ergebnis entspricht der rohen
  TVScraper-Datenbank,
- Eventkopie entspricht dem realen Event,
- direktes META entspricht der Eventkopie,
- persistierte Originalgenres entsprechen META,
- Browse-Klasse ist Movie,
- kanonische Filmgenres sind, soweit vorhanden, vollständig erhalten.

`FAIL` benennt den ersten oder mehrere exakte Übergänge:

- `live-detached-mismatch`
- `detached-meta-mismatch`
- `raw-live-mismatch`
- `etypes-not-movie`
- `meta-persistence-mismatch`
- `missing-state-not-persisted`
- `not-found-state-not-persisted`
- `etype-persistence-mismatch`
- `event-identity-mismatch`
- `browse-class-not-movie`

Wenn alle Übergänge über das vollständige Filmfenster übereinstimmen und
TVScraper selbst leere Genrelisten liefert, lautet die fachliche Diagnose
`tvscraper-data-sparse-complete`. Bei einer begrenzten Stichprobe wird dies
explizit nur als `tvscraper-data-sparse-sample` ausgewiesen.

## Architekturgrenzen

Unverändert bleiben:

- Browser kommuniziert nur mit VDR-Suite-REST.
- Browser kommuniziert nicht mit TVScraper, TMDB oder IMDb.
- Die EPG-Timeline wird nicht verändert oder neu gewrappt.
- Recordings 2 bleibt fachlich getrennt.
- PR-99-LiveRemote-/Overlay-Ownership bleibt unverändert.
- Backend-Isolation und persistente Evidenz bleiben erhalten.
- `MCOMPARE` ist keine öffentliche Browser-API.
