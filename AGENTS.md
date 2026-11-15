# VDR-Suite Agent and Repository Workflow Rules

## AGENTS.md-Änderungen ausschließlich nach persönlicher Erlaubnis

**Nur Änderungen an Agents.md mit meiner persönlichen Erlaubnis, oder nach meiner Anordnung!**

Diese Regel gilt für jede `AGENTS.md`-Datei im Repository. Ohne die ausdrückliche persönliche Genehmigung oder konkrete Anordnung des Maintainers dürfen Assistenten solche Dateien weder ändern noch vermeintliche Fehler darin eigenmächtig korrigieren.

These rules apply to all automated assistants and agent-driven repository work.
They supplement the technical, security and phase-specific contracts in `docs/`.

## Höchste Maintainer-Vorgabe: KEINE neuen Sperren ohne vorherige Genehmigung (13.11.2026)

**Ausdrückliche Anordnung des Maintainers: KEINE NEUEN SPERREN, GATES ODER BERECHTIGUNGSREGELN eigenmächtig einführen. AUSNAHMEN NUR, WENN TATSÄCHLICH NOTWENDIG, UND AUSSCHLIESSLICH NACH MEINER VORHERIGEN AUSDRÜCKLICHEN GENEHMIGUNG.**

- Gilt für Core, Public API, Backend-Agent, Add-ons, VDR-Integration, TV-/Web-Clients, Installer, Build-/Testskripte und Konfiguration. Dazu zählen insbesondere neue Ablehnungen, künstliche Vorbedingungen, Capability-/Lease-/Aktivierungs-/Authentifizierungs-Zusatzprüfungen, Fail-closed-Fallbacks, deaktivierte Buttons/Funktionen, erzwungene Konfigurationspflichten, zusätzliche Freigabeschritte, Installationsabbrüche und restriktive Änderungen vorhandener Regeln.
- **Keine Sperre als vermeintlichen Sicherheits-, Robustheits-, Architektur- oder Distributionsunabhängigkeits-Fix einbauen.** Ein nicht erkannter Pfad, ein fehlender optionaler Agent/Transport oder ein unsicherer Status darf nicht zum Anlass genommen werden, bisher funktionierende unabhängige Funktionen pauschal abzuschalten. Vorhandene VDR-/Backend-Informationen und bestehende Abläufe zuerst wiederverwenden; keine distributionsspezifischen Pfade fest codieren.
- **Wenn eine zusätzliche blockierende Maßnahme wirklich unvermeidbar erscheint:** Arbeit an genau dieser Maßnahme **vor dem Coding stoppen**, den konkreten technischen Grund und nachgewiesenen Schaden, die betroffenen Funktionen/Distributionen, die mildere Alternative und den geplanten Umfang erläutern und die persönliche Entscheidung des Maintainers einholen. **Schweigen, allgemeine Entwicklungsfreigabe, ein bestandener Test oder eine angebliche Sicherheitsbest-Practice sind keine Zustimmung.** Ohne ausdrückliches Ja keine Implementierung, kein Commit und kein Deployment dieser Sperre.
- Bereits existierende fachlich erforderliche Autorisierung und Schutzvorgaben bleiben bestehen; sie dürfen weder stillschweigend erweitert noch umgangen oder entfernt werden. Diese Regel erlaubt keine eigenmächtige Abschaltung bestehender Berechtigungen. **Neue oder verschärfte Regeln benötigen die oben beschriebene Zustimmung.**
- Fehler transparent melden und den konkret betroffenen Vorgang diagnostizieren, statt zusätzliche globale Sperren zu erfinden. Bei ungewissem Ergebnis niemals denselben mutierenden Auftrag unkontrolliert erneut ausführen. Alle zulässigen, unabhängigen Bedien- und Lesefunktionen verfügbar halten.
- **Vorrang:** Bei Widerspruch zu älteren AGENTS-/Roadmap-/ADR-/Sicherheits- oder Installationsanweisungen für *neu einzuführende Sperren* ist zuerst die konkrete Genehmigung des Maintainers einzuholen. Diese Regel gilt verbindlich für alle künftigen Agent-Änderungen.


## Verbindlich: yaVDR-Installationsbefehle in der bestehenden Root-Shell ausführen (03.11.2026)

**Ausdrückliche Maintainer-Anordnung: Installationen als `root` ausführen.**
Der Operator ist bereits als `root` auf dem yaVDR angemeldet. Für
Installationsblöcke und dazugehörige Git-Fetches, Vorbereitung, Staging,
Deployment, Verifikation und Servicebefehle **keinen Benutzerwechsel**
mit `runuser`, `sudo -u`, `su` oder ähnlichem verwenden; auch nicht
stillschweigend Git als `yavdr` ausführen. Die Root-Shell bleibt bestehen.
Ein fehlgeschlagener Befehl darf sie nicht durch ein nacktes `exit` beenden.

Wenn für die beauftragte Installation ein inkrementeller C++-Build
erforderlich ist und der Maintainer die Ausführung ausdrücklich als
`root` verlangt, wird auch dieser Build im bestehenden Root-Kontext
ausgeführt. Diese ausdrückliche Anordnung hat für diesen Installationsfall
Vorrang vor älteren Anweisungen zum unprivilegierten Build. Vorhandene
verifizierte Binaries und den Objekt-Cache zuerst wiederverwenden;
Root-Ausführung ist **keine** Erlaubnis für unnötige Neubauten, neue
Branches/Worktrees, rekursive `chmod`/`chown`, Änderung fremder
Schlüssel/Berechtigungen, DB-Backups oder das Überschreiben nicht
zugehöriger Installationsdateien. Bereits vorhandene Cache-Rechte nicht
präventiv verändern; mögliche Root-Dateieigentümer im Build-Cache
berücksichtigen.

Diese Regel betrifft die Ausführung am yaVDR, nicht die Autorisierung
von VDR-Suite-Benutzern, Add-ons oder API-Aktionen. Wo ältere
Installationsabschnitte pauschal `runuser`, `sudo -u yavdr` oder einen
unprivilegierten C++-Build verlangen, gilt für den hier ausdrücklich
angeordneten Root-Installationsablauf diese neuere Vorgabe.

## Verbindliche Architekturregel: SSE geht vor Polling (03.11.2026)

**Ausdrückliche Maintainer-Anordnung: SSE GEHT VOR POLLING.** Diese
Priorität gilt für alle zukünftigen VDR-Suite-Entwicklungen, insbesondere für
langlaufende Media-Tools-Aufträge und automatische Frontend-Statusupdates.

- Bei asynchronen Statusänderungen, Abschluss, Fehlern und nachgelagertem
  Readback **zuerst den vorhandenen ereignisgesteuerten SSE-Rückkanal** nutzen
  oder passend erweitern. Core/Backend veröffentlicht den bestätigten
  Zustandswechsel; das Frontend reagiert auf das Ereignis.
- Kein zyklisches `setTimeout`/`setInterval`-Status-Polling als Standardlösung,
  kein festes Abfragekontingent und kein zeitlicher UI-Abbruch für einen
  weiterhin laufenden Auftrag. Auch nach stundenlangem Reencoding muss ein
  bestätigter Abschluss ohne Seiten-Refresh sichtbar werden.
- Bei initialem Öffnen und nach SSE-Wiederverbindung den aktuellen, dauerhaften
  Core-Zustand **einmalig** synchronisieren; verpasste Ereignisse dürfen weder
  hängen bleiben noch denselben Auftrag erneut starten. Die Ausführung selbst
  darf nicht vom Browser oder dessen SSE-Verbindung abhängen.
- Polling nur verwenden, wenn ein echter Push-Kanal technisch nachweisbar
  ungeeignet oder nicht verfügbar ist und diese Ausnahme **vorher mit dem
  Maintainer abgestimmt** wurde. Es darf nicht als stillschweigender Fallback
  erneut eingeführt werden. Bestehende Agent-Transportmechanismen sind nicht
  automatisch Frontend-Status-Polling.
- Für neue Ereignisverträge gezielte Tests für langen Auftrag,
  Disconnect/Reconnect, korrekte Autorisierung, Fehler und Abschluss vorsehen;
  die bestehende Core-Operationsautorität, Idempotenz und Ergebnisbestätigung
  nicht durch einen zweiten Statusspeicher ersetzen.

## Verbindlich: Schnellen inkrementellen C++-Daemon-Build erhalten (30.10.2026)

**Ausdrückliche Maintainer-Anordnung:** Den funktionierenden inkrementellen C++-Build
für lokale Entwicklung, gezielte Tests und Daemon-Updates beibehalten. Nicht
leichtfertig durch einen vollständigen Neuaufbau bei jedem Commit ersetzen.
Referenz: Commit `d80c077ae13eee04e404ccb63e5ab299f3c051bf`;
auf yaVDR wurde der Daemon nach zuvor ungefähr 18 Minuten mit dem
wiederverwendeten Objekt-Cache in 47 Sekunden gebaut. Das ist ein gemessener
Referenzwert, **kein** garantierter Grenzwert oder neues CI-Zeitlimit.

- `tools/build_cpp_cached.py` und `mk/object-cache.mk` sind die vorhandene
  Build-Cache-Integration. Unveränderte C/C++-Objekte bleiben über lokale
  Quellcode-Commits hinweg verwendbar; geänderte Quellen und betroffene Header
  müssen weiterhin korrekt neu kompiliert werden.
- Eine neue `VDR_SUITE_SERVER_VERSION` darf **nicht** sämtliche sonst
  unveränderten Objekte invalidieren. Versionsabhängige Übersetzungseinheiten
  müssen weiterhin die korrekte aktuelle Versionskennung erhalten; die
  Versionsidentität des fertigen Daemons darf nicht veralten.
- Bestehende parallele Kompilierung erhalten (derzeit standardmäßig zwei Jobs,
  über `VDR_SUITE_CXX_JOBS` einstellbar). Den Objekt-Cache nicht durch
  wechselnde Build-Verzeichnisse pro Commit, unnötige frische Checkouts,
  pauschales `make clean` oder automatische Cache-Löschung umgehen. Bei
  notwendiger getrennter Binär-Ausgabe nach Möglichkeit denselben kompatiblen
  `OBJECT_CACHE_DIR` weiterverwenden.
- Änderungen am C++-Build-System dürfen diese Eigenschaft nicht unbemerkt
  regressieren. Der vorhandene Test
  `python3 tools/test_build_cpp_cached.py` (integriert über
  `check-cpp-object-cache` / `test-build-artifact-paths`) prüft
  Objekt-Wiederverwendung, Versionstrennung und Header-Invalidierung.
  **Keinen zusätzlichen CI-Warte- oder Installations-Gate erfinden.**
- Ein absichtlicher vollständiger lokaler Neuaufbau bleibt zulässig, wenn
  tatsächliche inkompatible Compiler-/Build-Optionen, geänderte breite
  Abhängigkeiten, ein defekter Cache oder eine ausdrückliche Maintainer-Anweisung
  ihn begründen. Die Optimierung nicht ohne Zustimmung des Maintainers
  pauschal zurückbauen.

**Ausnahme – Debian-/Ubuntu-Paketbau und Release-Artefakte:** Debian/Ubuntu-
Pakete müssen aus definierten Quellen und Abhängigkeiten **sauber,
reproduzierbar und isoliert** baubar bleiben. Ein Paketbuild darf deshalb
explizit ohne lokalen Entwicklungs-Cache aus einem frischen Build-Verzeichnis
starten; er darf keine möglicherweise veralteten oder fremden Objektdateien
aus dem Entwickler-Cache übernehmen. Das gilt auch für andere Release-/CI-
Workflows, soweit deren Clean-Build-Vertrag dies verlangt. **Die Cache-Regel
für lokale Daemon-Entwicklung darf Clean-Paketbuilds weder verbieten noch
beeinflussen.**

## Media Tools nach PR #556: offizieller Quellstand `main` (03.11.2026)

**Verbindliche Maintainer-Vorgabe: Neue Git-Branches nur nach ausdrücklicher
Zustimmung des Maintainers anlegen.** Das gilt für Feature-, Fix- und
Ausweichbranches. Keine neuen Branches oder Worktrees als Installationsumweg
erzeugen; bestehende Branches nicht ohne Auftrag löschen oder überschreiben.

**Ausdrücklich erlaubt:** Zwischen bereits vorhandenen Branches wechseln
(z. B. `git switch main`), `git fetch` und einen bestehenden Branch mit
`git pull --ff-only` aktualisieren. Für diese normalen Git-Operationen
ist keine zusätzliche Freigabe erforderlich, wenn sie zur beauftragten
Entwicklung, zum Test oder zur Installation gehören. Dabei lokale Änderungen
erhalten; keine destruktiven Git-Befehle als Abkürzung verwenden.

**Media-Tools-Quellstand:** Nach PR #556 ist `main` die kanonische Quelle
für Media Tools und Add-ons. Der frühere Arbeitsbranch ist keine
verpflichtende Installations- oder Entwicklungsbasis. Historischer
Merge-Commit, Branch-Bereinigung und externe Rezept-/PR-Abhängigkeiten:
[Branch-/PR-Lebenszyklus](docs/development/branch-pull-request-lifecycle.md).
Alte Branches nur auf ausdrückliche Anweisung und nach Prüfung aller
Abhängigkeiten löschen. Ein Merge ersetzt keine Paket-/Hardwareabnahme.

## Lokalen Quellstand vor Build, Test und Installation prüfen (04.11.2026)

**Branchwechsel ist normaler Arbeitsablauf, keine Branch-Neuanlage.**
Für eine beauftragte Entwicklung oder Installation darf der Assistent
selbstständig auf einen **bereits vorhandenen** passenden Branch wechseln,
insbesondere auf `main`, und diesen per `git fetch` und
`git pull --ff-only` aktualisieren. Dafür ist keine gesonderte Nachfrage
erforderlich. Die Zustimmungspflicht betrifft **nur das Anlegen neuer
Branches**; sie darf nicht als Verbot von `git switch main` ausgelegt werden.

- Vor dem Wechsel den tatsächlichen Checkout und `git status` prüfen.
  Bei lokal geänderten oder unversionierten Dateien nichts verwerfen oder
  überschreiben. Einen gefahrlosen normalen Wechsel zulassen; wenn Git
  dadurch Änderungen überschreiben würde, gezielt stoppen und den
  Konflikt melden. Kein automatisches `reset --hard`, `git clean`,
  erzwungenes `checkout` oder ungefragtes `stash`.
- Vor dem Build/Test/Staging/Deployment das Git-Top-Level, den **danach
  tatsächlich ausgecheckten** Branch und `git rev-parse HEAD` im
  verwendeten Checkout prüfen. Die für den Auftrag vereinbarte Quelle
  ist maßgeblich; nach Media Tools PR #556 ist `main` die kanonische
  Quelle. Einen ausdrücklich festgelegten Commit gegen den Quellbranch
  verifizieren.
- Ist der Checkout noch auf einem alten Arbeitsbranch, **nicht einfach
  wegen des Branch-Namens abbrechen**: den bestehenden Zielbranch regulär
  auswählen und aktualisieren, danach den gewünschten Commit verifizieren.
  Nur bei tatsächlichem Konflikt, fehlendem Zielbranch, nicht
  vorwärtsgerichtetem Pull oder nicht passendem Quellstand mit einem
  konkreten Fehler stoppen. Kein `FALSCHER_BRANCH` als künstliches
  Hindernis für einen sicheren, erlaubten Wechsel.
- Keine neuen Git-Branches ohne ausdrückliche Freigabe und keine
  eigenmächtigen Löschungen, Force-Pushes oder History-Rewrites. Bereits
  vorhandene Branches, Änderungen und unversionierte Dateien bewahren.
- **Reine gezielte Frontend-Dateiupdates** aus einem fest verifizierten
  Commit benötigen weiterhin weder Branchwechsel noch einen lokalen
  Build und werden nicht durch einen abweichenden lokalen Branch blockiert.
- Installations- und Testblöcke müssen nach dem gegebenenfalls nötigen
  Branchwechsel den tatsächlichen Quellstand überprüfen und Fehler
  verständlich melden. Keine stillen Abbrüche oder ungeschützten
  Git-Operationen.

## GitHub SSH identity verification before console commands (28.10.2026)

**Verbindlich:** Vor GitHub-SSH-Mutationen die tatsächliche Git-Remote-
und SSH-Identität des bestehenden Root-Kontos lesend überprüfen. Niemals
einen anderen Unix-Benutzer oder dessen SSH-Berechtigung vermuten,
Schlüssel kopieren, Berechtigungen spekulativ verändern oder die
interaktive Root-Shell durch `exit` verlassen. Eine erfolgreiche
`ls-remote`-Abfrage bestätigt noch keine Push-Berechtigung. Bei
Authentifizierungsfehlern den konkreten Vorgang stoppen; vorhandene
autorisierte GitHub-Connectoren können unabhängig verwendet werden.
Branch-Löschungen erfordern eine ausdrückliche Freigabe und Prüfung
bestehender Abhängigkeiten.

**Pflichtlektüre vor GitHub-SSH-Befehlen am yaVDR:**
[Nachgewiesene SSH-Probleme und genaue Prüfverfahren](docs/development/agent-yavdr-github-ssh.md).
Die neuere Root-Shell-Anordnung in diesem Dokument hat Vorrang.

## Usability First – No Unapproved Restrictions

Neue Sicherheits-, Berechtigungs-, Aktivierungs- oder Installationsbeschränkungen dürfen nicht eigenständig eingeführt werden. Bestehende Schutzmechanismen dürfen nicht ohne begründeten Bedarf verschärft werden. Änderungen müssen die Benutzerfreundlichkeit erhalten oder verbessern. Zusätzliche Prüfungen, die keine konkret nachweisbare Fehlerklasse verhindern, sind zu vermeiden. Bereits bestätigte Testergebnisse dürfen nicht ohne Anlass erneut angefordert werden.

### Verbindlich: Keine präventiven Sperrprüfungen bei einfachen Dateiupdates (29.10.2026)

**Ausdrückliche Maintainer-Anordnung: Absicherung nur, wenn ohne sie eine konkrete Gefahr schwerer System- oder Datenbeschädigung besteht. Keine vorsorglichen Abbruchprüfungen bei trivialen Updates.** KISS und Usability First haben Vorrang vor spekulativen Prüfketten.

- **Zwei Frontend-Dateien ersetzen ist kein System-Deployment.** Für ein gezieltes Update bereits bekannter Dateien wie `app.js` und `modules/media-import.js` aus einem festgelegten Commit direkt die beiden Dateien aktualisieren. Keine vollständige Runtime-Stage erzeugen, versiegeln, abgleichen oder deployen; keine Komplettinstallation als Umweg.
- **Kein Branch-/Detached-HEAD-Gate für Einzeldatei-Updates.** Der lokale Checkout darf einen anderen Branch oder Detached HEAD haben. Bei einem ausdrücklich festgelegten Remote-Commit sind solche Zustände irrelevant und dürfen das Dateiupdate nicht verhindern. Branchprüfungen für tatsächlich geplante Builds oder Repository-Änderungen bleiben davon getrennt.
- **Kein Stage-Drift-Gate für Einzeldatei-Updates.** Ein Manifest-/Stage-Abgleich, der lediglich ältere Versionen dieser zwei Frontend-Dateien meldet, ist kein Schaden und kein Grund für einen Abbruch. Die alte Stage bei diesem Vorgang nicht als autoritative Live-Basis behandeln.
- **Keine künstlichen Vorbedingungen.** Keine neuen Capability-, Aktivierungs-, Agent-, Lease-, Datenbank-, CI-, Dienst- oder Installer-Prüfketten; keine ungefragten Backups, Builds oder Dienstneustarts für reine statische Dateiupdates. Ein erfolgreich abgeschlossener Funktionstest ist nicht bei jeder Installation erneut einzufordern.
- **Schutzprüfungen nur bei konkretem Schadensrisiko.** Eine zusätzliche blockierende Prüfung benötigt eine belegbare, durch das Update verursachte Gefahr schwerer und nicht ohne Weiteres reversibler System- oder Datenbeschädigung. Die bloße Möglichkeit eines fehlgeschlagenen Updates, ein veraltetes Manifest, ein anderer Branch oder ein vorübergehend nicht erreichbarer Agent reichen dafür nicht. Technisch notwendige Dateioperationen dürfen natürlich ihr tatsächliches Ergebnis melden.
- **Kleiner Änderungsumfang, kleine Lösung.** Bei einem Zwei-Dateien-Update nur die zwei bezeichneten Dateien ersetzen und den tatsächlichen Ausgang melden. Keine neuen Regeln, Skriptframeworks oder globalen Freigaben erfinden. Bestehende serverseitige Autorisierung für echte Import-, Lösch- und andere Datenmutationen bleibt unberührt.

**Vorrang bei Widerspruch:** Diese speziell angeordnete Ausnahme für reine Einzeldatei-Updates geht den älteren lokalen Branch-, Stage- und Installations-Preflight-Regeln vor. Sie gilt nicht für C++-Builds, Eingriffe in Datenbanken, Aufnahmen, Dienste oder andere potentiell systemkritische Änderungen.

### Verbindlich: Agent-Störungen dürfen die Bedienoberfläche nicht blockieren (29.10.2026)

**Ausdrückliche Maintainer-Anordnung: Eine installierte und bereits verwendete Funktion bleibt unabhängig von vorübergehenden Agent-, Capability-, Lease- und Transportstörungen navigierbar und lesend bedienbar.** Das betrifft ausdrücklich Media Tools / Media Import, gilt aber als Usability-Grundsatz für alle Suite-Module.

- **Navigation ist keine Capability-Freigabe.** Ein negatives, ausbleibendes, veraltetes oder temporär fehlerhaftes Capability-Ergebnis darf einen bekannten Tab nicht ausblenden, deaktivieren, zwangsweise wechseln oder die geöffnete Ansicht neu mounten. Den konkreten Status lokal und verständlich anzeigen, statt die gesamte Funktion zu sperren.
- **Keine Dauerprüf- und Sperrschleifen.** Keine wiederkehrenden Fünf-Sekunden-Gates, automatischen Capability-Neuprüfungen beim bloßen Tabwechsel, versteckten Polling-Kaskaden oder wiederholten Freigabeanforderungen, die die UI unbenutzbar machen. Eine gezielte Statusaktualisierung darf nicht die Navigation oder unabhängige Aktionen blockieren.
- **Ein fehlgeschlagener Read darf vorhandene Daten nicht vernichten.** HTTP 503, Agent-Ausfall, unvollständiger Scan oder verzögerte Antwort dürfen die zuletzt erfolgreich geladenen Ordner, Episoden, Metadaten, Filter oder eine gültige sichtbare Auswahl nicht einfach leeren. Neue Scanergebnisse erst nach vollständigem erfolgreichen Lesen/Validieren übernehmen. Vorhandene Daten nötigenfalls ausdrücklich als *zuletzt geladen / möglicherweise veraltet* kennzeichnen; niemals als frisch bestätigt ausgeben.
- **Ausfall lokal begrenzen.** Ein Problem bei Quelle A oder Auftrag A darf Quelle B und unabhängige lesende Bedienhandlungen nicht global sperren. Ein bestätigter Abschluss muss zugehörige temporäre UI-Sperren zuverlässig lösen. Technische Fehlerursachen und konkrete Wiederherstellungswege anzeigen, keine generischen Aktivierungs- oder Berechtigungswarnungen ohne Befund.
- **Bestehende Autorisierung nicht umgehen.** Explizite Abmeldung/entzogene Berechtigung beendet den geschützten Zugriff. Importieren, Umbenennen, Markieren, Löschen und andere Mutationen benötigen weiterhin die normale serverseitige Prüfung der konkreten Aktion sowie ihren tatsächlichen Ergebnis-/Readback-Nachweis. Alte oder zwischengespeicherte Ansichten sind niemals eine Mutationsfreigabe. Ungewisse Aufträge niemals automatisch erneut dispatchen.
- **Regressionstests sind verpflichtend:** temporäres negatives Capability-Ergebnis, Agent-503 beim ersten und bei einem späteren Scan, Tabwechsel während des Fehlers, Erhalt der letzten erfolgreichen Liste, Entsperrung unabhängiger Aktionen und korrekte Abmeldung testen. Nicht nur Erfolgsmeldungen oder Mockup prüfen, sondern das tatsächliche Webfrontend-Verhalten.

### Verbindlich: KISS-Updates nur auf Basis der tatsächlich installierten Stage (29.10.2026)

**Nur für Stage-basierte Runtime-Deployments; nicht für gezielte Einzeldatei-Updates gemäß der vorstehenden Ausnahme.**

Ein Installationsblock darf eine alte, inzwischen überholte Runtime-Stage **nicht** als aktuelle Ausgangsbasis verwenden. Vor jedem inkrementellen Update die **zuletzt erfolgreich installierte und mit `install_runtime_stage.py check` verifizierte Stage** aus der realen Host-Ausgabe wählen. Eine ältere Stage meldet bei korrekt veränderten Dateien `RUNTIME_DEPLOYMENT_DRIFT`; diese erwartete Abweichung ist **kein Defekt der Live-Installation**. Bei einem solchen Fehler abbrechen, die tatsächliche aktuelle Stage ermitteln und ausschließlich von ihr aus neu versiegeln/deployen; kein blindes Zurückrollen, kein Drüberbügeln ungeprüfter Dateien. Für reine Webfrontend-Änderungen weder C++ neu bauen noch VDR/Daemon/Agent neu starten. Keine ungefragten Datenbanksicherungen, neuen Dienste oder weiteren Aktivierungsgates einführen.

## No hallucinated facts, user mandates, or blockers

**Hard rule:** Never invent or imply user instructions, approvals, prohibitions,
repository requirements, CI outcomes, tested behavior, repository paths, or
technical blockers. Separate **user instruction**, **actual repository rule**,
**verified source/tool result**, and **assistant recommendation/inference**.
If a rule was written by an assistant, do not attribute it to the user;
quote its actual wording and path before invoking it as a constraint. An
assistant-authored rule is not evidence that the user requested that rule.

Check the actual file or tool result before making factual claims. If a fact
cannot be verified, say **unverified**, not "confirmed"; if a conclusion is
inferred, label it an inference and its supporting observations. Do not
reinterpret a recommendation, pending test, or incidental discovery as a
mandatory stop gate. A queued, running, slow or unavailable CI job alone is
not a reason to stop already authorized independent analysis, implementation
or local tests. Continue bounded independent work without overstating its
validation, and explicitly report what is still untested. Never fabricate
successful builds, validations, deployments, or approvals.

## Non-negotiable scope, simplicity, and verification gates

**Priority:** These gates constrain every workstream, including the top-level
non-stop mandate, GitHub-first execution and commit/CI batching. "Continue
working" means finish the **already authorized, bounded task**, not discover
or implement new tasks. No speed, autonomy or non-stop instruction overrides
these gates.

1. **One task, one verifiable outcome.** Before changing product code, identify
   the concrete bug or authorized feature, its observable acceptance criterion
   and the smallest affected surface. Stay inside that boundary. Do not
   interpret "keep going" as permission to extend the project scope.
2. **Prove the need before coding.** Reproduce or trace the concrete failure
   and inspect existing safeguards before adding another guard. Hypothetical
   risks, possible future features and speculative hardening do not authorize
   implementation. Record unrelated findings without silently fixing them.
3. **Reuse existing architecture.** Prefer a small change in the current
   implementation. New database tables, services, persistent state fields,
   protocols, cross-cutting abstractions and broad refactors require BOTH
   evidence that the existing design cannot meet the agreed acceptance
   criterion AND the user's explicit authorization **before** implementation.
4. **Validate before declaring completion.** For C++, backend or
   runtime-contract changes, compile the exact candidate and run relevant
   functional/regression tests before claiming it works. State precisely
   which SHA was tested and which checks remain pending; never equate a
   pushed commit or queued CI with a passing build. A running or blocked CI
   job does NOT prohibit preparing and implementing the next already
   authorized slice in parallel. Keep work bounded, check compatibility,
   and test each candidate when tooling is available. Do not claim that an
   assistant-authored sequencing preference is a user requirement. Docs-only
   edits need proportionate documentation checks.
5. **No chain reaction of adjacent work.** Discovering an issue during a fix
   does not add it to the authorized task. Note it concisely. Only a proven
   defect blocking the current acceptance or a verified urgent security issue
   warrants interrupting the sequence; any required out-of-scope architectural
   change still needs explicit user authorization.
6. **Finish and stop at the agreed gate.** Once the bounded acceptance
   criterion and its required checks pass, report the result and stop. Do not
   invent follow-up implementation, additional hardening, refactoring,
   documentation cycles or CI-watching tasks. The non-stop mandate prohibits
   premature handoff, **not completion**.

**Mandatory pre-change check:** Before every product-code edit and subsequent
commit, ask: Is this change required for the authorized outcome? What evidence
demonstrates that? Can the existing design solve it? How will the exact result
be compiled and tested? If any answer is missing, perform a bounded diagnostic
instead of writing more code. These scope gates take precedence wherever
another section could be read as encouraging speculative expansion or
unverified commit batching.

## Top-level non-stop execution mandate

Once the user has authorized a bounded workstream, never voluntarily stop,
hand off, or end the working response while any authorized implementation,
diagnosis, validation, CI resolution, runtime preparation, repository operation
or other executable step remains. Continue using the available tools and
repository operations until the requested end state is reached.

Status updates are progress reports, not stopping points. A completed analysis,
intermediate commit, pushed head, running or queued CI job, available next
implementation step, chat length, or ordinary turn boundary is never a reason
to end an already-approved workstream. When a check fails, diagnose and repair
the demonstrated cause and continue; do not turn a fixable failure into a
handoff to the user.

An intermediate finding is never an end state. Never convert a diagnosis,
hypothesis, partial validation, partial implementation, commit, CI result,
runtime observation or progress summary into a final response while authorized
executable work remains. Continue immediately with the next executable step.

Every working response that is permitted to end must end with the heading
`## Testblock` followed by exactly one ordinary fenced `bash` block containing
the concrete branch-/head-specific commands the user can run to validate the
current candidate. The test block is the final content; no prose, status, offer
or summary may follow it. If no truthful test block can yet be produced because
the candidate is not testable, the response is not permitted to end; continue
working instead.

There is no generic repository permission to stop an authorized workstream.
A genuinely external dependency or genuinely new user decision may make the
remaining requested operation temporarily impossible, but that is a blocked
wait state rather than permission to abandon the workstream. Before reporting
such a blocker, exhaust every independent approved operation, re-read all
available authoritative repository and CI state, and identify the exact external
event or input without treating the response boundary as project completion.
Existing authorization counts: do not ask again or stop at a PR-state change,
merge, runtime action or other gate that the user already explicitly approved.
If an unexpected remote change or another safety condition can be resolved by
re-reading authoritative state, resolve it and continue.

If a GitHub Actions run for the relevant head is required for the next already-
authorized gate, do not end the working response while that run is still known
to be queued or in progress. Continue independent approved work while it runs
and re-read the run before any response termination. Never report a stale
non-terminal CI snapshot as though it were the end of the workstream.

## Product boundaries and real-VDR acceptance (2026-10-22)

**Binding rule:** Isolated test success, a passing CI matrix, a compilable
daemon, and an installed package are different evidence levels. None of them
alone proves that a capability works against the operator's real VDR.

**Keep architecture bounded:**

- Use one production owner for each API operation, domain action, persistence
  mutation, backend dispatch and readback. Route through the existing ownership
  chain; never implement a second parallel or test-only productive path just
  to make an isolated test green.
- Extend an existing module when its responsibility already includes the new
  behavior. Split/create a module only for a clear independent responsibility,
  lifecycle, dependency boundary or reuse requirement. Do not split solely to
  manufacture another test target, slice or Make include.
- Respect the documented source-group boundaries in
  `docs/development/phase-56.21-build-boundary-map.md`. Keep mocks, smoke
  helpers and test HTTP servers out of the production composition; keep
  transport parsing out of reusable domain/core code.
- Prefer completing one vertical production flow over adding successive
  wrappers, adapters and new slices around an unfinished flow. Any newly
  introduced layer must name the production dependency/boundary it protects.

**Require meaningful end-to-end evidence:**

- An API capability is implementation-complete only when the *real production
  path* is connected where applicable: public route -> authentication and
  authorization -> controller -> domain service -> durable state -> backend
  adapter/command -> VDR outcome/readback -> public response, including the
  frontend/client path for client-visible capabilities. Mark nonapplicable
  stages explicitly rather than inventing them.
- Focused unit and contract tests catch regressions while coding, but do not
  multiply near-identical unit tests or mock-only test binaries as a
  replacement for the productive composition test. Add at least one test
  covering the actual production route/composition for each changed API flow.
- Track evidence separately as `UNIT/CONTRACT`, `PRODUCTION-INTEGRATION`,
  `CI`, and `REAL-VDR`. Never collapse these into one `GREEN` or
  `DONE` claim. A missing or failed level remains clearly
  `NOT_VERIFIED` or `FAILED`, not implicitly passed.
- Maintain one cumulative API-v1 acceptance checklist (by actual route and
  capability) covering authentication/grants, valid and invalid requests,
  expected state mutation, persistence, backend command and readback,
  externally observable result, and failure paths as applicable.
  Close the implementation scope before requesting the operator's **final
  real-VDR API-v1 acceptance run**. Do not claim real-VDR acceptance from
  simulations, CI mocks, a helper's `--help`, or an HTTP 200 alone.
- Live-VDR acceptance must operate on a verified installed code candidate,
  with explicit preflight, safe read-only probes first, and separate operator
  authorization for any destructive or scheduling mutations. The real-VDR
  checklist is the final acceptance authority, not permission to bypass
  targeted safe tests for changed code beforehand.

**Limit test/build overhead:**

- Reuse accepted unchanged test evidence and verified build artifacts.
  Run focused tests on changed code during implementation and full required CI
  once at the final merge boundary under the existing CI rules.
- Do not run packaging, full regression, entire daemon builds or hosted-CI
  waits per tiny slice unless that exact changed surface requires it. Do not
  keep producing new slices solely because earlier tests were green while the
  user-visible workflow remains unfinished.

## GitHub-first execution

Use GitHub-first execution whenever the connected GitHub tools can perform the
operation safely. Read, edit, commit, push and inspect repository state through
the GitHub connector instead of asking the user to copy shell commands.

Use the local checkout only when the work genuinely requires compilation,
generated artifacts, focused local tests, access to the installed yaVDR runtime,
or an operation that the GitHub connector cannot perform safely.

Never replace an existing file from a truncated or partial fetch. Fetch the
complete content or use a bounded edit strategy. Recheck the remote branch head
before every write and inspect the resulting commit or diff before treating the
change as complete.

## Ubuntu command fidelity: no `.sh` SVDRP shortcut

On the user's Ubuntu/yaVDR host, **never propose or invoke `svdrpsend.sh`** (nor silently substitute a shell-script wrapper for an executable). This is an explicit operator prohibition, not a fallback preference. Do not confuse MLD-specific `svdrpsend.sh` behavior or forum examples with the Ubuntu host.

For all VDR/SVDRP operator test blocks, verify the Ubuntu-installed command name, binary path, supported syntax and service context from actual host/package evidence before issuing commands. If the exact working Ubuntu invocation is unverified, avoid issuing a guessed command; perform a bounded read-only identification instead. Use the verified native Ubuntu command, never a `.sh` workaround. Shell code blocks may use Bash for orchestration; the prohibition concerns invoking `.sh` wrappers as replacement utilities.

## Absolute prohibition on guessing: verified evidence before instructions

**Guessing operational facts is strictly forbidden.** Never invent, assume,
estimate, extrapolate from an earlier phase, or hand the user plausible-looking
commands when correctness is verifiable. Urgency, prior success, or pressure to
finish never waives this rule. The user's time, yaVDR installation, and live
project state must not be used to discover preventable mistakes.

**Before writing code, starting tests, or handing over executable commands,
establish the required facts from the current authoritative source:**

- Verify the intended repository, branch, **exact** remote head SHA, checkout
  root, tested SHA, and whether all necessary fixes are present. Do not
  present an older checkout's result as evidence for the current PR head.
- Read the current Makefile and included makefiles (or inspected target list)
  to prove the actual build/test targets, their prerequisites and flags.
  **Never invent a Make target, executable name or path.**
- Derive HTTP status codes, authentication and authorization behavior from
  the actual gate, router, service and tests. **Never guess a status code,**
  and never weaken a security check just to match an unsupported expectation.
- Establish intended output paths, user privileges, possible side effects and
  whether the action touches the installed service. Host-dependent facts require
  a read-only compatibility check, without unapproved storage-reserve gates.
- Check the expected head **before** handing off a long-running, detached or
  resource-intensive build. Never accidentally start an old checkout after
  fixing a newer branch.

**The test block itself must fail closed on wrong provenance.** It must
verify the checkout root, actual SHA against the selected immutable test
candidate (or an explicitly verified tracking policy), required commands
and Make targets, and applicable non-storage compatibility checks *before* work begins.
If the branch advanced, fetch and establish the corrected candidate first.
Do not give a detached build script that silently tests stale code.
Reuse accepted evidence for unaffected inputs; do not restart tests only
because documentation or unrelated code changed.

**A background process is not proof of a completed build.** `nohup`, a
printed PID or an absent result file alone does not prove the job is still
running. Detached jobs require a recorded exact SHA, log, process identity,
terminal return codes, and a read-only status command distinguishing running,
completed successfully, completed with error, and unexpectedly missing.
Never report success before the result is known. Continue to honor existing
minimal-validation and shell-option rules.

**If evidence is unavailable, stop the affected unsafe action, not the entire
authorized workstream.** Retrieve authoritative information or perform a
bounded read-only diagnostic; continue independent approved work. State
precisely what is unknown rather than manufacturing an answer. Never shift
basic repository fact-checking to the user. Diagnose a failed user test
against the exact tested SHA and demonstrated failure before requesting
another test.

**Mandatory pre-handoff self-check:** Inspect every command, checkout path,
Make target, branch/SHA precondition, expected status, side effect, output,
failure path and success criterion against the current repository and
applicable `AGENTS.md` requirements. Correct known discrepancies before
handing over any test or acceptance block. If some fact is only knowable on
the host, build a harmless read-only check into the block instead of
pretending it has already been established.

## Temporary worktree and test-artifact lifecycle

Temporary Git worktrees, detached test checkouts, build directories and staging
directories created for a bounded test or acceptance step must be treated as
short-lived resources, not retained project state.

As soon as the test or acceptance step no longer needs them:

1. verify that the exact temporary worktree contains no uncommitted or otherwise
   unsaved work that belongs to the active workstream;
2. remove that exact worktree immediately with `git worktree remove` (use
   `--force` only when the worktree is intentionally disposable);
3. remove the test-specific build/stage directories created for that step;
4. run `git worktree prune` to discard stale metadata.

Do not defer this cleanup to a later phase or accumulate obsolete worktrees in
`/tmp`, `/home`, `/root` or tool-managed worktree directories. Test blocks
that create temporary worktrees or substantial temporary build/stage trees must
include their own cleanup path in the same bounded workflow.

Before deleting anything, identify the exact resource created by the current
test. Never delete or prune a still-active worktree, build tree or stage tree
belonging to another concurrent workstream. In particular, shared or parallel
MU/client/runtime work must remain untouched unless its own workstream has
finished and its state has been checked.

Do not create large disposable build trees when verified existing artifacts can
be reused. Report actual filesystem write failures without speculative cleanup.

### Installations-Cleanup: manuelle Builds versus Ubuntu-/Debian-Pakete (08.11.2026)

- **Manuelle Quellcode-/Runtime-Installation:** Temporäre Checkouts, Build-
  und Stage-Verzeichnisse nur dann automatisch entfernen, wenn sie **durch
  genau diesen Lauf** erzeugt wurden und die Installation einschließlich
  der abschließenden Deployment-Prüfung erfolgreich war. Bei Fehlern den
  Diagnose-/Rollback-Stand erhalten und den Pfad ausgeben. Bereits vorhandene,
  fremde oder bewusst separat verwaltete Arbeitsverzeichnisse nicht löschen.
  Der bestehende `make install-runtime-managed`-Weg räumt seine eigene
  temporäre Stage nach erfolgreicher Prüfung auf; einzeln aufgerufene
  Stage-/Deploy-/Check-Ziele behalten ihre vom Operator verwaltete Stage.
- **Inkrementelle Entwicklung:** Den zentralen Checkout samt `.build`,
  insbesondere `.build/obj` und kompatiblen `OBJECT_CACHE_DIR`, niemals
  durch Installations-Cleanup entfernen oder durch frische Build-Checkouts
  ersetzen. Verifizierte, versiegelte Daemon-Kandidaten sowie für einen
  möglichen Rollback benötigte Binaries/Receipts bleiben ebenfalls erhalten.
  Keine automatische Löschung von Datenbanken, Datenbank-Backups,
  Aufnahmen, Live-Konfiguration oder persistentem Runtime-State.
- **Ubuntu-/Debian-Paket (`.deb`):** Paketbau und dessen temporäre
  Quell-/Objekt-/Staging-Verzeichnisse gehören in die isolierte
  **Paketbauumgebung**, nicht in das installierte Paket. Eine normale
  `apt`-/`dpkg`-Installation auf dem Zielsystem kompiliert VDR-Suite nicht
  und darf keine solchen Build-Reste erzeugen. Aufräumen ist Aufgabe des
  abgeschlossenen Paketbau-Workflows, **nicht** eines `postinst`-, `prerm`-
  oder anderen Maintainer-Skripts auf dem Zielsystem. Falls auf dem
  Zielsystem selbst Pakete gebaut werden, nur dessen eigene temporäre
  Build-Arbeitsverzeichnisse gezielt bereinigen; fertige `.deb`-Artefakte
  und ausdrücklich benötigte Protokolle erhalten.
- **Keine pauschalen Löschungen:** Weder alte `/tmp`-/`/home`-/`/root`-
  Verzeichnisse nach Namensmuster noch globale Cache-Verzeichnisse oder
  fremde Worktrees automatisch entfernen. Keine zusätzlichen Backups,
  Dienstneustarts oder Rechteänderungen bloß für das Cleanup.

**Verbindliche Umsetzungsregel fuer neue oder geaenderte Installer, Build-Skripte
und Installations-Bash-Bloecke:** Den selbst erzeugten temporaeren Pfad beim
Anlegen merken und im selben Ablauf nach bestaetigter Installation/Verifikation
gezielt entfernen. Bei Fehlschlag Pfad zur Diagnose ausgeben und stehen lassen.
Ein blosses `AGENTS.md`-Versprechen ohne Cleanup im ausfuehrbaren Ablauf
reicht nicht: den Erfolgspfad, den Fehlerpfad und den Erhalt von
`BUILD_DIR`/`OBJECT_CACHE_DIR` durch einen fokussierten Regressionstest
absichern. `--prepare`-Artefakte, operatorverwaltete Stages und root-eigene
Rollback-Snapshots nur durch eine ausdrueckliche separate Aktion entfernen.

## KISS: daemon-only replacement is independent of the running Git branch

**Binding rule for `tools/install_daemon.py` and EVERY future install block:**

- The installed daemon is a **replaceable executable**, not the parent of the
  next build. The old and new source commits may be siblings, diverged,
  ancestors, or on unrelated branches. **Never add an ancestor/merge-base,
  branch-equality, “installed source must be X”, or CI-completion gate** as a
  prerequisite for replacing the daemon. Do not merge/cherry-pick feature
  branches solely to satisfy an invented installer ancestry rule.
- `check` determines the requested target SHA, existing verified binary/seal
  and **actual database risk**. Source-to-source `git diff OLD NEW` works
  without ancestor relationships. Existing historical source information is
  needed for assessing schema changes, **not** to dictate upgrade direction.
  Preserve database, pairing, configuration and recordings.
- **KISS sequence: `check` → reuse sealed target if present, otherwise build
  exactly once in the existing authorized root shell on yaVDR → separately
  approved `apply` as root.** No needless rebuild after interrupted installation.
  Do not switch Unix users for the installation workflow.
- **Never put GitHub Actions queries, CI polling, `sleep` loops, CI waits or
  external CI status checks in any installation command block or installer.**
  CI is a separate development/merge concern, not an execution step in the
  user's installation. Deliver an accurate existing-installer block promptly.
- Preserve the regression tests
  `test_divergent_daemon_history_is_not_an_install_blocker` and
  `test_divergent_previous_commit_reuses_sealed_target_without_compiling`.
  They protect this invariant. **Do not replace them with stricter old-version checks.**

## Mandatory established-feature parity and competitor comparison

Before extending a working feature to Public-v1, VIDAA, Kodi, another client
or any replacement interface, compare it against the **existing, proven
VDR-Suite Webfrontend** behavior and the authoritative service implementation.
Write down a short acceptance matrix of inputs, output totals, ordering,
metadata/title behavior, pagination, errors, grants and live integration.
Use exactly one shared domain service and index. Public-v1 adapters may
project and authorize results but must not introduce a second search algorithm,
change the effective title/ranking, or turn one stale record into failure for
otherwise valid results. Require one automated Web-vs-Public-v1 parity
regression with realistic production examples before installation acceptance.
If a required parity behavior cannot be preserved, explicitly document why
and keep the migration unaccepted until the user approves the deviation.

Additionally, for every user-facing feature or client migration, include a
concise, evidence-based comparison with the **relevant competing product**
(e.g. Kodi and Plex for TV/media discovery and playback). Mark unavailable or
unverified competitor behavior as such. Use this comparison to identify
usability gaps, not as permission for speculative features, security bypasses,
duplicated infrastructure or unrelated refactoring. The already working
Webfrontend remains the minimum functional baseline, independent of what a
competitor does.

## Public-v1 VIDAA search permission contract (2026-10-15)

**Verbindlich bei Änderungen an Device-Suche oder Grants:** Vorher den
[Public-v1-VIDAA-Berechtigungsvertrag](docs/development/public-v1-vidaa-search-permissions.md)
vollständig lesen und seine bestehenden Berechtigungen samt Tests
beibehalten. Keine stillschweigende Aufweichung der Autorisierung.

## Mandatory: NEVER compile the same daemon twice / reuse before building

**ABSOLUTE PROHIBITION:** Do not compile the same daemon twice. Re-running a
daemon compilation simply because a chat restarted, a deployment failed, a
new temporary checkout was created, an agent changed, CI ran, or a
**documentation-only commit changed HEAD** is forbidden. Reuse a previously
validated build; build only when absolutely necessary and **provably no
suitable verified artifact exists**.

**Required read-only gate BEFORE any new checkout, test build or installation
block is proposed or executed:**

1. Establish the **last product-code candidate SHA** (the daemon binary's
   exact source/build provenance) separately from the **current repository
   documentation HEAD**. For a docs-only commit, verify the diff has no
   daemon source, generated/version-stamp inputs, build options, dependency,
   toolchain or packaging changes. Do **not** treat a changed docs Git SHA
   by itself as a reason to rebuild. Reuse the last verified code-SHA and
   its already accepted build/test evidence.
2. Inspect the **actually installed and running** daemon and its source
   marker, the target code-SHA, and all *known* previous, authorized
   checkout/build locations. Do not blindly discard or replace old
   checkouts or assume a newly cloned tree has the build cache. Ask for a
   small read-only host preflight if installed-build evidence is not
   available to the agent. Record the exact path and source SHA.
3. Within the correct, clean checkout of that code-SHA, run the installer's
   read-only `check --sha FULL40`; inspect `INSTALLED_SOURCE_SHA`,
   `TARGET_SOURCE_SHA`, `BUILD=REUSABLE` versus
   `BUILD=NOT_SEALED`, the candidate path, and the running service
   identity. `check` validates an existing sealed candidate's SHA-256
   and embedded `git-<source-prefix>` marker. Do not infer valid reuse
   from an artifact filename, timestamp, or identical-looking bytes alone.
4. If the intended code is **already installed and running correctly**,
   report `BUILD=SKIPPED_ALREADY_INSTALLED` and
   `APPLY=SKIPPED_ALREADY_INSTALLED`. Do not build, restart, or redeploy.
5. If `BUILD=REUSABLE` and the sealed candidate is verified, report
   `BUILD=SKIPPED_REUSABLE` and **go directly to the separately gated
   root `apply`**; **do not invoke `build` at all**.
6. If the seal is absent, inspect the known artifact directory:
   `build` may certify an existing, complete, valid binary without a
   compilation (`BUILD=RECOVERED_NO_COMPILE`). Use that supported
   recovery rather than recompiling. **Only if there is no valid binary
   or seal anywhere in the identified trusted candidate checkout(s)**
   and product-code changes actually require it, authorize **one**
   build in the already authorized yaVDR root shell, under the installer's existing provenance gates. An invalid/mismatched artifact is a STOP requiring
   diagnosis; do not overwrite or rebuild over it speculatively.
7. For an existing build in another checkout, first establish exact
   source provenance, seal integrity, build options and trust. Do not
   silently copy, relabel or re-seal a binary as a different commit,
   weaken `verify_binary`, or declare two commits equivalent without
   proving the build inputs match. Prefer using its original verified
   checkout and `apply`. If safe reuse cannot be proven, **STOP for
   diagnosis** instead of automatic recompilation or new clones.
8. An actual, reviewed change to daemon build inputs, a missing valid
   binary after exhaustive bounded inspection, or an explicitly
   demonstrated corrupt/nonreusable artifact can warrant **one** new
   compile, never automatically. Report **why** the old build was
   unsuitable before invoking the build tool. On yaVDR, keep the operator's
   existing root shell for the build; `apply` remains separately authorized.

**Confirmed installer behavior:** `tools/install_daemon.py check` prints
`BUILD=REUSABLE` only after validating the seal and binary. Its `build`
action internally prints `BUILD=REUSED` if sealed or
`BUILD=RECOVERED_NO_COMPILE` if the already-linked binary can be verified.
This safety optimization is **per checkout and source SHA**; it does
**not** discover builds in other checkouts. Therefore the operator/agent
**must check existing artifacts before creating a new checkout**.
Do not confuse "build command is idempotent" with authorization to launch it
repeatedly.

## Canonical runtime/frontend deployment

**Verbindlich für vollständige yaVDR-Runtime-/Frontend-Deployments:**
Nur die vorhandene gestagte `install-runtime`-/`DESTDIR`-Komposition
deployen, niemals den ungepackten Frontend-Quellbaum als Live-Payload
verwenden. Die Installation erst nach
`RUNTIME_DEPLOYMENT_MATCH=YES` als verifiziert melden.
**Vor einem solchen Deployment vollständig lesen:**
[Canonical Runtime-Deployment-Vertrag](docs/development/install-runtime-deployment-reliability.md).
Die gesondert genehmigten reinen Einzeldatei-Updates unter
„Usability First“ sind keine vollständigen Runtime-Deployments.

## KISS: no arbitrary storage-reserve gates for Media Tools installation

The maintainer explicitly rejected mandatory filesystem-capacity preflights and
fixed free-space reserves (such as 4 GiB or 15%) for Media Tools staging,
installation and updates on 2026-10-28. Do not reintroduce such restrictions
in shell installers, stage composers, test blocks, or equivalent wrappers.
Reuse the already-built binaries and sealed stages whenever available.
Report real filesystem I/O failures plainly; do not perform speculative cleanup
or delete existing user data. This instruction does not authorize changes to VDR
recordings, permissions, configuration or access grants.

## Testblock repository entry

Every local VDR-Suite test or acceptance block handed to the user must enter the
intended VDR-Suite checkout before any Git, build, test, install, service or
runtime command. The first executable repository action in the block must be an
explicit `cd` to the checkout root, or a safe resolver whose successful result
is immediately `cd`'d before any repository command. Never assume the user's
shell is still in the repository merely because a previous command or message
was.

When the checkout path is already known from the current acceptance context,
use that exact absolute path. Do not rely on `~/vdr-suite` in blocks that may be
run after `sudo`, `sudo su`, `su`, or another user change, because `~` may resolve
to a different home directory. If the exact path is not known, resolve the
current checkout and bounded known absolute candidates without discarding a
valid current `vdr-suite` worktree, then enter the resolved root. A failed
hard-coded candidate must never cause `STOP` while the current directory is
already inside the correct checkout.

Immediately after entering, verify the repository root with
`git rev-parse --show-toplevel` and, when useful, the expected repository name
before branch/head validation. Test blocks must be self-contained and safe to
paste from an arbitrary shell working directory.

## Frontend lifecycle and composition gates

For cross-cutting frontend behavior, prove the real production composition and
lifecycle before treating an isolated implementation as integrated. Playback
work must follow `docs/development/frontend-playback-integration-contract.md`.

A wrapper around an exported method is not proof that the real browser action
passes through that wrapper. Owners may bind DOM controls directly to internal
closures before decorators are installed. Session-bound extensions must observe
the canonical owner/session lifecycle or an explicit ownership event from owner
creation through replacement, stop/restart and destroy; they must not rely only
on intercepting `start()`, `stop()`, seek or selection methods.

Tests for a new cross-cutting control must exercise the same composition root and
owner topology used by the production view, including transport replacement
when production can perform it. A direct unit call to the decorator method is
useful but insufficient as the integration test. At least one regression must
prove the user-style action -> canonical owner/session transition -> expected
Suite request/state change chain.

Keep owner-level UI outside replaceable transport DOM and extend established
owners instead of adding parallel player, request, restart or cleanup paths.
When persistent browser objects are involved, installed bundle identity and
syntax are deployment evidence only; runtime acceptance must recreate the
relevant owner before claiming the new lifecycle integration is active.

## Continuous approved work

Continue through all already-approved steps of a bounded workstream without
artificial confirmation pauses. Do not stop after analysis, after each file, or
after each commit when the next operation is already covered by the user's
instruction and the repository contract.

Do not hand a candidate to the user for acceptance testing while the current
branch head has a known implementation, runtime-wiring, packaging, deployment or
validation gap that prevents the requested acceptance scenario from being tested
truthfully. Such an intermediate head is not a testable candidate. Continue the
approved implementation and stabilization work until the requested acceptance
scope is actually runnable.

Potential blockers are not stop permissions. They matter only when they make
every remaining authorized operation impossible after all safe evidence and
independent work have been exhausted. Examples include an unrecoverable remote
branch conflict, inseparable unrelated changes, unavailable required source
content, a genuine security/data-loss/runtime compatibility decision requiring
new input, an irreparable required stabilization failure, or an exact gated
operation for which authorization has genuinely never been granted. In each
case, continue every other approved operation first and preserve the workstream
as blocked rather than declaring it complete.

## Commit, push, CI and minimal necessary validation

**Grundsatz:** Safe, tested product code comes first. Complete the bounded
implementation and its relevant focused tests before full-CI stabilization.
Use the shortest checks that can still affect the next decision. Do not turn
unrelated CI, documentation or tests into extra installation blockers.

**Commits and iterative work:**
- Create small, coherent, scoped commits; push each finished commit promptly
  with fast-forward-only semantics. Do not await GitHub Actions after every
  commit. Continue already-approved independent work while earlier runs are
  queued or running. Superseded intermediate runs need separate diagnosis
  only if they reveal a problem affecting the current candidate.
- Select checks by the *actual changed surface*, before its relevant gate:
  - frontend JS/CSS/HTML: focused frontend tests and
    `frontend-regression-test` before installing the candidate;
  - frontend installation or packaging wiring: relevant staging/packaging
    check as well;
  - backend, daemon, C++, runtime contracts: matching build,
    fast-regression, architecture and other directly affected checks;
  - documentation only: review content, text, links and formatting;
    normally the minimum local check is `git diff --check`.
- A required failed check must be diagnosed and repaired before crossing its
  gate. Reproduce with the smallest relevant check and fix only the
  demonstrated cause; do not hide failures with unrelated commits or bypass
  relevant tests, security requirements or branch protection.
- Do not run broad local Make targets, full regression suites, packaging
  builds or local copies of hosted CI on unchanged code when GitHub Actions
  already supplies the needed evidence. Additional full documentation suites
  are not justified for routine docs-only edits unless the exact change
  cannot be appropriately validated through the final-head CI.

**CI and acceptance gates:**
- During draft development and targeted runtime acceptance, require only
  surface-relevant tests. An unrelated workflow job must not block a
  separately authorized installation or acceptance test merely because it
  belongs to the same workflow; establish that the failure is genuinely
  unrelated before continuing. Targeted tests for changed product code
  remain mandatory before the corresponding risky action.
- The **complete repository-required CI graph for the exact final head**
  is mandatory at Ready-for-review, merge into `main`, phase closeout or
  another explicitly required full-stabilization gate. Before merging to
  `main`, all required final-head checks must pass, together with any
  distinct safety/hardware/branch-protection requirements. Do not merge red
  or unverified code.
- A successful **local build of the exact daemon candidate** counts as its
  compilation evidence. Do not require an additional hosted build solely
  to prove compilation a second time; this does not waive distinct
  functional/security checks or hosted checks imposed by branch protection.
- After an authorized merge, verify the actual merged `main` SHA and its
  hosted CI. If it exposes a defect, repair the current `main` through the
  permitted branch/PR procedure, then verify the newly merged result.
  Never rewrite `main`, bypass PR restrictions or overlap others' work.

**Documentation and evidence reuse:**
- Documentation-only changes do **not** trigger a daemon rebuild, fresh
  runtime/hardware acceptance, CI polling/waiting or a new commit merely
  to obtain a green check. Commit, inspect and report the actual docs
  change, then proceed with remaining authorized work. GitHub Actions may
  run automatically; full CI is evaluated at its actual final-head gate.
- Reuse accepted CI, builds, runtime checks and immutable fingerprints
  until a *relevant input* changes. A new chat, unrelated commit or docs
  update alone is not a reason to repeat verification.
- Use the local yaVDR host for evidence GitHub cannot supply: installed
  binary, service, real configuration, database integrity and controlled
  live-VDR acceptance. Do not repeat recorded runtime acceptance merely
  because documentation changed.
- Never introduce speculative validators, duplicate safety gates, large
  recovery scripts or repeated CI polling. Status documentation and
  closeout notes should be updated at a meaningful product milestone
  rather than repeatedly blocking productive changes.

## Branch and pull-request safety

Keep updates fast-forward-only. Never force-push, rebase or rewrite published
history unless the user explicitly approves that exact operation.

Do not mark a Draft pull request Ready, merge it, close it, enable auto-merge,
change its base, or mutate review state or PR metadata without explicit user
approval.

Keep each implementation slice within its documented boundary. Do not combine
unrelated phases, runtime installation, broad refactors or deferred features
merely to reduce the number of commits.

### Main-first branch lifecycle and grandfathered work

Follow [Branch- und Pull-Request-Lebenszyklus](docs/development/branch-pull-request-lifecycle.md).
New work starts on the **current `main`** of the correct repository; do
not create nested successor branches for the same work. **Branches and PR
stacks already active before 2026-10-18 are grandfathered**: never
force-rebase, rename, close, merge, delete or replace them just to satisfy
the new rule. Any new stacked branch needs explicit approval and a documented
dependency.

After an authorized merge confirm `Merged/Closed`, exact `main` head and
relevant CI, then delete the head branch **only after** checking Launchpad
recipes, dependent PRs, workflows and other consumers. Report
`BRANCH=RETAINED` with the reason if needed or if deletion is unavailable.
Branch protection always takes precedence over a request for a direct
`main` write.

## Reporting

Report completed commits, the current remote head, relevant validation and the
next actual gate. Do not present every pushed commit as a reason to pause. When
CI is intentionally batched, state that the final current head is the one whose
CI result matters.

Whenever a GitHub Actions run exists for the relevant head, every CI status
report must include a directly clickable link to that exact run. Also state the
workflow or run number, run ID and head commit so the user never has to search
for the run manually. This applies while the run is queued, running, failed,
retried or completed. Do not provide only a textual CI status. When no run has
been assigned yet, state that explicitly and provide the link as soon as the run
exists.

Before ending any working response, verify that the requested end state has
actually been reached. If a relevant CI run exists for the current head and its
result matters to the next already-authorized gate, re-read that run immediately
before the response ends. Never finish with a statement that the next step is
still executable with the available tools; perform that step instead.

## Testblock shell option safety

VDR-Suite and yaVDR test/acceptance blocks must never enable shell fail-fast or
unset-variable abort semantics. Do not use `set -e`, `set -u`, `set -o pipefail`
or combined forms such as `set -euo pipefail`. Test blocks must keep the
interactive shell alive so later diagnostics remain visible. At the beginning
of every handed-off test block explicitly neutralize inherited settings with:

```bash
set +e
set +u
set +o pipefail
```
