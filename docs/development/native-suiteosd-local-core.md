# Native SuiteOSD: local Core access

The native VDR plugin already runs in the VDR service process. Opening Home,
search or cast search must never prompt for a browser username/password.

This optional local HTTP transport reuses the existing Core router, search
index, account and grant repositories, and playback history. It is not a
network endpoint, not a browser session and not a Device credential. The
same listener thread handles the existing TCP listener and a filesystem
AF_UNIX socket. Only the configured Unix service UID is admitted, verified
with SO_PEERCRED; HTTP headers cannot establish this identity. The socket
directory must be owned by the daemon and not writable by other users.
The daemon binds the native transport to one explicitly selected existing
active Human Account; permissions are resolved from the existing grants on
each request. No database schema or grant vocabulary changes.

Deployment: configure the daemon service once with
VDR_SUITE_LOCAL_OSD_USER (actual VDR Unix service user) and
VDR_SUITE_LOCAL_OSD_ACCOUNT_ID (existing account ID). The socket defaults to
/run/vdr-suite/osd.sock; VDR_SUITE_LOCAL_OSD_SOCKET can override it on both
services. The directory is provided by the service deployment (e.g.
RuntimeDirectory=vdr-suite for a root-owned daemon). No user password is
stored or entered on the TV. The native interface serves the existing
search/recording/discovery/artwork read paths and list operations of the
existing playback history endpoints. It does not expose administrative
mutations. The original TCP interface and its browser/device credentials
are unchanged. Missing local configuration leaves existing TCP operation
unchanged; the OSD retains the native VDR library fallback.

Acceptance: real socket vs. TCP spoofed header, exact local UID, existing
actor and grants, default vs. ungranted backend, account lifecycle,
same search counts/order, recording paths and Core presentation, and same
account history. Native OSD test verifies AF_UNIX HTTP without Browser
credentials. Full end-to-end and hardware acceptance are still required.

The production HTTP server regression additionally creates an isolated existing-account fixture, compares the same recording query with the existing HTTP path, verifies denied backend scope, reaches the existing history dispatcher without a browser credential, rejects out-of-scope operations and checks account deactivation. It does not establish real-yaVDR acceptance.
