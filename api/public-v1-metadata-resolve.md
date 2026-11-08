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

Die Film-Jahresangabe ist optional. Der Resolver verwendet den
vorhandenen `TmdbRecordingMetadataCandidateProvider` mit dessen
konfiguriertem Credential-/Sprachmodell. Er prüft eindeutige
Kandidatenidentitäten, Aliasnamen und bei Serien die genaue
Staffel-/Episodennummer. Abweichende oder mehrdeutige Ergebnisse
werden niemals als Treffer deklariert.

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

Dieser Slice ist ein **deterministischer Resolver für vorhandene,
eindeutig nummerierte Serienepisoden bzw. Titel/Jahr-Filme**. Er ist keine
unscharfe KI-Suche und vergibt keine Identität bei mehreren Treffern.
Weitere nicht-franchisebezogene Titelaliasnamen, internationale Namensvarianten,
Sprach-Fallback, späte Providerverfügbarkeit und Mehrfachquellen-Provenienz
werden nach gesonderter Vertragsprüfung ergänzt.

Die Samsung- und VIDAA-Repositorys müssen den neuen API-v1-Aufruf selbst
integrieren und auf echten TVs abnehmen; die Server-Schnittstelle ist dafür
nun zentral definiert.
