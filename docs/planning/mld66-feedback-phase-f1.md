# Phase MLD.F1 — Skippy-Feedback: Bedienung und Funktionsabnahme auf MLD 6.6

**Status:** PLAN / Quellenstand 27.10.2026. Implementierungsarbeiten sind nur dort als vorhanden markiert, wo sie nachweislich im Branch vorliegen. **Keine reale MLD-Gesamtabnahme.**

**Repository:** `hotzenplotz5/vdr-suite`  
**Arbeitsbranch:** `work/mld66-skippy-feedback-20261027`  
**Abzweig:** `main` `cc6cc4678fb10a4376d931c21fbf2ae14e2d7038`  
**Feedback und Plugin-Matrix:** [MLD-Feedbackprotokoll](../development/mld66-skippy-feedback-20261027.md)  
**Übergeordnete Distributionsintegration:** [MLD.1A–MLD.1E](mld-6.6-integration.md)  
**Suchtimer-API-Vertrag:** [Public API v1 SearchTimer](../../api/search-timers.md)

Diese **Feedbackphase** ist ein begrenzter Arbeitspfad innerhalb des bestehenden
MLD-Ziels, **keine** Umnummerierung von Phase 70, kein neuer Produkt-Fork,
keine Ersatzimplementierung von Public API v1 und keine automatische
Freigabe der MLD-Pakete. Quellverbesserungen bleiben auch für Ubuntu
nutzbar; MLD-spezifische Integration wird als eigene Distributionsschicht
behandelt. `main`, der alte MLD-Integrationsbranch und produktive Systeme
werden durch die Planung nicht verändert.

## Ziel: Was kann skippy danach tatsächlich testen?

1. Im Browser werden Sendernamen in Suchtimer-Treffern und verständliche
   verfügbare Berechtigungen angezeigt.
2. Suchtimer lassen sich mit den für epgsearch relevanten Einstellungen in
   **einem** Editor konfigurieren, in der Vorschau prüfen und bei ausdrücklichem
   Wunsch **direkt aktiv** speichern. Das Backend-Ergebnis wird zuverlässig
   bestätigt oder als ungeklärt gemeldet; kein doppeltes Anlegen.
3. Die Live-TV-Vorschau spielt den ausgewählten Sender, sobald der reale
   Provider dafür technisch verfügbar ist; andernfalls benennt die Oberfläche
   den tatsächlichen Fehler verständlich.
4. Ein vorhandener VDR-Mitschnitt lässt sich im Browser starten und abspielen,
   soweit Quellformat/Clientprofil unterstützt werden.
5. Die Installation erklärt nötige und optionale Komponenten, darunter
   epgsearch, SuiteBridge, RESTfulAPI, TVScraper, Logos, Videotext und
   gegebenenfalls Live-Provider/FFmpeg, ohne pauschal Plugins zu verlangen.
6. Timer-Vor-/Nachlauf wird nach MLD-Paketaktualisierung kontrolliert geprüft.
   Bereits akzeptierte Fernbedienung und Kanallogos bleiben regressionsfrei.

## Slice-Plan

| Slice | Ziel und begrenzte Umsetzung | Abnahmekriterium | Stand |
| --- | --- | --- | --- |
| **F1.0 – Bestehende UI-Fixes absichern** | Bereits eingecheckte Kanal-ID-zu-Klarname-Auflösung und scopeabhängige Berechtigungsauswahl gezielt testen. Angezeigte Admin-Expansion ist keine neue serverseitige Berechtigung; Add-on-Rechte bleiben unabhängig. | Tests auf Branch-HEAD grün; Vorschau bleibt bei fehlender Kanalliste nutzbar; Berechtigungsdialog bietet keine effektiven Doppelrechte, Server-Gates bleiben unverändert; später MLD-UI-Readback. | **SOURCE IMPLEMENTED; FOCUSED TEST OBSERVED; MLD OPEN** |
| **F1.1 – Suchtimer-CREATE-Fehler klären** | Am bestehenden Browser- und Public-v1-Pfad `POST /api/v1/search-timers` den tatsächlichen Grund für „requested public API resource is temporarily unavailable“ ermitteln: Version/Route, Agent, Backend, Anbieterfähigkeit, Berechtigung, Operationstatus und nativer Readback. Nur nach belegtem Defekt minimal korrigieren. | Genau ein vollständig nachvollziehbarer Zustand je CREATE-Anfrage (angelegt, nachweislich ohne Wirkung abgelehnt oder ungeklärt); keine unbeabsichtigte zweite Anlage und keine Rückkehr zum ungeschützten Legacy-Schreibpfad. | **SOURCE DIAGNOSTICS IMPLEMENTED; MLD ROOT CAUSE OPEN** |
| **F1.2 – Vollständiger epgsearch-Editor** | Vorhandene umfangreiche Felder/Preview-Struktur wieder als ein konsistentes Browserformular nutzen; Kanal-/Zeit-/Dauerfilter, Vergleiche, Wiederholungen, Blacklists, Verzeichnis, Vor-/Nachlauf und Aktiv-Wunsch beibehalten. Prüfen, welche Felder der aktive produktive Public-v1-Vertrag speichert; für nicht unterstützte Felder kein scheinbar erfolgreiches Speichern. | Ein Editor mit allen technisch unterstützten epgsearch-Einstellungen, vorbefüllt beim Aufruf aus dem EPG; alle gespeicherten Felder entsprechen nativer Readback-Konfiguration. Bis F1.3 ggf. sichtbar eingeschränkte Speicherfunktion statt stillschweigendem Datenverlust. | **EDITOR PARTIAL; FULL-FIELD WRITE OPEN** |
| **F1.3 – Suchtimer direkt aktiv speichern** | Browser-Session nutzt den bereits vorhandenen vollständigen, serverseitig autorisierten epgsearch-CREATE-Controller mit einem POST für inaktiv/aktiv; ausschließlich dieser produktive Browserweg, keine Device-CAS-Umgehung. Backend-Zuordnung ist im CREATE-Service an dessen RESTfulAPI-Client gebunden; Readback prüft Native-ID, Zustand und gespeicherte Regeloptionen. | Alle im Editor angebotenen Einstellungen in einem Aufruf speicherbar; „aktiv“ wird erst nach eindeutigem Readback bestätigt. Bei unklarem Ergebnis keine automatische Wiederholung. Reale MLD-VDR-Installation bleibt separate Abnahme. | **SOURCE IMPLEMENTED; COMPILED CI AND REAL MLD OPEN** |
| **F1.4 – Live-TV-Vorschau** | `live_backend_authority_required` gegen aktuelle Agent-/Backend-Zuordnung und Providerwahl zurückverfolgen; danach bestehenden Live-Pfad/Browser-Medienprofil nutzen. Streamdev nur dann einbinden, wenn es zum vorhandenen Adapter passt; keine zweite unautorisierte Streaming-API. | Auswählbarer Sender liefert Bild und Ton in unterstütztem MLD-Browser; Start/Stop/Kanalwechsel räumen Sessions auf. Fehlender Provider gibt eine treffende Diagnose, nicht irreführend „Live-TV beendet“. | **OPEN** |
| **F1.5 – VDR-Aufnahmen wiedergeben** | Durchgängigen bestehenden Pfad Aufnahme -> gültige MediaSession -> zugängliche Medienbytes -> Browser-Player untersuchen; MLD-Aufnahmeverzeichnis, URL/Proxy, FFmpeg/ffprobe, Codec/Container und readback getrennt belegen. Keinen Host-Dateipfad fest verdrahten. | Eine konkrete vorhandene TS-Aufnahme startet mit Bild/Ton; Pause/Stopp funktionieren; nicht abspielbares Format erklärt den Grund, ohne Aufnahmeänderung. | **OPEN** |
| **F1.6 – MLD-Pluginübersicht und Timer-Margins** | Plugin-/Abhängigkeitsmatrix aus `mld66-skippy-feedback-20261027.md` in verständliche Installationsdoku mit MLD-Paket-/Capability-Mapping überführen; benötigter Videotext-/Live-Providerweg wird ausdrücklich geklärt. Timer-Vor-/Nachlauf nach bestätigter MLD-Version mit nicht-destruktiver Prüfung und ggf. autorisiertem Einzeltimer testen. | Für jede angebotene Funktion steht „Basis / bedingt nötig / optional / nicht verfügbar“ fest; tatsächliche MLD-Rezept- und VDR-ABI-Version belegt. Margins entsprechen bei kontrolliertem Test den Einstellungen. | **PARTIAL DOC / MLD MARGINS OPEN** |
| **F1.7 – Gemeinsamer MLD-Testkandidat und Rückmeldung** | Quell-HEAD, JS/Backend-/Provider-Tests und ABI-erforderliche Paketstände zusammenführen; geänderte Dateien korrekt paketieren, MLD-Installation/Upgrade auf ausgewiesenem Teststand, protokollierte Golden-User-Journeys mit skippy. Keine generelle Paketveröffentlichung und kein Merge nach `main` ohne Freigabe. | Zu jedem Punkt genau `PASS / FAIL / BLOCKED` und installierte Versions-/SHA-Belege, besonders SearchTimer, Live, Recording, Timer-Margins; keine ungeklärten mutierenden Testoperationen. | **OPEN** |

## Reihenfolge, ohne andere Aufgaben künstlich zu blockieren

- **Sofort:** F1.0 abschließen und F1.1 diagnostizieren. Vorhandene
  Suchtimer-Operationen mit unklarem Ausgang nur lesen; kein erneutes CREATE.
- **Danach:** F1.2 und F1.3 als ein gemeinsames Benutzerziel, aber getrennte
  nachprüfbare Implementierungsslices. Die bestehende vollständige
  Eingabestruktur wiederverwenden, nicht von Null schreiben.
- **Parallel möglich, aber mit eigenem Abschluss:** F1.4 Live-TV und F1.5
  Aufnahmewiedergabe; MediaSession- und Provider-Gates getrennt prüfen.
- **Zum Testpaket:** F1.6 Dokumentation/Timer-Margins und F1.7 Releaseprüfung.
  Skippy soll möglichst **einen konsistenten Testkandidaten** statt ständig
  einzelner, noch unvollständiger Patches installieren.

Der MLD.1-Inventarstand dokumentiert einen RESTfulAPI-`recordings.json`-
Timeout und mögliche VDR-Neustarts aus einem früheren MLD-Maintainer-Test.
Dies ist **keine** bestätigte Diagnose von skippys aktuellem Host. Erst
Read-only-Preflight; keine aggressiven Recording-Abfragen auf einem
instabilen VDR und keine Vermischung zweier Testumgebungen.

## Implementierungsbefund auf diesem Branch

**F1.1 – differenzierte CREATE-Diagnose und Operationsabfrage (Quellcode):**
Die Oberfläche bietet nach einer angenommenen CREATE-Operation
`Anlagevorgang prüfen` und ruft die bereits existierende, actor-gebundene
`GET /api/v1/operations/{operationId}`-Route ausschließlich lesend auf.
Das Operationsergebnis wird gegen die ursprüngliche ID und das
ausgewählte Backend geprüft; keine automatische zweite CREATE-Mutation.
Ein serverbestätigtes `succeeded` bestätigt **inaktive** Anlage,
nicht Aktivierung. Bei nicht bestätigtem Ausgang bleibt die Operation
gehalten. Das wurde mit dem echten UI-Submit-Handler und einem
gemockten Public-v1-Readback geprüft.

**F1.1 – 503-Diagnose (Quellcode):** Die
Public-v1-HTTP-Route unterscheidet jetzt ein schreibgesperrtes/offline
Backend, einen nicht gültig angemeldeten Agenten, die fehlende
SearchTimer-Provider-/SuiteBridge-Anbindung und nicht bestätigte
VDR-Vor-/Nachlaufwerte. Der Browser zeigt diesen Fall auf Deutsch
einschließlich Request-ID, sofern die API sie liefert. Der Fehlergrund
auf skippys *installierter* MLD bleibt **unbestätigt**. Eine klarere
HTTP-Fehlermeldung ist noch kein behobener Host-Konfigurationsfehler.

**F1.2 – erweiterte Eingabemaske (teilweise):** Der Editor zeigt erneut
Verzeichnis, Kanal, VPS, Priorität, Lebensdauer, Dauer, Wiederholungen,
Blacklist und Vor-/Nachlauf. Die bisherige Public-v1-CREATE-Payload
bleibt absichtlich auf Textsuche mit `active:false` begrenzt; weitere
Einstellungen dürfen nicht stillschweigend verworfen werden. Der
Browser blockiert daher aktuell einen SAVE mit nicht unterstützten
Feldern und erklärt die Einschränkungen der EPG-Vorschau. Ein
verlustfreies epgsearch-Full-Field-SAVE ist **noch nicht implementiert**.

**F1.3 – direkter Browser-CREATE (Quellimplementierung):** Der
bestehende *Browser-Session*-Controller `POST /api/vdr/searchtimers`
ist bereits für vollständige epgsearch-Optionen, inklusive
`active:true`, implementiert und über `SecurityHttpGate` mit
`searchtimers.create` autorisiert. Der MLD-Webeditor nutzt jetzt
diesen **bestehenden produktiven Browserweg** statt des begrenzten
Device-Public-v1-CREATE-Endpunkts. Das ist kein zweiter Writer und
keine Device-Berechtigungsumgehung: Externe Geräte verwenden weiterhin
Public-v1 und dessen inaktive CREATE-Admission.

Ein Browserdialog sendet genau einen POST. Der Server prüft vor
Dispatch, dass die vom Browser gewählte Backend-ID zum echten
RESTfulAPI-Client dieses Controllers passt; andernfalls null Writes.
Anschließend liest die Oberfläche die vollständige Browser-SearchTimer-
Liste für denselben Backend- und Suchbegriff und vergleicht die
gemeldete **native ID**, den Zustand und die verfügbaren
Regeloptionen. Kanal-IDs, Channel-Modus, Uhrzeit- und Wochentagsfilter
sowie JSON-Array-Felder für Blacklists/Extended EPG bleiben in der
VDR-/JSON-Projektion erhalten. Eine unvollständige Liste, abweichende
Felder oder unbekannter HTTP-Ausgang gelten als **nicht bestätigt**;
kein automatischer oder zweiter POST aus demselben Dialog.

Das bestehende [Issue #518](https://github.com/hotzenplotz5/vdr-suite/issues/518)
zu generationsgeschützten **Änderungen bestehender Regeln** wird dadurch
nicht als gelöst ausgegeben. Eine neue Regel im Browser einmalig aktiv
anzulegen ist kein CAS-UPDATE eines bestehenden numerischen epgsearch-
Rule-IDs. Auf einer realen MLD-Installation müssen korrekte
epgsearch-/RESTfulAPI-Provider-Versionen, Browser-Gates, CREATE, READBACK
und aktiver Zustand trotzdem **separat überprüft werden**.

**Teststatus:** Der vollständige Browser-Formular-Test (aktive und
inaktive Anlage, Advanced-Felder, Readback-Fehler, Null-Duplikat-Writes)
wurde in einer isolierten JavaScript-DOM-/V8-Umgebung erfolgreich
ausgeführt. Bestehende C++-Tests sind für providergebundene CREATE-
Zuordnung sowie epgsearch-Array-/Zeit-/Kanal-ID-Readback erweitert.
Ein exakter C++-Kompilier-/CI-Lauf sowie Browser-E2E und REAL-MLD-
Abnahme waren bei dieser Quelländerung noch nicht nachgewiesen.

## Definition of Done für jeden Slice

- **Quellbeleg:** exakter Branch-HEAD, tatsächlicher Produktivpfad, eine
  minimale Änderung und konkretes beobachtbares Nutzerergebnis.
- **Tests:** fokussierte Unit-/Contract-Prüfung und für HTTP-/Provider-
  Änderungen mindestens ein Test des echten produktiven Routing-Pfads;
  CI-/MLD-Paket-/REAL-VDR-Status **getrennt** festhalten.
- **Integration:** keine erneute Authentifizierungs-/Streaming-/Schreibengine;
  unveränderte Rollen-/CSRF-/Backend-Grenzen; niemals vertrauliche
  Host-Konfigurationen oder Credentials in Testprotokollen veröffentlichen.
- **Hardware:** auf skippys installiertem MLD-Candidate nur lesend beginnen.
  Änderungen an Timern, Suchtimern, Aufnahmen oder Diensten erfordern eine
  getrennte kontrollierte Freigabe und nachprüfbaren Native-Readback.
- **Fertig bedeutet:** sichtbare Funktion funktioniert auf dem unterstützten
  Testpfad und ist belegbar – nicht nur ein Commit oder erfolgreiches Mock.

## Gewollte Grenzen

- Keine Änderungen an `AGENTS.md`, kein `main`-Merge,
  kein Force-Push/Löschen von Branches, kein automatisches Paket-Deployment.
- Keine neuen produktiven Sicherheitsregeln auf Verdacht;
  bestehende, nachgewiesene Autorisierungsgrenzen werden respektiert.
- Kein ungeprüftes Reaktivieren breit suchender epgsearch-Regeln:
  Eine weit gefasste Regel erzeugte zuvor sehr viele Timer auf yaVDR.
- Diese Phase beseitigt Nutzerprobleme, ersetzt nicht MLD.1B–MLD.1E
  hinsichtlich genereller Plattform- und Paketfreigabe.

**Arbeitsregel für die Fortsetzung:** Bei „nächster MLD.F1-Slice“ zuerst
den tatsächlich neuesten Branch-HEAD lesen, den ersten noch offenen
Beleg/Abnahmezustand heranziehen und genau **ein** überschaubares
Nutzerergebnis einschließlich fokussiertem Test abschließen.
