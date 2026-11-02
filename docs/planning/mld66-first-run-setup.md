# MLD.F2 – Benutzerfreundliche Erstinstallation und lokale Backend-Einrichtung

**Arbeitsbranch:** `work/mld66-skippy-feedback-20261027` (kein weiterer Branch).  
**Status:** In Umsetzung ab 02.11.2026; keine MLD-Laufzeitfreigabe.  
**Anlass:** skippys MLD 6.6.0: VDR und Suite-Daemon aktiv, Agent-Dienst inaktiv, `identity` und `enrollment` fehlen, `AGENT_DB=NO_ACTIVE_AGENT_ROWS`, Live-TV meldet `live_backend_authority_required`. FFmpeg ist vorhanden.

## Ziel / Nutzererlebnis

Nach Paketinstallation öffnet der Betreiber die VDR-Suite-Weboberfläche, legt den ersten Administrator an und erhält einen benutzbaren lokalen VDR. **Keine manuelle Backend-ID, kein Enrollment-Kommando, keine Dateiablage und kein systemctl-Aufruf als Teil des normalen Benutzerwegs.** Die Einrichtung läuft mit sichtbarem Fortschritt, verständlicher Fehlermeldung und Möglichkeit zur Wiederaufnahme. Sie verändert keine bestehenden VDR-Timer, Sender, Aufnahmen oder Benutzerkonten.

Die bestehende First-Admin-Weboberfläche und deren Claim-/Session-Logik bleiben Eigentümer der Kontoeinrichtung. Backend-Agent-Enrollment bleibt bei `BackendAgentLifecycleService` und `vdr-suite-backend-agent-enroll`; der Agent selbst konsumiert das geschützte Enrollment-Paket und schreibt seine Identität. `vdr-suite-backend-agent-autostart` bleibt der vorhandene Weg für den geprüften Autostart. **Kein zweiter Agent, keine zweite Datenbank, keine MLD-Sonderimplementierung der Suite.**

## Verifizierter Quellbefund

- `web/frontend/api/live-remote-client-api.js`: First-Admin-Status und -Claim bereits mit Formular, Login und Browser-Session.
- `apps/tools/backend_agent_enrollment_create.cpp`: bereits registriertes Backend wird mit einem einmaligen, zeitlich begrenzten Enrollment-Paket verknüpft.
- `core/agent/src/BackendAgentClient.cpp`: bei fehlender `identity` wird ein vorhandenes `enrollment` eingelesen; nach erfolgreichem Enrollment wird die Identity gespeichert und das Enrollment-Paket entfernt.
- `tools/vdr_suite_backend_agent_autostart.py`: schützt den Autostart eines **bereits laufenden, registrierten** Agenten. Start/Enrollment sind nicht enthalten.
- `core/agent/src/BackendAgentLiveProviderRuntime.cpp`: `prepare()` verlangt vor dem Stream eine Agent-Zuordnung; `BackendAgentLiveProviderAuthority` verlangt zusätzlich einen passenden Kanal-Snapshot. Ein bloß laufender Dienst genügt deshalb nicht.
- Die vorhandene SuiteBridge-Embedded-Observation ist **nicht** automatisch eine registrierte `BackendAgentRecord`-Identität für den genannten Live-TV-Pfad.

## Slices und Abnahme

| Slice | Umsetzung (kleinstmögliche bestehende Oberfläche) | Nachweis / Fertig wenn | Stand |
| --- | --- | --- | --- |
| **F2.0 – Ist-Zustand und Nutzerfehler** | Den bestätigten Agent-/Backend-Fehler mit First-Admin-Flow, bestehender Enrollment-Logik, Paket- und Service-Zustand verknüpfen. Im bestehenden Live-TV-Frontend statt technischer Authority-Codes eine zutreffende Meldung über fehlende Backend-Einrichtung anzeigen; Initialfehler beendet den Start bereits ohne Recovery-Loop. | `web/frontend/live-tv-view.js` klassifiziert Code oder Nachricht; der vorhandene DOM-Stabilitätstest enthält zwei neue Fehlerfälle und hält den verschlüsselten-Sender-Fall intakt. Fokus-V8-Test des produktiven JS-Pfads PASS, CI und MLD-Installation separat. | **SOURCE IMPLEMENTED; FOCUSED V8 PASS; CI / MLD OPEN** |
| **F2.1 – Lokalen Installationsfall erkennen** | Nach normalem Admin-Claim vorhandene Backend-/SuiteBridge-Fakten lesend prüfen: genau ein lokaler VDR oder bereits konfigurierte/mehrere Backends; keine neue harte Paketabhängigkeit. Bestehende funktionierende Konfiguration wird nicht verändert. | Auf frischem Einzel-VDR wird eindeutig `setup_required` erkannt; auf bestehender Installation `already_configured`; bei unklarer Topologie kein automatischer Eingriff. | **SOURCE PARTIAL: read-only preflight; browser/install integration OPEN** |
| **F2.2 – Agent automatisch einmalig registrieren** | Im von der Installation kontrollierten lokalen privilegierten Einrichtungskontext die vorhandene Agent-Konfiguration aus tatsächlichem Backend-/Control-Plane-Kontext nutzen; vorhandenes Enrollment-Werkzeug aufrufen, geschütztes Paket an den bereits vorgesehenen Ort legen, vorhandene Identität niemals überschreiben. Ein einmaliger Setup-Schritt nach erfolgreichem Admin-Claim, kein offenes anonymes Setup-API. | Derselbe Backend-ID-/Datenbank-/Konfigurationskontext, einmalige Registrierung, korrekt geschützte Identity, keine ausgegebenen Secrets und kein zweiter Agent bei Wiederholung. | **SOURCE PARTIAL: one-time package preparation implemented; automatic trigger / MLD real test OPEN** |
| **F2.3 – Dienststart und Kanalbeobachtung** | Vorhandenen Agent-Dienst nach erfolgreichem Enrollment einmalig starten; lokale Senderbeobachtung auf die tatsächlich verwendete MLD-`channels.conf` konfigurieren und Provider-Fakten prüfen. Erst bei Online-/Backend-Health-Readback den vorhandenen Autostart-Helfer nutzen. Keine pauschalen Neustarts von VDR oder Daemon. | Agent online, gültige Lease, passende `BackendAgentRecord`, aktueller `channels`-Cursor, Autostart nach Hostneustart; kein neuer Systemdienst. | **SOURCE PARTIAL: Agent start + live Channel readback + autostart wired; MLD config / actual runtime OPEN** |
| **F2.4 – Web-Setup-Abschluss und Wiederaufnahme** | First-Admin-Webablauf um tatsächlichen lokalen Backend-Fortschritt ergänzen. Kein technischer Fragebogen; ggf. ein verständlicher Fehler mit `Erneut prüfen`, ohne doppelte Registrierung oder erneute Kontoanlage. Bereits eingerichtete Systeme kommen direkt zur normalen Oberfläche. | Ein Setup-Ablauf im Browser; Fehler lassen sich ohne Konsole erneut prüfen; Authentifizierung und bestehende Admin-Session bleiben erhalten. | **SOURCE IMPLEMENTED: existing health readback and simple retry banner; installer flag / real MLD OPEN** |
| **F2.5 – Distribution und Paketierung** | Bereits vorhandene Dateien/Tools über gemeinsame Runtime-/Package-Contracts installieren; nur Service-User, VDR-ABI, Senderpfad und privilegierten lokalen Setup-Auslöser im MLD-Paket anpassen. Ubuntu/yaVDR mit bestehendem Agent bleibt unverändert. | MLD-/Ubuntu-Paketinhalt, Rechte, Bootverhalten und Upgrade ohne Identity-/DB-Verlust geprüft; kein Debian-Skript in MLD kopiert. | **SOURCE PARTIAL: MLD staging target and idempotent installer entrypoint implemented; external recipe/HTTPS/host acceptance OPEN** |
| **F2.6 – Echte Abnahme mit skippy** | Einen installierten, SHA-identischen MLD-Testkandidaten nach Installationsfreigabe gemeinsam testen. Danach F1.4 Live-TV mit Start, Senderwechsel, Bild/Ton und Stop. | `FIRST_ADMIN=PASS`, `LOCAL_BACKEND_SETUP=PASS`, `AGENT_ONLINE=PASS`, `CHANNEL_OBSERVATION=PASS`, `LIVE_TV=PASS` auf echter MLD-Installation; Fehler getrennt dokumentieren. | **OFFEN** |

## Umsetzungslinien / Nicht-Ziele

1. **Erstbestandsaufnahme vor Mutation:** Die vom Installer verwaltete lokale Instanz und ihr Dienstkonto müssen tatsächlich bekannt sein. Keine automatische Registrierung auf unklaren Mehrbackend-/Remote-Installationen.
2. **Keine neue Sicherheits- oder Architektur-Schicht:** vorhandene administrative Enrollment- und Authentifizierungsfunktionen benutzen; falls deren Privileggrenze einen automatischen Browserauslöser nicht zulässt, zuerst den bestehenden installerseitigen Ausführungskontext nachweisen, statt einen zweiten öffentlichen HTTP-Endpunkt zu erfinden.
3. **Kein manueller Endnutzer-Workaround als Fertigmeldung.** Diagnose-Skripte sind nur Entwickler-/Testwerkzeuge.
4. **Isolierte Evidenz:** Quellcode-, CI-, Paket- und REAL-MLD-Ergebnisse getrennt berichten. Ein geändertes Frontend ist nicht gleich vollständige Auto-Einrichtung.
5. **Kein Media-Tools-Code, kein Main-Merge und keine Installation auf yaVDR oder MLD durch diese Roadmap.** F1.4 wird nach F2.3 wieder aufgenommen.

**Abhängigkeit:** Das automatische privilegierte Setup muss dort starten können, wo das MLD-Paket die Agent-Konfiguration und Service-Rechte tatsächlich verwaltet; genau dieser Paket-/Laufzeit-Auslöser ist bisher nicht nachgewiesen. F2.1/F2.2 dürfen daher nicht durch ein unautorisiertes Frontend-`systemctl` ersetzt werden.

### F2.1 Quellstand

Der bereits installierte Helper `vdr-suite-backend-agent-autostart` bietet
jetzt `--setup-status` als **ausschließlich lesende** Vorprüfung.
Er unterscheidet fehlenden/laufenden VDR und Daemon, fehlende Agent-
Installation, unvollständige Konfiguration, bereits vorhandene Identity
oder Enrollment sowie den Fall `ENROLLMENT_REQUIRED`. Er liest keine
geschützten Geheimnisse und startet oder registriert keinen Agenten.
Isolierte Tests in `tools/test_packaging_backend_agent_autostart.py`
belegen die zustandsbezogene Ausgabe ohne Schreiboperationen.

**Nicht damit erledigt:** Die tatsächliche lokale SuiteBridge-Fähigkeit,
ein verbindlicher installer-seitiger Setup-Aufruf, Backend-ID-/Kanalpfad-
Übernahme und die Web-Statusanzeige fehlen noch. Das Hilfswerkzeug ist
ein interner Baustein für F2.2 bis F2.4 und keine Installationsanweisung
an skippy.

### F2.2 Quellstand

Der bestehende installierte Helper
`vdr-suite-backend-agent-autostart --prepare-enrollment`
stellt nun einen **ausschließlich für den vertrauenswürdigen
Installationskontext gedachten** einmaligen Enrollment-Schritt bereit.
Er benutzt `vdr-suite-backend-agent-enroll`, bestehende Backend-ID
`default` und den Standardpfad der lokalen Control-Plane-Datenbank.
Er verlangt ein frisches Agent-Verzeichnis sowie eine bereits gültige
Control-Plane-URL, verweigert vorhandene Identity/Enrollment-Dateien
und übergibt nur das geschützte Enrollment-Paket an das bestehende
Agent-Dienstkonto. Es werden keine Agent-Secrets ausgegeben und
kein systemd-Dienst gestartet oder aktiviert.

Isolierte Tests prüfen den Aufruf des vorhandenen Enrollment-Werkzeugs,
die einmalige Ausführung, die Dateiübergabe sowie die Ablehnung von
nicht privilegierten oder bereits registrierten Zuständen. Der
bestehende `test-ci-packaging`-Verbund enthält diesen Test jetzt als
`test-mld-agent-first-run`.

**Wichtig:** Dies ist noch **keine vollständige automatische Registrierung
nach Browser-Claim**: Aufruf durch den MLD-Installer, MLD-spezifischer
Dienstbenutzer und tatsächlich übereinstimmender Datenbankpfad/HTTPS-Origin
sind unter F2.5 noch zu verbinden; das Registrieren über die Agent-Laufzeit,
die Channel-Observation und der Autostart folgen in F2.3.

### F2.3 Quellstand

Der vorhandene installierte Helper besitzt jetzt eine installerinterne
Fortsetzung `--activate-local` für ein **bereits vorbereitetes**
Enrollment-Paket. Der Agent wird exakt einmal durch die bestehende
`vdr-suite-backend-agent.service` gestartet (kein VDR-/Daemon-Neustart).
Danach wird nicht bloß `systemctl` als Erfolg interpretiert: die
vorhandene `--status`-Authority muss den registrierten, aktiven Agenten
mit gültigem Backend-Health-Readback melden. Zusätzlich muss ein
zu diesem Backend und Agenten passender aktueller
`channelObservation`-Cursor mit mindestens einem Sender bestätigt
sein. Erst anschließend darf der bestehende Boot-Aktivierungsweg
`systemctl enable` ausführen; bei fehlender Beobachtung bleibt der
Autostart unverändert.

Der Test `test-mld-agent-first-run` deckt den positiven Ablauf,
fehlende Kanal-Konfiguration, Readback-Mismatch und abgelaufenen
Kanal-Cursor ohne echte systemd-Änderungen ab.

**Weiter offen:** Die MLD-Paketierung muss vor Agentstart den
**tatsächlichen** `CHANNELS_CONF_PATH` und
`ADAPTERS=channels-conf` / `OBSERVATION_DOMAINS=backend-health,channels`
eintragen. In MLD darf `CHANNELS_CONF_PATH` wie dokumentiert ein
Symlink auf die vom Benutzer ausgewählte Senderliste sein; geprüft wird
das vorhandene reguläre Zieldokument, nicht die Ablehnung des Symlinks.
Die Paketvorlage schaltet diese Beobachtung absichtlich
noch nicht standardmäßig ein. Der bestehende Helper nimmt keine
unbekannten MLD-Pfade an. Der automatische, für Benutzer unsichtbare
Installer-/Browser-Aufruf folgt in F2.4/F2.5; die echte MLD- und
Neustart-Abnahme bleibt F2.6.

### F2.4 Quellstand

**Einfacher Ablauf ohne zweiten Wizard:** Die bestehende
`/api/backends/default/health`-Antwort erhält optional
`localBackendSetup.status`, abgeleitet aus der bestehenden
registrierten Agent-Identität, aktueller Lease, Backend-Health-
und Channel-Beobachtung. Fester Status:
`unregistered`, `agent_offline`, `health_pending`,
`channels_pending` oder `ready`. Der Daemon bietet das
Feld **nur nach explizitem Installations-Opt-in**
`VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true` an; bestehende
Ubuntu/yaVDR-Installationen ohne Flag bleiben unverändert.
Kein neuer HTTP-Endpunkt, keine neue Browser-Privilegierung.

Das bestehende First-Admin-/Login-Frontend zeigt nach Anmeldung
nur bei unvollständiger Backend-Bereitschaft einen kurzen
Statushinweis mit `Erneut prüfen`. Der Status wird automatisch
erneut lesend abgefragt, bis `ready` gemeldet wird.
Bei fertigem oder nicht optiertem Backend wird kein Panel gezeigt.
Keine erneute Kontoanlage, kein Enrollment-/systemctl-Aufruf
im Browser. Der vorhandene Login bleibt unabhängig vom
Backend-Status nutzbar.

Frontend-Regression deckt Registrierung ausstehend, Kanäle
ausstehend, fertig, normale Installation und manuelle Status-
Wiederprüfung ab. Zusätzlich sind die Anmeldung mit bereits
bestehendem Administrator ohne erneuten Claim und der Fall einer
fehlgeschlagenen Health-Abfrage mit sichtbarem `Erneut prüfen`
und erfolgreicher Wiederprüfung abgedeckt. Der
Backend-Health-Routertest prüft die optionale Antwort ohne neues
Feld für andere Backends.

`ready` bedeutet hier **Agent/Channel-Setup bereit**, nicht
automatisch erfolgreiche Bild-/Ton-Wiedergabe; die separat
geprüfte SuiteBridge-Live-Provider-Freigabe und der Realtest
bleiben F2.6.

**Offen für die echte Ersteinrichtung:** F2.5 muss beim MLD-
Paket den *tatsächlichen* Kanalpfad und Agent-Dienstnutzer
setzen und den privilegierten lokalen First-Run-Schritt
auslösen. Ohne diesen Aufruf zeigt F2.4 nur den wahren
Ist-Zustand, repariert aber kein fehlendes Enrollment.
Keine MLD-Laufzeit-Abnahme und keine automatische Registrierung
durch diese UI-Änderung.

### F2.5 Quellstand

Im vorhandenen Agent-Autostart-Werkzeug gibt es nun
`--installer-onboard` als **einmaligen, root-eigenen Paket-/First-Run-Aufruf**.
Er verbindet die bereits bestehenden Schritte aus F2.2/F2.3:

- `ENROLLMENT_REQUIRED`: gültige lokale HTTPS-/Agent-/Kanal-Konfiguration
  verlangen, einmalig Enrollment vorbereiten, vorhandenen Agenten starten,
  Health- und Channel-Readback abwarten, erst dann Autostart freigeben.
- `ENROLLMENT_PENDING`: vorhandenes Enrollment **weiterverwenden**,
  keine zweite Token-Ausgabe, anschließend Agentstart und Readback.
- `IDENTITY_PRESENT`: einen **bereits laufenden** Agenten mit
  gültigem Backend-/Kanal-Readback bestätigen und ggf. nur dessen
  Boot-Aktivierung abschließen; keine erneute Enrollment-Erzeugung.
- Ungültiger Dienst, fehlender VDR, abweichendes Backend, fehlende
  Kanal-Konfiguration oder untrusted URL: **blockiert vor einer Mutation**.

`tools/mld66_stage_first_run.py` und der neue Make-Target
`stage-mld-first-run-config` konfigurieren im **Paket-Staging**, nicht
im Live-System, die bekannten zwei Originaldateien
`backend-agent.conf` und `vdr-suite-daemon.default`.
Die Anpassung setzt Channel-Observation auf den von der MLD-Rezeptur
ausgewählten `channels.conf`-Pfad und aktiviert das bereits
implementierte Web-Health-Statusfeld. Ein tatsächlicher lokaler
**HTTPS**-Control-Plane-Origin muss von der Rezeptur übergeben werden.
Ohne Origin läuft das Staging nicht stillschweigend weiter. Bereits
individuell gesetzte Origins/Senderpfade und die Original-Identity
werden nicht überschrieben. Normales `make install` auf Ubuntu/yaVDR
ruft dieses MLD-Ziel nie auf.

Tests im bestehenden Packaging-Verbund prüfen die wiederholbare
Enrollment-Orchestrierung, vorhandene Identität, fehlende
Vorbedingungen, MLD-Staging-Idempotenz und Schutz vor Symlink-Escape
in das Host-`/etc`.

**F2.5 ist noch nicht abgeschlossen:** Das eigentliche
MLD-BitBake-Rezept ist nicht in diesem Repository vorhanden und wurde
hier nicht mutiert. Es muss die neue Staging-Funktion übernehmen und
die bereits paketierte `--installer-onboard`-Funktion in seinem
trusted first-run-Lebenszyklus ausführen (nach VDR/Daemon-Start).
Die MLD HTTPS/TLS-Origin, CA-Vertrauen, Dienstkonto,
Paket-Reihenfolge und echte Persistenz nach Neustart sind nicht
bestätigt. Weitere technische Details und der konkrete Übergabevertrag
stehen in [`packaging/mld66/README-first-run.md`](../../packaging/mld66/README-first-run.md).
Ohne die native Rezept-Integration behebt dieses Ergebnis noch
**keine** frische skippy-Installation.
