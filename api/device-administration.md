# Public-v1 Device pairing approval and grant administration

## Navigation

- [API developer entry](README.md)
- [OpenAPI 3.1 partial contract](openapi.json)
- [TV pairing bootstrap and one-time credential](device-pairing.md)
- [Human Account grant boundary](accounts-and-grants.md)
- [MU.10B accepted administrator pairing approval](../docs/development/post-phase69-mu10b-admin-pairing-approval.md)
- [MU.10E1 Device grant contract](../docs/development/post-phase69-mu10e1-device-grant-administration.md)
- [Device/Credential revoke and rotation](device-lifecycle.md)

## Source baseline and authority

Cross-checked against `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11). These are administrative Public-v1 HTTP resources already present in code. Accepted historical notes show MU.10B real yaVDR approval acceptance, but Device-grant documentation from MU.10E1 itself has an earlier candidate snapshot; current merged code existence does **not** establish this documentation audit's own live acceptance.

The approving Human Account is **not** the Device's identity. After pairing credential issuance, the Device owns a dedicated Service Actor with **zero default grants**. The administrator must explicitly grant resource permissions to that Service Actor through the canonical grant authority.

## 1. Pending Pairing Request administration

```http
GET /api/v1/device-pairings?limit=50&sort=pairingRequestId&order=asc
```

Requires `device.pairing.view@*`. Only pending, unexpired requests appear. The list is paginated with an opaque `dp1_` cursor and has no collection ETag. It includes secret-free metadata and `decision` fields, but **never** returns `userCode` or `pairingToken`.

```http
GET /api/v1/device-pairings/{pairingRequestId}
```

Without `X-VDR-Suite-Pairing-Token`, this is the **admin item** route; it requires `device.pairing.view@*` and returns a strong opaque `ETag` reflecting the Pairing Request revision, with a secret-free `decision` object (`decidedByActorId`, `decidedAt`, null before decision).

**Important:** The same URL **with** `X-VDR-Suite-Pairing-Token` is a separate TV bootstrap read. It returns only `DevicePairingStatus`, not an admin ETag, even if an administrator happens to have a browser session. Never mix the two credential modes.

## 2. Approve or reject a pending request

```http
POST /api/v1/device-pairings/{pairingRequestId}
Content-Type: application/json
If-Match: <opaque strong ETag from admin item GET>

{"decision":"approve"}
```

Allowed `decision` values are `approve` and `reject` only. Requires `device.pairing.decide@*` and protected-mutation Browser CSRF where applicable. Max body 1024 bytes, no query or Idempotency-Key. Stale revision: `412 revision_conflict`; already decided: `409 operation_conflict`; expired: `410 pairing_expired`; missing precondition: `428 precondition_required`.

A success returns HTTP 200 with updated administrative Pairing Request metadata and a new ETag. **Approval creates no Actor, Device, Credential or grant**. The waiting TV still must separately issue its credential using its own short-lived token.

## 3. List paired Service Devices (new administrator read)

```http
GET /api/v1/devices?limit=25
```

Requires the global administrator grant `devices.grants.view@*`. This
secret-free collection lists **persistent Service Devices** from
`security_devices`, not Human Account Browser sessions and not the
pending-only `device-pairings` endpoint. No Credential ID, bearer token,
pairing token or session information is exposed.

The response contains each real `deviceId`, `actorId`, `displayName`,
`active` and `revoked`, sorted by `deviceId`. `page.hasMore` and
`page.nextCursor` implement bounded keyset pagination (1–100 per page);
the cursor is a versioned `dv1_` marker. A malformed or duplicate query
parameter is rejected. The route uses `Cache-Control: no-store` and
returns 503 rather than falsely advertising an empty inventory on DB
failure. Revoked devices remain visible, but the existing grant mutation
path prevents authorization through revoked identities.

The frontend's **Settings → Gekoppelte Geräte (TV & Apps)** lists those
devices separately from Human Accounts. Selecting an active device reads
its existing `GET /api/v1/devices/{deviceId}/grants` strong ETag.
Grant and revoke remain explicit `POST /api/v1/devices/{deviceId}/grants`
operations with Browser CSRF, `If-Match`, actual backend selection and
user confirmation; a 412 forces a read-only refresh, never an automatic
mutation retry. `timers.create` and `searchtimers.create` cannot be
granted to `*`.

**Webfrontend-Aktualisierung (PR #539):** Unter `Einstellungen` werden
`Wartende Geräte` und `Gekoppelte Geräte (TV & Apps)` beim Öffnen
und danach alle zehn Sekunden automatisch lesend abgefragt, solange
die Seite sichtbar ist. Unveränderte Ergebnisse behalten Auswahl und
Berechtigungseditor. Beim Verlassen der Einstellungen endet der
Polling-Lebenszyklus; in ausgeblendeten Browsertabs gibt es keine
Netzwerkabfragen. Bei manuell aufgeblätterten Listen mit mehr als einer
Seite bleibt die erweiterte Liste erhalten, statt im Hintergrund durch
die erste Seite ersetzt zu werden. Die Schaltfläche
`Anfragen aktualisieren` bleibt als manueller Fallback vorhanden.
**Es gibt keinerlei automatische Genehmigung, Freigabe oder Änderung von
Berechtigungen.** Die Browser-Session und vorhandene CSRF-/ETag-Prüfung
bleiben die einzigen Berechtigungswege.

Web UI minimum: account administration must remain unchanged, and the
device admin must never infer a VIDAA Device from a Human session. This
matches the basic product expectation of separately manageable paired TV
clients (Kodi/Plex offer managed client/user authorization flows; their
exact menus and per-backend grant semantics have not been verified here).

## Samsung pairing, duplicate identities and polling — observed on real TV (2026-10-25)

The Samsung Tizen 5.5 client completed pairing, one-time credential activation and a real authenticated Channel read. A VS Code Tizen `Run Project` reinstall/update caused the client to report **“Kein gespeichertes Device-Credential”** on its next test. The older Service Device remained active in the server inventory, but its former client-held secret was no longer available to that installation. A newly approved and activated pairing resulted in **a second** `Samsung TV · VDR-Suite` entry; the two display names are not unique IDs, and the older Device is **not** automatically reactivated or reused.

The new Device initially lacked the required `channels.view` grant, as proved by the TV's empty accessible-Backend state. The administrator then granted `channels.view@default` to the **new** Device and the client loaded 20 real channels. Grant state for the old Device does not automatically migrate. **Do not infer Device-specific authorization from identical display names or from an apparently selected row in the browser UI.** Always use the canonical Device identity and its own grant-set read/revision.

**Ten-second admin polling:** PR #539 implements read-only refresh while Settings remains visible. In this session, the new Device eventually appeared, but the precise refresh interval was **not measured**; seeing the entry later is not proof of a ten-second SLA. Verify visibility/tab lifecycle, preservation of the selected Device, and per-Device grant-editor identity in focused UI tests before calling that part hardware-accepted.

**Device-Aufräumfunktion (Frontend-Kandidat, Hardware noch offen):** In `Einstellungen → Gekoppelte Geräte (TV & Apps)` kennzeichnet die Ansicht gleichnamige Geräte durch ihre **Device-ID**. `Gerät entfernen` widerruft ausschließlich die exakt ausgewählte Device-Identität durch den bestehenden `GET/POST /api/v1/devices/{deviceId}/lifecycle`-Vertrag mit frischem Lifecycle-ETag, ausdrücklicher Browserbestätigung, Admin-Session und CSRF. Das ist **kein physisches Löschen**: Der Server behält historische Device-Datensätze für Nachvollziehbarkeit; widerrufene Einträge werden standardmäßig ausgeblendet und können über `Widerrufene anzeigen` wieder eingeblendet werden. Der Widerruf macht **alle Credentials dieses Geräts** ungültig, kann nicht rückgängig gemacht werden und sollte erst nach Abgleich der aktuellen TV-Device-ID ausgeführt werden. Gleiche Anzeigenamen sind kein Beweis für ein altes Gerät. Es gibt weder Sammellöschung noch automatische Widerrufe oder Grant-Übertragung. Source-/CI-Evidenz und realer Browser-/TV-Abnahmestatus bleiben getrennt.

For acceptance status and cross-client overlap, use the [single cumulative hardware matrix](../docs/development/public-v1-final-hardware-acceptance.md). A Samsung Tizen pairing test says nothing about independent VIDAA playback/secure storage acceptance.

## 3. Read a Device's grants

```http
GET /api/v1/devices/{deviceId}/grants
```

Requires global `devices.grants.view@*`. The returned representation contains `deviceId`, the Device's dedicated Service `actorId`, normalized `items` tuples, and `supportedPermissions`. A strong opaque Device grant-set ETag is returned. `If-None-Match` may yield bodyless 304. The grant-set ETag is **not** the Device, Credential or Human Account revision.

The supported Device grants are the following reviewed permissions:

- `channels.view`
- `recordings.view` (read-only Public-v1 recording search)
- `epg.view` (read-only Public-v1 EPG search and Now/Next)
- `timers.view`
- `timers.create` (EPG-selected native Timer intent)
- `timers.modify` (modify/toggle one authorized native Timer)
- `timers.delete` (delete one authorized native Timer)
- `searchtimers.view` (SearchTimer list and preview, independently of EPG read)
- `searchtimers.create` (inactive SearchTimer CREATE; RESTfulAPI one-shot, not epgsearch-native atomic CAS)
- `searchtimers.modify` (native generation-fenced SearchTimer update/deactivation)
- `searchtimers.execute` (explicit SearchTimer activation)
- `searchtimers.delete` (native generation-fenced SearchTimer deletion)
- `media.live.play`
- `media.recording.play`

No `role.admin`, `role.read-only`, `accounts.*`, `remote.control` or arbitrary strings are allowed here. Grant `backendId` is an explicit product Backend identity or the configured wildcard `*`, **except** `timers.create`, `timers.modify`, `timers.delete`, `searchtimers.create`, `searchtimers.modify`, `searchtimers.execute` and `searchtimers.delete`, which require an exact concrete backend. Grant availability is not proof of currently connected backend capability or real-hardware acceptance.

## 4. Ensure or revoke one grant

```http
POST /api/v1/devices/{deviceId}/grants
Content-Type: application/json
If-Match: <strong Device grant-set ETag>

{"permission":"channels.view","backendId":"backend-one","active":true}
```

Requires global `devices.grants.modify@*` and Browser CSRF where applicable. The body is closed (`permission`, `backendId`, `active`) and limited to 4096 bytes; no query or Idempotency-Key. `active:true` ensures the tuple exists; `active:false` revokes it. Successful mutations return HTTP 200 with the **current entire Device grant set** and ETag.

A stale ETag generally yields `412 revision_conflict` unless the tuple is already in the requested final state (desired-state idempotence). Do not retry by switching to Human Account `/grants` or using a new unrelated administrator target.

A newly issued Device Credential can subsequently authenticate with `Authorization: VDR-Suite-Device <credentialId>.<credentialSecret>`. Server-side authorization still evaluates its grants on each request.

The native EPG Timer CREATE route (`POST /api/v1/epg/timer-intents`) also accepts a verified Human administrator Browser Session with a matching `role.admin` grant and a valid CSRF token. Ordinary Human users with only `timers.create` are not accepted on this route. Device clients still require a verified Device Credential and the exact `timers.create@<backendId>` grant. This is an explicit administrator capability, not Device impersonation.

## Source and regression anchors

- [Public REST pairing/Device grant runtime](rest/src/PublicApiRuntime.cpp)
- [Security admin and Device authorization](../core/security/include/SecurityHttpGate.h)
- [Pairing HTTP tests](rest/tests/test_public_device_pairing.cpp)
- [Device grant HTTP tests](rest/tests/test_public_device_grants.cpp)
- [Device grant security tests](../core/security/tests/test_public_device_grants_security.cpp)
- [Reference client Device grants tests](../clients/reference-js/tests/test_public_v1_device_grants_client.js)

This documentation does **not** add the Device revoke/rotation, pairing administrative UI or real VIDAA runtime test to the OpenAPI subset. Those are independently gated contracts.

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)

### SearchTimer grant and mutation

An administrator may explicitly grant `searchtimers.view` and
`searchtimers.create` **independently per backend**. The latter permits
one confirmed `POST /api/v1/search-timers?backend=<id>` through the
durable, idempotent SearchTimer one-shot operation. This is distinct
from Suite TimerAssignments and native VDR Timer CREATE. Browser
sessions and legacy paths never inherit paired Device CREATE rights.
A 202 response is an Operation, not guaranteed backend recording.

## Webfrontend: wartende Gerätekopplungen

Unter **Einstellungen → Wartende Geräte** liest die Human-Admin-Sitzung die
ausstehenden, nicht abgelaufenen Anfragen über
`GET /api/v1/device-pairings?limit=25&sort=pairingRequestId&order=asc`.
Die Anzeige von vorhandenen Geräten bleibt unabhängig davon verfügbar.
Mit **Anfragen aktualisieren** wird erneut gelesen; bei mehreren Seiten erfolgt
die Navigation ausschließlich mit dem vom Server ausgegebenen `dp1_`-Cursor.

Bei **Genehmigen** oder **Ablehnen** liest der Browser das betreffende
Administrator-Item **erneut**, prüft dessen unveränderte Metadaten,
verlangt die ausdrückliche Bestätigung und entscheidet per `POST` mit
`If-Match`, vorhandener Human-Admin-Browsersitzung und CSRF-Header.
Eine 409/410/412/428 löst **keinen** automatischen Schreib-Retry aus.
Die Liste enthält absichtlich **weder Kopplungscode noch Pairing-Token**;
nur selbst angestoßene Geräte anhand Gerätename/Typ/Ablaufzeit genehmigen.

Eine Genehmigung stellt **noch keine Geräteidentität und keine Rechte** aus.
Der TV muss anschließend `Status prüfen` und `Gerät aktivieren` ausführen;
danach sind Device-Grants getrennt zu vergeben. Kein VDR-/Backend-Agent-Neustart
ist Bestandteil des Kopplungsvorgangs.

Status: Webfrontend-Kandidat implementiert; Deployment und realer Browser-/TV-Test
sind separat zu bestätigen.
