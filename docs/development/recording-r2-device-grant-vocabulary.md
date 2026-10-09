# Recording R2 – device read grant vocabulary correction

Status: code candidate only; build/CI, controlled runtime deployment, admin grant mutation and real TV collection acceptance remain pending.

The public-v1 GET /api/v1/recordings?backendId=default requires recordings.view@default. On 2026-10-09 the paired Hisense VIDAA TV was active, but had neither an active backend-scoped permission grant nor recordings.view, while vdr_recording_cache contained 1033 ready entries for default. The R2 TV displayed "Keine Einträge vorhanden".

MU.10E1's original DeviceGrantAdministrationService whitelist excluded recordings.view. HumanAccountGrantAdministrationService::supportedPermissionSet also excluded it, so the canonical admin grant endpoint could not issue the read grant. The API's supportedPermissions list likewise omitted it. These three surfaces now recognize recordings.view; scope validation remains bounded, admin access still requires a Browser Session, CSRF and strong If-Match, and grant changes are transactional and audited. No arbitrary device authority, admin role or recording mutation permission is introduced.

Tests check the Human Account read-grant vocabulary and backend scope, Device Service Actor allowlist, persistence on a paired authenticated Device, revoke, and supportedPermissions advertisement. Historic MU.10E1 documentation still describes the original allowlist correctly. This extension belongs to Recording R2.

Never repair this by manually inserting security_actor_permission_grants rows: that would bypass the administered revision/audit flow. The next step is a new server build and sealed staging of the exact patched commit, tests and controlled install; then grant ONLY recordings.view@default to the existing Hisense through POST /api/v1/devices/{deviceId}/grants using an authorized admin Browser Session, anti-CSRF, and a fresh ETag from GET. A TV screenshot without recordings does not constitute accepted read behavior.
