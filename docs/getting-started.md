# VDR-Suite: Getting started

## Who is this for?

This page is for someone who has just found VDR-Suite and wants to understand what they can actually try. VDR-Suite **complements a running Video Disk Recorder (VDR)**; it does not install tuners, replace VDR, or turn a plain web browser into an independent TV server.

- **I already have VDR-Suite deployed:** follow the read-only checks below, then open the Web frontend.
- **I want to compile or contribute:** follow the [isolated developer quickstart](development/quickstart.md).
- **I want to install VDR-Suite via an Ubuntu test PPA or have just installed it:** start with [Erste Schritte nach der Installation (Deutsch)](erste-schritte-nach-installation.md), then check the [packaging status](../packaging/README.md) and [deployment contract](development/install-runtime-deployment-reliability.md). The PPA is experimental and not a generally supported in-place migration for an existing VDR/yaVDR. Do not copy individual frontend files into a running system.

## What VDR-Suite adds

VDR retains control of native recordings, tuners, timers and plugin execution. VDR-Suite adds a controlled server/API and a modern Web interface for channel guide, browsing recordings, metadata, playback, and protected actions. The [feature overview](project-status-dashboard.md) separates established components from separately gated TV/Device work.

The [separate Hisense/VIDAA client](https://github.com/hotzenplotz5/vdr-suite-vidaa) is a distinct application and repository; don't confuse its merged source candidates with successful installation or playback on your own television.

## First contact with an **existing** installation

1. Find the HTTPS URL and proxy prefix chosen by the administrator of that particular server. No universal host, port, or password is assumed.
2. Open the existing VDR-Suite Web address in a browser. The actual path may be `/vdr-suite/` or a deployment-specific prefix.
3. If the deployment exposes the Public-v1 discovery API, inspect it without a write operation:

   ```bash
   SUITE_BASE='https://YOUR-SERVER/YOUR-OPTIONAL-PREFIX'
   curl --fail-with-body -sS "${SUITE_BASE}/api/v1"
   curl --fail-with-body -sS "${SUITE_BASE}/api/v1/capabilities"
   ```

   Substitute the true prefix immediately **before** `/api/v1`, or use the bare HTTPS origin if your proxy exposes it at the root. These are source-documented *discovery* requests; a deployed reverse proxy may still require authentication. Discovery does not establish that media playback works or grant permissions.
4. In the interface, browse Channels/EPG and Recordings exposed by your configured backend. Signing in, backend enrollment, grants and Device pairing depend on your installed setup; no default administrator credentials are published by the project.
5. Treat a playable Recording or channel as a *separate* runtime acceptance question. Device capabilities, codec choice and indexed recording duration can vary by VDR source and client.

**Don't** run package installation, modify `/etc`, change service accounts, or initialize the production database just to explore the documentation.

## If you do not yet have a server installation

At present there are two different paths:

| Goal | Start with |
| --- | --- |
| Understand what the software does | [Product overview](project-overview.md), [feature dashboard](project-status-dashboard.md) |
| Build and run non-invasive tests on a disposable machine | [Developer quickstart](development/quickstart.md) |
| Follow the Ubuntu test-PPA and first-admin setup path | **[Erste Schritte nach der Installation (Deutsch)](erste-schritte-nach-installation.md)** and [experimental PPA](https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite) |
| Plan a controlled installation on a real VDR/yaVDR system | [Packaging status](../packaging/README.md), [staged deployment and verification](development/install-runtime-deployment-reliability.md) |
| Connect an external application | [Public-v1 API introduction](../api/README.md), [partial OpenAPI 3.1](../api/openapi.json) |
| Develop on Hisense VIDAA | [Dedicated VIDAA repository](https://github.com/hotzenplotz5/vdr-suite-vidaa) |

The staging deployment path intentionally requires an inspected checkout, sufficient space, backups, runtime configuration/permissions and an explicit install acceptance process. It is **not** a casual first-run demo. Do not interpret a successful GitHub build as proof that the binary on your VDR has been updated.

## What is the current state?

Read [Current State](CURRENT.md) for bounded repository status, [the feature dashboard](project-status-dashboard.md) for a newcomer-oriented overview, and [the documentation home](index.md) for topic navigation. The canonical status and actual installed state are not interchangeable.

## Back

- [Project README](../README.md)
- [Documentation Home](index.md)
