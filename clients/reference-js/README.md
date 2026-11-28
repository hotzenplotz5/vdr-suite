# VDR-Suite Public-v1 JavaScript Reference Client

This directory contains a small reference implementation for the deliberately
stable public-v1 client boundary.

It is **not** a published npm package and it is not the browser Web Client API.
The accepted initial discovery slice covers:

- `GET /api/v1`;
- `GET /api/v1/capabilities`;
- `GET /api/v1/backends`.

The accepted next bounded read extension adds:

- `GET /api/v1/channels?backendId=...` with explicit 1–16 source selection,
  keyset pagination and unchanged partial-source metadata.

The accepted TimerAssignment collection extension adds:

- `GET /api/v1/timer-assignments?backend=...` as a single-backend Suite-owned
  keyset collection with no collection ETag and no native-VDR fallback.

The accepted revisioned item extension adds:

- `GET /api/v1/timer-assignments/{timerAssignmentId}?backend=...` with the
  accepted opaque ETag and `If-None-Match -> 304` contract.

The accepted mutation extension adds:

- `POST /api/v1/timer-assignments/{timerAssignmentId}?backend=...` for the
  accepted Timer CREATE admission. The caller supplies the item ETag and
  Idempotency-Key; the reference client performs one POST and returns the
  accepted Operation representation without retrying or polling automatically.

The final stable-contract read extension adds:

- `GET /api/v1/operations/{operationId}` with the accepted opaque ETag and
  `If-None-Match -> 304` contract. It is an explicit one-shot read; the
  reference client does not add a polling loop.

The caller supplies the Suite origin, transport and any authentication headers
or credentials. The reference client does not invent login/session behavior,
does not know browser-session/cache/provider routes, and performs no automatic
retry or route fallback.

The implementation exists to make the public contract executable for future TV,
mobile, desktop, Kodi and automation clients without pretending that the
bundled browser's richer pre-v1 routes are equivalent.

## P1 Device recording progress — source candidate

The client also offers `getContinueWatching({backendId, headers})`: a bounded list of public rec_ IDs, confirmed positions and nullable durations. Both GET variants require an authenticated Device with the exact play grant.

A currently verified Device with `media.recording.play@backendId` may use
`getRecordingProgress({backendId, recordingId, headers})` and
`updateRecordingProgress({backendId, recordingId, operationId,
positionSeconds, expectedRevision, sessionId, headers})`. Use
`clearRecordingProgress({backendId, recordingId, operationId,
expectedRevision, headers})` to commit a zero-position reset.

The recording ID is an opaque public `rec_` identity; it is never a VDR path.
Read `revision` before the first write; send its current value as
`expectedRevision` on subsequent checkpoints, pause and stop. On 409, re-read
and resolve the conflict from the client's observed playback, **not** by
blind retry. Persist only measured playback positions, not elapsed session time. Nonzero progress must reference an active, exact-recording, Device-owned `sessionId`; zero reset may omit it.
Resume playback by explicitly creating the existing Device Recording
MediaSession with `startPositionSeconds` *only if the chosen profile actually
supports it*. Session CREATE is not automatically invoked by these helpers.

Device pairing does not bind its Service Actor to a Human Account. Consequently
this initial projection is **Device-actor-private**, not cross-account or
cross-Device shared history; it cannot read a Browser Human Account's saved
Continue Watching position. Device credentials are provided by the caller,
never synthesized by the library. The browser uses its original CSRF-protected
legacy route unchanged; the new route does not accept a browser session.


### Explicit Playback MediaSession resume

\`createRecordingPlaybackSession({backendId, recordingId, capabilities,
startPositionSeconds, headers})\` uses the existing Public-v1 Device Recording
Session CREATE operation. The caller must supply *actual measured* codec,
container and protocol capabilities. A positive \`startPositionSeconds\` is
optional and is only attempted after the user explicitly requests resume;
the server may reject unsuitable profiles with 409 or out-of-range positions
with 422. Only index-ready HLS profiles currently provide positive start-at.
Read the returned \`mediaSession.playback.resume\` before claiming resume worked.
Use \`getRecordingPlaybackSession({sessionId,headers})\` and
\`stopRecordingPlaybackSession({sessionId,backendId,headers})\`; the latter
uses the current owned-device STOP contract. Media grant delivery remains
the existing guarded gateway, never private /api/media routes.

### API.V1.F7.2 – Device player-profile readiness

`getRecordingPlaybackCapabilities({appVersion,firmwareVersion,playerVersion,
engine,headers})` sends the existing authenticated, read-only
`GET /api/v1/recording-playback-sessions/capabilities` with the four
explicit profile fields (`engine` is `mse`, `native` or `external`).
The server binds the Device identity to the credential, not a client-supplied
query value. A response describes recorded server-side profile evidence;
it is **not** proof of physical decoder playback and does not start a
MediaSession, set a profile or automatically change the selected player.
A client may explicitly opt in to the matching `capabilityProfile` for
a subsequent CREATE only when appropriate. No retry or polling is added.

### API.V1.F7.1 – Native player transport and audio tracks

The existing `createRecordingPlaybackSession` helper now forwards optional
`nativeMediaAccess: true` and `audioTrackId: 'audio-2'` to the already
implemented Device MediaSession CREATE. Neither field is sent by default.
`nativeMediaAccess` requests the same existing session-scoped media bearer
for a native TV player whose HTTP requests cannot share the browser's
HttpOnly cookie jar; it does not create a new login or media endpoint.
**Treat the returned `nativeMediaAccess.cookie` and `.headerValue` as
secrets:** use them only in the active player transport, never in a URL,
log, storage or error text. STOP/revocation invalidates the session grant.

`getRecordingPlaybackTracks({sessionId,headers})` reads the existing
owned-session Public-v1 `/tracks` projection, not a native Recording ID.
Audio switching remains an explicit caller-triggered replacement of the
Recording MediaSession, using a valid track ID from the current inventory.
No automatic Session CREATE, retry, polling or client-side player is added.
Source/CI coverage is not a real TV playback or audio-switch acceptance.

### Admin-assigned playback ownership

The reference client offers `getDevicePlaybackAccount({deviceId,headers})`
and `setDevicePlaybackAccount({deviceId,accountId,expectedRevision,headers})`.
These are strictly **Browser-administrator** operations. Supply the Browser
Session and CSRF header through the existing credentials and header settings;
never send a Device Credential to create a Human-Account link. An empty
`accountId` explicitly unlinks the device with an incremented revision.
A paired Device without an explicit link continues under its separate Service
actor. The server never trusts account IDs supplied by a Device while reading
or updating recording progress.

The bundled browser Continue Watching implementation remains on its
CSRF-protected established endpoint; it does not call private routes from
Public-v1 reference clients. It now uses a revision handshake at playback
owner creation and versioned checkpoints. HTTP 409 intentionally blocks an
older player from replaying a stale position.

## API.V1.F2 – caller-owned Authentifizierungs-Transporte

Der vorhandene `createClient({baseUrl, fetch, headers, credentials})`
übernimmt die Authentifizierung des Aufrufers; es wird kein
zweiter Login-, Cache- oder Session-Owner eingeführt.

- **Browser:** Default-Cookie `vdr_suite_session` via `credentials:'include'`;
  für geschützte Browser-Mutationen außerdem `X-CSRF-Token`.
- **Device:** `Authorization: VDR-Suite-Device <id>.<secret>`;
  ausdrücklich kein Bearer-Token und kein Browser-Cookie.
- **Pairing:** `X-VDR-Suite-Pairing-Token` für den begrenzten
  Bootstrap-Status und die einmalige Credential-Ausgabe.
- **Managed Basic:** nur wenn der Server es bereits entsprechend
  konfiguriert hat und SecurityHttpGate es an dieser Route zulässt.

Keine dieser Varianten erzeugt automatisch Grants. Die Bibliothek
führt weder spekulative Mutations-Retries noch Legacy-Route-Fallbacks aus.
Siehe [F2-Nachweis](../../docs/development/api-v1-f2-security-parity.md).

## API.V1.F3 – Read-only Discovery, Channel, EPG und Suche

Der Reference-JS-Client ergänzt die bereits akzeptierten `getBackends`
und `getChannels` um fünf reine Public-v1-Reads:

```js
const client = VdrSuitePublicV1Client.createClient({
  baseUrl: 'https://suite.example',
  headers: {Authorization: 'VDR-Suite-Device <credentialId>.<secret>'}
});
const backendId = 'default';
const channels = await client.getChannels({
  query: {backendIds: [backendId], sort: 'backendId,channelNumber,channelId'}
});
const neighbor = await client.getAdjacentChannel({
  query: {backendId, relativeTo: 'S19.2E-1-1019-10301', direction: 'up'}
});
const events = await client.getEpgWindow({
  query: {backendId, channelId: 'S19.2E-1-1019-10301',
    fromTime: '1763733600', untilTime: '1763906400', limit: 100}
});
const nowNext = await client.getEpgNowNext({
  query: {backendId, channelId: 'S19.2E-1-1019-10301',
    fromTime: '1763733600'}
});
const epgMatches = await client.searchEpg({
  query: {backendId, query: 'Tatort München', limit: 30, offset: 0}
});
const recordings = await client.searchRecordings({
  query: {backendId, query: 'Café & Urlaub', limit: 30, offset: 0}
});
```

Die genannten Unix-Zeiten sind Beispielwerte. Der Client erzeugt **keinen**
Login und keine zusätzlichen Grants. Ein EPG-Fenster wird seitenweise
weitergelesen, indem `events.page.nextCursor` unverändert und zusammen
mit **identischem** Backend, Sender und Zeitfenster erneut übergeben wird.
EPG-/Recording-Suche verwenden explizit eine Quelle und `limit/offset`;
die Aufnahmen liefern opake `rec_`-IDs. Ein `503` oder `409`
wird als typisierter Fehler weitergegeben, nicht durch private Routen
umgangen. Der anonyme Pairing-Prozess bleibt davon unberührt.

Die bestehende Web-API ist **nicht** automatisch kompatibel mit diesen
Rückgabeformen. Migrationsgrenzen und offene Web-vs-v1-Abnahme:
[F3-Paritätsbericht](../../docs/development/api-v1-f3-discovery-epg-search-parity.md).

## API.V1.F4 — Recording-Browse, Genre und Metadaten

Auf dem bestehenden Public-v1-Referenzclient sind zusätzlich zu
`getRecordings`, `searchRecordings`, `getRecordingArtwork`,
`getRecordingArtworkImage` und `resolveMetadata` diese
**lesenden** Methoden verfügbar:

```js
const recordingId = 'rec_0123456789abcdef0123456789abcdef';
const client = VdrSuitePublicV1Client.createClient({
  baseUrl: 'https://suite.example',
  headers: {Authorization: 'VDR-Suite-Device <id>.<secret>'}
});
const root = await client.getRecordingFolders({
  query: {backendId: 'home-vdr', limit: 50, offset: 0}
});
const directory = await client.getRecordingFolders({
  query: {backendId: 'home-vdr', folderId: 'fld1_0123456789abcdef0123456789abcdef',
          limit: 50, offset: 0}
});
const details = await client.getRecordingDetail({backendId: 'home-vdr', recordingId});
const genres = await client.getRecordingGenres({
  query: {backendId: 'home-vdr', locale: 'de', limit: 30, offset: 0}
});
const genreRecords = await client.getGenreRecordings({
  query: {backendId: 'home-vdr', genreId: 'fantasy', locale: 'de',
          limit: 30, offset: 0}
});
```

Ordner und Genres verwenden `offset/totalCount`-Paging,
nicht den flachen Recording-`rc1_`-Cursor. IDs bleiben
opak, Artwork-Missing liefert entweder eine leere
Referenzliste oder einen typisierten 404 für ein konkretes Bild;
keine synthetischen Provider-URLs oder 404-Fallbacks.
Metadatenauflösung via `resolveMetadata` bleibt ein
unverbindlicher Kandidaten-Read (technisch POST). Kein
Playback-, Rename-, Move- oder Delete-Helper wird durch
diese neuen Methoden ausgelöst.

**Wichtig:** Die browserprivaten Folder-/Home-/Genre-/Search-DTOs
sind **nicht** automatisch 1:1 gleichwertig. Die
[Web-vs-Public-v1 F4-Matrix](../../docs/development/api-v1-f4-recording-metadata-artwork-parity.md)
dokumentiert die bekannten Grenzen vor einer Web-Migration.

## API.V1.F5.2 – Native Timer modifications and delete (explicit)

The existing Public-v1 endpoints are available through
`modifyNativeTimer({timerId,backendId,ifMatch,idempotencyKey,payload,headers})`
for `POST /api/v1/native-timers/{timerId}/modifications`, and
`deleteNativeTimer({timerId,backendId,ifMatch,idempotencyKey,headers})`
for `DELETE /api/v1/native-timers/{timerId}`.
The mutation payload is caller-owned: `{kind:'toggle',enabled:false|true}`
or the server-defined complete `kind:'update'` specification.
Both methods forward the original strong ETag from a verified managed/adopted
Timer and one logical operation's idempotency key. The server controls
admission, revision, ownership, device/browser permissions and readback.
A Browser supplies its existing CSRF token through `headers`.
The helper returns `{status,data,location}` for the async Operation;
`202` is **not** a confirmed native Timer change. No implicit retry,
automatic Operation polling, timer creation or legacy fallback is added.
This is a source mock contract, not a new real-Timer mutation test.

## API.V1.F5.1 – Native Timer snapshot and conditional detail

`getNativeTimers({query:{backendId,offset,limit},headers})` reads the
existing VDR-owned `GET /api/v1/native-timers` snapshot with
offset pagination (maximum `limit:30`). `getNativeTimer({timerId,backendId,
ifNoneMatch,headers})` reads an exact native Timer item and returns
`{status,etag,data}`; a verified bound item may yield HTTP 304.
An unbound item has no guaranteed ETag. Both operations are read-only.

These are **not** `TimerAssignments` or `TimerIntents` and cannot
replace the existing browser's live Timer editor or arbitrary CREATE.
No native Timer modification or delete helper is introduced in F5.1.
The previously proved yaVDR Timer write lifecycle is not repeated.

## API.V1.F5.4 — Native Timer read fields

`getNativeTimers` and `getNativeTimer` return four additional
snapshot-owned values: `directory` (string), `priority` and
`lifetime` (integers) and `vps` (boolean). The detail ETag
still represents the existing binding-verified native Timer
configuration. Opaque provider `aux` is not exposed because it
is not covered by that cross-backend fingerprint.
This is additive read parity, **not** a migration of the
browser's legacy full Timer CRUD editor.

## API.V1.F6 — Native Marks/Cut source candidate

The reference client exposes `getRecordingMarks`, `getRecordingCutState`,
`submitRecordingMarksMutation` and `submitRecordingCut` against the
public `/api/v1/recordings/{recordingId}/marks|cut?backendId=...` routes.
Writes need a caller-owned `payload` whose backend/Recording IDs match the
URL, native `operationId`, `operationRevision`, and 32-hex
`expectedMarksRevision`. The existing browser CSRF proof is caller-owned.
No retry after an uncertain POST. `recordings.edit@backendId` grants native
marks/cut for Human Accounts, but Device grant support and complete
Browser/Device parity are not hardware-accepted. Existing private Web routes
remain usable. See [F6 source contract](../../docs/development/api-v1-f6-recording-marks-cut-scope.md).
