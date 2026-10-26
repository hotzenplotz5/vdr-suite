# Optional Add-on Platform — Execution Roadmap

**Scope:** Non-numbered add-on productization workstream; this document does
not reopen completed VDR-Suite phases or authorize Phase 70.
**PR #551 source base:** Merge `ab8eb1f6990e9e0e24bdf5ee9ca0182368c9df0b` is in `main`.
The historical branch `work/addon-platform-registry-lifecycle` was recreated
at its exact merged PR #551 head for reference; it is **not** an install
source. MT.HA integration and acceptance changes use the single working branch
`work/mt-ha-media-tools-hardware-acceptance`; **no automatic merge, deployment,
activation, or PPA publication**. Verify actual `main` and docs/CURRENT.md
again before every source- or host-specific approval.

**Binding architecture:** [ADR-0068](../adr/ADR-0068-optional-addons-and-source-packaging.md),
[ADR-0037](../adr/ADR-0037-packaging-install-api-boundary.md) and the
[existing API/backend security owners](../development/current-architecture-state.md).

## Product boundary

The platform recognizes independent optional identities for **Media Tools (Rectools)**,
**Images** and **Music**, but **this roadmap's productive implementation and
acceptance scope is Media Tools only**. Images and Music are separate future
add-on workstreams, not milestones or prerequisites of Media Tools. Their
currently inert manifests remain independent package-contract fixtures.
TVScraper is **not an add-on**: its VDR plugin remains a native metadata
provider integrated through SuiteBridge and the Suite Core under
[ADR-0036](../adr/ADR-0036-tvscraper-recording-metadata-integration.md).

The existing browser UI module registry does not confer backend authority.
No add-on may self-register permission grants, HTTP routes, arbitrary code,
package install scripts or executable capability merely by dropping a manifest.


## MT.HA — manuelle Hardwareabnahme bis zur Launchpad-Freigabe (26.10.2026)

**Verbindliche Reihenfolge:** Erst rein lesende Inventur; dann überprüfte
manuelle Installation/Übernahme, echte Hardware- und Funktionsabnahme,
Fehlerbehebung, isolierte Regressionen, zweite Distribution, Update/Remove/
Recovery, Release-GO und **erst danach** Ubuntu-Quell-/Binärpakete.
`main` bleibt bis zur expliziten Mergefreigabe unverändert.
`AGENTS.md` bleibt unverändert. Alle produktiven Medienaktionen nur mit
isolierten Testdateien und gesonderter Freigabe.

| Slice | Abnahme und bisheriger Nachweis | Status |
| --- | --- | --- |
| **MT.HA.0 – Read-only Inventur** | GitHub-Verifizierung: PR #551 auf `main`; `main` war beim Abgleich inzwischen bei `f9ffb76094e8b3b37d7420e84b6d31827e21226f`. Auf yaVDR/Resolute sind Import-/Check-/Shrink-Worker, Root-Approval, Source-TSV und Agent-Drop-in bereits vorhanden; Agent ist `inactive/dead`, Suite und VDR sind `active/running`. Manuelle Suite-Dateien sind nicht durch dpkg verwaltet. Das neue `media-tools-install-inventory.json` wurde im übermittelten Hostinventar nicht gefunden; fehlendes Inventar/Herkunft müssen unabhängig geprüft werden. Live-DB mit WAL: keine ungeprüfte Migration oder reine Dateikopie. | **PARTIAL / PENDING**: Host-Quelle, Stage, Approval-Wirkung, Byteprovenienz, aktiver Datenbankpfad, Verbindungsstatus und Speicherreservierung noch offen. |
| **MT.HA.1 – Manuelle Installation/Übernahme** | Erstinstallation nur bei belegtem `absent`; vorhandene Media Tools erfordern geprüften Update-/Migrationspfad. Installer muss auf offiziellen `origin/main` statt historischem Entwicklungsbranch pinnen; bestehende Fremd-/Teilstaat-Sperren bleiben. Kein Build als root. | **PENDING**; keine Live-Ausführung freigegeben. |
| **MT.HA.2 – Hardwareintegration** | Dateimodi, Siegel, Handler-PIN, Registry, Agent-Enrollment, Core, API, Web, Grants und Zustände INSTALLED/CONFIGURED/ENABLED/ACTIVE/CONNECTED/AUTHORIZED/FUNCTIONAL unabhängig nachweisen; Dienst-/Boot-Test separat. | **PENDING** |
| **MT.HA.3 – Import/Check/Shrink** | Positive und negative End-to-End-Fälle, nur Testmedien, VDR-Readback, Fehler-/Abbruch-/Ressourcen- und Restartfälle; fehlende Funktionen als NOT IMPLEMENTED kennzeichnen. | **PENDING** |
| **MT.HA.4 – Root Causes/Regressionen** | Reproduzierbare Defekte auf genau einem Integrationsbranch beheben, fokussierte Regressionen und erneute Hardware-Reproduktion, ohne Umgehung von Approval/Fence/DB-Schutz. | **PENDING** |
| **MT.HA.5 – Distributionen** | Gemeinsame verifizierte Stage mit getrennten Hostprofilen für Ubuntu/yaVDR, Debian, MLD 6.6; zweite echte Distribution erforderlich. Keine aus Ubuntu erratenen MLD-Pfade/Dienste. | **PENDING** |
| **MT.HA.6 – Update/Remove/Recovery** | Reinstall, alte Version, synthetische P7/P8-Historie, aktive WAL/Backups, Abbruch/Rollback, Berechtigungswiderruf; MT.LC2–LC5 separat abnehmen. | **PENDING** |
| **MT.HA.7 – Technisches GO** | Matrix je Commit, Distribution, Befehl, Ist/Soll, PASS/FAIL/PENDING/NOT IMPLEMENTED und Restrisiko; GO nur bei vollständigen Pflichtnachweisen. | **PENDING** |
| **MT.HA.8 – Launchpad Paketierung** | Erst nach GO echtes optionales Worker-Paket statt `~scaffold1`, Noble/Resolute-ABI und Paketinhalte/Upgrade/Rollback, Source- und Binärbuild, Lint, Upload-Vorbereitung. **Kein Upload ohne separate Freigabe**. | **BLOCKED (MT.HA.7)** |

**MT.HA.0B – Operatorbefund, noch rein lesend (26.10.2026):**
Ubuntu 26.04.1 LTS, installiertes `vdr`/`vdr-dev`
`2.8.3-0yavdr0~resolute`. Der ursprüngliche Host-Checkout
`65335e6a5caec4ed05eb8202a1cd2d095cff22c0` steht auf
`work/public-v1-recording-actions`; **ihn nicht für MT.HA umschalten**.
GitHub bestätigt, dass dieser Checkout-Commit ein Vorfahre des zum
MT.HA-Branchstart gepinnten `main`-Commits
`f9ffb76094e8b3b37d7420e84b6d31827e21226f` ist. Git-Status
der bereits getrackten Hostdateien wurde leer gemeldet; das ist kein
Nachweis für den installierten Binärstand.

Die Root-Approval-Datei meldet `schemaVersion=1`, `version=0.1.0`,
`origin=manual`, `state=approved` und ihr Importhandler-SHA256
**stimmt mit dem vorhandenen Worker überein**. Das ist Konsistenz mit
einem Root-Beleg, noch **kein unabhängiger Build-Provenienznachweis**,
keine effektive Core-Berechtigung und kein erfolgreicher Auftrag.
Konfiguriert sind die Importquelle `/srv/video/Filme` und das
VDR-Ziel `/srv/vdr/video.00`; der Agent-`ExecStart` enthält
`--enable-media-import` und `--enable-media-shrink`, der Agent
selbst ist `inactive/dead`. Die leere Import-Running-Lockdatei
ist vorhanden (kein Nachweis, ob eine exklusive Sperre gehalten wird).
Das neue `media-tools-install-inventory.json` fehlt tatsächlich.

**Neue Blocker:** Der MT.I2-`live-update` verlangt exakt verifizierte
alte und neue versiegelte **vollständige** Suite+Add-on-Stages;
eine installierte ältere, ohne neues Inventar aufgebaute Media-Tools-
Runtime erfüllt dies nicht automatisch. Erst alte Stage/Herkunft und
sicheres Migrationsprotokoll klären, keinesfalls Approval-/Dateien
löschen, um eine scheinbare Erstinstallation zu erzeugen.
`/` hat etwa 45 GiB frei (218 GiB Dateisystem), `/srv` etwa
512 GiB frei (3,6 TiB FS). Beim gegenwärtigen Erstinstaller ergäbe
`WORKDIR` auf `/` aus summierten Budgets von 19 GiB sowie der
15%-Reserve von etwa 33 GiB eine Mindestfreiheit von **ca. 52 GiB**:
die aktuelle Erstinstallations-Ressourcenprüfung würde deshalb
voraussichtlich **FAIL** melden. `/srv` erfüllt bei 86% Belegung
die 15%-Reserve ebenfalls nicht, wenn es als Stage-/Backup-Speicherort
gewählt würde. Reserve niemals umgehen; ausschließlich lesend
ermittelte tatsächliche Werte und einen separaten angepassten
Install-/Wartungsplan verwenden. SQLite: etwa 3,74 GB DB plus
222 MB WAL; keine produktive Migration oder reine Dateikopie.

**CI-Grenze des MT.HA-Draft-PR:** Der isolierte neue
`test_media_tools_installer_provenance.py` wurde im
Packaging-Regression-Job am Stand
`bf87e2123136f2e56b6deca31d85b5ef31acd521`
ausgeführt, Packaging-Job `PASS`. Die vollständige
`VDR-Suite CI` war an diesem Commit jedoch `FAIL`,
und zwar in `test_backend_agent_osd_transport.cpp:367`
(`test_authenticated_vertical`, Assertion/abort aus
`test-phase68-osd-authenticated-transport`).
Ursache und Zusammenhang noch offen; **kein Gesamt-CI-GO**.
Der neue Dokumentationscommit verlangt ohnehin erneute exakte
HEAD-CI-Prüfung.

**MT.HA.0D – Tatsächliche Altversion nachgewiesen (26.10.2026, Operator-Ausgabe):**
`/home/yavdr/vdr-suite-media-tools.nKoDO2yg/candidate.txt` pinnt den
Quellcommit `79983f409ead2fbb159b05f1903c7e5d4160aeeb` vom
24.10.2026; die zweite Zeile stimmt mit dem gegenwärtigen
Root-Approval-Handler-Digest überein. Dieser Quellcommit ist durch
GitHub als Vorfahr des rekonstruierten PR-#551-Branch-HEADs
`99c228ff6e929f91c14cf558fb7f1fc7b45f834d` belegt.
Der zugehörige Suite-only-Stage meldet 91 Dateien (Schema 2),
die alte `stage-with-media-tools` 97 Dateien (Schema 2);
beide haben **noch kein neues
`media-tools-install-inventory.json`**. Bei der gezielten
Operatorprüfung meldeten **alle drei** alten Media-Tools-Worker
(`media-import`, `media-check`, `media-shrink`) sowohl
`STAGE=MATCH` als auch `LIVE=MATCH`; der Import-Worker ist
ebenfalls mit dem hinterlegten Root-Approval-Digest konsistent.
Das stellt eine belastbare Byte-Korrelation zur erhaltenen Stage
her, **noch keine vollständige Stage-Siegel-/Release-Signaturprüfung**.

**Separate Core-Version!** Die beiden Live-Binärdateien
`/usr/sbin/vdr-suite-daemon` und
`/usr/sbin/vdr-suite-backend-agent` melden
`LIVE=SIZE_DIFFERS` gegenüber beiden alten, stageintern
passenden Dateien. Deshalb ist die alte 97-Dateien-Stage **kein
bytegenaues Backup der heute installierten Gesamt-Suite**,
kann den bestehenden MT.I2-`live-update`-Vertrag mit
`check(old, /)` nicht erfüllen und darf keinesfalls als
vollständige Runtime zurückgespielt werden.

Ein zweites Verzeichnis
`/home/yavdr/vdr-suite-media-tools.NvHwafAY` pinnt
`1bc3232f4f5fe5a4daf42e108f76c6708398c402`, dessen
`candidate.txt`-Handler-Digest nicht mit dem aktiven
Root-Approval übereinstimmt; enthalten ist nur der
91-Dateien-Suite-only-Stage. Dieses Verzeichnis ist **nicht**
die nachgewiesene aktive Media-Tools-Basis.

**MT.HA.0E – Vollständiger Altstage-Audit auf yaVDR (26.10.2026):**
Der Operator hat den vorhandenen unveränderten
`stage-with-media-tools` mittels `load_manifest`,
`verify_sealed_stage` und `deployment_differences` aus
`tools/install_runtime_stage.py` geprüft.
`STAGE_INTEGRITY=PASS` für **97 Dateien und 34 Verzeichnisse**;
`LIVE_COMPLETE_MATCH=False`; **17 Live-Abweichungen**:
15 `content`, 2 `missing`. Abweichende Binärdateien:
`vdr-suite-daemon`, `vdr-suite-backend-agent`,
`vdr-suite-backend-agent-admin`,
`vdr-suite-backend-agent-command-admin`,
`vdr-suite-backend-agent-enroll`; dazu zehn abweichende
Frontend-Dateien einschließlich `index.html`, `app.js`
und `public-v1-client.js`. Die zwei auf dem Host
fehlenden Frontend-Dateien:
`usr/share/vdr-suite/web/frontend/modules/media-import.js`
und `usr/share/vdr-suite/web/frontend/settings-addons.js`.
Beide sind weiterhin **quellseitig auf `main` vorhanden und
vom `mk/install.mk`-Staging vorgesehen**, und die aktuelle
Quell-`index.html` referenziert sie.
Ob auch die installierte `index.html` diese Skripte
lädt, ist separat rein lesend zu klären. Keine Aussage
über aktive Browser-Funktionsfähigkeit ohne HTTP-/UI-Test.

**Grenzen:** Diese interne Siegelprüfung ist keine externe
Releasesignatur. Die alte Gesamtstage entspricht **nicht**
der heute installierten Gesamt-Suite und darf weder per
`deploy-install-runtime` überlagert noch als Gesamtsystem-
Rollback oder `live-update`-Ausgangsstage missbraucht
werden. Die zuvor nachgewiesenen drei passenden alten
Media-Tools-Worker bleiben unverändert. Für die
ggf. unvollständige Frontend-Installation braucht es einen
gesonderten aktuellen-Commit-/Stage-Abgleich und einen
isoliert geprüften Deployment-/Recovery-Plan mit separater
Betreiberfreigabe.

**Nächste Pflichtprüfung:** Referenzen der **installierten**
`index.html` auf die beiden fehlenden Module sowie den aktiven
Frontend-/Runtime-Quellstand rein lesend nachweisen. Danach
komponentenscharfen, rückrollbaren **Legacy-Adoption-/Migrationsvertrag**
für die vorhandenen Worker/den aktuellen Suite-Binärstand entwickeln und
isoliert testen. Nichts auf dem yaVDR deployen, keine laufenden
Dienste stoppen, kein ungeprüftes Gesamt-Suite-Rollback.

**Aktueller Hostbefund ist keine neue Erstinstallation.** Der bisherige
`install_media_tools.sh` ist ein Ubuntu/yaVDR-Serviceadapter und prüft
bewusst fehlende Media-Tools-Dateien; bereits vorhandene Approval-/Worker-
Dateien sind kein Anlass, diese Schutzgrenzen zu übergehen. Die zwei
`addon.json`-Kopien stimmten beim Hostinventar in SHA256 überein,
nicht aber automatisch mit einer freigegebenen Stage-/Manifestversion.

## MT.LC — vollständiger Add-on-Lebenszyklus (Freigabearbeit)

**Zielvertrag (Maintainerentscheidung 26.10.2026):** Media Tools ist ein
**optional installierbares und entfernbares Add-on**, nicht bloß ein fest
verdrahteter Menüpunkt. Main enthält die gemeinsam benötigten Backend-,
Sicherheits-, API-v1- und Agent-Verträge; die drei ausführbaren
Media-Tools-Worker bleiben außerhalb des Basis-Installationspayloads.
**Source-Merge und Produktivaktivierung sind getrennte Entscheidungen.**
Keine neuen Entwicklungsbranches, Änderungen an `AGENTS.md` nur mit
ausdrücklicher persönlicher Maintainerfreigabe, kein ungeprüfter
yaVDR-Eingriff. Die gezielten AGENTS-Widerspruchskorrekturen wurden am
26.10.2026 ausdrücklich genehmigt; das ist keine Dauerfreigabe.

| Slice | Umsetzungsziel und Nachweis | Grenze |
| --- | --- | --- |
| **MT.LC0 – Inventar/Ownership** | Bestehende manuelle Stage, dpkg-Paket, Handler-Digest, Backend-Zuordnung und aktivierte Absicht über bestehende Registry-, Preflight- und Trust-Owner trennen; Status mindestens not_installed, installed_disabled, active, unavailable_or_partial, migration_required und unknown. Jeder Status braucht **unabhängige Backendbelege**, nicht bloß einen Aktivierungswunsch oder Frontend-Manifest-Eintrag. | Ein nicht verifizierter Provider darf im UI **nicht** als sicher nicht installiert bezeichnet werden. Remote Backend ohne vertrauenswürdige Inventurdaten meldet unknown. |
| **MT.LC1 – Geschützter Status im Web** | Administratoransicht für vorhandene/nicht vorhandene Module; wirksame Aktivierung einer installierten Version und lediglich gespeicherten CAS-Intent unterscheiden. Media-Import-Menü/Aktionen nur bei effektiver, autorisierter Capability anzeigen. Tests für fehlende Installation, verlorenen Agent-Lease, fehlende Berechtigung und deaktivierten Handler. | Die bisherige Browser-Aktivierungs-API installiert und entfernt **nichts**. Kein falscher Installieren-Button. |
| **MT.LC2 – Install/Upgrade** | Vorhandene versiegelte, unabhängig freigegebene Stage oder das optionale .deb wiederverwenden; **eng begrenzter privilegierter Backend-Ausführungsweg** nur nach geprüftem Paketbesitz, vollständigem Preflight, verifizierter Herkunft, Space-/DB-/Rollback-Plan, Wartungsfence und gesonderter Administratorbestätigung. Kein Browser-supplied Dateipfad, Paketname, Shell oder Kommando; keine Vermischung manueller Stage mit dpkg. | Der Suite-Daemon (auch wenn der Host ihn als root betreibt) und der unprivilegierte Backend-Agent dürfen **nicht** durch HTTP zu einem allgemeinen Root-/dpkg-Executor werden. Ohne freigegebenen privilegierten Owner nur sichere Vorschau, keine produktive Mutation. |
| **MT.LC3 – Deaktivieren/Entfernen** | Zuerst neue Aufträge sperren, laufende/ungewisse Imports und Shrinks drainen oder **fail closed**, Root-/Core-Handlerfreigabe widerrufen und Status abgleichen. Dann genau die unabhängig verifizierten Paketdateien entfernen: dpkg-installiert über Paketverwalter, manuell nur gegen versiegeltes Besitz-/Digest-Inventar und ausdrücklichen Wartungs-/Rollback-Vertrag. | **Nie** Aufnahmen, manuelle Metadaten, DB, Quellmedien, fremde Dateien oder unbekannte Zustände mitentfernen. Purge/Zustandslöschung sind eigene ausdrücklich bestätigte Entscheidungen. |
| **MT.LC4 – UI-Lifecycle** | In der Administratoransicht Installieren, Aktivieren, Deaktivieren, Aktualisieren und Entfernen **nur** für nachgewiesen zulässige Zustände und vorhandene sichere Backend-Aktionen anbieten; konkrete Vorschau, zusätzliche Bestätigung, Operation-ID, Fortschritt, Fehler, Readback und Rollbackstatus zeigen. | Kein Button ohne dahinterliegende autorisierte, getestete Aktion. Remote Agent ist kein Paketmanager. |
| **MT.LC5 – Abnahme** | Gefährdungs-/Negativtests (CSRF, Actor-/Backend-Scope, Paketfälschung, manuell↔dpkg-Kollision, SQLite/P7, Agent-Lease, laufende Jobs, Abbruch, Downgrade/Rollback); exakte HEAD-CI; isolierte Debian-Lifecycle-Tests und **separat** echte yaVDR-Abnahme MT.1–MT.5. | Source-Merge nur bei abgeschlossener Code-/Sicherheitsprüfung; reale Paketänderungen, Import, Shrink und Merge nach main erfordern jeweils ausdrückliche Betreiberfreigabe. |

**Iststand:** Die Web-Einstellungen unterstützen eine geschützte
Aktivierungsabsicht und Quellenkonfiguration, jedoch **keinen**
sicheren Web-Paketinstaller oder -Deinstaller. Die vorhandenen Worker sind
optional paketierbar; Daemon-/Agent-Grundlagen bereits im Suite-Build.
Die aktuelle Source-/Web-Korrektur führt **keine** privilegierte Ausführung
ein. MT.LC2–LC5 dürfen nicht als PASS gelten, bevor ihre jeweiligen
Akzeptanznachweise vorliegen.


### MT.LC – Backend-/Merge-Vorprüfung (26.10.2026)

PR #551: sechs GitHub-Workflows am bisherigen Stand
b5cd57e053be41faff45405055b37a0ed2da8df7 erfolgreich; **nach jeder
weiteren Änderung muss die neue HEAD-CI erneut geprüft werden**.
Der native TV-Medienheader-Commit c7cb87f7d95f4a7a32733f61c40d320081619e48
wurde nach Prüfung der drei nicht überlappenden Dateien mit Merge-Commit
1d6b634ed91294915940ac10d5c652644a1bcff3 in den bestehenden
Add-on-Branch übernommen. GitHub verifiziert damit **0 Commits hinter main**
und keinen Mergekonflikt. Der neue exakte HEAD benötigt trotzdem seine
eigene Gesamt-CI.

**SQLite/P7-Grenzen:** Vier neu hinzugekommene DDL-Owner sind vom historischen
P7-Versuch noch nicht abgedeckt. Die vorhandenen, jetzt erweiterten
Core-Tests verifizieren für AddonActivationIntentRepository und
MediaShrinkReviewRepository jeweils das echte SQLite-Backup, wiederholtes
ensureSchema und den Erhalt vorhandener Daten. Die Agent-DDL-Index-Owner
BackendAgentMediaImportAssignment und BackendAgentMediaShrinkAssignment
benötigen eigene historische Upgrade-/Row-Preservation-Nachweise.
Alle vier bleiben im P7-SQL-Inventar als PENDING eingetragen, bis die
entsprechende historische und Paket-Migrationsabnahme tatsächlich vorliegt.
DevicePlaybackAccountBindingService auf main bleibt unabhängig P7 PENDING.

**AGENTS.md-Grenze:** Der PR enthält geänderte und zusätzliche AGENTS.md.
Der Maintainer hat ausdrücklich die Korrektur der branch-fixierten
Media-Tools-Regel und der impliziten Pflicht zu automatischen AGENTS-Edits
genehmigt. Die allgemeinen Sicherheits-/Core-Regeln bleiben bestehen.
Images und Music behalten als inaktive Modul-Scaffolds ihre laut
`addon_contract.py` erforderlichen AGENTS.md-Dateien mit den übergreifenden
Schutzgrenzen; ihre spezifische Architektur- und Produktgestaltung ist in
eigenen späteren Modul-Workstreams zu überprüfen. Das autorisiert weder
künftige AGENTS-Edits pauschal noch den Merge nach main, Live-Installationen
oder Paketänderungen.

## Actual install and packaging boundary

The current yaVDR has VDR-Suite installed by the existing conventional
source/manual deployment procedure, **not** by an installed VDR-Suite Debian
package. Its working SuiteBridge/VDR plugin and metadata functions must
continue to work unchanged. Add-on productization must support this mode
without pretending that a manually installed Suite is present in the dpkg
database.

Three independent concepts must never be conflated:

- **Source/manual deployment:** a later reviewed core/Agent handler with a
  trusted administrator-installed binary, explicit fixed module identity,
  version, provenance/pinned digest and lifecycle ownership. The present
  standalone C++ executable and metadata staging are **not** yet such a
  trusted/runtime-registered installation.
- **Debian/Ubuntu packaging:** package-building tools and daily builds
  already exist as development/release infrastructure. They do not prove that
  the current host has installed any particular package or binary.
- **Optional dpkg inspection:** `installed_registry.py --verify-dpkg`
  compares local dpkg ledger entries only when explicitly requested. This is
  a non-authorizing package diagnostic for the future Debian deployment
  path, **not an activation prerequisite for manually installed Suite**.

Both eventual deployment modes must enter the **same** core-owned handler,
permissions, backend fencing and operation/audit contract. No manifest path
or metadata-only presence may bootstrap trust.

## Milestones and acceptance gates

| Milestone | Product outcome | Source evidence at this checkpoint | Completion gate |
| --- | --- | --- | --- |
| A1. Optional source contract | Three independent, disabled manifests, module rules, isolated source staging and .deb scaffolds | Implemented; isolated packaging/tests accepted before TVScraper removal | Build/test all three only; no implicit base install |
| A2. Add-on inventory and discovery | Read-only core allowlist and strict metadata parsing, independent of installed VDR-Suite packaging | Metadata-only registry implemented; synthetic dpkg diagnostics added | Three add-on IDs only; no activation; default scan works without dpkg; optional ledger regression separate |
| A2M. Manual-install registration path | Explicit safe declaration of a source-built, administrator-installed and integrity-pinned backend handler | **Partial:** root-admin issue/revoke, native approval/descriptor and version-matched installed manifest snapshot, nonroot Agent-file-access CI; no automatic activation; disabled Core registry definition | Fixed reviewed module-to-binary mapping, version and digest, secure owner/mode checks, revocation/uninstall detection; no caller-selected executable |
| A3. Core access/desired-state preview | Canonical actor/backend policy, revision-fenced SQLite intent, read-only eligibility preview | C++ isolated checks 21 + 17 + 20 accepted before TVScraper removal | No execution permit; denied by default and on any lost evidence |
| A4. Trusted runtime provider | Common core authority over either reviewed manual-install or reviewed Debian package provenance, with fixed binary identity and healthy backend-local handler | **Partial:** disabled Core admission, pinned identity/generation, approval comparison, root-owned installed manifest/revocation checks and unprivileged Agent-path CI; no package-derived Core execution permit. The actual Agent executor now re-hashes the pinned descriptor before exec, under its shared maintenance lock | Both paths use the same registry, signed/admin-approved provenance policy and negative trust tests; neither dpkg alone nor source files confer authority |
| A5. Administration and operation rights | Admin enable/disable with audit/idempotency/migrations; scoped Media Tools grants | Durable audit/CAS and exact backend-scoped import/shrink grants exist; HTTP grant eligibility checks live Agent capabilities, exact-revision intent no-ops avoid duplicate writes, and desired-enable is now atomically blocked by the global Media Tools maintenance freeze (including cross-connection tests). **Desired-enable is now additionally CAS/audit-bound to the exact Agent identity, instance and backend generation. Legacy/unbound enabled rows and later Agent enrollment changes cannot satisfy Import/Shrink operation fences or non-executable admission preview. This does not itself authenticate the provider. Admin enable remains deliberately denied until trusted Core provider and authenticated activation route are integrated and verified.** | Proper grant vocabulary, authorization, CSRF, preconditions, revocation and upgrade/removal semantics |
| A6. Protected backend execution | Fenced command/worker, bounded resources and crash-safe durable jobs | **Source-integrated, not host-accepted:** canonical operation fences, authenticated Agent-poll and receipt revalidation, optional pinned Import and explicitly opted-in HEVC Stage/Publish executors, durable once-only recovery and child-lifetime maintenance locks. No default add-on activation. | Agent generation and job fencing, cancellation/retry/readback, no arbitrary paths or shell |
| A7. Public API and client UI | Entitlement-filtered discovery, safe admin surface, addon action/status | **A7.1–A7.5 isolated source implementation and canonical install staging validated; frontend owner-handoff/late-response regressions included. Not deployed.** Capability, opaque source listing, protected admission and actor-owned Operation GET reuse Core and canonical frontend. | Versioned v1 routes, grants/session/CSRF tests, actual UI composition, source expiry and uncertain-state checks; safe install staging and separately authorized host acceptance before production |
| M1. Media Tools import | Single-file safe remux to VDR recording | Source-integrated C++ remux/transcode with bounded IO, approved source roots, pinned Agent worker, protected staged VDR result and Core reconciliation; full installed-backend acceptance remains open | Fix missing TOCTOU/fd, timeouts, crash recovery and recording reconciliation before live writes |
| M2. Media Tools Check | Read-only FFprobe/FFmpeg recording integrity report | **M2.0–M2.2 source-integrated:** bounded read-only TS/FFprobe/FFmpeg checker, canonical recording-ID resolver, backend-local daemon job and actor-owned API/status/Recording UI; remote Agent jobs and installed host acceptance remain open | No markers, messages or recording-file writes; bounded parser/job/tests |
| M3. Media Tools Shrink | Optional space reduction preserving original until separately verified | **Source-integrated, not runtime-accepted:** C++ stage/review/publish, Core Operations, Agent opt-in, API and Recording Hero exist; HTTP grant eligibility is wired; the global maintenance freeze covers both HEVC actions; a child-inherited package lock protects ffmpeg, and Stage records the concatenated original duration, and Publish re-probes the unchanged multi-segment source via a read-only concat: TS stream instead of summing segment durations; tests cover discontinuities and mismatched source duration. Newly added tests remain source-level; trusted administration and real backend acceptance are still open | Explicit consent, staged output/verification, extra VDR recording reconciliation and original preservation; no silent replacement |

**A1–A3 are foundations, not proof of a usable add-on manager.** The
non-executing admission preview may report synthetic policy eligibility but
its `executable` field always remains false. A consistent local dpkg ledger
is **not** cryptographic package provenance: dpkg's local database and MD5
records can be changed together. Do not derive `packageTrusted` or effective
capabilities from that diagnostic.

## A2M/A4 installed-metadata binding checkpoint

The Core/Agent install snapshot now requires an existing, root-controlled
`/usr/share/vdr-suite/addons/rectools/addon.json` in addition to the
independent administrator approval and descriptor-pinned fixed handler.
The disabled schema-v1 manifest must match the approved module, package and
version and may declare no entrypoint, capability, permission or dependency.
Missing, replaced, symlinked, writable, malformed or version-mismatched
metadata denies a fresh snapshot. A compiled, reviewed handler protocol
version (`0.1.0`) also rejects unreviewed binary upgrades even when their
manifest and administrator approval agree. For a separately reviewed
same-version binary replacement, the root-only approval issuer now supports
an explicit conditional `rotate`: it checks the old approved SHA-256,
matches the new independently approved digest against the installed binary,
takes a directory lock and atomically publishes the new root-owned record.
A revoked record cannot be rotated back to approved, and reinstalling the
old executable cannot resurrect a revoked approval. This is an isolated
operator lifecycle primitive, **not** a package maintainer hook or automatic
upgrade. Approval and metadata are re-read after binary hashing; the pinned
executor also checks them again immediately before dispatch. The
Agent's Import executor now re-hashes the **already pinned open file
descriptor** under the shared maintenance lock just before executing it,
rejecting in-place binary modifications after authorization without reopening
the executable pathname. Targeted isolated regression tests cover altered
bytes and descriptor identity; no host acceptance is claimed.
Root-owned, nonroot-Agent and synthetic command lifecycle tests use
disposable fixture installations. This is the same installation contract for
source/manual staging and future Debian packaging, **not** permission or a
release-signature substitute. Production activation, upgrade choreography,
installed-package verification and real-host acceptance remain open.

### A5 Agent-lease activation binding (source integrated)

Explicit desired-enable now persists an additional, exact Agent identity,
instance and backend-generation binding at the administrator's current
CAS revision. The binding is written/deleted atomically with the intent and
accountability event. Reasserting enable for a *different* enrolled Agent
requires a new revision and an audited administrator action, even when the
desired boolean remains true. Import, HEVC Stage/Publish and the read-only
Core admission preview reject bindings that do not match the current Agent.
Pre-migration enabled rows lack this binding and therefore deny execution,
but remain readable and explicitly disableable. The new binding is NOT
proof of installation provenance or package trust; only independently
approved provider evidence may create a productive enable.

### A2M/A4 remote-backend trust gap (now guarded in Core, acceptance pending)

The installed snapshot from `inspectMediaToolsInstallSnapshot("/")` is a
**local filesystem observation only**. Core must NOT treat a binary
installed beside the daemon as proof of the package on a remote backend,
even when that backend's Agent advertises the corresponding command.
The productive provider must obtain authenticated backend-local provenance
bound to the administrator-approved digest, version, revocation state,
backend generation and exact Agent handler lease; Core must recheck it at
dispatch. Capability advertisements alone are insufficient. The Agent-side dispatch
now additionally rechecks backend generation and Agent instance immediately
before fexecve; this is an independent local fence, **not** Core trust in a
remote installation. Until the server can bind authenticated backend-local
provenance to independently reviewed administrator approval, desired state is
not effective activation, and the
administrator enable API remains gated.

## A4 read-only package maintenance admission checkpoint

The new `AddonMediaToolsMaintenanceReadService` queries the **existing**
Core `AddonActivationIntentRepository` and `MutationOperationRepository`
for one exact backend. It denies a maintenance snapshot unless an
administrator explicitly disabled the module with a durable revision and
no `media.import` operation remains in `accepted`, `queued`,
`dispatching`, `executed_unverified` or `outcome_unknown`.
Missing schemas and saturated operation listings deny as well; an unknown
outcome is never silently considered resolved. Isolated tests use real
in-memory SQLite and exercise cross-backend scoping and large listings.
This is **diagnostic only**: it is not an exclusive admission/Agent drain
lease, does not stop new work, and cannot authorize package updates,
uninstalls or re-activation. Before any genuine release workflow, Core
must coordinate a durable no-new-admission fence, already executing
Agent commands, authenticated effective rights, all relevant backends,
package provenance, rollback and audited administrator intent.

## A4 durable global Media Tools reservation freeze (source integrated)

The Core `AddonMediaImportMaintenanceFence` persists a **global** freeze
for the fixed `rectools` Media Import, HEVC Stage and HEVC Publish
action families in the *same* SQLite authority as ADR-0042 operations. A global administrator changes its state
only through a revision-fenced, audited SQLite write transaction. The
canonical `MutationOperationRepository` now checks the freeze inside
the SAME `BEGIN IMMEDIATE` transaction used for all three Media Tools
reservations, across all backends and independent SQLite connections.
New Media Tools operations cannot slip between preflight and reservation;
existing exact idempotent replays remain readable. The administrator
desired-enable CAS checks this freeze in its own serialized write transaction;
reaffirmed enable is blocked, but explicit disable remains possible.
Uninitialized/missing or invalid gate schema fails closed for all three
actions and for desired-enable, while
unrelated timer operations keep their original semantics. An explicit,
audited revision-fenced unfreeze changes only the reservation gate, never
administrator desired-enabled state or executable permissions.

This is **not** an authorized package upgrade, exclusive Agent child drain,
or a live maintenance command. The earlier read-only diagnostic must
still show explicitly disabled activation and zero unresolved operations;
in-flight Agent work, complete multi-backend readiness and trusted package
provenance must be checked separately. No public API or startup activation
is added in this slice.

## A4 Agent child-lifetime drain lock checkpoint

A fixed, root-controlled
`/etc/vdr-suite/addon-approvals/rectools.import-running.lock` inode
complements the global Core SQLite admission freeze and read-only durable
operations inspection. Media Import and HEVC Stage/Publish now refuse to run if the protected
lock inode is missing or unsafe. Import holds its shared `flock` through
handler `fexecve`, and the HEVC Agent wrapper passes the same protected
lock into FFmpeg/VDR `execv` children. An Agent crash must not silently
release the advisory lock while any writing child still lives. Disposable CI fixture
tests include an exec'd non-recording child and a blocked exclusive
maintenance acquisition. This change is source-only: the production install
has no automatic lock creation and the default-disabled module remains
unchanged. A future package/source upgrade owner must explicitly stage,
lock and hold the exact inode while it performs a separately approved
transaction, plus freeze new operations, await authoritative job and
Agent reconciliation across backends, validate the package/signature
provenance, and handle rollback safely. None of these later package
actions is activated by this checkpoint.

## A4 combined global maintenance inspection checkpoint

The internal `AddonMediaToolsMaintenanceInspection` now combines an
exclusive, fixed-inode Media Import handler-lock probe with one coherent
SQLite `BEGIN IMMEDIATE` snapshot of the actual Core freeze, all stored
`rectools` desired-state rows, all unresolved `media.import` Operations
(without the earlier 256-result ceiling), Agent command assignment states,
missing persisted Agent results and remaining Media Import capability
advertisements. Missing schemas/unsafe lock/unknown Agent states all
deny. Root fixture CI exercises the genuine activation, operations, audit
and freeze repositories plus **synthetic Agent schema rows** matching the
actual Agent command owner. The separate server-side
`MediaImportOperationFence` also now rejects frozen imports at final
receipt/capability revalidation, not only at new reservation time.

This diagnostic **does not** grant an install token, retain the lock after
returning, prove that every runtime Agent owns the same state file, revoke
current actor grants, validate Debian/PPA release provenance, or perform a
rollback-safe package transaction. A future authorized maintenance owner
must acquire and HOLD exclusive locks, ensure a durable freeze throughout
the update, bind all database/Agent identities, independently validate
signatures, and explicitly manage rollback/unfreeze. This is not a live
host acceptance or package-installation release.

## A4 retained exclusive maintenance diagnostic (source only)

`AddonMediaToolsMaintenanceInspection::inspectAndHold()` now returns a
move-only diagnostic object that retains the same verified **exclusive**
`rectools.import-running.lock` kernel lock after the global Core/Agent
quiescence snapshot returns. Its `holdsExclusiveLock()` proof is false
after move-from, after release, and for a non-exclusive lock mode.
`inspect()` keeps its old point-in-time contract and releases the lock
immediately. Root-controlled CI checks held-lock contention, transfer,
release, denied diagnostics and absence of leaked locks; no package
files or media are changed.

**No maintainer operation is authorized by this result.** Administrator
intent, global SQLite freeze, Agent command evidence and package provenance
can change after the snapshot, even while the handler inode remains locked.
Before an actual Ubuntu `.deb` upgrade/removal a separately designed
package transaction must own the lock for its *whole* lifetime, serialize
freeze ownership and revalidate current durable state and trusted release
bytes under that ownership, then handle rollback safely. Lock inode
replacement, privilege, host runtime and real Debian maintenance hooks
remain unaccepted.

## A4 durable exclusive maintenance owner checkpoint

An additive `addon_media_import_maintenance_owners` SQLite table now
records a *single global* Core-controlled maintenance owner independently
from the existing frozen-admission revision row. Once the fixed module is
globally frozen, a resolved administrator may acquire durable ownership
only with the **currently held exclusive root-controlled handler lock**
and exact optimistic freeze revision. The private Core method generates
a fresh 128-bit opaque token from the OS CSPRNG, increments the shared
revision and commits the owner plus an Accountability event in the same
`BEGIN IMMEDIATE` transaction. It returns that token once to the
authorized coordinator, never through `read()` or the public API.

While ownership exists, **all** legacy `setFrozen` calls, including
attempted unfreeze by another administrator or the original owner,
are denied; another claim also fails. Release requires the original
administrator, token, exact revision and a still-held exclusive kernel
lock. It is separately audited and **leaves the global freeze active**.
A failed audit must roll back both owner and revision changes.
Crashes do not automatically clear the owner, even after a kernel lock
is released; there is deliberately no timeout, implicit steal, automatic
recovery, package hook or live maintenance activation. The isolated
root fixture checks cross-connection behavior, owner mismatch, replay,
audit rollback, clean release and persistent crash denial.

This is **not** the Ubuntu package transaction yet: obtaining and holding
the lock while applying exact signed artifacts, validating Agent/DB
identity after restart, implementing a safe audited recovery of an
abandoned owner and rollback under that ownership are still required.
No VDR service or real recording is touched.

## Recommended execution order

1. Finish A2's **no-dpkg default** and rejection regressions, using only
   isolated test roots. The optional dpkg fixture test is future-package
   compatibility evidence, not a current yaVDR acceptance requirement.
2. Design/implement A2M for the actual manual/source deployment: locate
   independently approved installed handler ownership and immutable binary
   identity. Do not infer trust from a checkout, staged manifest or arbitrary
   administrator-provided executable path.
3. Unify A2M with A4's package pathway under **one** core-owned provider
   identity and handler trust check. Keep daily-build and PPA/deb acceptance
   separate from the manually installed host until packages are installed.
4. Finish A5 before runtime execution: exact grants, audit, durable admin
   intent, revocation/update reconciliation and explicit opt-in.
5. Integrate a single controlled Media Tools import with A6: constrained
   source roots, staging, durable jobs, VDR indexing/readback,
   recording reconciliation and no source deletion.
6. Do not install or enable A7 API/UI in production until separately authorized controlled real yaVDR acceptance; source-level client integration and isolated CI may precede deployment.
7. Finish read-only M2 Check and non-destructive M3 Shrink with Media Tools
   installation/Agent/runtime acceptance. Images and Music are independent
   future workstreams and must not gate this rollout.

## Validation, release and safety boundaries

- Offline targets (separate focused groups, not an instruction to rerun all
  of them after each doc change): `check-addon-contract`,
  `check-addon-registry`, `check-addon-deb`,
  `test-addon-media-tools-package`, `test-addon-media-import`,
  `test-addon-access-policy`, `test-addon-activation-intent`,
  `test-addon-admission-preview`; use an isolated workspace and an exact
  candidate. Never inspect the host's dpkg database as part of routine tests.
- Never run prototype `import ... --confirm-writes` against an installed
  VDR recording tree. No add-on installation, service restart, or database
  migration without explicit separate real-host authorization and preflight.
- Every runtime slice must reuse existing Suite-owned actor/session/CSRF,
  backend authorization and Agent fencing, accountability, job state and
  recording-cache reconciliation. A successful local C++ test is not live
  backend acceptance.
- Update [installed-registry contract](../development/addon-installed-registry-contract.md),
  [source packaging](../development/addon-source-packaging.md),
  [Media Tools audit](../development/rectools-cpp-import-audit.md) and the
  affected module's README/AGENTS/manifests at coherent milestones.

## Gate before productive `vdr-suite-addon-media-tools` work

Do **not** equate an inert C++ import preview or a successful
`ProviderBinding` validation with readiness to enable Media Tools.
Implementation may proceed on the existing add-on branch, but the product
integration cutover is gated on concrete reviewed evidence:

1. **Core provider:** manual and Debian paths supply independently verified
   approval, exact binary/version identity, revocation and backend-local
   provenance; no caller-created `RuntimeEvidence.packageTrusted`.
2. **Administrative lifecycle:** canonical global-admin checks, audit and
   idempotent revision-fenced enable/disable, explicit removal/upgrade
   invalidation and durable backend-scoped state.
3. **Operation authorization:** the established grant administration
   supports an exact backend-scoped import permission; generic administrator,
   recorder or device rights cannot substitute.
4. **Backend agent:** one registered command handler with descriptor/path
   confinement, generation/lease fencing, deadline and resource limits,
   cancellation and durable outcome reconciliation.
5. **Public API:** versioned protected endpoints, CSRF, account/device scopes
   and capability discovery derived from effective backend state.
6. **Media acceptance boundary:** no source deletion, no legacy worker,
   verification of created VDR recording/index, capacity preflight,
   authoritative reconciliation, and crash/duplicate tests.

Until all six are satisfied and focused isolated/host-specific checks pass,
keep `AddonHandlerRegistry::kMediaToolsCandidate.executable=false`,
`RuntimeEvidence.packageTrusted=false` in diagnostic adapters, and the
Media Tools manifest without executable capabilities. Use existing Suite
security, operation/job, Agent and frontend owners; do not fork them.

## Media Tools A6 code ownership

New add-on-specific A6 contract and tests live under
`addons/media-tools/` on the **existing** add-on branch.
There is no new branch or repository. The existing standalone C++17
importer under `modules/rectools/` is not duplicated; a future
single tested source/packaging migration must move it rather than fork it.
The add-on request envelope is non-executing and does not replace the
Core-owned command, permission or operation lifecycle. Runtime integration
and API effective capability projection remain open.

## A6 implementation location (same branch, no repo fork)

The existing standalone C++17 media importer was **moved** (Git blobs
retained, old implementation paths removed) from `modules/rectools/`
into `addons/media-tools/include/`, `src/` and `tests/`. No
duplicate implementation exists. The old module directory owns only
the existing disabled metadata manifest and its staging/packaging
documentation. All new Media Tools-specific code is under
`addons/media-tools/`, while Core/Agent continue to own execution,
authorization, backend generations, jobs and recording reconciliation.

The first A6 module-local command boundary consumes canonical
`MutationOperation` and `BackendAgentCommandAssignment` values,
with denied mismatched identity/generation/deadlines and no executor.
It is **not** registered or advertised. Compiled importer/package tests
are included in the isolated Add-on CI after the source relocation.

## A7.5 Web source integration checkpoint

The optional `Media Import` tab belongs to the current `index.html` /
`app.js` production composition. It is hidden without a live, actor/backend-
scoped `addons.media.import` capability and invalidated on backend/session
changes. The module reads only opaque source IDs, submits with browser CSRF
and one unique Idempotency-Key, then follows the existing actor-owned Operation.
It displays `outcome_unknown` without claiming success or submitting a second
job. An in-flight HTTP admission remains owned by the shared backend attempt
across tab re-composition: a late accepted Operation is adopted by the current
view, a definite pre-admission rejection releases the lock, and a lost reply
continues to block duplicate submission. Logout discards this in-memory
association so late responses cannot restore the prior session's Operation.
The source/test/CI owner is `web/frontend/modules/media-import.js`,
`web/frontend/tests/test_media_import_composition.js` and
`make test-ci-frontend`; isolated packaging checks additionally run
`make test-install-staging` and `make test-install-runtime-deployment`.
This milestone does not activate Media Check, Shrink or real-host execution,
and is not a claim of productive deployment.

## M1.R — Rectools-compatible three-way video-encode decision (2026-10-24)

**Product requirement:** Preserve the vdr-rectools develop workflow
(confirm_encoding, handle_osd_confirm, explorer.php):
**local re-encode**, **desktop/HandBrake processing** and **skip**.
The .pc_encode, .skipped and .duplicate suffixes are meaningful source
states, not media titles or automatic import candidates.
The obsolete world-writable .vdr-rectools.prompt must never be used.

All slices remain on work/addon-platform-registry-lifecycle.
This section is a work/acceptance inventory, **not** a capability
advertisement or authorization to touch live recordings.

| Slice | Required vertical result and acceptance | Verified status |
| --- | --- | --- |
| R0 — Baseline and codec admission | Existing importer refuses marker-suffixed inputs; real FFprobe classifies codecs and refuses encoding without independent approval. Regression against the old Die Herrschaft des Feuers.pc_encode.mkv filename and normal compatible remux. | **Source and isolated CI PASS** at 4930162de42a117aa06fa7443aacef5319b810ad. Incorrect fake FFprobe newline fixed. |
| R1 — Canonical decision ownership | Exact source identity/codec preview from trusted Agent observation; authenticated, backend-scoped, idempotent local / desktop / skip decisions recorded durably by existing ADR-0042 Core Operations. Actor and per-backend rights, revision/lease, handler approval and no-path request contract checked again at dispatch. | **Open**. Current import API accepts only sourceId; that is **not** encode consent. |
| R2 — Local encoding | A local decision bound to the *same* immutable Core Operation must reach the pinned Agent handler with explicit, single-use encode authority. Time/CPU/disk bounds and source preservation; independent recording index/native cache reconciliation remains the only success gate. | **Open**. Operator-only CLI flag is not a Public-v1 grant. |
| R3 — PC deferral / skip | Write the requested compatible .pc_encode / .skipped marker atomically under the authorized source root, with no untrusted browser path, source loss, duplicate rename or automatic resumption. .duplicate remains distinguishable; negative tests for links/races/collisions. | **Open**. Markers are currently rejected/hidden, not manageable in Suite. |
| R4 — Deliberate completion/release | Include marked items in an authenticated read-only, name/codec-aware Media Tools view, separate from runnable sources. Confirm unchanged identity of PC-replaced media before an explicit new release/import; reject old or concurrent approval and uncertain outcomes. | **Open**. Legacy filename recovery is not wired to Core. |
| R5 — Notification and frontend | Optional information-only email with no approving or unauthenticated action URL; three buttons, deferred/skip/duplicate filters, source identity, reapproval and canonical Operation progress in the existing Webfrontend owner. Pass CSRF, per-backend grants, logout, late reply, duplicate click and production composition regressions. | **Open**. Present UI lists opaque runnable IDs only. |
| R6 — Install and acceptance | Source/manual installation via a sealed runtime stage, non-duplicated accepted binaries, independent handler approval and Agent opt-in; no dependency on a matching Suite Debian package. Synthetic end-to-end Agent/API/UI plus read-only yaVDR preflight, then separately approved sample-media write acceptance. MLD needs its own service/path adapter. | **Open**. The yaVDR install adapter now accepts a loaded, cleanly inactive Agent but refuses failed units, unsafe metadata parent permissions and unreviewed host/package state. |

**Release blocker:** No productive Media Import, local re-encode or three-way
frontend activation may be claimed on the basis of R0 or the installer
preflight correction. The known yaVDR Backend Agent was inactive/dead,
while Suite daemon was active; its actual service configuration, installed
metadata ancestor access and independently reviewed candidate still need a
read-only host check. No automatic chmod/chown, package overlay, VDR
restart or import write is authorized. A source/manual installer must
preserve the existing Suite configuration, SQLite DB and recordings;
Core owns recording identity/reconciliation and TVScraper owns metadata.

## MT — Installation und UX-Folgeroadmap (24.10.2026)

**Geplant, offen:** [Media Tools — Installation, Import und Aufnahmeaktionen](../../addons/media-tools/ROADMAP.md).

- **MT.I0–I5 zuerst:** ein versiegelter Build-/Staging-Vertrag für manuelle Installationen **und** späteren Ubuntu/Debian-`.deb`-Paketbau. yaVDR/Ubuntu: belegte Ausgangsbasis. openSUSE/MLD: nur mit tatsächlich verifizierten Hostparametern, ohne erfundene Dienst-/Pfad-Defaults.
- **MT.UX0–UX7 danach:** Media Import direkt hinter Aufnahmen; Standardquelle rekursiv lesen, gezielte Auswahl/Explorer, Vorschau und bewusster Import; Shrink als Aufnahmeaktion in Details und Kartenmenü. Kein zweiter Worker oder Rechtepfad.
- Alte M1.R- und Core-/Agent-/API-Sicherheitsverträge bleiben maßgeblich. Keine Installation, Rechteänderung, Aufnahmeänderung oder automatische Aktivierung durch diese Dokumentation.
