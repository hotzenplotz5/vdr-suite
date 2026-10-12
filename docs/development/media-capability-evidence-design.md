# Media capability evidence and playback negotiation — design draft

Status: design only. No public endpoint, no VIDAA hardware acceptance.

## Existing authority

- Keep `MediaSourceDescriptor` and `FfprobeRecordingSource` as the existing recording metadata path.
- Keep `MediaPresentationSelector` for pass-through -> remux -> transcode ordering.
- Keep `MediaTranscodePolicy` and scoped `MediaSession` / Device grants for execution.
- Never add a second ffprobe consumer to the live-TV socket hot path.
- Do not generate or alter VDR indexes as a probe.

## Capability evidence (future additive API, not implemented)

A client can propose scoped `ClientMediaCapabilities` based on local decoding inspection. Separate engine-specific evidence for native player vs MSE, and label individual codec/container/transport combinations with `verified-playback`, `runtime-reported`, `profile-hint`, `unsupported` or `unknown`. A profile hint alone must never be treated as verified decoding. Keep unknown as unknown.

Potential probes: HTMLMediaElement.canPlayType, MediaSource.isTypeSupported, MediaCapabilities.decodingInfo, platform SDK queries and bounded playback outcome signals. Report actual video and audio readiness independently; a ready HTTP MediaSession is not evidence of successful decode. Optional external device databases or Plex-like profiles are non-authoritative hints, subject to licensing and provenance review.

## Suggested contract boundaries

1. Authenticate the existing Device identity and verify backend-scoped playback grants before accessing recording facts.
2. Accept bounded versioned capability evidence (not arbitrary paths, commands or FFmpeg options).
3. Resolve an authorized Suite Recording ID through existing recording source resolver; fingerprint it; reuse existing ffprobe results subject to resource/timeout limits.
4. Project a sanitized source summary and compatible candidate presentations without native paths, worker details or media credentials.
5. Select with the existing presentation selector and transcode policy; a capability report must not confer authorization or bypass capacity limits.
6. Revalidate grant and source fingerprint when issuing MediaSession and protected media artifacts.
7. When a player reports a classified failure, retry at most once with a strictly compatible, policy-approved alternative; stop on authentication, revocation, missing media, invalid index or unknown-seek failures. No hidden protocol fallback.

## Compatibility rollout

- Slice A (in PR #463): enrich existing bounded recording ffprobe metadata, without changing selection.
- Slice B: exhaustive parser hardening and source fingerprint-based metadata reuse; independent admission and resource bounds.
- Slice C: versioned owner-authorized read-only probe projection with OpenAPI schemas and negative tests.
- Slice D: evidence-aware device capability negotiation and cautious override registry with TTL and firmware/browser version scoping.
- Slice E: explicit classified failure/retry contract, separate web and real-VIDAA hardware acceptance.

### Invariants

Never infer seekability solely from the container or the presence of duration; preserve index-backed seek/status semantics. Never infer HDR output just from HDR input. Never send native filesystem paths or credentials. Never transcode or deploy just because an informational probe endpoint was called.
