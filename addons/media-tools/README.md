# Media Tools add-on

**Neue Folgeroadmap (24.10.2026):** [Media Tools — Installation, Import und Aufnahmeaktionen](ROADMAP.md). Zuerst eine einfache, konfigurierbare und für spätere Ubuntu/Debian-Pakete gemeinsame Installations-/Staging-Routine (yaVDR als Referenz; openSUSE/MLD ohne erfundene Defaults), dann Rectools-einfacher rekursiver Standardscan, optionaler Explorer, Shrink in den Aufnahmeaktionen und Media Import direkt nach Aufnahmen. **Plan, noch keine Implementierung/Live-Abnahme.**

This is the canonical location for **new Media Tools-specific A6 code**.
All development stays on `work/addon-platform-registry-lifecycle`; there
is no new branch, fork or repository.

The existing C++17 importer is maintained here under `include/`, `src/`
and `tests/` (moved, not copied, from the legacy source layout).
Manifest and metadata-only packaging remain under `modules/rectools/`.

Media-Tools-specific Core authorization, reviews and maintenance adapters live at
`core/addons/media-tools/{include,src,tests}`; generic add-on platform services
remain at `core/addons/{include,src,tests}`. Neither directory is the optional
addon worker or a separately packaged module.

Suite Core owns accounts, grants, command reservation, durable operations,
backend generation, capability discovery and recording reconciliation.
Media Tools owns only import, strictly read-only check and non-destructive
HEVC shrink. All three have source-level components. The HEVC Agent, Public
API v1 and Recording Hero are wired in source, but complete local integration,
installation and real yaVDR acceptance remain unverified. No repair functionality
is included or permitted (see `modules/rectools/AGENTS.md`).

The add-on includes the pinned descriptor executor and protected Core/Agent
command/results path. The production Agent executable has a **strictly explicit**
`--enable-media-import` opt-in composition; it still requires a root-controlled
approval, metadata, source inventory and Core authorization. Merely installing
an add-on or setting COMMAND_TYPES cannot activate imports. No service has been
installed or restarted as part of this implementation, and the module manifest
remains disabled.

## MT.I0/MT.I1 — belegte Installationsbasis

Die [MT.I0-Installationsinventur](INSTALLATION-BASELINE.md) trennt die
tatsächlich fest codierten Runtime-/Handlerpfade, vorhandene
yaVDR/systemd-Adapter, isolierte Stage-Tests, bestehende Debian-Pakete und
unbekannte MLD-/openSUSE-Eigenschaften. **Das ist keine neue Host-Abnahme.**

Die bestehende `tools/addons/stage_media_tools_runtime.py` schreibt jetzt
in die bereits versiegelte `stage-with-media-tools` zusätzlich die
`usr/share/vdr-suite/addons/rectools/media-tools-install-inventory.json`
mit Modul-/Paketversion, vorbereitetem Source-Commit, unabhängig
freigegebenem Import-Handler-SHA256 und Rollen/Hashes/Größen/Modi der
**sieben** Add-on-Dateien. Die Inventur wird mit der vorhandenen
`install_runtime_stage.py`-Stage versiegelt und lässt sich ohne
Produktivänderung lesen/validieren:

```text
python3 tools/addons/stage_media_tools_runtime.py --verify-stage ABSOLUTE_STAGE \
  --approved-handler-sha256 UNABHAENGIG_GEPRUEFTER_SHA256
```

Diese Funktion ist nur eine Offline-Integritätsprüfung, weder Signatur noch
Installer, Paketbau, Ausführungsrecht, Live-Rechtekorrektur oder Aktivierung.
Die bisherige `~cpppreview1`-Paketierung liest diese Stage **noch nicht**;
das ist Gegenstand von MT.I4 und darf nicht als erledigt gemeldet werden.
Zu MT.I2 gehören gesondert Update-/Rollback-/Besitzprüfungen. Der
bestehende Live-Installer bleibt bis dahin ausschließlich der ausdrücklich
genehmigungspflichtige yaVDR-Adapter, nicht das neue universelle CLI.

## MT.I2 — Installer/Updater: read-only preflight (in Arbeit)

Die **bestehende** `tools/addons/install_media_tools.sh` hat drei neue,
nichtprivilegierte und ausdrücklich **nicht installierende** Befehle:

```bash
# Bereits gebaute und versiegelte Stage unverändert wiederverwenden:
bash tools/addons/install_media_tools.sh prepare \
  "$WORK" "$INDEPENDENTLY_REVIEWED_HANDLER_SHA256"

bash tools/addons/install_media_tools.sh check \
  "$WORK/stage-with-media-tools" / "$INDEPENDENTLY_REVIEWED_HANDLER_SHA256"

bash tools/addons/install_media_tools.sh verify \
  "$WORK/stage-with-media-tools" / "$INDEPENDENTLY_REVIEWED_HANDLER_SHA256"
```

`$WORK` ist das bereits vorbereitete Arbeitsverzeichnis; der bisherige
`COMMIT --prepare BACKEND`-Aufruf bleibt für einen erforderlichen Neubuild
erhalten. Der neue `prepare`-Aufruf verifiziert dagegen die **schon
vorhandenen** drei Binärdateien, `candidate.txt` und die Stage, ohne
erneut zu kompilieren. `check` und `verify` lesen den
**gesamten** MT.I1-Stage-Seal, das versionierte Media-Tools-Dateiinventar,
sämtliche acht Add-on-Dateien (inklusive Inventar), die erkennbaren
Freigabe-/Quell-/Lock-/Drop-in-Dateien und bei `/` den Debian-Dateibesitz.
`check` berichtet `absent`, `identical_payload`,
`partial_or_different`, `package_owned` oder
`existing_state_without_payload`; `verify` besteht nur bei exakt
identischen Add-on-Dateien. Die Diagnose folgt keinen Symlinks im
Installationsbaum, ändert keine Dateimodi und erstellt keine Dateien.

**Grenze:** `check`/`verify` bestätigen Add-on-Dateien, **nicht**
effektive Root-/Core-Freigaben, laufende Agenten, alle Suite-Dateien oder
eine sichere Paketmigration. Eine positive Prüfung startet **keine**
Installation. Der alte Erstinstallationsadapter verlangt jetzt zusätzlich
`MEDIA_TOOLS_INSTALL_CONFIRM=REVIEWED_FIRST_INSTALL`, eine exakte bisherige
Suite-only-Stage, root-kontrollierte Staging-Kopie und SQLite-Backup. Er
vergibt **keine** Zulassungen und startet **keine** Dienste; bei einem Fehler
nach Deployment versucht er den vorherigen Suite-Stand aus dem
verifizierten Snapshot wiederherzustellen und lässt die Dienste gestoppt.
Es werden weder vorhandene Dateirechte korrigiert noch Debian-Dateien
überlagert. **Das ist Quellcode, keine erneute yaVDR-Live-Abnahme.**

### MT.I2 — Update/Rollback in isolierten Testwurzeln

Der vorhandene CLI-Einstieg bietet zusätzlich `update` und `rollback`
**nur für bewusst angelegte, unprivilegierte Testverzeichnisse**. Die
eingesetzte `media_tools_runtime_transaction.py` führt **keine zweite
Deploy-Implementierung** ein; sie ruft `tools/install_runtime_stage.py`
mit den bereits versiegelten vorherigen und neuen Stages auf.

Voraussetzungen für den Test: zwei unabhängig anhand der Handler-SHA256
geprüfte, verschiedene Stages mit identischer Datei-/Verzeichnisstruktur
und gleichen Modi; eine exakt zur vorherigen Stage passende Testinstallation;
ein vorhandener exklusiv übernehmbarer Media-Tools-Wartungslock; eine
isolierte rootlose Testwurzel im Modus 0700 mit
`.vdr-suite-mt-i2-test-root` (Inhalt `MT_I2_SYNTHETIC_ROOT`, Modus 0600).
Das Testverzeichnis wird nicht vom CLI angelegt. Der alte versiegelte
Stage-Stand ist die bytegenaue Rücksicherungsquelle.

`update` spielt über den bestehenden Deploy die neue Stage ein und
prüft sie danach. Bei einem Fehler wird **automatisch** die alte Stage
erneut ausgerollt und verifiziert; `rollback` stellt sie nach einem
vollständig geprüften Update explizit wieder her. Ein absichtlich
ausgelöster Fehler, korrekte Vorwärts-/Rückwärtsinstallation, unpassende
Dateistruktur und fehlende Bestätigung sind isoliert getestet (CI
`media-tools-trust-gate`, Code-SHA `6473dedef05660219823d555c06f6775e34130e8`).

**Wichtig:** Beide Transaktionsaktionen verweigern den produktiven
Installationsroot `/` **immer**, auch für root. Sie verändern weder den
realen Host noch dessen Berechtigungen, Dienste, SQLite, Aufnahmen oder
Freigaben. Für die echte yaVDR-Updatefähigkeit fehlen noch die verifizierte
Host-Wartung, Paket-/Dateibesitz- und Versionsmigration, SQLite-Backup,
sichere Root-kontrollierte Stage und die ausdrücklich bestätigte,
zustandswahrende Dienststeuerung.



### MT.I2 — expliziter Offline-Host-Updatepfad (yaVDR/systemd)

`tools/addons/install_media_tools.sh live-update` und `live-rollback`
verwenden den **gleichen** bestehenden `install_runtime_stage.py`-Deploy
wie die synthetischen Tests. Der Hostpfad ist separat geschützt und setzt
zwingend voraus:

- Der Administrator hat **beide** bereits vorbereiteten, versiegelten
  Suite+Media-Tools-Stages samt unabhängig geprüfter alter/neuer
  Handler-SHA256. Der vollständig installierte vorherige Suite-Runtime-Stand
  muss zu seiner alten Stage passen. Struktur und Modus beider Stages müssen
  übereinstimmen; Paketbesitz oder abweichende Eigentümer stoppen die manuelle
  Routine.
- **Suite-Daemon und Backend-Agent sind zuvor bewusst gestoppt** und
  fehlerfrei inaktiv. Die Routine stoppt, startet oder aktiviert keinen
  Dienst selbst. Ein vorhandener root-kontrollierter exklusiver Media-Tools-
  Wartungslock muss frei sein.
- Bei `live-update` ist ein **neues** Verzeichnis unter einem
  root-kontrollierten, nicht fremdschreibbaren Parent für die Backups
  anzugeben, z. B. unter `/root`; dort legt das Skript unveränderliche
  Kopien beider Stages, ein SQLite-Backup und die Rücksicherungsinformationen
  an. Kein Root-Verzeichnis, dessen Inhalte bereits existieren, wird
  umkonfiguriert.
- Es gibt eine sichtbare Freigabekennung `LIVE-UPDATE-REVIEWED` bzw.
  `LIVE-ROLLBACK-REVIEWED`. Der Rollback prüft auch, dass sich Datenbank
  samt WAL/Journal seit dem Backup nicht geändert haben. Bei geänderter
  Datenbank stoppt der automatische Rollback **ohne Datenverlust** und
  fordert gesonderte Prüfung.

Der Dateiaustausch wird mit der zuvor geprüften Stage verifiziert. Bei
Fehlern rollt das Skript auf die gespeicherte alte Stage zurück; es startet
danach ausdrücklich **keinen** Dienst. Weder `addon.json` noch die
Installation erteilen Benutzerrechte, Backend/Core-Zulassungen oder einen
Agent-Opt-in. Freigaben, Konfiguration und Dienststart bleiben separate
ausdrückliche Schritte.

**Abnahmegrenze:** Die unprivilegierten Stage-/Rollbacktests prüfen die
gemeinsame Deploy-Mechanik, das Fail-Closed-Verhalten und die CLI. Ein
vollständiger produktiver yaVDR-Update-Test (einschließlich realer
Datenbank und Systemdienste) hat **nicht** stattgefunden. Falls altes
Suite-Stage-Backup, saubere Paketbesitzinformation, freie Ressourcen oder
Servicezustand nicht vorliegen, wird **nicht** aktualisiert. MLD/openSUSE
sind weiterhin kein zugesagtes Ziel.


## Host installation and activation (Slice 4)

**Status:** Source-level, package-free first-install path under development;
the current candidate is NOT cross-distribution-certified or accepted for
running yaVDR deployment. Existing Debian package installations require a
separately reviewed ownership/migration procedure. Do not run this installer
on an existing host with `vdr-suite-addon-media-tools` installed.

The common optional build output consists of the independently compiled
Media Import, Media Check and Media Shrink workers plus Suite-owned,
**non-executing** metadata. Core retains the effective capability and
authorization decisions. Once Media Import atomically creates and verifies
a VDR recording, **the existing native VDR recording-cache reconciliation**
confirms the recording; neither TVScraper nor Media Tools owns a second
recordings database or a separate public recording API.

The existing Suite install contract uses `make stage-install-runtime` and
the immutable `install_runtime_stage.py` manifest. The optional
[`stage_media_tools_runtime.py`](../../tools/addons/stage_media_tools_runtime.py)
composes already built Media Tools workers into a **separate** sealed
`stage-with-media-tools` beside the original Suite runtime stage. It does
not install Debian packages, compile programs, restart services or modify
the live host. A reviewed handler SHA-256 and exact prepared source SHA are
required, and the original sealed Suite stage is not modified.

`tools/addons/install_media_tools.sh` currently implements the **yaVDR
systemd installation adapter only**: service stop/restart, SQLite backup,
root-owned approval issuance and a systemd drop-in. Its standard media path
defaults are `/srv/video/Filme` and `/srv/vdr/video.00`, overridable for
the host. The systemd service's actual configured non-root user is used for
path access checks rather than hard-coding `vdr`.

**Distribution contract:** Installing on Debian, Ubuntu, MLD or another
system must NOT assume that VDR itself uses a specific account, video
directory, init system or plugin ABI. The native VDR video directory should
be obtained from the already existing SuiteBridge discovery wherever
available. Paths that belong to VDR-Suite's own trusted runtime layout
must remain consistent with the Core/Agent compile-time install contract.
The staged `PREFIX`/`DATADIR` install logic is not proof that every
runtime trust reader has configurable paths. These still require a verified
platform contract; **MLD is not yet supported by this installer**.

Public metadata directory mode `0755` is ordinary POSIX staging semantics
for nonsecret assets and is independent of the service user. It does not
authorize imports or broaden private directories. The deploy helper never
changes permission modes of pre-existing host directories. If a live
ancestor denies traversal, the system-specific installer must STOP before
modifying services. Do not propose recursive `chmod` or `chown` as a
portable remedy or claim that a corrected staging mode repairs that host.

Source installation still needs the independent backend-local handler
approval, Core expected SHA, frontend administrator activation and
backend-scoped user grants. None are conferred by file presence. The
[`configure_media_tools_paths.sh`](../../tools/addons/configure_media_tools_paths.sh)
helper is also currently a **yaVDR/systemd-only** adapter and must not
require a Debian package as proof of an independently approved handler.
The current Webfrontend does **not yet** provide authenticated import
source/destination administration; implement it via the existing Suite
security and API ownership, not a direct browser-to-root-file write.

The next gates before any further live deployment are: focused package-free
staging and helper regressions on the exact head; independent review of
the installed-file ownership/migration from the already installed
`~cpppreview1` Debian package; system-specific preflight; and a
separate real-VDR acceptance using test media. Existing source-built
Media Tools payloads must not be recompiled merely to repeat an earlier
accepted build.

## Core-owned A2M/A4 activation (Slice 3 implementation)

The Core checks an authenticated backend Agent's recent, lease-bound
installation observation against an **independently approved, backend-scoped**
root-owned expected record on the **Core host**:

`/etc/vdr-suite/addon-approvals/rectools.<backendId>.json`

Its expected SHA-256 must be independently reviewed; neither `addon.json`,
`dpkg` metadata nor an Agent-reported digest approves itself. The Core
record can be issued/rotated through `tools/addons/admin_approval_issuer.py`
with `--core-expected-only --backend-id BACKEND_ID`, and revoked with
`revoke --backend-id BACKEND_ID`. Issuance on the actual host additionally
requires the issuer's explicit `--allow-live-root` confirmation and
administrator privileges. **Do not run live-root issuance or install/restart
any services as part of a source/CI test.**

The authenticated administrator Browser Session API now exposes:

- `GET /api/v1/addons/rectools/activation?backendId=BACKEND_ID`:
  current persisted `desiredEnabled` and CAS `revision`. Global
  `role.admin@*` plus a valid browser session are mandatory.
- `POST /api/v1/addons/rectools/activation` with strict JSON:
  `{"backendId":"BACKEND_ID","desiredEnabled":true,"expectedRevision":0}`
  or the matching `false` disable. The first update uses revision 0; all
  later updates must use the latest read revision. The request requires a
  global administrator Browser Session, CSRF token and normal Suite request/
  correlation identifiers. A stale revision returns 409. Successful changes
  are CAS/audit-atomic in the canonical Suite Core database.

Enabling requires a writable backend, a live exact Agent
identity/instance/generation, an advertised Media Import command capability
and the currently matched Core/Agent handler identity. The approved
SHA-256 is recorded with the audited activation revision. Handler rotation
cannot inherit the previous desired-enable authorization: an administrator
must explicitly re-approve with the current revision after reviewing the new
expected hash. Migrated desired-enabled rows lacking a hash pin remain
ineffective. Disabling remains
available even if the handler disappears or the approval is revoked. The
existing import and HEVC-shrink Core fences independently recheck trust on
discovery/reservation and at the Agent receipt. An approval record, a saved
`desiredEnabled` bit or a granted permission **alone** cannot start work;
the Agent executable remains explicitly opted in.

These endpoints do **not** install code, load a plugin, start a service,
disable maintenance locks or make unverified remote execution safe.
`AddonHandlerRegistry::kMediaToolsCandidate.executable` and metadata-only
manifests remain fail-closed. Full end-to-end and real-host acceptance is the
separate remaining Slice 4, not proven by source-level activation tests.

## M2: erster tatsächlich nutzbarer Media Check (read-only)

Der neue eigenständige C++17-Befehl `vdr-suite-media-check` prüft die
**TS-Paketstruktur einer vorhandenen VDR-Aufnahme**, ohne sie zu verändern.
Die zugehörige Library liegt in `include/MediaCheck.h` und
`src/MediaCheck.cpp`, der reine Lese-CLI in
`src/media_check_main.cpp`. Der Aufruf lautet:

```text
vdr-suite-media-check check VIDEO_ROOT RECORDING_RELATIVE
```

`VIDEO_ROOT` ist ein absoluter, vom lokalen Backend-Betreiber vorgegebener
VDR-Video-Root; `RECORDING_RELATIVE` benennt eine darunterliegende,
normalisierte `.rec`-Aufnahme einschließlich Titelordner. Symlink-Pfade
und nicht zusammenhängende, fortlaufend nummerierte `00001.ts`,
`00002.ts`, … werden abgelehnt. Das Ergebnis ist ein JSON-Objekt mit
`status` (`ok`, `warnings`, `errors`), analysierten Paketen/Segmenten,
TS-Synchronisationsfehlern, Transportfehler-Indikatoren, fehlerhaften
Adaptationsfeldern, unvollständigen Paketen, PID-Kontinuitätswarnungen,
Discontinuity-Flags sowie Indexdatei-Präsenz. Die Daten werden nur
lesend über Dateideskriptoren eingelesen. Exitstatus 0 steht für
`ok` oder `warnings`, 1 für erkannte Strukturfehler, 2 für eine
unvollständige/unsichere Prüfung.

Both read-only Media Check modes additionally inspect relevant VDR
sidecars (`index`, `info`, `marks`, `resume`, `.srt`, `.nfo`).
The JSON includes `sidecarReports` and counts of unsafe/nonregular or
obviously empty sidecars. FFprobe receives only a verified read-only TS
descriptor on stdin, never a browser-selected recording path; it reports
video/audio codec, resolution, channel count and container attributes
in the bounded `ffprobe` result. The Recording Hero summarizes these
attributes without implementing a separate scanner.

**M2.1 FFmpeg-Decodierung:** Die zusätzliche Aktion `check-decode`
verarbeitet die nummerierten TS-Segmente **als einen einzigen Stream** und
decodiert Video/Audio in den FFmpeg-`null`-Muxer, ohne Output-Dateien
anzulegen. Nur ein festes, nicht vom API-Nutzer auswählbares
`/usr/bin/ffmpeg` wird gestartet; die Eingabe kommt ausschließlich
über eine Pipe aus bereits geprüften, nur lesend geöffneten Segmenten.
FFmpeg wird mit `-xerror -err_detect explode` ausgeführt und nach
maximal 900 Sekunden beendet. Der JSON-Bericht enthält zusätzlich
`decoder.status` (`ok`, `errors`, `incomplete`),
`exitCode`, `timedOut`, `streamedBytes` sowie begrenzte Diagnosezeilen.
Timeouts und fehlendes FFmpeg sind **unvollständige Prüfungen**, keine
erfolgreich geprüften Aufnahmen. CLI-Exitstatus 0 = keine harten
Fehler, 1 = dekodierte/strukturelle Fehler, 2 = unvollständig/unsicher.

```text
vdr-suite-media-check check-decode VIDEO_ROOT RECORDING_RELATIVE
```

Dies ist eine vollständige Software-Decodierprobe über alle vorhandenen
TS-Segmente – nicht der Nachweis garantiert störungsfreier Wiedergabe
auf jedem Client. Die separat getestete `check`-Aktion bleibt ein
schneller reiner Struktur-Scan. **M2.2/M2.3 Entwicklungsstand:** Die Versioned API kann per
`POST /api/v1/backends/{backendId}/media-checks` einen reinen Lesecheck
mit den Feldern `recordingId` (Suite-eigene `rec_...`-Kennung) und
`mode` (`structure` oder `decode`) starten. Das Ergebnis ist unter
`GET /api/v1/backends/{backendId}/media-checks/{checkId}` abrufbar.
`GET /api/v1/backends/{backendId}/capabilities/media-check` gibt die
lokale Verfügbarkeit zurück. Der bestehende Recordings-2-Hero zeigt
`Aufnahme prüfen` und das Prüfergebnis an.

**Wichtige Grenzen:** Diese Entwicklungsversion arbeitet ausschließlich
auf serverlokal vorhandenen VDR-Videoverzeichnissen. Die
Daemon-Umgebungsvariable `VDR_SUITE_MEDIA_CHECK_VIDEO_ROOTS` enthält
die expliziten, backend-spezifischen Wurzeln im Format
`backendId=/ABSOLUTER/VIDEO_ROOT` (mehrere Einträge mit Semikolon).
Ohne diese Konfiguration wird die Funktion nicht angeboten.
Die Aufnahme wird serverseitig anhand der `rec_...`-Kennung und eines
aktuellen Backend-Snapshots erneut aufgelöst. Browserpfade werden nie
ausgeführt. Genau eine Prüfung läuft gleichzeitig; abgeschlossene Berichte
werden in der **bestehenden Suite-Core-SQLite-Datenbank** archiviert und
über den Check-ID-Endpunkt nach einem Daemon-Neustart wiedergefunden.
Eine beim Absturz laufende Prüfung wird als `incomplete` gespeichert,
nicht als Erfolg. Jeder Status-Abruf prüft das backend-spezifische Recht
`media.recording.play` erneut und bindet den Bericht an den ursprünglichen
Akteur und das Backend. Die Anbindung an Remote-Agenten und die Migration
in die Core-Operations-Jobausführung fehlen weiterhin. Es gibt
weiterhin **keine** produktive Add-on-Aktivierung oder Paketinstallation,
keinen automatischen Reparatur-, Lösch- oder Mailversand. Eine echte
yaVDR-Abnahme ist ausdrücklich noch offen.

`make addon-media-check` kompiliert nur die CLI;
`make test-addon-media-check` und
`make test-addon-media-check-api` verwenden ausschließlich künstliche
TS-Pakete und temporäre Verzeichnisse und prüfen die Aufnahmen auf
Unverändertheit. Der Hero-/Client-Vertrag wird durch
`web/frontend/tests/test_media_check_composition.js` geprüft. Keine Marker, Mail, Reparatur,
Index-Neuerzeugung, Änderung der Aufnahme, automatische Paketinstallation
oder Add-on-Aktivierung. Das bestehende Media-Import-Interface bleibt
unberührt; `addon.json` bewirbt Media Check noch nicht.

## Legacy Rectools: PC delegation and re-encode decision (reference)

Audited reference: `hotzenplotz5/vdr-rectools`, `develop`
(`6fffd260d913c7ff4094aea0c289bb58026fcc7f`),
`usr/share/vdr-rectools/functions.sh` (`confirm_encoding`,
`process_import`, import `find` scan), `rectools_confirm.php`,
`explorer.php`, `lang/de.sh`.

The old behavior is valuable and must not be mistaken for filename debris:

- Compatible H.264/MPEG-2 imports use remux (stream-copy); codecs needing
  re-encode cause an explicit choice when `ASK_BEFORE_ENCODE=1`.
- The request is reported by e-mail, and the user can decide in the
  dashboard, OSD or console. The old console `confirm` command offers
  yes/no; the third PC action is available via dashboard/OSD.
- **YES:** encode on VDR. **NO:** rename the source to
  `<stem>.skipped.<ext>`. **MANUAL:** rename the source to
  `<stem>.pc_encode.<ext>`, hand the file to a separate PC (for example
  HandBrake over SMB), then return/release the completed movie explicitly.
  Duplicate sources use `.duplicate.` too.
- The legacy import scan intentionally excludes these marked files.
  Rectools does **not** execute HandBrakeCLI remotely; the remote processing
  itself is external/manual. E-mail is a notification, not an authenticated
  approval endpoint.
- The old prompt file `.vdr-rectools.prompt`, which was chmodded 0666,
  must **not** be copied to VDR-Suite's security model.

**Implemented source-level gates (not yet deployed):** The independent
C++ importer's `inspectImport()` rejects stems ending in `.pc_encode`,
`.skipped` or `.duplicate` (case-insensitive) before staging.
`vdr-suite-media-import plan SOURCE_ROOT VIDEO_ROOT FILE` uses bounded,
read-only FFprobe to report `VIDEO_CODEC` and
`VIDEO_REENCODE_REQUIRED=YES|NO`. A normal
`import ... --confirm-writes` may remux copy-compatible H.264/HEVC/MPEG-2,
but cannot perform a **video** re-encode without the separate operator-only
`--approve-video-reencode` CLI flag. The CLI first checks the current
codec without staging or recording mutations; if a decision is missing it
prints `IMPORT_STATUS=DECISION_REQUIRED` and exits 3 with the source intact.
The Agent's pinned executor passes
only `--confirm-writes`, so the actor-facing API cannot silently grant local
video encoding. No source file is deleted or renamed by a refused import.
C++ and real-FFmpeg regressions cover the refusal and explicit approval,
as well as the observed `Die Herrschaft des Feuers.pc_encode.mkv`.
These are committed **source changes**, not a verified build or a deployed
upgrade to the existing yaVDR binaries.

**Open integration slices, NOT shipped:** (1) A backend-scoped,
durable Core decision in the existing authenticated API v1 Operation
(identity-bound and idempotent) for `local`, `desktop`, or `skip`;
(2) enforce that choice in the protected Agent's pinned assignment and
preflight, instead of allowing a browser/manifest/filename to confer
execution permission; (3) safely record PC delegation and skip without
a world-writable prompt, so desktop-return can be intentionally resumed
and existing VDR reconciliation still owns the new recording; (4) an
optional notification transport for actionable e-mail and the frontend
buttons with status/authorization checks. **Neither the three-way
Public API, email delivery, nor the UI decision operation is yet wired.**
Do not treat an authenticated import `sourceId` as approval for video
encoding or expose the new operator CLI flag to browsers. The existing
source/agent/Core operation system should be extended, never copied into
a second queue.

## MT.UX1 – bounded read-only source discovery (code candidate)

The existing root-owned backend `media-import.tsv` remains the only source
authority. The Agent performs an iterative recursive read-only traversal; it
does not follow directory symlinks and does not accept paths from the browser.
Eligible aged sources are mapped to opaque identity IDs and revalidated by
the Agent before import execution. Its current safety limits are **4096
visited directory entries and a maximum of 100 eligible films** per scan/import
selection, as explicitly chosen for MT.UX1. Typical runs of 5–10 films are
unaffected.

An oversized inventory no longer discards all verified items: the bounded
snapshot is published with `partial=true`. The Agent reports IDs, advisory
labels and completeness from one scan; absent/older status is treated as
partial. Core rechecks provider approval, actor/backend scope and live Agent
lease. Public v1 paginates the **existing** observed source snapshot in
32-item pages and binds cursors to actor/backend/catalog revision. The browser
may display `Scan abgeschlossen` only for `meta.partial=false`.

**Import ceiling:** if more than 100 eligible films are found, the scanner
retains at most 100 verified source IDs and sends `partial=true`. The UI
explains that only up to 100 films can be imported per run and another scan
may be needed later. Scans that reach the 4096-entry safety limit are also
reported incomplete, even when fewer than 100 films were found. **There is
no requirement to support a >100-film batch or unbounded continuation.**
This code candidate has not had a confirmed C++ build, CI run or real yaVDR
acceptance. No package, service, VDR recording, filesystem
owner or live configuration was changed by these source edits.

## MT.UX2 – backend-bound source names (partial source implementation)

An authorized backend may already have multiple rows in the **root-owned**
`/etc/vdr-suite/addon-sources/media-import.tsv` inventory. MT.UX2 extends
the existing five-column record with one optional, presentation-only sixth
column:

```text
BACKEND_ID<TAB>OPAQUE_ROOT_ID<TAB>SOURCE_ROOT<TAB>VIDEO_ROOT<TAB>*<TAB>DISPLAY_NAME
```

The first row for each backend is shown as the default **when named**;
subsequent named rows are displayed with their own prefix. Example:
`Standard: Filme · Matrix` and `Serien · Andor`. Old five-column rows
retain their exact previous appearance. The alias is at most 64 bytes and
must not contain slashes, backslashes, tabs or control characters. It is
display metadata only: neither a new backend, permission, path nor runnable
source is authorized by a label. Existing rooted-file inspection, source-ID
mapping, filesystem symlink rejection, Agent revalidation and the 100-film
catalog cap remain mandatory. The web client receives **no source path**.

**Erstinstallation:** Der vorhandene root-only
`tools/addons/configure_media_tools_paths.sh` richtet bei Bedarf
eine einzige Standardquelle und die VDR-Zielablage ein. Das ist ein
Wartungswerkzeug, **nicht** der normale Weg für weitere Importquellen.
Die zusätzliche Quellenverwaltung übernimmt direkt die Weboberfläche.

**Einfache Webverwaltung (lokaler Backend-Agent):** Die vorhandene
Administratoranmeldung schützt `GET/POST /api/v1/addons/rectools/sources`
einschließlich der vorhandenen CSRF- und Zugriffsprüfung. In den Media-Tools-
Einstellungen lassen sich die bereits gespeicherten Importverzeichnisse
anzeigen, mit Namen versehen, hinzufügen, entfernen, als Standard nach
oben schieben und durch einen normalen **Speichern**-Knopf übernehmen.
Keine Shell-Befehle im Browser, keine neuen Rollen, kein eigener Dienst und
kein neuer Datenbankbereich. Die VDR-Zielablage bleibt unverändert.

Der Daemon der ausgelieferten lokalen systemd-Unit läuft standardmäßig als
root und kann deshalb das vorhandene **root-kontrollierte** Inventar direkt
und atomar ersetzen. Die Serverfunktion prüft lokale Backend-ID, alte
Konfigurationsrevision, bestehende kanonische Verzeichnisse, getrennte
Quell- und VDR-Verzeichnisse sowie die vorhandene Media-Tools-
Wartungssperre. Jede andere Datei und die Dienste bleiben unverändert.
Der laufende Agent liest diese Quelle beim nächsten normalen Scan neu;
ein Neustart ist nicht erforderlich. Nur tatsächlich gespeicherte
Konfiguration wird mit `Gespeichert` quittiert.

**Grenze:** Die lokale Speichern-Funktion verweigert Änderungen an einem
Backend auf einem **anderen** Rechner, statt versehentlich das lokale
Inventar zu verändern. Das ist eine explizite, noch nicht unterstützte
Konfiguration, kein Auftrag zur Einführung einer weiteren Verwaltungs-
architektur. In nichtprivilegierten oder besonders gehärteten
Daemon-Installationen wird ein Root-Schreibversuch abgewiesen. Die
Server-/C++-/CI-/Real-VDR-Abnahme dieses noch nicht installierten
Branchcodes bleibt offen.

**Keine Live-Aktion in diesem Entwicklungsdurchlauf:** Die direkte
Web-Speicherfunktion ist ausschließlich Quellcode auf dem Arbeitsbranch.
Es wurde kein Dienst installiert, neu gestartet oder umkonfiguriert.
Entfernen oder Umsortieren der Quellen ändert die zugeordneten Auswahl-IDs;
der Core-/Agent-Weg prüft jede Auswahl bei der Ausführung erneut.
Der bestehende Offline-Konfigurator bleibt für Wartung verfügbar, ist
aber kein Bestandteil des normalen Web-Workflows.

**Browser administration:** Der lokale Speichervorgang ist auf dem Branch
implementiert; für entfernte Backend-Rechner noch nicht. Vollständiger
C++-Build, API-/Browsertests und echte VDR-Abnahme stehen noch aus.

## MT.UX4.2 – Backend-bestätigte Nur-Lese-Vorschau (Quellcode)

Nach einem erfolgreichen Importquellenscan wählt der Browser einen Film
über dessen **opaque `sourceId`**. Der vorhandene Public-v1-Client stellt
`GET /api/v1/backends/{backendId}/media-import-sources/{sourceId}/preview`.
Die Public API prüft den authentifizierten Backend-Scope; Core prüft
zusätzlich die wirksame Importberechtigung, den Agent-Lease und die
freigegebene Installation. Eine Anfrage löst ausschließlich eine kurzlebige,
auf diese ID begrenzte Vorschau-Anforderung aus.

Der vorhandene Backend-Agent erhält die ID beim regulären, authentifizierten
Quellenabgleich und löst **nur diese Datei** über das root-kontrollierte
Inventar auf. Er vergleicht Dateiidentität (Gerät/Inode/Größe/Änderungszeit),
verwendet die normalen `inspectImportPreview`-, `probeImportVideoCodec`-
und `inspectImportCapacity`-Prüfungen und sendet lesend Titel,
video-root-relativen VDR-Zielordner, Codec, Dateigröße, Dublettenstatus,
Umkodierungsbedarf und Speicherwerte zurück. Der angezeigte erforderliche
Speicher schließt die vorgeschriebene Reserve ein. Keine Datei- oder
Aufnahmepfade aus HTTP werden für die Quellauflösung akzeptiert.

Bei der ersten Antwort kann `pending` stehen; das Webfrontend wartet
begrenzt auf die Antwort und verwirft veraltete Ergebnisse beim Wechsel der
Auswahl. Selbst eine erfolgreiche Vorschau **erteilt keine Import- oder
Video-Umkodierungsfreigabe**. Ein später bestätigter Import prüft alle
Quellen-, Ziel- und Berechtigungsbedingungen erneut und verwendet weiterhin
den produktiven Core-/Agent-Operationsweg. Die Hardware-/Live-VDR-Abnahme
sowie ein vollständiger C++-/Integrations-/CI-Lauf auf dem endgültigen SHA
stehen aus. Weder VDR-Dienst noch Medien wurden hier verändert.

## MT.UX4.1 – relative VDR target folders (source candidate)

The approved import **root maps directly to the configured VDR video root**.
The existing `inspectImport` now preserves each source file's real relative
parent hierarchy, such as `Serien/Andor/Staffel 1/Folge01.mkv` becoming
`<VDR video root>/Serien/Andor/Staffel 1/Folge01/<timestamp>.rec`.
Root-level imports remain `<VDR video root>/<title>/<timestamp>.rec`.
The importer rejects unsafe/VDR-incompatible directory components and
symlinked target parents instead of flattening or silently renaming them.
Two movies with the same filename in different valid source directories
therefore have independent destinations and duplicate checks.

Scan and import inspection are read-only. FFmpeg staging, duration/codec
validation, VDR index generation, capacity checks and source-identity checks
still happen **before** missing VDR directories are created. Newly created,
still-empty directories are cleaned up after publication failure; pre-existing
target directories are left alone. The final `.rec` publication remains the
existing atomic no-overwrite operation and uses the same Core/Agent readback
contract.

The browser shows the **verified source folder** in its existing selected-
film preview and does not guess the eventual target folder or codec. The
separate **read-only selected-file backend preview** with a trustworthy
resolved destination, video codec, source size, free capacity and duplicate
status is not yet implemented; never present those values as known.
The C++ compilation, isolated recording fixtures, CI and real-VDR acceptance
are also still pending. No source files, deployed VDR installation, or
recording assets were modified as part of this GitHub work.

## MT.UX3 – bounded browser search and subset selection (partial)

Within the **existing authorized Agent/Core source snapshot**, the browser
now offers a title/source-name search, an importable/marked/all filter and
read-only match counts. Search/filter changes invalidate previous film and
batch selections. Only the currently visible verified opaque IDs are
eligible for the new checkbox multi-selection; the separate chosen-only
batch action requires explicit user confirmation. The old all-visible
batch action remains, and every film still receives its own Core operation,
idempotency key, Agent source revalidation and independent VDR readback.
Local re-encode consent is **never** inherited by a batch. Maximum remains
100 verified sources for all roots together. No new endpoint, queue, path
lookup, marker mutation or filesystem write occurs during browsing.

**Relative folder navigation added as a source candidate:** the trusted
local inventory now derives a path **relative to its configured source root**
from each inspected source. When several approved roots contain candidate
films, a virtual, unambiguous root-name component separates their trees
(e.g. `Standard: Filme/` and `Serien/`); it is a display namespace, not
part of either real filesystem path. The Agent reports the optional `sourceFolders`
array alongside the same opaque source IDs and labels; Core validates each
relative component and retains the folder only in its ephemeral,
lease-/generation-fenced catalog. Public v1 exposes optional `folder` per
item, includes folder metadata in its actor-/backend-specific snapshot
cursor, and rejects absolute, traversal, control or malformed paths.
The browser creates breadcrumb buttons and immediate child-folder buttons
only from those verified read-only fields; a click navigates **locally**
through the bounded snapshot and never sends paths to an importer.
Root-level films appear at the root, nested films at the corresponding
folder, and free-text search intentionally finds matching films across
folders. Older Agents without folder metadata remain compatible and
present the same flat selection as before.

**Pending validation:** this crosses Agent observation, Core ephemeral
catalog, Public v1 and the web panel. Source-level tests exist, but this
candidate has **not** had a complete C++ build, CI or real VDR integration
test. The eventual end-to-end tests must exercise source-root changes,
backend actor scope, stale page cursors, Unicode folders, symlink/traversal
denials and folder-level filtered batch selection.

## Importer: source-preserving metadata and codec fallback

The existing backend C++ import implementation remains the only importer.
A root-controlled opaque source selection proceeds through the existing
Suite API v1 and Core/Agent operation identity and verifies the recording
via the existing VDR recording-cache reconciliation. Video in H.264, HEVC
or MPEG-2 uses stream copy; other video codecs use bounded software
H.264/AAC encoding with FFmpeg and fixed arguments. In both cases a private
input snapshot and output-size limit protect the staging volume. External
tools have an explicit deadline, and the original source file is never
moved or deleted. Capacity preflight leaves the required free-space reserve.

If an adjacent same-stem `.nfo` exists, bounded plain-XML fields
`title`, `outline`, `plot`, `year`, and `genre` are copied into
the resulting recording's native VDR `info` file. XML beyond the
supported simple tags/entities is rejected rather than silently interpreted.
An adjacent same-stem `.srt` is preserved as `00001.srt`, the
existing Suite subtitle reader's preferred sidecar. Language-tagged
companions such as `Movie.de.srt` and `Movie.en-US.srt` are also copied
as bounded `subtitles.de.srt` or `subtitles.en-US.srt`. Originals remain
untouched; all sidecars together have a 16 MiB staging budget. Compatible
video/audio tracks remain stream copy; incompatible video is encoded as
H.264, incompatible audio as AAC. The title directory is matched to the NFO
title when supplied and duplicate recordings are rejected.

An explicitly selected browser **Mehrfachimport** can now submit currently
authorized source IDs sequentially through separate durable Core Operations.
It advances only after canonical `succeeded` readback, never after an
ambiguous Agent result, and stops if its bounded status polling ends. This
is a foreground browser feature, **not** an unattended daemon scheduler.

### HEVC shrink: standalone engine and source-integrated workflow

At HEVC Stage, the protected candidate marker records the validated source
fingerprint **and the exact concatenated TS duration** used to verify encoding.
The independent Publish verifier now checks both the staged duration and a
read-only re-probe of the original source TS byte stream, for single- and
multi-segment recordings. Multi-part VDR TS files are exposed through
FFprobe's read-only `concat:` protocol using their verified absolute
segment paths. Individual per-file duration values are **not** added:
TS timestamps may jump at file boundaries. A missing, stale, manipulated or
unsafe source/duration marker and shortened HEVC candidates fail closed.
This check creates no new TS snapshot and never writes the source recording.
The isolated regressions cover a valid timestamp discontinuity, a changed
combined source duration and a concat-protocol delimiter in the recording path.


In the opted-in production Backend Agent, the HEVC executor is wrapped by
`ProtectedMediaShrinkAgentExecutor`: each stage or publish must acquire the
same fixed, root-controlled Media Tools shared execution lock as Import,
and `NativeProcessRunner` preserves the borrowed lock FD across its ffmpeg/vdr
`execv` children. The lock is held across the complete HEVC operation, not
only individual subprocess calls, so an Agent crash while an FFmpeg child is
still executing does not falsely release the package-maintenance lock.
This does not authorize installation, enable an add-on or grant permissions.


`make addon-media-shrink test-addon-media-shrink` builds the real C++
converter and its isolated source-preservation tests. A backend-local
administrator can inspect `vdr-suite-media-shrink plan VIDEO_ROOT RELATIVE.rec`
without writing, explicitly stage a private HEVC candidate with
`stage VIDEO_ROOT RELATIVE.rec --confirm-staging` and separately publish
as a **new** recording with
`publish VIDEO_ROOT RELATIVE.rec CANDIDATE --confirm-new-recording`.
Existing TS segments, the VDR index and native `info` sidecar are
validated; the original recording is never replaced or deleted. Optional
`--max-height=720`, `--night-mode`, and
`--encoder=software|vaapi|qsv|nvenc` affect staging only. Hardware
failure can fall back to software. Compatible AAC/AC3/E-AC3/MP2 audio
is preserved by stream copy; incompatible audio is encoded to AAC (and
night mode deliberately uses AAC). FFprobe duration/codec, smaller output
size and VDR index must pass before review. Publication rechecks the
source identity, the staged metadata/subtitle bytes and retained HEVC
space savings before an atomic no-replace rename. The standalone CLI is
not the public mutation path; Core, Agent and API now compose the stage/publish
workflow in source. Productive end-to-end acceptance is still outstanding.

`MediaShrinkOperationSelection.h` validates path-free API/Agent payloads
with opaque `rec_...` and `shk_...` identities; it does not itself authorize
or execute writes. The current daemon now reserves canonical Core Operations
and forwards authorized Stage/Publish to an explicitly opted-in Agent.
Browser filesystem paths remain forbidden. The isolated invalid-shape
regressions run in `test-addon-media-shrink`.

### HEVC API v1 and durable Core review lifecycle

The Recording Hero exposes **HEVC-Vorprüfung** only when an explicitly
configured local backend video root and current snapshot are available.
Its canonical JS client lives at `clients/reference-js/public-v1-client.js`
and is reused through the original frontend symlink.
`POST /api/v1/backends/{backendId}/media-shrink-plans` accepts only an
opaque `rec_...` identifier; it resolves the path server-side and calls
`inspectShrink` **read-only**. The security gate requires an authenticated
actor, backend-specific `media.recording.play`, CSRF and accountability.
`GET .../capabilities/media-shrink` returns a read-only planning flag
and an effective write-availability flag, which may only be true with a
live Agent capability and current authorization/activation. Stage and Publish
use distinct explicit authenticated POST routes and durable Operations.
`GET /api/v1/backends/{backendId}/media-shrink-reviews/{reviewId}`
reads the actor-bound candidate state. Each read rechecks the backend-scoped
recording-read grant; only an authorized current review can allow Publish.
No native filesystem path or Agent secret is returned.

Core `MediaShrinkReviewRepository` stores an actor/backend-scoped review
binding in the **existing SQLite authority**, not a second job queue.
A candidate requires a succeeded `media.shrink.stage` Operation with
matching opaque result reference and generation. A separate reserved
`media.shrink.publish` Operation claims the review under an exact revision.
Unresolved/ambiguous publication remains claimed, never freely retryable.
A succeeded canonical publish Operation with new opaque recording ID is
necessary to mark the review published. The daemon initializes this schema
alongside the existing Operations store; disposable tests cover optimistic
revisions, cross-actor isolation and database restart.

The canonical Agent wire parser now recognizes exact, version-1 path-free
`addon.media.shrink.stage` and `addon.media.shrink.publish` payloads.
`MediaShrinkCommandFence.h` only assesses consistency with the immutable
Core Operation and payload, never authorization. A configured
`COMMAND_TYPES` entry **cannot advertise or execute** either command
without a trusted Agent executor. Tests cover forged paths, malformed
options, stale backend generations, request/operation mismatches and
terminal publication replay.

The existing durable Agent command inbox invokes an explicitly injected
`IMediaShrinkAgentExecutor` for stage or publish. The shipped Agent offers
`--enable-media-shrink --media-shrink-state-dir ABSOLUTE` as an explicit
opt-in, not an always-on capability. Availability comes from current
reviewed handler trust and authorization, not the manifest or
`COMMAND_TYPES`. The Agent persists a `starting` crash fence before
calling an executor. A recovered attempt is reported as
`outcome_unknown/reconcile_only`, never rerun. Even a returned opaque
`shk_...` ID is **not** proof that VDR has accepted a recording; Core
must independently validate the correct operation and readback. The
Agent runtime regression uses a synthetic executor and makes no media
changes.

**Not yet end-to-end accepted:** an opted-in Agent executor, opaque
candidate binding, exact backend-scoped `addons.media.shrink` grant, Core
Stage/Publish Operations, browser controls and publish recording-cache
reconciliation exist in the current source. Their complete runtime path,
including actual grant eligibility in HTTP composition, needs local proof.
The addon manifest remains disabled; no production VDR recordings have
been modified in this workstream.

### Administrator-opted unattended imports (source integration)

The **existing daemon event loop** can now evaluate a single backend's
Agent-published sources once a minute during a configured local-time window.
The root-managed `/etc/default/vdr-suite-daemon` can opt in using all three
non-secret values below; their absence or an invalid window keeps automatic
imports disabled:

```ini
VDR_SUITE_MEDIA_IMPORT_AUTO_ACTOR=existing_service_actor_id
VDR_SUITE_MEDIA_IMPORT_AUTO_BACKEND=existing_backend_id
VDR_SUITE_MEDIA_IMPORT_AUTO_WINDOW=01:00-05:00
```

The actor must already exist as an active **Service** identity, hold the
backend-specific `addons.media.import` grant, and pass all current Core
activation, Agent capability and backend write fences. The administrator
must separately authorize the Agent's local inventory and writable staging
path. Configuring the three values does not create a service identity,
grant privileges or enable an add-on. There is no new queue: scheduling
calls the same Core admission/assignment used by API v1. At most one
source is submitted per minute after consulting the existing durable
`media.import` Operations. A running or ambiguous operation blocks the
next one. Stable idempotency keys prevent retries and duplicate imports
across daemon restarts; a failed source requires manual review. Windows
may cross midnight (for example `23:00-04:00`). There is no browser
toggle or automatic production deployment yet.

**Not yet production-complete:** the Agent's 600-second assignment
deadline limits long imported video transcodes. Unattended scheduling is
implemented in source but has not been accepted on a real VDR host.
External subtitle search or playback language preference, VDR-compatible
chapter conversion, remote Media Check, and shrink through authorized API
v1/Core Operations remain open. The existing Suite recording-cache enrichment and artwork owners
must remain the only metadata/scraper architecture. A host acceptance
using real VDR recordings has not been authorized or performed. The
default backend-agent systemd unit also denies writes outside its own
state directory; administrative activation requires a separately approved
write-path policy, not a permissive package default.

## Installed handler metadata gate (A2M/A4)

A root-approved handler is no longer considered available from binary bytes and
an approval JSON alone. Its separate installed
`/usr/share/vdr-suite/addons/rectools/addon.json` must also be root-owned,
free of writable/symlinked parents and exactly match the approved module,
package and version. The current schema-v1 manifest must remain a disabled
scaffold with empty executable, permission and capability declarations.
Source/manual installations and eventual Debian packages use this same fixed
path; neither dpkg presence nor the manifest itself grants authorization.
The combined snapshot revalidates administrator approval after hashing, and
the pinned Agent executor revalidates the installed manifest and approval at
dispatch. It also re-hashes the **same pinned ELF descriptor** immediately
before `fexecve`, after acquiring the inherited shared maintenance lock;
neither a pathname reopen nor an authorization-time digest cache suffices.
Synthetic descriptor and executor tests reject in-place post-authorization
binary changes; these are not a production backend acceptance test.
The recognized handler interface is fixed at reviewed version `0.1.0`;
a newer installed binary/manifest does not gain execution compatibility by
self-reporting a matching version to an approval. A privileged,
administrator-invoked `admin_approval_issuer.py rotate` command can
atomically exchange one active **same-version** approval for another, but
only when the exact old SHA-256 is supplied and the replacement binary matches
a separate approved digest. The record is never overwritten by `issue`,
and `rotate` cannot reverse `revoke`. Root-owned disposable CI fixtures
test CLI rotation, stale expectations, replay, missing binary, missing
metadata, and persistent denial after uninstall/reinstall. No Debian
maintainer script invokes this command; a future package upgrade/remove
sequence must explicitly coordinate disable, drain, revoke, install,
review, and optional re-approval before real activation. Product activation
and full installer/upgrade handling remain separate acceptance steps.

## A4 preflight for later package maintenance (read-only)

Core now exposes an internal read-only maintenance inspection that checks
the canonical per-backend administrative disabled intent and the real durable
`media.import` operations. Accepted, queued, dispatching,
executed-unverified and outcome-unknown operations block a locally quiescent
diagnostic result; missing state or a saturated operation query also blocks it.
This does not drain the Agent, prevent new admissions, serialize an installer,
audit a package transaction or prove signed Debian provenance. **No package
install, upgrade, uninstall or source/recording modification is performed.**

## A4 global reservation freeze (Core source only)

The global Core-owned `AddonMediaImportMaintenanceFence` now records
administrator-audited, revision-fenced freeze/unfreeze state in the same
SQLite file as the existing durable import operation store. New
`media.import` reservations are checked *inside* the operation insert
transaction, across every backend; unrelated timer operations are unaffected.
The default remains the normal disabled Add-on contract. Missing schema
denies new imports, and previous exact operation replays are still readable.

This does **not** stop an already executing Agent process, grant an update,
install a package, or reconcile an unknown result. No public management route
is exposed. The read-only no-unresolved-import inspection, stopped Agent,
independently reviewed replacement bytes, package provenance and explicit
host approval remain necessary before any real package maintenance.

## A4 Agent child-lifetime drain fence (isolated source)

The fixed root-controlled
`/etc/vdr-suite/addon-approvals/rectools.import-running.lock` is a separate,
persistent, empty, root-owned and non-writable-by-others **run-lock inode**.
Before opting in, a source/manual or future Debian installation must
independently stage it under the same reviewed installation root; neither
the Agent nor a package action creates it on demand. Missing, symlinked,
writable, replaced or otherwise unsafe lock evidence denies Media Import
at both Agent capability discovery and actual handler dispatch.

A pinned Media Import executor holds a **shared advisory Linux flock**
from BEFORE its final administrator/manifest/authorization/source rechecks,
through `fork`, and across the imported executable's `fexecve` (including
its ffmpeg descendants). The unprivileged Agent retains its own shared FD
while waiting; the child explicitly inherits the lock FD even if its Agent
parent crashes. A future maintenance owner must obtain and **hold** an
exclusive lock on this exact protected inode throughout all changes to the
approved binary and its associated metadata, but only AFTER globally freezing
new imports, explicitly disabling the addon for all affected backends,
draining canonical `MutationOperation` and durable Agent assignments,
and independently verifying package provenance. An exclusive try-lock
without those other checks is NOT a safety proof or an update permit.

Disposable root-controlled CI fixtures exercise missing/writable/contended
locks, the real unprivileged Agent, and an ELF child that retains the kernel
lock after exec. No installer or package hooks currently acquire this lock;
the release maintenance protocol and full host-specific acceptance remain
unimplemented. No VDR data or actual recordings are modified by these tests.

## A4 global Core/Agent quiescence inspection (diagnostic)

A Core-only, admin-gated `AddonMediaToolsMaintenanceInspection` now
combines the global frozen-admissions revision, all backend activation
intents, all unresolved Media Import Operations, Agent assignments and
acknowledged results, residual Agent capability advertisements and the
root-owned exclusive handler lock. It refuses a positive diagnostic if a
state table is unavailable, an import is uncertain, an assignment has not
completed, an Agent still advertises the command, or another process
holds the shared execution lock. The source-only test uses disposable
root fixtures and a synthetic schema compatible with actual Agent rows.
The server also rechecks the durable freeze when an already assigned Agent
command seeks receipt authorization.

**Still not a maintenance permit.** The inspection releases the exclusive
lock immediately; no installer is coupled to it and a competing admin
could otherwise unfreeze. A final authorized package owner must hold
locks through the whole transaction, attest exact Agent state/database
identity, review release provenance and provide safe rollback. None of
this is enabled on a live host.

## A4 retained exclusive inspection handle (not installation)

The global Core/Agent maintenance inspector also has an optional
`inspectAndHold()` diagnostic. On success, the move-only result keeps
the *same root-owned* exclusive Media Import lock across C++ ownership
handoffs, rather than dropping it at the end of the snapshot. A moved-from,
released or shared lock cannot count as an exclusive hold. Root CI verifies
that concurrent imports cannot acquire their shared lock while the handle
remains held. Failures never return a usable hold.

This diagnostic does not freeze future administrator writes, and its
positive snapshot can become stale. It deliberately offers no package
operation, automatic unfreeze, handler replacement, rollback, runtime
activation or public API. The eventual Ubuntu maintainer must hold its
own exclusive maintenance lease through the independently authorized
upgrade/remove process and recheck Core and Agent state under durable
maintenance ownership.

## A4 durable ownership of a frozen maintenance window

The Core-owned global Media Import admission freeze now has a *separate*
additive SQLite maintenance-owner record. It may be claimed only by a
resolved administrator who already holds the exclusive fixed handler
lock and supplies the exact frozen revision. Core creates an unpredictable
128-bit ownership token, atomically audits the claim and increments the
freeze revision. As long as that record exists, even the original
administrator cannot unfreeze via the legacy API; another administrator
cannot acquire a replacement owner. Releasing ownership requires the
same actor, token, exact revision, exclusive lock and a successful audit,
and leaves admissions **frozen** until a separate intentional unfreeze.
If the owner crashes, its durable claim remains after the kernel lock
disappears, preventing silent takeover. There is no auto-expiry.

This is source-only coordination, **not** permission to install or
remove `.deb` files. A future authorized package transaction must
preserve the same exclusive handle, confirm current agent and database
state and signed package provenance, manage safe rollback, and define
a separate audited root recovery for abandoned owners before deploying.
The module remains disabled, with no live install, public endpoint or
implicit permission grant.

## A6 work in progress

The optional importer is a single C++17 implementation under this directory,
without a second repository or branch. The read-only canonical Agent bridge
now rejects non-dispatching operations, stale generation, missing Agent
instance/claim identity, mismatched request fingerprint and operations without
recording reconciliation policy. This is **not** a registered executor.

The import plan now records the source device, inode and nanosecond modification
time. A same-size replacement between `plan` and explicit `import`
is rejected before probing or remuxing; focused tests exercise this swap.
Source bytes are never deleted by this importer. The existing Agent command
inbox and durable receipts are integrated in isolated tests; productive
registration and real backend inventory/grant adapters remain gated.
No production capability is advertised.

### Immutable per-import media input

After validating a planned source's identity, the offline importer now
copies the opened bytes into a private, bounded staging snapshot on the
destination filesystem. Both `ffprobe` and `ffmpeg` consume this
same snapshot instead of reopening the source pathname. The copy is
checked for length and original file identity; the snapshot is removed
before atomic no-overwrite recording promotion. The caller-owned
source is never deleted. Focused tests confirm that a replaced
same-size file is rejected before probing, and that probing and
remuxing share the same private input. This does **not** substitute
for Core's canonical recording reconciliation and Agent job lifecycle.


## A6 isolated durable dispatch and pinned execution candidate

The existing Core Agent command inbox now accepts the strict opaque
`addon.media.import` assignment schema with `reconciliation_required`,
persists an accepted receipt, writes `starting` before invoking an explicitly
injected executor, and emits a durable unknown/unverified result for recording
reconciliation. Crash recovery never blindly runs a remux twice.

`PinnedMediaImportAgentExecutor` is the concrete add-on-owned Agent execution
implementation. It rereads root approval, retains the verified binary descriptor
from the common Core install snapshot, resolves only a local inventory token,
rechecks the effective backend-specific authorization hook, and invokes the pinned
descriptor without a shell with bounded child lifetime. No file paths are read
from the API command payload; the original recording is never deleted.

The trusted inventory and grant adapters and the server-side operation completion
are **not yet wired to production**. `apps/agent/main.cpp` deliberately injects
no executor or command advertisement. This is testable in isolated CI without
mutating yaVDR; it is not yet a product-ready Media Import feature.

The isolated A6 CI now also exercises `BackendAgentClientRuntime` itself with an injected synthetic executor: authenticated heartbeat, command poll, durable receipt, execution, result acknowledgement and subsequent heartbeat replay without a second dispatch. The shipped Agent still injects no Media Tools executor.

The existing Agent delivery service additionally refuses Media Import receipts unless its trusted `setMediaImportReceiptFenceCheck` verifies the exact reserved operation, effective grant and claim. The daemon now installs that callback using the real MutationOperation, immutable payload, account/grant, backend and Agent-capability authorities. Durable Agent results update the *same* MutationOperation to outcome_unknown or an appropriate verified rejection via a revision-fenced transition; neither remux exit nor Agent receipt marks an import succeeded. Production execution still remains disabled because apps/agent/main.cpp does not install or advertise an executor and productive backend inventory/grant adapters remain unavailable. Positive and negative tests operate exclusively on in-memory SQLite and synthetic Agent contexts.

The pinned executor snapshots the trusted local source/target resolution during authorization and rejects remapped inventory selections immediately before `fexecve`; CI explicitly tests the swap after authorization as a negative case.


### A6 authoritative recording readback (isolated)

The pinned handler's bounded stdout is parsed according to the actual C++17
`std::filesystem::path` quoted-output format. The Agent stores only a
SHA-256 correlation digest of the committed absolute `.rec` directory,
never the source or destination pathname, in the existing durable command
result. The protected local Agent state stores this identity in its scoped
version-4 Media Import representation, with compatibility for older command
state versions. A synthetic non-writing ELF fixture exercises this output
through the real pinned-fd execution path in CI.

The daemon reuses its *existing*, read-only, coalesced native Recording cache
refresh. Only a successful refresh that **started after** the Agent result,
matches the backend, contains exactly one matching VDR recording with
non-empty native identity and title, and matches the persisted command,
operation, and immutable payload may advance the shared
`MutationOperation` to `succeeded` by revision-fenced CAS. Empty, stale,
unmatched, ambiguous, mismatched-generation and replay cases remain denied
or `outcome_unknown`. An exact Agent result replay after `succeeded` can
be acknowledged without another dispatch or state regression.

A crash after native promotion but before a usable handler receipt can
remain `outcome_unknown` and requires independent operator reconciliation;
there is deliberately no blind import retry. Recording-cache path
normalization differences likewise fail closed rather than invent success.
**This is not production activation:** trusted local inventory/grant runtime
composition, final operator/readback acceptance and effective capability API
discovery still require further implementation and CI. No live VDR data is
modified by hosted tests.

### A6.1 local source mapping (default denied)

`RootOwnedMediaImportInventory` reads a *backend-local*, root-owned
`/etc/vdr-suite/addon-sources/media-import.tsv` (relative to its trusted
installation root). Each TSV row binds one exact backend ID, opaque source ID
(minimum 16 URL-safe characters), configured source root, VDR video root and
a source file. No payload-derived paths, scanning service or second database
are introduced. Missing, symlinked, writable, ambiguous, malformed or
unreadable mappings deny. Every selected entry passes the existing C++17
`inspectImport` checks and carries source inode, device, size and mtime
identity into the executor's second resolution before `fexecve`. The ledger
does **not** grant import execution; the current production Agent remains
unregistered/default-deny until effective server-side authorization is wired.
Only disposable, root-controlled CI fixtures are used by its tests.

### A6.2 effective Core permission revalidation (not activated)

`ReceiptRevalidatingMediaImportAuthorization` reads the existing durable
Agent inbox and replays its *previously acknowledged exact receipt* over the
existing authenticated Agent transport. The server reevaluates the canonical
operation, current backend-specific grant, live Agent identity/generation,
lease, claim and command. Only an exact successful receipt **replay** allows
dispatch; missing or inconsistent state, revocation, network or server failure
deny. The check occurs both before and after durable `starting`, before the
pinned handler is executed. It does not create a second permission database.
Production composition remains default-deny until the full activation gates.

### A6.3 server activation gate

The *same* Core `AddonActivationIntentRepository` now supplies a necessary
per-backend administrator desired-enabled condition to the **actual** media
import operation/receipt fence. Missing schema, absent row, disabled intent or
database failure denies before dispatch even when a user still has the
independent `addons.media.import` grant. It is not an execution capability
by itself. The isolated command-delivery test exercises default deny and
administrative disable/re-enable while retaining the existing grant tests.
No production Agent registration or handler activation is performed by this.

### A6.3 explicit production Agent composition

`apps/agent/main.cpp` now owns the real Media Import Agent composition:
root-controlled `RootOwnedMediaImportInventory`, effective Core receipt
revalidation and the Add-on's `PinnedMediaImportAgentExecutor`. The Agent
config parser still rejects `addon.*` in `COMMAND_TYPES`. Only an
**explicit** `--enable-media-import` runtime option installs this
composition; the packaged normal Agent starts without it and continues
default-deny. Even with opt-in, capability discovery requires a current
Agent identity/generation, trusted pinned install bytes and at least one
valid root-owned local source. At each command, the server also requires an
enabled audited per-backend administrator activation intent, effective
actor grant, live Agent, receipt and immutable operation. All requirements
are checked again before the handler runs. `Media Check` and `Shrink`
remain disabled.

The root-only A6.3 composition test uses the *actual* Agent capability
discovery function, the real protected local command inbox, the real
install-snapshot and root-ledger adapters, the new Core receipt-replay
authorization adapter and the Add-on pinned-fd executor. Its synthetic
handler is a copied `/bin/true` ELF; no VDR recordings are touched.
Negative cases include default missing executor, revocation, stale backend
generation, changed Agent instance between authorize/execute, writable
inventory, in-place handler byte tampering and a modified source before
execution.

### A6.4 contiguous real-Agent synthetic lifecycle

The isolated root-only composition test also drives the *existing* protected
Agent inbox, first authenticated receipt, two exact effective-grant replays,
durable `starting`, pinned `/bin/true` ELF execution, protected result
persistence, authenticated result acknowledgement and restart replay without
a second execution. Revocation immediately after receipt but before
dispatch denies, and recovery from persistent `starting` does not run an
importer. Every test uses disposable CI root fixtures and no real VDR
recordings. The real Core command-delivery test separately proves that only
fresh matching recording-cache readback can transition an operation to
`succeeded`. The explicit `--enable-media-import` option is not allowed
in `--once`, credential-rotation or native-probe modes.

### A7.5 capability-gated Webfrontend (source integration only)

The shipped Webfrontend's real `index.html` script composition loads
`modules/media-import.js` before `app.js`. The established backend
selector and `app.js` module-navigation owner show the initially hidden
**Media Import** tab only when authenticated
`GET /api/v1/backends/{backendId}/capabilities/media-import` returns
the exact backend, capability and `available:true`. Backend switches,
session expiry and logout invalidate that view; activation checks again.

The view lists only opaque `sourceId` tokens using the public v1
source-list route, refreshes the ephemeral list before submitting, and sends
only one `sourceId` with a cryptographically random, caller-owned
`Idempotency-Key` and the normal browser session CSRF header through
`POST /api/v1/backends/{backendId}/media-imports`.
It observes only the existing authenticated
`GET /api/v1/operations/{operationId}` resource, with bounded active-view
reads and explicit manual refresh. Core states including
`executed_unverified` and `outcome_unknown` are never represented as
success. Unknown POST results block a fresh request for that backend in
the current browser session; there is no automatic resubmission.
A new import is offered only after a terminal operation state.

This is **source integration, not a production release**. Only an explicitly
opted-in and trusted Agent, effective Core grant, activation intent and
independent native Recording readback can authorize and confirm execution.
No production frontend installation or host acceptance has occurred; Media
Check and Shrink remain disabled. The production composition and user-action
regression is `web/frontend/tests/test_media_import_composition.js`.

### A7.4 per-backend source list, no filesystem paths

An explicitly opted-in Agent reads its root-controlled TSV locally and
reports at most 32 freshly inspected opaque `sourceId` tokens via
authenticated `POST /api/agent/v1/observations/media-import-sources`.
Missing approval or an invalid inventory sends an empty snapshot. The daemon
holds this report only in RAM, bound to the current Agent actor, instance,
backend generation, active lease and a maximum 90-second age.

Authenticated `GET /api/v1/backends/{backendId}/media-import-sources`
rechecks the SecurityHttpGate discovery scope and Core's live
`addons.media.import` authorization/activation/Agent capability before
exposing the bounded IDs. Missing/expired observations yield 503, denied
users 403, unscoped backends 404. No paths or writable source claims are
returned. Actual import rereads and pins the local source from the
protected ledger and requires the canonical VDR recording readback.

### A7.3 protected Media Import admission

`POST /api/v1/backends/{backendId}/media-imports` accepts only
`{"sourceId":"opaque_source_123456"}` and a valid
`Idempotency-Key`. The common SecurityHttpGate requires
authenticated backend-specific `addons.media.import` authorization,
browser-session CSRF and audit. The Core admission service separately
reevaluates the effective grant, agent lease/capability, enabled
administrator intent and writable backend. It atomically reserves one
ADR-0042 `media.import` operation with immutable payload and
scope-specific idempotency. The daemon then invokes the *existing* system
authority Agent assignment path and returns the same v1 Operation resource
on retry. No source pathname, output pathname, arbitrary command body or
new media worker queue is accepted. A failing assignment can leave an
accepted, non-executed durable operation which the same-key caller may
safely resubmit; terminal operations are never reassigned.

A public POST is *not* proof of verified execution: the Agent needs
separate root-owned inventory selection and explicit opt-in. Completion
requires independent readback of the new VDR recording.

### A7.2 authenticated public API v1 capability read

`GET /api/v1/backends/{backendId}/capabilities/media-import` is a
read-only actor/backend-scoped view. The existing HTTP gate supplies
independently authenticated discovery scopes; an out-of-scope backend
returns 404. The daemon only delegates the live check to Core's
`effectiveMediaImportCapability`. An absent delegate or unavailable Agent,
activation or effective user permission yields `available:false`, not an
optimistic flag or information about internal rejection reasons. The route
never accepts a path selected by the caller as an import source, creates
no operation and performs no import. This is **not** a production launch.

### A7.1 authoritative backend capability discovery

`MediaImportOperationFence::effectiveMediaImportCapability` evaluates the
current nonrevoked Agent identity, live lease and exact published
`addon.media.import` capability **together with** the existing Core
user/service actor, backend-scoped effective grant, writable backend and
audited administrator enable intent. It shares the dispatch code's actor and
backend authority evaluator rather than duplicating a permission system.
Missing/stale/invalid input is default-deny. This is a read-only capability,
not a mutation or permission grant. The real Core operation/command fixture
covers the enabled, grantless, disabled, wrong backend and stale lease cases.
