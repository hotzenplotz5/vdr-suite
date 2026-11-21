# Public-v1 Device Live-TV playback (development candidate)

**Scope:** This is a Device-only Live-TV control plane stacked on Public-v1
generic Recording/EPG search (PR #479). It does not publish the browser
`POST /api/media/sessions` route as a client API. A real Hisense acceptance,
API-contract acceptance and media pipeline verification are still pending.

## Versioned contract

- `POST /api/v1/live-playback-sessions`: verified Device credential,
  `media.live.play@backendId`. Body uses the existing
  `LiveMediaSessionRequestParser` contract: `resourceKind="live-channel"`,
  `backendId`, `channelId` and bounded `capabilities` arrays. The
  Device variant refuses `replacesSessionId` to prohibit cross-Device
  replacement.
- `GET /api/v1/live-playback-sessions/{ms_id}`: Device proof,
  current owner and current persistent grant, reduced read model.
- `POST /api/v1/live-playback-sessions/{ms_id}/stop`: verified Device proof,
  `media.live.play` grant, matching owner/session/backend, explicit
  `operation="stop"` and `resourceKind="live-channel"`.
- Media: `/vdr-suite/api/v1/media/sessions/{ms_id}/live/stream.mp4`.
  The response's short-lived Secure/HttpOnly/SameSite cookie is scoped to
  that versioned media path; no secrets are placed in query params or JS.
  Native provider URLs, FIFO paths, lease IDs and recording identities
  must never be returned to the TV.

`PublicDevicePlaybackRegistry` tracks the Device session owner and resource
kind; its Public-v1 Media-Gateway authorizer rechecks the *persistent* Actor,
Device, Credential and resource-specific grant on every media GET. A live
session needs `media.live.play`, not `media.recording.play`; a Recording
session must retain its existing policy unchanged. Unknown/revoked sessions
fail closed. A denied post-issuance owner registration stops the created
server session rather than orphaning the tuner.

Live video uses the pre-existing `LiveMediaSessionController`,
`BackendAgentLiveProviderRuntime` and Media Gateway
`live-progressive-fmp4` profile. **There is no separate decoder or streamdev
proxy.** The API cannot promise successful tuning, source codecs, transcoding
or TV decoding without runtime evidence. Session replacement/switching is
client STOP-then-CREATE; no same-actor cross-Device replacement shortcut.

## Negative acceptance requirements

Test lack of auth, browser-only credentials, only `channels.view`, only
`media.recording.play`, revoked `media.live.play`, wrong backend,
wrong Device credential, forged session ID, status/stop after revocation,
legacy-path attempt with Device cookie, and no unexpected media secret
or native source ID in responses. A non-responding provider and failed
provisioning must neither expose a playable URL nor leak a live tuner lease.

## Deployment order

1. Both Suite feature PRs pass CI at their exact candidate heads.
2. Build/install the daemon **as unprivileged yavdr** using its own vetted
   installer. Back up configuration, database/migrations and executable;
   verify in-place Public-v1 authentication and media gateway rules.
3. Only after the running backend is verified, use VIDAA's existing static
   trial installer on the matching candidate. It checks current backend
   binary markers, exact Git ancestry, SHA, installed app assets, HTTPS,
   backup, and rollback. It never edits the daemon or TV firmware.
4. On the Hisense: select Live-TV channel, verify real picture/audio,
   STOP/BACK and receiver release; repeat from EPG Now/Next.
   Also test protected search for Hobbit, lower/upper case and paged EPG.

Until steps 1–4 are evidenced, **do not merge for release or provide
an installation-as-success claim**.
