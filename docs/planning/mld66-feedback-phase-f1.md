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
| **F1.1 – Suchtimer-CREATE-Fehler klären** | Am bestehenden Browser- und Public-v1-Pfad `POST /api/v1/search-timers` den tatsächlichen Grund für „requested public API resource is temporarily unavailable“ ermitteln: Version/Route, Agent, Backend, Anbieterfähigkeit, Berechtigung, Operationstatus und nativer Readback. Nur nach belegtem Defekt minimal korrigieren. | Genau ein vollständig nachvollziehbarer Zustand je CREATE-Anfrage (angelegt, nachweislich ohne Wirkung abgelehnt oder ungeklärt); keine unbeabsichtigte zweite Anlage und keine Rückkehr zum ungeschützten Legacy-Schreibpfad. | **NEXT / OPEN** |
| **F1.2 – Vollständiger epgsearch-Editor** | Vorhandene umfangreiche Felder/Preview-Struktur wieder als ein konsistentes Browserformular nutzen; Kanal-/Zeit-/Dauerfilter, Vergleiche, Wiederholungen, Blacklists, Verzeichnis, Vor-/Nachlauf und Aktiv-Wunsch beibehalten. Prüfen, welche Felder der aktive produktive Public-v1-Vertrag speichert; für nicht unterstützte Felder kein scheinbar erfolgreiches Speichern. | Ein Editor mit allen technisch unterstützten epgsearch-Einstellungen, vorbefüllt beim Aufruf aus dem EPG; alle gespeicherten Felder entsprechen nativer Readback-Konfiguration. Bis F1.3 ggf. sichtbar eingeschränkte Speicherfunktion statt stillschweigendem Datenverlust. | **OPEN** |
| **F1.3 – Suchtimer direkt aktiv speichern** | Bestehende Public-v1-Operationen/epgsearch-Integration für einmaliges vollständiges CREATE und optionales Aktivieren zusammenführen; native Generation-/Revision-/Readback-Voraussetzungen aus Issue #518 beachten. Keine zweite Schreibengine und keine automatische Wiederholung bei `outcome_unknown`. | Benutzer kann „Inaktiv speichern“ oder „Aktiv speichern“ wählen; gewünschte Optionen sind nach serverbestätigtem Readback vorhanden, aktiver Zustand stimmt. Unsichere Anbieter-/ABI-Konstellation wird klar als nicht unterstützt dargestellt. | **OPEN; depends on F1.1/F1.2/provider evidence** |
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
