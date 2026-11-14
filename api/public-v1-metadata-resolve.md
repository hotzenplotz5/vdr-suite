# META.CORE1 — zentraler Metadata Resolver (Public API v1)

Status: Quellcode-Slice auf `main`; produktive Abnahme erst nach lokalem
inkrementellem Daemon-Build, Tests und yaVDR-Prüfung.

## Einziger automatischer Matching-Vertrag

`POST /api/v1/backends/{backendId}/metadata/resolve`

Ein authentifizierter Client mit der bereits vorhandenen
`recordings.view`-Berechtigung für den ausgewählten Backendkontext
verwendet `Content-Type: application/json`.

Serienepisode:

```json
{"title":"Star Wars-Andor","kind":"series","seasonNumber":2,"episodeNumber":4}
```

Film:

```json
{"title":"Forrest Gump","kind":"movie","year":1994}
```

Die Jahresangabe ist für Filme und Serien optional. Bei gleichnamigen
Serien wird das Jahr des **Serienstarts** aus TMDb `first_air_date`
verglichen (nicht das Ausstrahlungsjahr einer einzelnen Folge).
Bei `Foundation (2021)` ist `year: 2021` daher entscheidend.
Ohne hinreichende Identität bleibt das Ergebnis `ambiguous`.

Der Resolver verwendet den **vorhandenen** `TmdbRecordingMetadataCandidateProvider`
mit dessen konfiguriertem Credential-/Sprachmodell. Seit MT.META7.2
(Quellcode-Slice 14.11.2026) gelten generische, sprachneutrale
Titelhinweise: Satzzeichen, gebräuchliche Artikel, Teil-/Kapitelbezeichnungen
(`Deel`, `Teil`, `Part`, `Chapter` usw.), numerische und ausgeschriebene
Folgenummern sowie sinnvoll abgetrennte Serientitel können die
*Kandidatensuche* erweitern. TMDb-`title` **und** `originalTitle` werden
anschließend aber erneut gegen den vollständigen Ursprungstitel bewertet.
Eine Verkürzung allein ist niemals ein Trefferbeleg. Mit dem optionalen
Jahr wird weiterhin das Film-Release bzw. der Serienstart abgeglichen;
bei Serien folgt die genaue Staffel-/Episodenprüfung. Nur ein eindeutiger
Kandidat darf `matched` ergeben; gleich starke Kandidaten bleiben
`ambiguous`. Providerprobleme liefern weiterhin HTTP 503.

Eindeutige Episode:

```json
{
  "status":"matched",
  "mediaType":"episode",
  "series":{"kind":"series","providerId":"tmdb","externalNamespace":"tv","externalId":"1399","title":"Star Wars: Andor"},
  "episode":{"kind":"episode","providerId":"tmdb","externalNamespace":"tv-episode","externalId":"123","parentExternalId":"1399","title":"Waren Sie schon einmal in Ghorman","seasonNumber":2,"episodeNumber":4}
}
```

Die vollständigen Serien-/Episodenobjekte besitzen die bereits etablierten
`RecordingMetadataCandidate`-Eigenschaften (`overview`,
`posterReference`, `originalTitle`, `releaseDate` etc.).

Andere Ergebnisse (HTTP 200): `{"status":"not_found"}` oder
`{"status":"ambiguous"}`. Invalides Format: HTTP 400; nicht
verfügbarer Provider: HTTP 503; unzulässige Berechtigung nach bestehendem
HTTP-Gate.

## Regeln

- **Read-only.** Keine DB- oder `info`-Mutation, kein Import und keine
  Auftragsreservierung durch `resolve`.
- Metadaten nur durch den bestehenden expliziten
  `/api/backends/{backendId}/recordings/metadata/assign`-Weg speichern.
  Das zugehörige Revisions-/Konfliktverhalten bleibt erhalten.
- Home und Media Tools nutzen den zentralen Resolver bei der
  automatischen Episodenerkennung. Die vorhandenen manuellen
  `search`/`seasons`/`episodes`-Routen bleiben für bewusst gewählte
  Treffer verfügbar.
- Der Referenzclient exportiert `resolveMetadata(options)`, damit
  Samsung/VIDAA und neue Clients keine eigenen TMDb-Titelmatcher schreiben.
- Kein Frontend-Polling: Der Resolver ist eine synchrone, durch eine
  Nutzeraktion/Quellenauswahl angestoßene Lookup-Operation. Dauerhafte
  Import-Operationen bleiben im bestehenden SSE-/Core-Vertrag.
- Kein neuer TMDb-Provider, keine zweite Datenbank oder separate Berechtigungsrolle.

## Grenzen und nächste Slices

MT.META7.2 vereinheitlicht den automatischen Film- und Serien-Resolver:
Media Import verwendet `POST /metadata/resolve` auch für Filme,
statt im Browser einen separaten TMDb-Exakttitelvergleich auszuführen.
Die manuelle Such-/Auswahloberfläche bleibt für mehrdeutige oder nicht
aufgelöste Titel vorhanden. Der Resolver ist **keine unscharfe KI-Suche**:
Eine vollständig anders übersetzte Filmbezeichnung ohne passenden
lokalisierten Titel oder `originalTitle` lässt sich daraus nicht
zuverlässig ableiten. Zusätzliche Kandidaten sind kein Grund, willkürlich
eine falsche TMDb-ID zu übernehmen. Serien-ID-Wiederverwendung für ganze
Staffeln (META7.3), vollständiges Artwork (META7.4) und persistente
Import-/Bestands-Übernahme (META7.5) sind separate Slices.
Quellcode ist noch nicht als reales yaVDR-Deployment von META7.2 abgenommen.

Die Samsung- und VIDAA-Repositorys müssen den neuen API-v1-Aufruf selbst
integrieren und auf echten TVs abnehmen; die Server-Schnittstelle ist dafür
nun zentral definiert.
