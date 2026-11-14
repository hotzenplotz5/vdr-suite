# MT — Media Tools: Installation, Import und Aufnahmeaktionen

**Konsolidierter Status vom 11.11.2026.** Offizieller Quellbranch: `main`. Codebasis vor dieser Dokumentationsbereinigung: `e3a8f2993fbc0709af55f5f048287304dcde7f6a`. **Lesbarer Einstieg:** [Media-Tools-README](README.md). **Installation nach Distribution:** [README.install.md](README.install.md). Historische technische Details: [IMPLEMENTATION-NOTES.md](IMPLEMENTATION-NOTES.md). Übergreifende volatile VDR-Suite-Statusautorität: [CURRENT.md](../../docs/CURRENT.md).

**Leseregel:** Die folgende Tabelle ist der aktuelle *konsolidierte* Media-Tools-Befund. Weiter unten bleiben datierte Entwicklungs-, Zwischen- und Testprotokolle erhalten. Historische Aussagen wie „noch nicht implementiert“, „kein Live-Test“ oder „nächster Slice“ sind **nicht** erneut maßgeblich, wenn spätere Implementierungen, Abnahmen oder die hier ausdrücklich dokumentierte Betreiberbestätigung sie überholt haben. Frühere PASS-Belege behalten ihren exakten damaligen Geltungsbereich.

## Konsolidierte Statusmatrix – 11.11.2026

| Bereich / Slice | Implementierung und vorhandene Evidenz | Aktuelle Einstufung / Rest |
| --- | --- | --- |
| **MT.EVENT1–4 – Langläufer und Operation-SSE** | Dauerhafter Core-Auftrag und 24h-Deadline, fortlaufender autorisierter `/api/v1/operations/{id}/events`-Stream, SSE-Frontend, einmaliger Readback bei Öffnen/Reconnect, Browser-Neustart-Recovery und gezielte Regressionen im Quellstand vorhanden. | **INTEGRIERT; Betreiber meldet Funktions-/Langläufer-Test PASS (11.11.)**. Die konkreten Laufzeiten, Host-Logs und Unterfälle wurden in diesem Dokumentationsabgleich nicht unabhängig erneut eingelesen; deshalb keine neue Sechs-Stunden-Messung behaupten. |
| **MT.EVENT5 – browserunabhängiger Sammelimport** | Ganzer bestätigter Stapel (2–100 Quellen) wird in einer Core-Transaktion reserviert; Daemon setzt nach bestätigtem Vorgänger autonom fort; Frontend kann denselben Stapel wiederfinden. API-/Frontend-Vertrag und isolierter SQLite-Neustarttest einschließlich sieben Quellen vorhanden. | **INTEGRIERT; Betreiber meldet Real-Funktionstest PASS (11.11.)**. Nicht erneut als fehlende Implementierung führen und bestandene Bedienabläufe nicht ohne neue Ursache wiederholen. Ein separater, maschinenlesbarer Testlog/Runtime-SHA wurde in diesem Abgleich nicht vorgelegt. |
| **MT.ENC3.1 / MT.ENC4 – H.264/HEVC und Wiedergabe** | Typisierte Agent-Encoderwerte; realer H.264-Core-/Agent-/VDR-Readback sowie HEVC-Direktimport, native Indizes, Android-Bild/Ton und vollständige Wiedergabe dokumentiert. Das zusammengesetzte fünfteilige Produktiv-Playback-Bundle wurde am 11.11. mit Syntax-, HTTP-Readback- und Seiten-Bundle-Prüfung wiederhergestellt. | **REAL-yaVDR PASS für die dokumentierten Einzelpfade.** Nutzer bestätigt den anschließenden praktischen Wiedergabe-/Seek-Test mit `Pass!`. Weitere Samsung-/VIDAA-Client-Profile, konkrete Hardware-Encoder-Profile und nicht einzeln belegte Unterfälle bleiben davon getrennt. |
| **MT.META1–5 – Daten/Artwork/Serienansicht** | Responsive Frontend, lokale NFO-/FFprobe-Daten, vorhandenes Artwork, Serien-/Staffel-/Episodenansicht und automatische eindeutige TMDb-Kandidatenbindung im Produktcode; Quell-/UI-Regressionen vorhanden. | **IMPLEMENTIERT**; die umfassende Screenshot-/Geräteabnahme des gesamten Mockups nicht als automatisch bestätigt ausweisen. |
| **MT.META6 – persistente Metadaten/`info`** | Eingefrorene TMDb-Metadaten im Core/Agent/Worker, native `info`-Writer- und Korrekturfunktion, automatischer Suite-Assignment-/Cache-Pfad im Produktcode. Foundation S01E06–E08 mit sichtbaren Metadaten und Backfill S01E05 auf yaVDR dokumentiert; isolierter Native-`info`-Test PASS. | **REAL-yaVDR NATIVE-`info` READBACK: 50/50 PASS (11.11.2026).** Foundation 26/26 inkl. S01E05 und Andor nach gezielter Altbestandskorrektur 24/24 mit konsistentem `T`/`S`/`D`. **Zusätzlicher Film „Caligola“: REAL-yaVDR `info`-Readback PASS nach gezielter Entfernung ausschließlich des alten technischen `S`-Markers.** Host meldet 48/48 gezielte C++-Checks PASS, `CALIGOLA_FIX=PASS`, unveränderte korrekte `T`/`D`, Import-Provenienz erhalten und `VDR_REFRESH=NOTIFIED`. Neben den 48/48 C++-Checks und dem nativen Einzelaufnahme-Readback liegt inzwischen **auch die erfolgreiche Live-Aktivierung der aktualisierten Runtime** vor (siehe Deploy-Readback unten). Ein **neuer Filmimport nach dem Deployment** wurde dadurch noch nicht gesondert durchgeführt. Für Andor S01E01/S01E02: `READBACK=PASS` 2/2 und `VDR_REFRESH=NOTIFIED`. Nicht belegt: tatsächliche Neuauflistung durch VDR/Suite-Cache/TV-Client nach der Benachrichtigung; kein erneuter Video-Integritätstest. |
| **MT.SCAN1–3 – automatische Quellprüfung** | Produktpfade für sofortige Quellenliste, serverseitige Vorprüfungen ohne Browser, persistenten Quell-Preview-Cache, pro-Film-Status und interaktive Priorität sowie Frontend-/Agent-Regressionen im aktuellen `main` vorhanden; der 11.11.-Quellabgleich prüfte die tatsächlichen Aufrufstellen. | **MT.SCAN1/2 INTEGRIERT; MT.SCAN3 REAL-SICHTTEILABNAHME PASS (11.11.)**: Betreiber-Screenshots zeigen 3/3 automatische technische Prüfungen, 7 Quellen (davon 4 bestehende VDR-Aufnahmen) und aktuelle Detailvorschau. Nicht durch diese Screenshots bewiesen: erneute Anmeldung, echter Agent-Ausfall, manueller Prioritätswechsel oder SSE-Abschluss-Readback. Kein zweiter Scanner notwendig. |
| **MT.REC / MT.UX – Oberfläche** | Drei-Spalten-Dashboard, responsive Zweispalten- und mobile 700-px-Einzelansichten, Ordner/Serien, Marker, Quellstatus, Vorschau, Batch- und Dublettenaktionen im Produktcode; vorhandene Frontend-Kompositionstests prüfen DOM-Navigation/Backend-Mutationsfreiheit. | **REAL-Desktop-SCREENSHOT UX-FIX PASS (11.11.)**: Produktive Drei-Spalten-Ansicht mit Technikdaten/Artwork. Betreiber-Readback des installierten JS: `FRONTEND_INSTALLED=YES`, `MEDIA_IMPORT_UX_FIX=PASS`, Quellcommit `e32a0ec188da9fadc4741d7b68700d1b545b4b9b`, Blob `ed156d9799ea4ffc0fb7852101a9a1a70a9ea298`. **Neuer echter Browser-Screenshot nach Installation bestätigt die entdoppelte TMDb-Auswahl bei „Caligola“.** **MT.UX6-Kartenmenü mit HEVC-Vorprüfungsnavigation ist quellseitig implementiert und isoliert getestet** (11.11., Details unten). Mobile Touch-/Gesamtmockup- und neue Kartenaktions-REAL-VDR-Abnahme bleiben getrennt. |
| **MT.ENC3.2 – Media Tools Encoding im Web** | Eigene Settings-Karte für H.264-Import und HEVC-Shrink, Software-Qualitätsprofile x264 19/23/27, x265 23/27/30 sowie getrennte fast/medium/slow-Presets. Reuse der vier bestehenden Werte in `backend-agent.conf` und der vorhandenen Admin-Browser-Session. Der Agent liest vor einem neuen Job die bekannte Konfiguration erneut ein und friert sie ein; die bestehende Wiedergabe-Transkodierung bleibt getrennt. Hardware-Encoder behalten ihre eigenen Workerparameter. | **SOURCE implementiert; ISOLATED TEST und REAL yaVDR noch nicht bestätigt** (14.11.2026). Test: `node web/frontend/tests/test_media_tools_encoding_settings.js`; Real-Abnahme einschließlich SSE, VDR-Readback und Seek offen. |
| **MT.I0–I5 / MT.HA – Installation und Portabilität** | Gemeinsame Stage/Hash-Inventur, yaVDR-Pfade, synthetische MLD-/openSUSE-Portabilitätsregressionen und getrennter inaktiver `.deb`-Payload-Code vorhanden. | **TEILWEISE TESTET/IMPLEMENTIERT**; reales MLD/openSUSE-Media-Tools-Hosting, Gesamtpaket-/PPA-Integration, Altbestand-Migration und freigegebene Installations-/Rollback-Abnahme weiterhin offen. Siehe [Distributionsübersicht](README.install.md). |
| **BAS.1–5 – Backend-Agent Simplification** | Als nächste getrennte Refactoringarbeit nach Media Tools vereinbart. | **GEPLANT; NICHT GESTARTET.** Keine neue Architekturarbeit durch diese Dokumentationskorrektur. |

### MT.UX6 – Aufnahmeaktionen direkt in der Aufnahmeliste (11.11.2026, SOURCE/ISOLATED PASS)

**Implementiert auf dem einzigen genehmigten Branch `main`:** `79f41400b778177fa62807fe8c0d180032ab6377` ergänzt in `recordings2-browser-view.js` ein `⋯`-Menü pro Aufnahme. Das Menü prüft **erst beim Öffnen** ausschließlich lesend die bereits vorhandene backendbezogene Public-v1-Capability `addons.media.shrink`. Eine nicht verfügbare Vorprüfung führt zu einer verständlichen Anzeige, nicht zu einem neuen POST. Bei vorhandener Planfähigkeit führt die Aktion zur gleichen Aufnahme-ID und zum vorhandenen Detail-Hero. `9bf07db883cc4efee4cf9b37678f3ca56d8c741f` öffnet dort das **eine** bereits implementierte HEVC-Plan-/Stage-/Review-/Publish-Panel unter `Aufnahmeaktionen`. Keine zweite Shrink-Implementierung, kein spontaner FFmpeg-Start und keine Veröffentlichung ohne ausdrückliche Bestätigung.

**Tests und Fehlerkorrektur:** `03c7e148ace981458ebdc24bf7779733c443d096` erweitert `test_media_shrink_hero_flow.js` um Kartenmenü, Capability-Readback, Auswahl der unveränderten Recording-ID, Öffnen des Hero-Owners und nachgelagerte Plan-/Stage-/Review-/Publish-Schritte. Der isoliert ausgeführte JavaScript-/V8-Test mit den **tatsächlich vom Remote-HEAD gelesenen drei Produkt-/Testdateien** meldet `MEDIA_SHRINK_HERO_OWNER_STAGE_REVIEW_PUBLISH=PASS`. Der Add-on-CI-Frontendjob auf Zwischen-HEAD `9bf07db` entdeckte bei `test-recordings2-runtime` ein reduziertes Test-DOM ohne `document.getElementById`; der darauf begrenzte Fehler wurde in `9a96f5477c48b6488f5e6653841d3fc13c0589c9` korrigiert, danach bestand derselbe isolierte V8-Test erneut. **Ein vollständiger neuer Node-/CI-Gesamt-PASS ist davon nicht abzuleiten.** Die parallel bekannte Add-on-CI scheiterte zusätzlich und unabhängig an `shrink candidate sidecar is missing or changed` im FFmpeg-Test.

**Ergänzende Negativ- und Layoutregression:** `24ea17d2d04ed39b31b7f7718f493a0491d6c18a` prüft die abgelehnte HEVC-Planfähigkeit: kein freigeschalteter Kartenbutton, keine Navigation und keine Mutation. `c69d3b48ecbecfa0c36b2b7e7073f0d0c9d2369e` verankert das `⋯`-Popover mit fester Triggerbreite und eigenem Menüpanel, statt beim Aufklappen die Karte zu verbreitern. `1872894218fb35de2cf28101a30a33317aef0ff5` passt die DOM-/Negativregression an; erneuter gezielter V8-Lauf der aktuellen Browser-, Hero- und Testquellen: `MEDIA_SHRINK_HERO_OWNER_STAGE_REVIEW_PUBLISH=PASS`. Eine reale responsive Browser-Sichtprüfung bleibt offen.

**Abnahmegrenzen:** `SOURCE=IMPLEMENTED`, `ISOLATED_CARD_TO_HERO=PASS`; `REAL_BROWSER/TOUCH=NOT_VERIFIED`, `REAL_yaVDR_ACTION=NOT_VERIFIED`. Keine Installation, kein Dienstneustart, kein Test-Encode und keine Änderung produktiver Aufnahmen durch diese Source-Arbeit. Der übrige MT.UX6-Aufnahmekarten-/Mockup-Abgleich bleibt als separate reale Sichtprüfung offen.

### MT.SCAN3 / MT.REC-UX – reale Desktop-Screenshots und gezielter Frontend-Fix (11.11.2026)

**Befund anhand zweier Betreiber-Screenshots der laufenden yaVDR-Weboberfläche (installierte Runtime-SHA im Screenshot nicht erkennbar):** Das Drei-Spalten-Layout „Serien und Ordner / Filme und Episoden / Vorschau und Aktionen“ ist real sichtbar. Scanmeldung `Technische Hintergrundprüfung: 3 von 3 geprüft`; sieben unmarkierte Quellen, davon vier mit `Bereits importiert – VDR-Aufnahme vorhanden` und drei noch nicht importiert. Für „Grudge – der Fluch 2“ werden H.264, ca. 1,2 GiB, 1 Std. 32 Min., vorhandenes Cover und ein geprüftes VDR-Ziel angezeigt. Der zweite Screenshot hält einen **laufenden** Import mit `Import läuft` und pausierten Löschbuttons („Der vorherige Auftrag läuft noch“) fest. Der Betreiber bestätigt später **„der fluch ist abgeschlossen“**; ein exakter späterer Operation-GET, Screenshot des terminalen Zustands oder ein erneuter Agent-Inventarabgleich liegt damit **nicht** automatisch vor. Keine Behauptung, dass SSE-Readback und anschließende Freigabe der Dublettenbuttons dadurch bereits sichtbar bestanden hätten.

**Belegter UX-Mangel und begrenzter Codefix:** Der in der rechten Spalte manuell ausgewählte TMDb-Treffer zeigte seinen Plot sowohl in der Trefferkarte als auch in der Bestätigung, samt zweitem großem Poster unter dem bereits vorhandenen Haupt-Artwork. In `web/frontend/modules/media-import.js::renderOnline` wird die Synopsis eines ausgewählten Kandidaten nun **nur noch einmal** in der Bestätigung angezeigt; das doppelte Bild dort entfällt. `renderPreviewCard` verwendet weiterhin die vorhandene lokale/Provider-Poster- und Episodenbildlogik oberhalb der technischen Informationen. Such-, Auswahl-, Provider-ID-, Import-, Encoder-, Quell- und Metadatenpersistenzpfade wurden nicht angefasst. `web/frontend/tests/test_media_import_composition.js` prüft die entdoppelte Ansicht zusätzlich zu den vorhandenen Vertragsfällen. **Beleg für diesen Commit:** Frontend- und Testdatei jeweils mit JavaScript-V8-Syntaxprüfung PASS; isolierter Aufruf des echten `renderOnline`-Funktionskörpers mit einem ausgewählten Film ergibt genau eine Synopsis, keine `media-import-online-poster`-Doppelbildinstanz und einen Bestätigungsblock (PASS). Eine komplette Node-/Browser-Regression und Installation des aktualisierten Frontends ist in dieser Sitzung **nicht** gelaufen; deren Ergebnis nicht behaupten.

**Dubletten nach Importende:** `deleteBlockReason` sperrt während einer noch laufenden Operation Quellmutationen sichtbar. Nach einem bestätigten terminalen Zustand liefert diese Funktion für **andere** Quellen keine globale Sperrmeldung; für dieselbe importierte Quelle wartet sie ggf. auf den verifizierten Quellenabgleich. `readOperation`/`updateInlineDeleteAvailability` aktualisieren die angezeigten Buttons bei Status-Readback. Sechs isolierte Funktionsaufrufe mit der bestehenden Guard-Implementierung waren PASS (running, terminal, gleiche/andere Quelle, `outcome_unknown`). **Das ist kein realer Nachweis**, dass die vier Dublettenbuttons auf dem Bildschirm nach „Grudge“ tatsächlich wieder anklickbar waren. Ohne solch einen Gegenbeleg keine spekulative Entfernung der bestehenden Operationsbindung.

**Nachtrag: echtes yaVDR-Einzeldatei-Deployment des UI-Fixes (11.11.2026, Betreiber-Log):** `FRONTEND_INSTALLED=YES`, anschließend `MEDIA_IMPORT_UX_FIX=PASS` und `COMMIT=e32a0ec188da9fadc4741d7b68700d1b545b4b9b`. Das operative Skript verglich den Git-Blob-Hash des installierten `/usr/share/vdr-suite/web/frontend/modules/media-import.js` mit `ed156d9799ea4ffc0fb7852101a9a1a70a9ea298` und prüfte `node --check` erfolgreich. **Damit ist die einzelne Datei am Live-Ziel nachweislich aktuell**, ohne Daemon-/Agent- oder VDR-Neustart. Das war ein **gezieltes Frontend-Dateiupdate**, keine komplette neu versiegelte Runtime-Stage; ausdrücklich **kein** `RUNTIME_DEPLOYMENT_MATCH=YES` behauptet. **Nachfolgender produktiver Browser-Screenshot liegt jetzt vor:** Auswahl „Caligola“, oben Hauptposter, darunter zwei Treffer und genau ein kompakter ausgewählter Bestätigungsbereich; **kein zweites ausgewähltes Poster/keine wiederholte ausgewählte Synopsis**. Der sichtbare „Caligola“-Import läuft dort noch; ein Abschluss ist aus diesem Bild nicht ablesbar. Ein tatsächlicher Post-Import-Button-Check liegt weiterhin nicht vor.

**Abnahmegrenzen:** Desktop-Scan und Listen-/Layoutsicht **REAL_SCREENSHOT_PARTIAL_PASS**; Betreiber bestätigt den früheren „Grudge“-Importabschluss **OPERATOR_REPORTED_COMPLETE**; entdoppeltes TMDb-UI **SOURCE_ISOLATED_RENDER_PASS + REAL_LIVE_FILE_HASH_PASS + REAL_BROWSER_SCREENSHOT_PASS**; Unblock der Duplikatbuttons nach Ende **SOURCE_GUARD_PASS / REAL_UNVERIFIED**; Mobilgerät **NOT_VERIFIED**. Für „Caligola“ zeigt derselbe spätere Screenshot nur einen laufenden Import. **Das bestehende Frontend friert die bereits vor dem Klick ausgewählten TMDb-Daten in `startImport`/`frozenImportMetadata` ein; manuelle Auswahl während eines laufenden Auftrags ändert dessen bereits akzeptierte Metadaten nicht rückwirkend.** Keine automatische Wiederholung des Imports und keine unbelegte nachträgliche Metadatenübernahme behaupten.

### MT.META6 – zusätzlicher realer Film-Readback „Caligola“ (11.11.2026)

**Betreiber-Readback aus der echten Aufnahme** `/srv/vdr/video.00/Horror/Caligola/2026-11-11.17.28.1-0.rec/info`: Eine `info` gefunden, `00001.ts` und `index` jeweils nicht leer, `T Caligula - Aufstieg und Fall eines Tyrannen` sowie `D` mit der bestätigten TMDb-Filmbeschreibung korrekt, **aber `S Imported via VDR-Suite Media Tools` blieb sichtbar**. Das ist **kein fehlgeschlagener Import**, sondern ein nachgewiesener Altwert in den nativen Filmbeschreibungen. Der vorherige Foundation-/Andor-50/50-Nachweis bleibt unverändert, betrifft aber andere Aufnahmen.

**Ursache im bestehenden Native-Writer und gezielter Produktfix auf `main`:** Bei bestätigten Filmen schreibt `renderNativeVdrRecordingInfo` bisher bei leerem lokalem Subtitle denselben generischen `S`-Marker wie bei unidentifizierten Medien. `mergeConfirmedMetadataIntoNativeVdrInfo` aktualisiert bei Filmen zwar `T` und `D`, ließ aber bisher grundsätzlich `S` unangetastet. Die neue minimale C++-Änderung führt **nur für bestätigte Filme** keinen generischen neuen `S`-Marker ein und entfernt bei einer späteren Korrektur exakt einen bestehenden `S Imported via VDR-Suite Media Tools`. Ein echter vorhandener `S`-Untertitel wird nicht gelöscht; Episoden bekommen weiter ihren echten Episodentitel. Nicht-T/S/D-Daten bleiben bytegleich. Die alte native Provenienz-/Dublettenerkennungslogik mit `.vdr-suite-import-origin` bleibt maßgeblich.

**Nachtrag 11.11.2026 – exakt getesteter Produktcode und reale Bestandskorrektur:** Der Betreiber hat den C++-Kandidaten aus `7860be24b4da7866187e83fe12cdae6cbe8aab18` mit `g++ -std=c++17 -Wall -Wextra -Werror` auf dem yaVDR isoliert kompiliert und die zugehörigen `test_media_import.cpp`-Regressionen ausgeführt: `MEDIA_IMPORT_CPP_TEST=PASS checks=48` und `MEDIA_IMPORT_CPP_TEST=PASS`. Anschließend wurde nur die vorhandene Aufnahme `Horror/Caligola/2026-11-11.17.28.1-0.rec/info` nach explizitem Provenienz-Readback angepasst: Der exakte Marker `S Imported via VDR-Suite Media Tools` wurde entfernt, die korrekten `T Caligula - Aufstieg und Fall eines Tyrannen` und `D Nach dem Tod des paranoiden Kaisers Tiberius …` blieben erhalten. Rückmeldung: `IMPORT_PROVENIENZ=ERHALTEN`, `CALIGOLA_S_PLACEHOLDER_REMOVED=YES`, `CALIGOLA_FIX=PASS`, `VDR_REFRESH=NOTIFIED`. Video-`00001.ts` und `index` wurden nicht umgeschrieben. Der zuvor beobachtete eine Filmfall ist **behoben**; keine erneute Aufnahme, keine zweite TMDb-Suche. `VDR_REFRESH=NOTIFIED` ist weiterhin kein separat beobachteter Web-/TV-Client-Refresh.

**Getrennte Auslieferungs-/CI-Grenze:** Der Host-Test kompiliert den C++-Writer, **installiert aber nicht** den aktualisierten `vdr-suite-backend-agent`; erst ein anschließendes offizielles Runtime-Deployment kann das geänderte Verhalten für neue Filmimporte aktivieren. Zum Produkt-SHA `7860be24b4da7866187e83fe12cdae6cbe8aab18` war die relevante Frontend-Regression der [Add-on-CI #1230](https://github.com/hotzenplotz5/vdr-suite/actions/runs/43062794795) erfolgreich. Die gesamte Add-on-CI war **FAIL** wegen eines separaten `test-addon-media-import-ffmpeg`-Laufs (`shrink candidate sidecar is missing or changed`); vor diesem Fehler waren `MEDIA_IMPORT_REAL_CODEC_FALLBACK=PASS` und `MEDIA_IMPORT_REAL_FFMPEG_SMOKE=PASS` protokolliert. Die Architektur-CI meldete daneben eine direkte SQLite-Nutzung in `core/daemon/src/DaemonRuntime.cpp`; keiner dieser Fehler liegt in den für den Caligola-Writer geänderten Zeilen. Keinen allgemeinen CI-GREEN- oder Deployment-PASS daraus ableiten.

**Nachtrag 11.11.2026 – produktives Runtime-Deployment und Live-Binary-Readback PASS:** Der Betreiber hat aus dem offiziellen `main` nach dem Caligola-Quellcodefix und dessen 48 C++-Tests die kanonische `stage-install-runtime` → `deploy-install-runtime` → `check-install-runtime-deployment`-Kette ausgeführt. Konkretes yaVDR-Protokoll: `STAGE=PASS`, `INSTALL_RUNTIME_STAGE_SEALED=YES DIRS=28 FILES=91`, `INSTALL_RUNTIME_DEPLOYED=YES`, `RUNTIME_DEPLOYMENT_MATCH=YES`, `PRESERVE_EXISTING_CONFIG=/etc/vdr-suite/backend-agent.conf`. Bei der nachfolgenden Dienstaktivierung zeigte ein erster Vergleich `ACTIVE_BINARY_MISMATCH` für den Backend-Agenten; die korrigierte Prüfung von tatsächlicher Prozess- und installierter Dateiidentität (Gerät/Inode via `stat -Lc '%d:%i'`) lieferte schließlich `vdr-suite-backend-agent=ACTIVE_BINARY=PASS`. Der Daemon lief zunächst noch mit der zuvor ausgetauschten ausführbaren Datei (`/usr/sbin/vdr-suite-daemon (deleted)`), wurde gezielt neugestartet und lieferte anschließend `ACTIVE_BINARY=PASS`. Abschließendes Betreiber-Protokoll: **`MEDIA_IMPORT_FUTURE_FIX=ACTIVE`**, **`BACKEND_AGENT=PASS`**, **`DAEMON=PASS`**, `VDR_RESTARTED=NO`. **Installation und laufende Binäraktivierung sind damit nachgewiesen**; eine weitere Build-/Installationsschleife oder ein erneuter Caligola-Import ist nicht nötig. Nicht damit bewiesen: ein zusätzlicher echter Filmimport nach diesem Deployment oder ein separat geprüfter TV-Client-Refresh. Der dokumentierte CI-Fehlerstatus bleibt davon getrennt.

### MT.META6 – realer nativer yaVDR-Readback (11.11.2026, Betreiber-Log)

**Quelle:** Vollständige vom Betreiber ausgeführte, ausschließlich lesende `find`/`awk`-Ausgabe mit physischem `VIDEO_ROOT=/srv/vdr/video.00` (Symlink `/srv/vdr/video` korrekt per `readlink -f` aufgelöst); Datum 11.11.2026. **Ergebnis: 50 native `info`-Dateien** unter `Foundation` und `Star Wars-Andor`; alle 50 hatten beim selben Check nichtleere `00001.ts` und `index` und lieferten `T`, `S` und `D`. „VDR_MEDIUM=COMPLETE“ bedeutet bei diesem Check nur Nichtleere, **keinen erneuten Dekodierungs-, Laufzeit- oder Integritätstest**.

- **Foundation: 26/26 korrekte native Titel** `T Foundation` mit echten Episodentiteln `S` und Beschreibungen `D`; auch die schon früher durch den bestehenden Metadatenweg ergänzte **S01E05 „Erwachen“** ist im echten `info` korrekt. Nicht erneut importieren oder korrigieren.
- **Andor: 22/24 mit `T Andor` und echten `S`/`D`**. Ausschließlich **S01E01 „Kassa“** und **S01E02 „Das bin ich“** haben zwar korrekte `S`/`D`, aber als `T` noch den sehr langen ursprünglichen Dateititel mit `DE - Star Wars_ Andor (2022) - S01E0…`.
- **Quellursache eingegrenzt, kein pauschaler MT.META6-Defekt:** `mergeConfirmedMetadataIntoNativeVdrInfo` kann `T` nur ändern, wenn im bereits bestätigten Metadatensatz ein `seriesTitle` vorhanden ist; der API-/Native-Writer benutzt dieses Feld bereits und hat Regressionstests. `repairSelectedLegacyImportInfo` liest dagegen aus der bestehenden alten, persistierten `ManualRecordingMetadataAssignment` nur Episodentitel/Beschreibung/Staffel/Folge und lässt `seriesTitle` leer. Der ältere `S Imported via VDR-Suite Media Tools`-Marker als Legacy-Bedingung ist bei den beiden bereits korrigierten Episoden-Untertiteln `S Kassa` und `S Das bin ich` zudem nicht mehr vorhanden. **Das erklärt den erhaltenen Dateinamen in `T` als plausiblen Altdatenpfad; die originale Operation/Provenienz der beiden Dateien wurde in diesem Hostcheck nicht gelesen.**
- **Kleinste noch offene Maßnahme:** Zuerst die konkrete vorhandene TMDb-Zuordnung, `info`-Provenienz und den bestehenden nativen Korrekturvertrag **für nur diese beiden Andor-Aufnahmen** lesend nachweisen. Danach ausschließlich über den vorhandenen autorisierten Writer mit bereits bekanntem Serientitel `Andor` korrigieren; keine globale Titelableitung aus Ordnernamen, keine neue TMDb-Suche, keine erneute Importoperation, keine direkte DB-Manipulation. Der bisherige reine `info`-Lesetest autorisiert noch keinen produktiven Schreibzugriff.

**Nachtrag 11.11.2026 – Betreiber hat beide Ausnahmen gezielt korrigiert:** Die beiden nach vollständigem Read-only-Befund eindeutig identifizierten historischen Andor-Aufnahmen wurden auf dem realen yaVDR ausschließlich in ihrer nativen `info` korrigiert. Terminalausgabe: `KORREKTUR_BEREIT: Kassa`, `KORREKTUR_BEREIT: Das bin ich`, `READBACK=PASS: T Andor / S Kassa`, `READBACK=PASS: T Andor / S Das bin ich`, `VDR_REFRESH=NOTIFIED` und `MT_META6_LEGACY_INFO_CORRECTIONS=2`. Die Änderung erfolgte am installierten Host durch den Betreiber, **nicht** durch eine neue Produktcode-Installation; `00001.ts` und `index` wurden nicht bearbeitet. Der Vorab-Lesetest hatte 26/26 Foundation und 22/24 Andor korrekt; damit **sind nun 26/26 Foundation und 24/24 Andor in der nativen Titel-/Episoden-/Beschreibungssicht PASS**. Insgesamt 50/50 `info`-Readbacks mit nichtleeren TS-/Index-Dateien. **Ein VDR-`.update`-Signal ist kein separat gemessener VDR-/Suite-Cache- oder Client-Refresh.** Eine spätere Neuanzeige der korrigierten Namen im echten Webfrontend/TV-Client ist deshalb noch unabhängig zu beobachten; das schränkt die bestätigte native `info`-Korrektur nicht ein.

**Status:** `MT.META6_REAL_INFO_READBACK=PASS_50_OF_50`; `MT.META6_LEGACY_ANDOR_CORRECTIONS=PASS_2_OF_2`; `MT.META6_VDR_REFRESH_NOTIFICATION=PASS`; `MT.META6_UI_REFRESH=NOT_VERIFIED`. Der zuvor dokumentierte Altbestandsfehler ist **behoben**, nicht mehr als offener Codefix oder erneuter Import aufzuführen.

**Explizite Betreiberabnahme für MT.EVENT5 und MT.EVENT2–4 (11.11.2026):** Im laufenden Austausch wurden Punkt 1 „browserunabhängiger Sammelimport“ und Punkt 2 „SSE/Langläufer“ ausdrücklich als erfolgreich getestet bestätigt. Diese Bestätigung zählt als akzeptierter Funktionsstand; sie ist keine nachträglich erfundene Testausgabe für jeden früher geplanten Grenzfall. Ohne neue Fehlersymptome oder nachgewiesene Änderung ist **keine Wiederholung dieser beiden Tests** vorgesehen.

**Nächste echte Arbeit:** Der native `info`-Readback ist nach 2/2 gezielten Andor-Korrekturen für **50/50 Aufnahmen PASS**. Ein tatsächlicher Refresh der Client-Ansicht ist davon getrennt. Direkt danach: **MT.SCAN3 und MT.REC/UX** mit gezieltem Quellcode-/Regression-/Mockup-Abgleich; nur echte Lücken korrigieren. Später die fehlenden Web-Encoding-Einstellungen und getrennte Client-/Distributionsabnahmen. Keine neuen Dienste, Worker, Queues, Branches, ungefragten Builds oder VDR-Neustarts.

---

## Historische Entwicklungs- und Abnahmeprotokolle

*Die folgenden datierten Abschnitte dokumentieren ihren jeweiligen damaligen Kenntnisstand. Bei Widersprüchen hat die konsolidierte Statusmatrix oben Vorrang.*

## MT.EVENT — Ereignisgesteuerte Medienaufträge ohne Status-Polling (Auftrag 03.11.2026)

**Statuskorrektur 11.11.2026:** MT.EVENT2–4 sind im aktuellen Produktcode integriert; Betreiber bestätigt die Funktions-/Langläuferabnahme als PASS. Die nachfolgenden Vermerke vom 03.–06.11. sind historische Zwischenstände, keine aktuellen Entwicklungsaufträge.

**Priorität: vor weiteren Media-Tools-UX-Erweiterungen.** **Stand 06.11.2026 auf dem kanonischen `main`: MT.EVENT1 im Produktcode bereits umgesetzt; Quellpfade geprüft und Langläufer-Vertrag gezielt regressionsgesichert. Eine reale mehrstündige yaVDR-Importabnahme liegt noch nicht vor.** Die frühere 4-Sekunden-/225-Reads-Statusabfrage ist im aktuellen Operationspfad bereits durch den actor-scoped Operation-SSE-Kanal (`createClientOperationUpdateSource`) und einmalige authentifizierte Readbacks ersetzt. Der älteren Roadmap-Vermerk über ausschließliche Umsetzung auf `fix/media-tools-root-scan-visible-results` / PR #556 ist überholt: `AGENTS.md` benennt `main` als offizielle Quelle. Die aktuelle SSE-Implementierung und ihre Ende-zu-Ende-Abnahme bleiben in MT.EVENT2–4 gesondert zu beurteilen.

**Verbindliches Verhalten:** Ein ausdrücklich gestarteter Import oder lokales Reencoding mit anschließendem Import wird vom vorhandenen Worker abgearbeitet; Browser, Pollingzähler und die Agent-Transportverbindung steuern nicht die Ausführungsdauer. Core bleibt die dauerhafte Operation-Autorität und führt Auftrag, Ergebnis und bestätigten VDR-Readback. Der Backend-Agent darf vorhandene authentifizierte Kommandos übermitteln und den Worker bedienen, aber nicht mit einer zusätzlichen, künstlichen Zehn-Minuten-Grenze einen ansonsten gültigen Core-Auftrag beenden. **Keine wiederholten GET-Statusabfragen mit Timer** und keine `225`- oder Ersatzbegrenzung im Frontend; niemals aus Zeitablauf Erfolg oder Scheitern ableiten.

| Slice | Umsetzung und Nachweis |
| --- | --- |
| **MT.EVENT1 – Laufzeitvertrag vereinheitlichen** | **Quellseitig erfüllt / realer Langläufer offen:** Core reserviert `now + 24 * 3600` (`MediaImportOperationAdmission.cpp`); `BackendAgentMediaImportAssignment.cpp` übernimmt unverändert `op.deadline`; `BackendAgentCommandClient.cpp` prüft die Frist vor Ausführung und persistiert Ergebnis/Quittungen. `PinnedMediaImportAgentExecutor.cpp::runPinned` wartet bis zur Core-Frist, mit periodischen Lease-Heartbeats ohne Abbruch bei Transportfehler; `MediaImport.cpp` erlaubt 24h für den nativen FFmpeg-Lauf. `MediaImportOperationFence` übernimmt nur dauerhafte Agent-Ergebnisse und bestätigt Erfolg erst nach VDR-Readback. Kein zusätzlicher 600-Sekunden-Abbruch gefunden. Sechs-Stunden-Deadline für Direkt-/lokalen Import in bestehendem Executor-Test ergänzt; tatsächliches sechs-stündiges Reencoding bleibt MT.EVENT4. |
| **MT.EVENT2 – Dauerhafter Abschluss-Push** | Auf die bestehende Core-Operation nach journalisiertem Statuswechsel ein backend-/operationsbezogenes SSE-Ereignis publizieren. Echten fortlaufenden Stream statt endlicher Replay-Antwort anbieten; Backend, Operation-ID und Revision/Sequenz transportieren, keine geheimen Tokens oder frei lesbaren Payloads. Erfassung gilt für `succeeded`, bestätigte Fehler, `outcome_unknown` und nachlaufenden `sourceCleanup`-Readback. Keine zweite Job-Queue oder neue Auftragsdatenbank. |
| **MT.EVENT3 – Frontend ohne Polling** | Die Media-Import-Ansicht abonniert Statusereignisse, prüft bei passenden Ereignissen genau einmal denselben Core-Operation-GET und zeigt korrekten Stand, automatisches Batch-Fortsetzen, Quellabgleich und fertigen Import an. Beim ersten Öffnen und nach Verbindungswiederherstellung einen einmaligen Readback durchführen. Kein 4-Sekunden-Timer, kein endlicher Statuspoll, kein Doppelimport. Andere Vorschau-/Inventurmechanismen nur ändern, soweit sie echte Status-Polling-Schleifen sind. |
| **MT.EVENT4 – Ausfall-/Langläufer-Abnahme** | Vertragstests für sechs Stunden simulierte Laufzeit, SSE-Disconnect/Reconnect, Backend-Neustart und bereits vor Wiederverbindung abgeschlossenen Auftrag; Test sowohl für Direktimport als auch Reencode+Import, Batch und Fehler. Frontend zeigt Abschluss ohne Reload; Runtime-/yaVDR-Test separat nach Freigabe und mit echtem sechs-Stunden-Auftrag. Keine neue Agent-/Capability-Freigabe, keine neuen Dienste/Branches. |

**MT.EVENT4 ergänzte Frontend-Regressionsfälle (06.11.2026):** `test_media_import_composition.js` prüft jetzt zusätzlich zum simulierten sechsstündigen Vorgang, dass ein unzuständiges Operationsereignis keinen Readback auslöst, `resync` und `EventSource.onopen` den bestehenden Auftrag genau einmal lesen, `outcome_unknown` bei Reconnect sichtbar wird ohne erneuten POST, eine während der Unterbrechung abgeschlossene `sourceCleanup` nach einem `succeeded`-Readback sichtbar wird und ein `failed_verified`-Batchglied per SSE keinen weiteren Auftrag auslöst. Diese Mock-/Vertragstests ersetzen weder einen Neustart der realen Core-/Agent-Prozesse noch den yaVDR-Feldtest. **Der Fall eines vollständig geschlossenen und neu gestarteten Browsers ohne gespeicherte Operation-ID ist damit nicht bewiesen** und bleibt als gesonderte Wiederaufnahmefrage offen. Keine Produktcodeänderung, kein zusätzlicher Worker oder Speicherzustand.

**MT.EVENT4 – verlorene Löschantwort beim Sammellöschen (09.11.2026,
Frontend-Fix):** Die Core-Operation wird nach einem unbestätigten POST zuerst
über das bestehende actor-/backendgebundene Journal lesend abgefragt. War
der Eintrag noch nicht sichtbar, folgt **genau ein** verzögerter GET nach
750 ms für das Sammellöschen; kein weiterer POST, kein neuer Key und kein
Polling. Erst bei eindeutig passender Source-ID, `delete_duplicate`,
Auftragszeit und Operation-ID wird derselbe Auftrag angehängt und nach
dessen normalem Status-Readback der Stapel abgeschlossen. Bleibt der
Eintrag unklar, bleibt die bisherige manuelle Bestätigung mit **dem
ursprünglichen Idempotency-Key** erhalten. Regression: zwei erfolgreich
gelöschte Dubletten, dritte HTTP-Antwort fehlt, Core-Eintrag erscheint
erst beim zweiten GET; genau drei statt vier Lösch-POSTs, kein
hängender Batch. REAL-Host-Abnahme separat. Keine erneute Datenmutation
durch die automatische Wiederherstellung.

**MT.EVENT4 Browser-Neustart-Recovery (06.11.2026, Code-Kandidat; inklusive ausstehender Source-Cleanup-Quittung):** Der vorhandene Core-Auftragsbestand wird für einen angemeldeten Benutzer und ein Backend lesend projiziert: `GET /api/v1/backends/{backend}/media-imports` liefert nur actor-eigene Operation-ID, opaque Source-ID, Zustand, Video-Entscheidung und Auftragszeit. Das Webfrontend bindet bei einem neuen Browser-Kontext einen noch laufenden/ungewissen Auftrag wieder an denselben SSE-Kanal; ein abgeschlossener Auftrag erscheint als read-only Hinweis. Keine neue Jobdatenbank, keine Wiederholung eines Imports und kein Status-Polling. **Regression und reale yaVDR-Abnahme sind nach der Implementierung separat zu bestätigen.**

**MT.EVENT1 Quellbefund (06.11.2026):** Der frühere separate Agent-Zehn-Minuten-Timeout ist auf `main` für den Import nicht vorhanden. Die 24-Stunden-Frist wird zentral bei der Core-Zulassung erzeugt und nicht beim Agent-Dispatch neu gestartet. Nach Ablauf der Frist vor Start wird der Auftrag ohne ausgeführten Worker abgelehnt; ein laufender Worker folgt der gleichen Frist. Ein kurzzeitig gescheiterter Lease-Heartbeat beendet ihn nicht; ein Agent-Neustart führt zur dauerhaften Ergebnis-Reconciliation, nicht zum erneuten Import. Der vorhandene Root-/Handler-Pin, die eindeutige Command-/Operation-ID, Source-Signatur, No-Overwrite-Promotion und die Bestätigung durch VDR-Readback wurden nicht verändert. **Keine neue Zeitgrenze, kein neuer Worker oder Dienst, keine Installation.** `test_backend_agent_command_delivery.cpp` prüft beide Zulassungswege mit Core-Deadline; `test_pinned_media_import_agent_executor.cpp` prüft jetzt ebenfalls eine sechs Stunden entfernte Frist für beide Workerpfade. Diese synthetischen Tests sind kein Nachweis eines realen sechsstündigen Imports.

**Abnahme:** Während ein Film über sechs Stunden encodiert, bleibt der bestehende Core-Auftrag maßgeblich. Erreicht er terminalen, wirklich bestätigten Status, aktualisiert Media Tools die offene Ansicht ohne Refresh. Ist der Browser zwischendurch geschlossen, wird beim erneuten Öffnen der unveränderte Auftrag gelesen. Fehler/ungewiss werden nicht als Erfolg bezeichnet. Das System sendet Ereignisse statt wiederholt den Operationsstatus abzufragen.

## MT.EVENT5 – Browserunabhängiger Sammelimport (integriert; Betreiberabnahme PASS 11.11.2026)

**Statuskorrektur 11.11.2026:** Produktcode und Tests sind vorhanden. Der Betreiber hat den Funktionsfall ausdrücklich als erfolgreich getestet bestätigt. Die nachfolgenden Beschreibungen der anfänglichen Browser-Abhängigkeit und des damaligen Code-Kandidaten bleiben zur Fehlerhistorie erhalten; `Live-Test offen` in älteren Absätzen bezeichnet keinen aktuellen Entwicklungsbedarf mehr.

**Reproduzierter Nutzerbefund:** Nach Bestätigung eines Stapels mit sieben
Foundation-Folgen und Schließen des Browsers wird der bereits gestartete
Einzelauftrag über Core/Agent weiter nachgehalten, **die nächste ausgewählte
Folge wird aber nicht automatisch gestartet**. Die Oberfläche stellt beim
Wiederöffnen einzelne vergangene Core-Operationen wieder her, nicht den Stapel
als fortzusetzenden Auftrag. Die Aussage, dass der laufende FFmpeg-Prozess
selbst durch das Schließen beendet wird, ist damit **nicht** belegt.

**Quellbefund auf `main` e31a532c5d4064e7c619f8185d3fecd5a1a0aec4:**
`web/frontend/modules/media-import.js::confirmBatch` legt `remaining`,
`completed` und `waiting` ausschließlich in `activeBatches` im
JavaScript-Speicher ab. `advanceBatch` setzt `resumeNext`; erst
`readOperation` ruft für den nächsten Film `startImport(view)` auf.
`restoreDurableAttempt` kennt dagegen nur einzelne
`media.import`-Operationen. Bei geschlossenem Browser existiert kein
Aufrufer für die noch nicht reservierten Folgeaufträge. Das ist **kein**
Problem der MT.ENC3.1-Encoder oder der bestehenden SSE-Ereignisse.

**Verbindliche Korrektur:**
1. Nach einer einmaligen, ausdrücklichen Stapelbestätigung die **gesamte**
   geordnete Auswahl aus ausschließlich vorgeprüften, direkt importierbaren
   Quellen und die zugehörigen bereits aufgelösten TMDb-Metadaten beim Core
   dauerhaft annehmen. Kein Import weiterer nicht ausgewählter Quellen.
2. Für jede Folge dieselben bestehenden ADR-0042-Operationen, immutable
   Payloads, Idempotency-Key-Fences, Agent-Kommandos, VDR-Readback und
   einzelnen Fehler-/Unknown-Zustände verwenden. Browser und SSE sind nur
   Beobachter, niemals Treiber des nächsten POST. Keine zusätzliche
   Auftragsdatenbank oder zweiter ausführender Worker.
3. Nach Tab-/Browser-Schließen, Seitenneustart und SSE-Unterbrechung muss
   der Core alle angenommenen Folgen **ohne Browser** weiter abarbeiten.
   Bereits bestätigte Folgen dürfen nicht wieder importiert werden; ein
   unklares Ergebnis darf niemals durch einen neuen Import derselben
   Quelle kaschiert werden.
4. Beim Wiederöffnen den Core-Auftrag samt Fortschritt und Fehlern aus
   dauerhaftem Zustand lesen und anschließend SSE verwenden; kein
   Status-Polling. Ein „Stop“ darf ausschließlich noch nicht begonnene
   Aufträge betreffen, niemals den aktiven Core-Import abbrechen.
5. Gezielt testen: sieben ausgewählte Folgen, Browser nach erstem POST
   geschlossen, alle sieben VDR-Readbacks mit korrekten TMDb-Metadaten;
   Browser- und Agent-Neustart, verlorene POST-Antwort, doppelte
   Idempotency-Key-Wiederholung sowie ein fehlerhafter Einzelfilm.
   SOURCE/UNIT, PRODUKTIONS-INTEGRATION und REAL-VDR separat kennzeichnen.

**Code-Kandidat (09.11.2026):** Der bestätigte Direktimport-Stapel
geht jetzt in **einem** `POST /media-imports` (`batch` mit 2–100
opaque Source-IDs und vorhandenen TMDb-Snapshots) an den Core. Dort werden
sämtliche einzelnen ADR-0042-`media.import`-Operationen samt immutable
Payload und deterministischen `mb1_<32hex>_NNN`-Schlüsseln in **einer**
SQLite-Transaktion reserviert. Der bestehende Daemon-Lifecycle-Tick
(2 Sekunden) weist nur die erste noch nicht gestartete Folge zu, für
die **alle Vorgänger durch den Core mit `succeeded` und nativer
Aufnahmebestätigung** abgeschlossen wurden. Die Agent-Kommandos, deren
Reconciliation, Autorisierung und die vorhandene Datenbank werden
wiederverwendet. Der Browser versendet **keine** Nachfolge-POSTs und
kann nach einem Neustart die Stapelgruppe anhand der bestehenden
actor-scoped `GET /media-imports`-Projektion wieder anbinden.

Ein bestätigter `outcome_unknown` oder Fehler stoppt weitere
Folgezuweisungen. Der bisherige nur lokal wirksame Stopp-Knopf wird für
solche serverseitig bereits angenommenen Stapel ausgeblendet; die
Sammellöschung bleibt wie bisher separat browsergesteuert.

**Status:** Implementierung und gezielte API-/Frontend-Regressionen
quellseitig ergänzt, **Build, Tests und realer Browser-Schließen-Test
auf yaVDR noch nicht bestätigt**. Ein bereits auf dem VDR installierter
MT.ENC3.1-Worker/Agent bleibt bis zur gesonderten Installation unverändert.

## Verbindliche Debug- und Sperrfreigabe-Regel (29.10.2026)

**Betreiberentscheidung:** Für **jede neue Sperrlogik** in Media Tools / Media Import
muss vor der Implementierung eine ausdrückliche Rücksprache und Freigabe
erfolgen. Das gilt für Webfrontend, Core, Backend-Agent, Add-on und Installer,
einschließlich indirekter Sperren durch globale Pending-/Unknown-Zustände.

Vor einem Vorschlag konkret benennen:
1. Welche **belegte Fehlerklasse** die Sperre verhindern soll;
2. **welche einzelne Quelle/Aktion** betroffen wäre und warum keine größere Reichweite;
3. sichtbare Benutzerwirkung, Aufhebung und mögliche Alternativen;
4. bestehende Prüfungen, die bereits ausreichen könnten.

**Ohne Freigabe keine neue Blockade, kein zusätzliches Gate und kein
pauschales Deaktivieren unabhängiger Filme oder Aktionen.** Bestehende
notwendige Authentifizierung/Autorisierung, Quellenidentität und Readback werden
durch diese Regel weder deaktiviert noch stillschweigend ausgeweitet.

**Diagnose zuerst:** Möglichst konkrete, *belegte* Status-/Fehlerangaben
aus den vorhandenen API-/Agent-/Core-Ergebnissen anzeigen. Die öffentliche
Operation liefert derzeit zwar ihren Core-Zustand, jedoch nicht die
Agent-Ausführungsstufe, den Dateisystemfehler oder eine einzelne
Löschquittung. Ein `outcome_unknown` ist daher keine bestätigte Löschung,
aber auch kein nachgewiesener Löschfehler. Fehlende Informationen als
*unbekannt* kennzeichnen, keine Ursachen erfinden. Technische Details
der Benutzeroberfläche dürfen Backend-ID, Quellkennung, Aktion,
Operations-ID, Status, letzte Statusabfrage und bereinigte Fehlerklasse
anzeigen, jedoch keine geheimen Tokens oder freie interne Fehlermeldung
ungeprüft ausgeben. Zur tieferen Ursachenfindung vorhandene lesende
Agent-/Command-Ergebnisdiagnose nutzen. Diagnostik führt keinen neuen Import,
keine Löschung, keinen Hintergrundjob und keine Berechtigungsänderung aus.
**Diese Regel erfordert weder DB-Backup noch neues Sicherheitsframework.**

## BAS — Backend-Agent Simplification (nächste Phase nach Media Tools; NOT STARTED)

**Verbindliche Nutzerentscheidung vom 28.10.2026:** Nach Abschluss der laufenden Media-Tools-/Media-Import-Arbeiten ist **Backend-Agent Simplification** die nächste Refactoring-Phase. Bis dahin werden die bereits freigegebenen MT-Arbeiten fortgeführt; BAS beginnt **nicht** automatisch durch diesen Roadmap-Eintrag. Die übergreifende Reihenfolge steht auch in [VDR-Suite Strict Roadmap](../../docs/planning/roadmap.md). Nummerierte Phase 70 wird dadurch weder umbenannt noch gestartet.

**Ziel:** Den bestehenden Backend-Agenten auf Transport, Quellbeobachtungen und tatsächliche Ausführung beschränken. Reine lesende Medienfunktionen (Scan, lokale Metadaten, Vorschau, Artwork) sollen ohne zusätzliche künstliche Aktivierungs- oder Ausführungsfreigaben funktionieren. Vorhandene Authentifizierung, notwendige Berechtigungen für Schreibaktionen, Importbestätigung und Readback bleiben erhalten. Keine neuen Rollen, Sicherheitsschichten, Dienste, Queues oder Datenbanken ohne erneuten expliziten Auftrag.

**Geplante, abgegrenzte Slices:**

- **BAS.1 – Tatsächliche Agent-Aufrufkette erfassen:** Je einen realen Scan-/Preview-/Importpfad gegen Code und Runtime protokollieren; überflüssige Rundreisen und doppelte Inventurläufe benennen, ohne neue Prüfungen einzuführen.
- **BAS.2 – Lesepfade vereinfachen:** Eine Quellbeobachtung liefert Quellenliste und ausgewählte Vorschau; dieselben Daten nicht mehrfach scannen. Fachliche Leseoperationen im vorhandenen Backend-/Media-Tools-Code lassen, wo sie hingehören.
- **BAS.3 – Fehlerdiagnose statt Sammelcode:** Konkreten technischen Fehler bis in das Agent-Log verfolgen und in der UI verständlich anzeigen. Fehlende Datei, veränderte Quelle, FFprobe-Fehler, Platzproblem und fehlende Zielschreibbarkeit nicht pauschal als `source_invalid` darstellen.
- **BAS.4 – Vorhandene Prüfungen konsolidieren:** Doppelte und unbegründete Schutzprüfungen entfernen, dabei bestehende Authentifizierung, Autorisierung vor echten Mutationen und Aufnahme-Readback nicht aushebeln. Jede beibehaltene Prüfung muss einer nachweisbaren Fehlerklasse dienen.
- **BAS.5 – Reale Benutzerwege und Installation:** CLI-Erkennung und Webvorschau derselben Datei gegentesten; ein bereits getesteter Commit, ein bestehender Staging-/Updateweg, ein nachvollziehbarer Rollback. Keine neuen Einmal-Installer für einzelne Bugs und kein erneutes Abfragen bestandener Tests ohne technischen Grund.

**Akzeptanz:** Ein bereits erkannter lokaler Film beziehungsweise eine Folge lässt sich über Browser und Backend-Agent zuverlässig anzeigen und technisch prüfen; die Fehlerursache ist bei Abweichung sichtbar; kein mehrfacher Vollscan pro ausgewählter Vorschau; Import und Readback bleiben funktional; Installation/Upgrade verwenden den vorhandenen versiegelten Runtime-Weg. Bestehende Schutzgrenzen bleiben bestehen, aber es werden keine neuen Restriktionen erfunden.

**Status 28.10.2026:** Die MT-bezogene Ein-Scan-Vereinfachung ist quellseitig umgesetzt; die Runtime wurde installiert und die Agent-Synchronisierung nach einem anfänglichen Fehlversuch bestätigt. Der Andor-Ende-zu-Ende-Test der neuen Version ist **noch offen**. BAS insgesamt ist damit ausdrücklich **nicht abgeschlossen und noch nicht begonnen**.


## MT.REC — Media Import: Fehlerkorrektur und Mockup-gerechtes Redesign (verbindliche Produktentscheidung 27.10.2026)

**Priorität vor kosmetischen Arbeiten:** zuerst die real beobachteten Defekte auf yaVDR reproduzieren und beheben, **danach** die Oberfläche so nah wie möglich an das im Chat am 27.10.2026 gezeigte Mockup bringen. Dieses Ziel ist **geplant, nicht implementiert oder auf REAL-VDR abgenommen**. Die historischen MT.UX-/MT.HA-Einträge weiter unten dokumentieren ältere Zwischenstände; bei Widersprüchen hat die hier festgehaltene aktuelle Nutzerentscheidung für die kommende Umsetzung Vorrang.

**Vorgabe des Betreibers:** Keine neuen Sicherheitsregeln, Rollen, zusätzlichen Freigabegates oder künstlichen Benutzungssperren erfinden. Die bestehenden authentifizierten Core-/Agent-/Public-v1-/Operation-Wege wiederverwenden. Keine neuen Worker, Importengines, Queues oder Dienste. Dies ist keine Anweisung, bestehende Berechtigungen zu umgehen. Keine Änderungen an `AGENTS.md` ohne eigene Genehmigung. Keine Neuinstallation, kein Dienstneustart und keine Dateirechteänderung als UX-Workaround.

### Verbindlicher Funktionsvertrag

1. **Rectools-Marker sind verbindlich, in beide Richtungen kompatibel.** `<stem>.skipped.<ext>` = bewusst übersprungen; `<stem>.pc_encode.<ext>` = externe Bearbeitung, etwa mit HandBrake; `<stem>.duplicate.<ext>` = vorhandene VDR-Aufnahme. Alle drei Marker gelten auch dann, wenn **vdr-rectools** sie vorher gesetzt hat. Die normale Importauswahl darf diese Dateien nicht als importierbar zählen oder eine Importvorschau als gewöhnlichen Film erzwingen. Angezeigter Status und tatsächlicher Dateiname müssen konsistent sein. Die gewöhnliche Bedienoberfläche verlangt **keine manuelle Freigabe** dieser Statusmarker; die vorhandene kollisionssichere `release`-Operation bleibt nur zur ausdrücklichen Fehlerkorrektur unter den eingeklappten technischen Details erreichbar und startet niemals einen Import.
2. **Dublette automatisch erkennen UND markieren; KEIN manueller „Als Duplikat markieren“-Button.** Wenn Media Tools eine **wirklich vorhandene** VDR-Aufnahme für den konkreten Quellkandidaten verlässlich nachweist, folgt automatisch die Rectools-kompatible `.duplicate`-Markierung. Ein Titeltreffer allein darf keine behauptete Dublette erzeugen. Der Scan-/Prüfschritt ermittelt den Befund lesend; die anschließende automatische Markierung ist eine **eigene, wiederholungs- und absturzverträgliche Mutation** über die **bereits vorhandene** Media-Import-Operation, Quellenidentität und deren Readback. Keine zusätzliche Benutzerentscheidung und kein neuer Freigabe-/Sicherheitsweg; keine Umbenennung bei ungeklärter/fehlender VDR-Bestätigung. Die Oberfläche zeigt den tatsächlichen Status und ggf. laufende/ungewisse Markierung deutlich an. Wird `.duplicate` bereits durch Rectools vorgefunden, nicht erneut umbenennen.
3. **Encoding-Auswahl bei notwendiger Umkodierung; HEVC zusätzlich freiwillig.** Kompatible H.264/HEVC/MPEG-2-Quellen behalten den klaren normalen Remux-/Importweg als Standard. Für HEVC kann die Bedienperson zusätzlich für genau diesen Film ausdrücklich **„Auf dem VDR nach H.264 umkodieren“** auswählen; ohne diese Auswahl bleibt das HEVC-Video unverändert. Bei `videoReencodeRequired=true` werden die notwendigen Wege angeboten: **„Auf dem VDR umkodieren“** (bestehender lokaler Worker und explizite erforderliche Zustimmung) oder **„Extern bearbeiten“** (am Rechner mit geöffnetem Webfrontend oder an einem beliebigen PC mit HandBrake/anderer Software; bestehender `.pc_encode`-Marker). „Überspringen“ bleibt eine optionale eigene Aktion. **Webbrowser-Transkodierung oder direkter Download/Upload dürfen nicht vorgetäuscht werden**: Datei-/Share-Zugang und tatsächlicher lokaler Workflow sind bei Umsetzung zu prüfen. Wo kein sicherer, schon vorhandener Transferweg existiert, den externen Ablauf mit zugänglicher Originalquelle und späterer Wiedervorlage verständlich erklären. Originaldatei erhalten; keine automatische Umkodierung bloß wegen Erkennung.
4. **Fehler sind Diagnose, nicht Benutzerquiz.** Bei nicht erfolgreicher Vorschau konkreten beobachteten Grund zeigen (`pending`, `unavailable`, Quellmarker, Codec-/Dateizugriff, Platz, Rechte/Backend, 401/403/503, Zeitüberschreitung), ohne interne IDs/Textwände im Hauptbereich. Fehlende Daten nicht als bestätigte technische Eignung oder als „importierbar“ ausgeben. Erneut prüfen durch einen klaren Button; Fehlerursache nicht durch zufällige Dateiauswahl verdecken.
5. **Keine automatische normale Importausführung.** Quellsuche, Vorschau und Dublettenentscheidung wählen keinen Film zum Importieren aus. Ein echter Import oder VDR-Re-Encode benötigt weiterhin die vorhandene zielgerichtete Benutzeraktion. Stapelverarbeitung verändert nur die bestätigte Auswahl. Eine erkannte Dublette darf dagegen ohne Zusatzklick gemäß Punkt 2 markiert werden.

### Verbindliches visuelles Ziel (Mockup 27.10.2026)

- **Navigation:** `Home → Kanäle → Aufnahmen → Media Import → …`; aktiver Modulreiter an der vorgesehenen Stelle, nicht links von Home. Den echten DOM-/CSS-/Runtime-Befund prüfen; kein erfundener HTML-Fix.
- **Breit nutzbares, responsives Dashboard** mit Kopfbereich `Media Import`, Importquelle, Suche, Film/Serie-Filter und verifizierten Statuszahlen. **Drei Spalten**: links Serien-/Ordnerbibliothek mit optional echten Backend-Postern; Mitte vollständige, übersichtliche Episoden-/Filmliste mit Staffel, Folge, Titel, Größe, Status und Checkbox; rechts eine große Vorschau-/Detailkarte mit optional echtem Artwork, Quelldaten, Codec, Ziel und **einer** situativ richtigen Primäraktion.
- **Marker visuell unterscheiden**, ohne manuelle Doppelklick-Workflows: Importfähig (Blau), `.skipped` (Gelb), `.pc_encode` (Violett), `.duplicate` bzw. bestätigte VDR-Dublette (Rot), bestätigter Import (Grün **nur mit VDR-Readback**), ausstehend/fehlerhaft neutral mit Klartext. Keine erfundenen Inhalte, Laufzeiten, Dateigrößen oder Poster. Bereits markierte Dateien bleiben als Status sichtbar; normale Anwender müssen die Marker nicht manuell zurücksetzen.
- **Aktionen passen zum Zustand**: importfähig → `Importieren`; echte Dublette → „Bereits vorhanden / automatisch als Duplikat markiert“ **ohne Duplikat-Button**; Re-Encode nötig → nur dann VDR-vs.-externe-Bearbeitung; markiert → nur Statusanzeige (bei `.pc_encode` Hinweis auf die später zurückgelegte Datei); eine seltene Korrektur ist ausschließlich in den technischen Details erreichbar. Keine pauschale Buttonreihe `Für PC markieren / Überspringen / Als Duplikat markieren` im normalen Preview.
- Batchauswahl integriert und gut sichtbar **unten** mit Auswahlzähler, `Alle auswählen`, `Auswahl aufheben`, `Auswahl prüfen`, später `Geeignete importieren`; keine riesige vertikale Debug-Formularseite, keine doppelt sichtbaren Knöpfe. Responsive auf schmalem Bildschirm als gestapelte Bibliothek/Liste/Detailansicht. Keyboard/Focus/Lesbarkeit mitprüfen.
- **Vergleichsmaßstab** Kodi/Plex: Medienbibliothek → Serie → Episode → Detail als vertraute Navigationsidee; produktspezifische aktuelle Fremdfunktionen hier **nicht verifiziert**. Keine Kopie fremder Marken-/Bildmaterialien oder spekulative Features.

### Verbindliche Mockup-Referenz und Rectools-Rücklauf (Nutzerentscheidung 27.10.2026)

**Versioniertes Referenzbild:** [Media Import – verbindliches Mockup vom 27.10.2026](media-import-reference-2026-10-27.png). Das Originalbild ist eine gestalterische Vorgabe, keine Zusicherung implementierter Datenquellen, Poster, Metadaten oder Funktionen.

Das **zweite, zuletzt gezeigte Media-Tools-Mockup** im Chat vom 27.10.2026 ist die visuelle Referenz für die folgende Webfrontend-Umsetzung: linke Media-Tools-Navigation, Importquelle und Scanübersicht, Statuszahlen, Verzeichnisbaum links, Film-/Episodenliste mit Status/technischer Kurzinfo in der Mitte, große Poster-/Metadaten-/Technik-/VDR-Vorschau rechts sowie kompakte Auswahl-/Importleiste unten. Die UI darf keine ungeprüften fiktiven Cover, Serienmetadaten oder Importfähigkeiten als echte Scanergebnisse präsentieren. **Bildreferenz bedeutet nicht, dass die dargestellten erweiterten Funktionen bereits implementiert sind.**

**Dateimarker exakt wie vdr-rectools `develop`:** `Film.pc_encode.mkv` (an HandBrake/PC delegiert), `Film.skipped.mkv` (übersprungen) und `Film.duplicate.mkv` (schon als VDR-Aufnahme vorhanden). Der produktive Rectools-Code nutzt `.pc_encode`, nicht `.encode`. Er ignoriert markierte Dateien beim gewöhnlichen Scan. Eine vom PC zurückgelegte neue, **unmarkierte** Datei wie `Film.mp4` wird beim nächsten Scan unabhängig als normaler Importkandidat erkannt. Ist der Ordner und der Titelstamm mit einer geparkten `.pc_encode`-Datei gleich, zeigt das Media-Import-Webfrontend diesen **Namenshinweis** an; er ist keine inhaltliche Identitätsgarantie, keine Importfreigabe und löst keine Dateioperation aus. Erst die bestehende technische Vorschau entscheidet über die Eignung; regulärer Import erfordert weiterhin die Benutzeraktion.

**Aufräumvertrag klar trennen:** Im geprüften Rectools-`develop` entfernt `process_import` nach erfolgreichem VDR-Import nur die tatsächlich importierte **unmarkierte Quelldatei**. Das geparkte `.pc_encode`-Original bleibt. Suite hält dieselbe Trennung: Importer und Core bestätigen zunächst unabhängig die neue Aufnahme; erst danach darf der bestehende Agent die eindeutig wiedererkannte unmarkierte Datei entfernen. Eine Namensähnlichkeit oder stabile `sourceId` beweist diese Identität nicht.

**PC-Encode-Rücklauf – bestehende Importpipeline:** Eine neue Datei wie `Film.mp4` neben `Film.pc_encode.mkv` wird eigenständig gescannt, per FFprobe geprüft und nur auf ausdrücklichen Core-Auftrag importiert. Der mögliche Zusammenhang zu `Film.pc_encode.mkv` ist ein reiner Anzeigehinweis, **keine automatische Medienzuordnung oder Löschfreigabe für das geparkte Original**. Ein neuer Scan kann andere Dateien unter denselben opaken IDs finden; Browserhistorie allein ist keine Bereinigungsquittung.

**MT.REC4.6/1 – Core→Agent-Readback (historisch, 27.10.2026):** Der Core reserviert nach positivem Recording-Cache-Readback die dauerhafte `addon.media.import.readback`-Nachricht vor dem CAS auf `succeeded`. Dieses zuerst implementierte Nachrichtenformat quittierte nur den Erhalt und löschte ausdrücklich keine Datei. Alte Readback-Journale ohne Quellsignatur bleiben weiterhin **nicht löschend**.

**MT.REC4.6/2 – Identitätsgebundene Quellenbereinigung (Quellstand PR #556, 27.10.2026):** Der bereits vorhandene C++-Importer prüft Geräte-/Inode-/Größen-/Nanosekunden-mtime-Signatur vor der Snapshot-Verarbeitung. Der Agent bindet die tatsächlich ausgeführte Quellbeobachtung (einschließlich absoluten Quellpfades **nur innerhalb des Hashes**) an das aufgezeichnete Importergebnis; das VDR-Pfadergebnis bleibt unabhängig davon und muss durch den Core-Recording-Cache eindeutig bestätigt werden. Das Core-Readback erzeugt eine Version-2-Nachricht mit der ursprünglichen Quellenidentität. Der Backend-Agent verwendet seinen vorhandenen Command-Receipt-/Result-Speicher und überprüft vor der Löschung die erneut inventarisierte Signatur sowie die noch unter dem Dateinamen liegende Datei. `.pc_encode`, `.skipped`, `.duplicate`, fremde/ersetzte Dateien und Quellen ohne eindeutigen Herkunftsnachweis werden nicht gelöscht. Die Quellbereinigung schreibt kein VDR-Medium, startet keinen zweiten Import und verändert den bereits bestätigten `succeeded`-Status nicht. Nach einer verlorenen Result-Quittung wird ausschließlich das dauerhafte Agent-Ergebnis erneut gesendet, niemals der ursprüngliche Import.

**Lesbarer Vertragsstatus:** `GET /api/v1/operations/{id}` kann nach bestätigtem Import optional den aus dem Agent-Journal projizierten Wert `sourceCleanup` (`pending`, `removed`, `already_absent`, `not_performed`, `failed`) liefern. Bei anderen Operationen fehlt das Feld. Die Versionskennung/ETag berücksichtigt den nachträglich eintreffenden Agent-Status. Das Webfrontend nutzt nur diesen Status für die Aussage zur Bereinigung; der gegenwärtige Dateibestand folgt weiter ausschließlich aus einem neuen Scan. Fehler bei der Bereinigung ändern keine bestätigte VDR-Operation in `failed`.

**Prüfgrenze (Stand dieses Commits):** Isolierte Quellenbereinigungs-, Agent-Neustart-/Lost-ACK-, Readback-, Core- und Frontend-Regressionen sind im Branch ergänzt. Eine **erfolgreiche lokale Ausführung dieser neuen Tests ist hier nicht nachgewiesen**; insbesondere ein kompletter C++-Build und CI-Abschluss dürfen noch nicht als bestanden gemeldet werden. **REAL-VDR-Abnahme/Installation auf yaVDR offen**, keine produktive Datei-/Diensteänderung vorgenommen. Der Slice ist funktional im Quellcode verbunden, aber bis zum bestandenen Testblock und separater Hostprüfung **nicht abgenommen**.

**Metadatenziel des Referenzmockups:** vorhandene lokale `.nfo`-Metadaten, Mediendaten und optional echte externe Film-/Seriendaten mit Artwork sollen vor dem Import erkennbar und, soweit vom VDR unterstützbar, in die VDR-Aufnahme übernommen werden. Der bestehende Importer überträgt aktuell bereits lokale NFO-Titel/Outline/Plot/Jahr/Genre in `info`; ein automatischer TMDb-Abgleich, Online-Cover, Fanart, Serien-Matching und die im Mockup dargestellten Metadaten-Tabs sind **noch kein implementierter End-to-End-Pfad** und benötigen eine gesonderte, überprüfbare Integration ohne erfundene Daten.

## MT.META — Desktop- und Handy-Medienoberfläche (vereinbart 27.10.2026)

**Referenzen:** Das bestehende versionierte Media-Import-Mockup bleibt für Desktop verbindlich (drei Spalten mit Ordnern, Film-/Episodenliste, Vorschau und Stapelleiste). Die neu besprochene mobile Darstellung zeigt dieselben Daten und Aktionen in getrennten Vollbreiten-Ansichten. Die Illustrationen enthalten Beispielposter und Beispieldaten, keine implementierten Metadaten oder Löschbefugnisse.

**Gemeinsame Architektur:** Weiterhin genau ein Webfrontend und die vorhandenen Public-v1-/Core-/Agent-Wege. Desktop zeigt gleichzeitig Ordner, Liste und Details; Tablet nutzt vorhandene responsive Aufteilung; Handy (< 700 CSS-px) zeigt jeweils **Ordner**, **Medien** oder **Details**. Kein neuer Importdienst, keine zweite Queue, keine eigene Handy-App; Zugriffs-/Bestätigungslogik bleibt unverändert. `.duplicate` wird niemals über einen neuen manuellen Duplikat-Button gesetzt. Auch ein allgemeiner „Datei löschen“-Button gehört nicht zum Import.

| Slice | Konkretes Ergebnis | Abnahme |
| --- | --- | --- |
| **MT.META1 – Responsive Navigation und Struktur** | Die vorhandene Desktop-Drei-Spalten-Komposition beibehalten; auf Handys zugängliche Ansichtsschalter Ordner/Medien/Details, Rückweg zur Liste, Quellauswahl springt zur Vorschau; normale Suche und Stapelkontrollen bleiben dieselben bestehenden Elemente. **Code in PR #556 erweitert**. | Reale DOM-/Frontend-Regression für Ansichtswechsel ohne POST, ausgewählte Quelle öffnet Detail, Fokus/Touch-Ziele und Desktop-Parität; Browser-Screenshot/REAL-VDR separat. |
| **MT.META2 – Lokale Metadaten** | Bereits aus `.nfo` und FFprobe vorhandene Daten entlang des bestehenden Lesewegs in der Detailkarte sichtbar machen: nur tatsächlich belegte Titel, Beschreibung, Jahr, Genre und technische Felder; fehlende Felder klar leer statt erfunden. | Echte NFO-/FFprobe-Fixtures bis zum Webfrontend, Status/Backend-Scope unverändert. |
| **MT.META3 – Lokales Artwork** | Bereits vorhandene lokale Poster/Fanart benutzen, sonst neutraler Platzhalter, dabei Backend-Berechtigung und Bildformate der bestehenden Asset-Route wiederverwenden. | Echte Bildbytes, nicht vorhandenes Bild, falsches MIME, Wechsel zwischen Quellen und mobile Poster-Darstellung. |
| **MT.META4 – Serien und Episoden** | Aus belegten Ordner-/Datei-/Metadateninformationen serienbezogene Gruppierung, Staffel-/Folgeansicht und mobile Auswahl; vorhandene eindeutige Source-IDs sowie der bestätigte Einzel-/Stapelimport bleiben maßgeblich. | Mehrere Staffeln/gleiche Titel, keine erfundenen Episodenidentitäten, markierte Originale unverändert. |
| **MT.META5 – Optionale Online-Metadaten** | TMDb soweit über bestehende Suite-Metadatendienste verfügbar ergänzen; explizite Trefferprüfung statt Titelraten, lokale NFO-Inhalte nicht still überschreiben. | Authentifizierte Datenquelle und Bildherkunft, mehrdeutige Treffer, Offline-Fallback, keine Fake-Bewertung. |
| **MT.META6 – VDR-Metadaten-Übernahme, Korrektur und Aufnahmeaktualisierung** | Bestätigte Serien-/Episoden-Metadaten auf die gespeicherte VDR-Aufnahme übertragen, bereits importierte Aufnahmen ohne erneuten Videoimport korrigierbar machen und nach erfolgreicher Veröffentlichung **die vorhandene VDR-/Suite-Aufnahmeliste automatisch aktualisieren**. | Tatsächliche `info`-/Metadata-Readback-Belege, neue Aufnahmen ohne Browser-Neuladen sichtbar, erneute Titel-/Artwork-Projektion nach Korrektur; Core-/Agent-/Frontend- und REAL-VDR-Nachweise getrennt. |


**Realer Foundation-Web-Abnahmestand (08.11.2026):** Auf yaVDR wurden
Foundation (2021) S01E06–E08 als zusammenhängender Sammelimport mit korrekt
zugeordneten TMDb-Folgentiteln/Artwork angezeigt. Die zuvor bereits importierte
S01E05 („Erwachen“) wurde durch den nachträglichen Backfill **ohne erneuten
Videoimport** vervollständigt; die Oberfläche meldete
`1 Folgen ergänzt; 0 bereits zugeordnet/nicht eindeutig; 0 fehlgeschlagen`
(Commit `15115744fb5920da062f453c12469ff4256a9a7e`).
**Abnahmegrenze:** Dies bestätigt die sichtbare Suite-Metadatenprojektion, nicht
automatisch den nativen `info`-Inhalt oder einen VDR-Neuladevorgang.
Der zusätzliche isolierte MT.META6.5-API→Writer→`info`→Readback-Test prüft
deshalb eine nachträgliche Episode-5-Korrektur mit erhaltenen TS-/Index-Bytes;
REAL-yaVDR-`info`-Readback bleibt getrennt zu verifizieren.

**09.11.2026 – isolierter yaVDR-Test PASS:** `make test-metadata-manual-recording-api` auf dem aktualisierten `main` ausgeführt; `FOUNDATION_NATIVE_TEST=PASS`. Der Test verwendet eine künstliche `.rec`-Aufnahme und bestätigt API→Writer→`info`-Readback, Erhalt der TS-/Index-Bytes und Revisionskonflikt. Die anschließende nur lesende Echtdateisuche unter `/srv/vdr/video`, `/var/lib/video` und `/video` lieferte **keine Ergebnisse**; das ist noch kein Negativbefund für die Dateien, weil `/srv/vdr/video` auf diesem yaVDR gemäß dem früheren VDR-Hostbefund auf `video.00` zeigt und `find` Symlink-Wurzeln ohne `-L` nicht durchläuft. Der nächste Hosttest verwendet deshalb die absoluten `backend_native_id`-Pfade des bestehenden `vdr_recording_cache`; bis dahin ist der native `info`-Inhalt von Foundation S01E05–E08 REAL **nicht verifiziert**.

### MT.META6 — verbindlicher Integrationsablauf (28.10.2026)

**Erweiterung 29.10.2026 – MT.META6 dauerhafte Suite-Zuordnung, Quellcode:**
Der bereits vor dem Import eindeutig erkannte TMDb-Kandidat wird nun als
abwärtskompatibler `v2`-Snapshot im **selben** unveränderlichen
`addon.media.import`-Payload gespeichert. Er enthält zusätzlich zur
Serien-/Folgenbeschreibung die genaue `tv-episode`-/`movie`-Provider-ID
sowie die vorhandene Posterreferenz (fehlende/ungültige Posterreferenz
ist optional und verhindert den Videoimport nicht).
Das Format `v1` bleibt lesbar; der bestehende Importvertragsweg und die
Command-Versionen 8/9 werden weiterverwendet. Keine zweite TMDb-Abfrage,
kein neues Datenbankschema, kein neuer Auftrag/Agent oder Scraper.

`DaemonRuntimeRecordingCache.cpp` liest bei der **ohnehin stattfindenden
VDR-Aufnahme-Cache-Aktualisierung** bereits erfolgreich reconciliierte
Importoperationen und deren ursprüngliche Agent-Recording-Path-Evidence.
Nur die exakt damit korrelierte native `.rec` wird über die bestehende
`MetadataRepository::assignManualRecordingMetadata`-Zuordnung mit dem
gefrorenen TMDb-Kandidaten verbunden. Eine schon vorhandene manuelle
Zuordnung wird nicht überschrieben. Derselbe Snapshot steht nach
Browserende/Daemon-Neustart weiterhin im Core-Journal zur Verfügung;
erneute Cache-Aktualisierung ist idempotent. Der Browser vermeidet für
diese v2-Aufträge den konkurrierenden nachträglichen
`metadata/assign`-POST. Bereits vorhandene Legacy-/manuelle
Reparaturpfade bleiben erhalten.

**Noch ungetestet und nicht installiert:** Der gesamte C++-Daemon-/Agent-
Build, Frontend-Kompositionstest, realer VDR-Readback inklusive
Poster-Materialisierung und Wiederanlauf. Die zugehörigen Unit-/
Vertragstests wurden im Repository ergänzt, aber hier nicht
ausgeführt. Bei fehlschlagender Artwork-/Metadatenzuordnung bleibt
der Videoimport erfolgreich; die Zuordnung wird nicht als erfolgreich
ausgegeben. Insbesondere offen bleiben die native `info`-Nachkorrektur
bestehender Andor S01E01–S01E05 via berechtigtem Backend-Schreibpfad,
zugehörige VDR-Aufnahmeaktualisierung, Real-yaVDR- und TV-App-Abnahme.

**Quellstand 29.10.2026 – MT.META6 Vorab-Metadaten-Snapshot umgesetzt, Abnahme offen:**
Der bestehende automatische TMDb-Suchpfad identifiziert eine eindeutige
Serienfolge bereits vor dem Import. `web/frontend/modules/media-import.js`
friert deren **schon gefundenen** Serientitel, Episodentitel, Beschreibung
und SxxExx in `metadata` ein und übergibt diesen Snapshot beim **ersten**
Media-Import-POST. Der bestehende Public-v1-Client/Router, die permanente
Core-`MutationOperation` inklusive Payload, die Agent-Zuweisung und der
bereits vorhandene angepinnte Import-Worker transportieren exakt diesen
Snapshot **ohne zweiten TMDb-Request**. Die neuen Payload-Versionen 8
(direkter Import) und 9 (explizit lokale Umkodierung) werden in den bestehenden
Core-/Agent-Prüfungen verwendet. Alte v1–v7-Import- und Marker-Operationen
bleiben unverändert gültig. Falls vor dem Import keine eindeutige TMDb-Auswahl
vorliegt, fehlt die optionale `metadata` und der normale
NFO-/Dateinamen-Import bleibt unverändert verfügbar.

Der Worker dekodiert die unveränderliche Auswahl und schreibt **beim
privaten Aufnahme-Staging** `T` (Serie), `S` (Episodentitel), `D`
(Beschreibung) in die native `info`. Das Video wird nur einmal importiert.
Die bestehende Suite-Zuordnung mit Artwork läuft weiterhin über den
bestehenden `metadata/assign`-Folgeschritt und sucht TMDb **nicht erneut**.
Bei nativer Titelkorrektur hält ein kleines
`.vdr-suite-import-origin`-Sidecar im neuen `.rec` den geprüften
ursprünglichen Quell-Dateizustand fest, damit der nächste Scan die Aufnahme
trotz veränderter `T`/`S`-Felder als Dublette erkennt. Die alten
`info`-Titel-/Provenienzregeln werden nicht geändert; es wurde keine neue
Sperrlogik, Datenbank, Agenteninstanz oder Speicherplatzprüfung eingeführt.

**Aktueller Nachweis:** JS-Quellcode syntaktisch geprüft; tatsächlicher
Public-v1-Client mit einem gemockten HTTP-Transport erfolgreich für
Legacy-Import, Metadatenimport und lokalen Metadatenimport ausgeführt;
die reine Browser-Snapshot-Funktion mit eindeutiger Episode und
Offline-Fallback isoliert bestanden. C++- und Browser-Kompositions-Regressionen
sind ergänzt, aber **noch nicht in einem Checkout ausgeführt**;
`REAL_YAVDR=NOT_INSTALLED/NOT_VERIFIED`. Ein Commit oder isolierter
JS-Test ist keine Funktionsabnahme.

**Weiter offen:** Suite-Poster-/Assignment-Abschluss nach
Browserende oder Agent-Neustart als serverseitig dauerhafte Zuordnung,
Nachkorrektur bestehender Andor-S01E01–S01E05-Aufnahmen über native
Recording-Identität ohne Videoimport, native `info`-Readback-Abnahme,
vollständige C++-/Web-/REAL-yaVDR-Tests und Client-Refresh-Nachweis.

**Quellstand 29.10.2026 (inkrementell, NICHT vollständig):**
Der bestehende Media-Import-Worker besitzt jetzt
`renderNativeVdrRecordingInfo(..., confirmedMetadata)` und verwendet die
Funktion beim privaten Staging neuer Aufnahmen. Für explizit bestätigte
Serienfolgen werden `T` (Serie), `S` (Episode) und `D` (Beschreibung)
gebildet; vorhandene NFO-Angaben bleiben Fallback. Die Typisierung
`ImportConfirmedMetadata` ist noch **nicht** durch die bestehende
Public-v1-/Core-/Agent-/CLI-Kette bis zum Worker verbunden. Ohne diese
Anbindung schreibt der produktive Import weiterhin die NFO-/Dateinamenwerte.

Für bestehende Aufnahmen steht außerdem die **reine, dateisystemfreie**
Funktion `mergeConfirmedMetadataIntoNativeVdrInfo(...)` bereit. Sie ersetzt
nur bestätigte `T`-/`S`-/`D`-Werte und belässt andere VDR-Informationssätze
(`C`, `E`, `F`, `P`, `X`) in ihrer ursprünglichen Reihenfolge.
Fehlende Provider-Felder löschen bestehende Werte nicht. Synthetische
Unit-Testfälle für Andor S01E01/S01E02, CRLF, unbekannte Felder,
Wiederholbarkeit sowie unveränderte TS-/Index-Ausführung sind im
`test_media_import.cpp` ergänzt. Eine isolierte C++17-Kompilierung
und ein Funktions-Smoke-Test der reinen Korrektur sind **PASS**; der echte
Repository-C++-Testlauf und Real-yaVDR sind **NOT_RUN/NOT_VERIFIED**.

**Noch offen und ausdrücklich nicht als fertig zu melden:**
(1) bestätigte Provider-Identität kompatibel/versioniert in den existierenden
Import-Operationsvertrag aufnehmen; (2) die unveränderliche Auswahl durch
API/Core/Agent/CLI bis zum Writer transportieren und durchgehende Tests
ausführen; (3) vorhandene Aufnahme über die reale backendgebundene
Recording-Identität nachkorrigieren, ohne freie Dateipfade oder zweiten
Videoimport; (4) Native-Info-Readback, VDR-/Suite-Refresh, Poster und Clients
auf realem VDR abnehmen. Bis dahin zeigt die Weboberfläche bei erfolgreicher
`metadata/assign`-Antwort ausdrücklich **nur Suite-Zuordnung** an.
Weder neue Sperren noch Datenbank-Backups wurden dafür eingeführt.

**Realer yaVDR-Befund:** Star Wars: Andor S01E01 und S01E02 wurden als vollständige `.rec`-Aufnahmen mit `info` und TS gefunden und erscheinen in VDR-Suite unter „Neu aufgenommen“ sowie bei den Serien. Es fehlen aber die Episodentitel, die TMDb-Episodenbeschreibung und das erwartete Serienposter. Die auf Dateinamen basierenden VDR-`info`-Felder `T DE - Star Wars_ Andor ...` und `S Imported via VDR-Suite Media Tools` belegen: Die TMDb-Identität aus MT.META5 bleibt derzeit ausschließlich browserseitige Vorschau. Das Wiedererscheinen in der Suite beweist **nicht**, dass die native VDR-Aufnahmeliste ohne Verzögerung aktualisiert wurde.

**MT.META6.1 – Metadaten-Identität statt Dateiname:** Den bestehenden authentifizierten TMDb-Kandidaten- und Recording-Metadata-Assignment-Lese-/Schreibpfad wiederverwenden. Eine eindeutig erkannte Episode (Serie, Staffel, Episodennummer, Provider-Identität) als tatsächliche Auswahl/Bestätigung behandeln; fehlende oder mehrdeutige Treffer bleiben ungelöst. Keine Metadaten aus unbestätigter fuzzy-Übereinstimmung an VDR-`info` schreiben; keine zusätzliche Datenbank, kein neuer Agent oder paralleler Scraper.

**MT.META6.2 – Aufnahme-Metadaten vor Veröffentlichung:** Die tatsächlich bestätigten, normalisierten Titel-/Episodentexte und, sofern zulässig, Bildreferenzen bis zum bestehenden Aufnahme-Metadaten-Owner führen. Bestehende NFO-Priorität/Quellherkunft ausdrücklich festlegen und VDR-`info` konsistent schreiben; die bestehende atomare Aufnahmepublikation und verifizierte TS-/Index-Dateien bleiben unverändert. Kein alleiniger Browser-POST, der eine abgeschlossene Aufnahme behauptet. Durchgehende Tests für S01E01 „Kassa“, S01E02 „Das bin ich“, unbekannte Folge, Offline-Fallback und geänderte Quelle.

**MT.META6.3 – Interner atomarer Info-Writer (Quellstand 29.10.2026):**
Die Funktion `updateConfirmedNativeVdrInfo` ergänzt die reine T/S/D-Transformation
für eine bereits vom Backend bestätigte native VDR-Aufnahme. Nur `info`
wird atomar ersetzt und nochmals gelesen; TS/Index/NFO werden nicht verändert.
Die vorhandene `.update`-Benachrichtigung wird nach einem tatsächlichen
Update verwendet. Wiederholte identische Korrekturen sind idempotent,
ohne zweite TMDb-Suche oder Videoimport. Synthetische C++-Tests erfassen
auch fremde Pfade und Symlink-Dateien. **Noch offen:** die autorisierte
Core-/Agent-Bindung an die bestätigte Recording-Identität, Poster-Readback,
echte S01E01–05 auf yaVDR und der VDR-/Suite-Client-Abgleich. Der interne
Writer ist ausdrücklich noch keine von der Weboberfläche erreichbare Aktion.

**MT.META6.3 – KISS-Anbindung (29.10.2026):** Der bestehende
authentifizierte `metadata/assign`-Aufruf übernimmt den bereits bekannten
Serientitel als flüchtigen Wert. Nach erfolgreicher persistenter Suite-
Zuordnung wird über den vorhandenen backendgebundenen Recording-Cache
die exakte native VDR-Aufnahme erkannt und der interne atomare `info`-Writer
aufgerufen. Die bereits ausgewählte Recording-Action-VDR-Root-Konfiguration
gilt ebenso für diese Aktion. Das bestehende Zuordnungsergebnis meldet
`nativeInfoUpdated` nur nach erfolgreichem Write/Readback. Bei nicht
verfügbarer nativer Schreibberechtigung bleibt die Suite-Zuordnung bestehen,
aber der native Erfolg wird nicht behauptet. Kein neuer Endpunkt, Agent,
Worker, TMDb-Request oder Datenbankschema. yaVDR-Dienstrechte und
vollständige Integration sind noch unabhängig zu prüfen.

**MT.META6.3 – Integrationsregression (29.10.2026):** Die vorhandene
`test-metadata-manual-recording-api`-Suite kompiliert jetzt den echten
Media-Tools-`info`-Writer mit ein. Eine bereits vorhandene, synthetische
Andor-S01E01-`.rec` durchläuft den authentifizierten
`recordings/metadata/assign`-Runtime-Aufruf, die echte persistente
Suite-Zuordnung sowie den atomaren `info`-Writer mit Readback. Der
Test belegt `T=Star Wars: Andor`, `S=Kassa`, `D` inklusive
Erhalt vorhandener `C/E/F`-Sätze, unveränderte TS-/Index-Bytes,
keine zusätzliche TMDb-Providerabfrage und `nativeInfoUpdated=true`
nur nach dem tatsächlich ausgeführten Writer. Die vorhandene Browser-
Regression prüft weiterhin, dass die Serienposterreferenz in demselben
`v2`-Snapshot und der vorab ausgewählte Serientitel im bestehenden
`metadata/assign`-Aufruf mitgehen. Dies ersetzt **nicht** die getrennte
Prüfung der produktiven Daemon-Konfiguration und der realen yaVDR-Rechte.

**MT.META6.3 – Alte Dubletten bleiben nach Korrektur erkennbar:** Der
bisherige Importer hat bei Legacy-Aufnahmen den Quelltitel in `T` und
die eigene Herkunft in `S Imported via VDR-Suite Media Tools`
hinterlassen. Weil die native TMDb-Korrektur beide Felder ersetzt,
bewahrt der Writer einmalig diese **bereits belegte** Herkunft über
denselben `.vdr-suite-import-origin`-Sidecar-Namen, den der neue
Importer bereits verwendet (`legacy-title`-Format für alte Imports).
Die bestehende Quelleninventur erkennt danach die alte Aufnahme weiterhin
als Dublette. Unverändert bleibt die stärkere `v1`-Signatur neuer
Importe; keine zusätzliche Datenbank oder FFprobe-Prüfung. Ein
synthetischer Test prüft den vollständigen Weg vor und nach
`T/S/D`-Korrektur.

**MT.META6.5 – Poster und Client-Refresh (29.10.2026, Source):**
Der v2-Import friert bei Serienfolgen nun das **bereits vorab gefundene
Serienposter** ein, statt irrtümlich das Episoden-Still zu verwenden; der
zusätzliche `metadata/assign`-Aufruf für alte Aufnahmen verwendet
dieselbe Serienposterreferenz. Die existierende Public-v1
`GET /api/v1/recordings/{rec_}/artwork`-Route liefert bei einer
gespeicherten manuellen Zuordnung deren Suite-Poster als vorhandenen
opaquen `artworkId` und liest die Bildbytes über
`VdrRecordingFolderController::getMetadataImage`, denselben
authentifizierten Backend-Owner wie das Web. Keine privaten Pfade,
Provider-URLs oder TMDb-Tokens in der Geräteantwort; fehlende Poster
bleiben leer bzw. 404, statt ein altes Scraper-Bild als neue Auswahl
auszugeben. Die bereits bestehende `recordings`-Invalidierung
(SnapshotChangeFeed -> LiveTransportService -> Web-Update/Device-SSE)
wird nach Zuordnung genutzt; native `info`-Korrekturen stoßen zudem
die vorhandene Recording-Cache-Refresh-Queue an. Es werden keine
TV-eigenen Dienste oder weiteren Poller hinzugefügt.
Web/TV-Geräte-Liveabnahme einschließlich erfolgreichem Bild-GET
bleibt separat.

**MT.META6.5 – Webdetail-Invalidierung (29.10.2026):**
`recordings2-folder-refresh.js` nutzt das vorhandene
`recordings`-SSE-Ereignis jetzt auch bei einer **geöffneten
Aufnahme-Detailansicht**. Die bestehende Metadaten-Detailkomponente
liest nur die bereits autorisierte Recording-Metadatenroute neu,
ersetzt Episodentext und Poster und aktualisiert die vorhandenen
Metadatenfelder. Wiedergabe, Navigation, Marks und Playback-DOM
werden nicht neu initialisiert; ältere parallele Antworten können
keine neueren Metadaten überschreiben. Der bestehende Ordner-Refresh
bleibt bis zum Schließen des Details ausgesetzt. Die JS-Regression
prüft den vollständigen Besitzer-Lebenszyklus und Backend-Scope.

**MT.META6.3 – Existierende Aufnahmen korrigieren:** Über die bereits vorhandene backendgebundene Recording-Identität und Metadatenzuordnung, nicht über einen frei übergebenen Dateisystempfad. Die zwei bereits importierten Andor-Folgen ohne zweite Umkodierung oder erneuten Import ergänzen. Eine Änderung von VDR-`info` oder Titelverzeichnissen nur über einen separat geprüften, mit VDR abgestimmten Mutationspfad; bestehende TS-/Index-Inhalte erhalten.

**MT.META6.4 – Aufnahmeaktualisierung als Bestandteil jedes Erfolgs:** Nach dem atomaren Import den bereits vorhandenen Recording-Cache-Refresh-Mechanismus verwenden. `DaemonRuntimeRecordingCache.cpp` führt `refreshRecordingCacheForAllBackends`, `mediaImportOperationFence_->reconcileRecordingCache` und `publishCompletedRecordingRefreshes` bereits; `DaemonRuntimePolling.cpp` fordert Cache-Aktualisierung bei `RecordingsChanged` an. `DaemonRuntimeInitialization.cpp` fordert **bereits** bei verifiziertem, angewendetem Agent-Importergebnis mit Recording-Pfadbeleg `recordingCacheRefreshQueue_.request(assignment.backendId, 2)` an. Die Lücke ist daher **nicht die fehlende Queue-Auslösung**, sondern der End-to-End-Nachweis dieses bestehenden Auslösers bis zur VDR-nativen Erkennung, zum committed Recording-Cache und zum `RecordingsChanged`-Ereignis an Web/Apps, einschließlich Restart/verspätetem Readback. Kein separates SVDRP-`UPDR`-Skript und keine zweite Refresh-Queue einführen. `succeeded` erst nach unabhängigem Aufnahme-Readback; bei verzögerter Aktualisierung weiterhin sichtbar „Bestätigung ausstehend“. Nach bestätigtem Erfolg aktualisieren sich „Neu aufgenommen“, Serien und Recordings 2 **ohne Browser-Reload**.

**MT.META6.1A – vorhandene Suite-Metadatenzuordnung nach Recording-Readback (Quellstand 28.10.2026):** Media Import friert einen exakten, zur ausgewählten SxxExx-Kennung passenden TMDb-Episodenkandidaten (beziehungsweise explizit gewählten Film) beim bestätigten Importstart ein. Erst wenn die bestehende Core-Operation `succeeded` zurückliefert und der Recording-Folder-Cache genau **eine** Aufnahme im verifizierten Importziel mit dem erwarteten Ausgangstitel enthält, sendet der Browser einen zweiten, getrennt nachvollziehbaren Aufruf über die **bestehende** authentifizierte `/api/backends/{backendId}/recordings/metadata/assign`-Route. Die vorhandene Metadatenzuordnung verwaltet Staffel, Folge, Text und bevorzugtes Serienposter; ohne eindeutigen Recording-Treffer/bei Offline-/Berechtigungsfehler bleibt der Import erfolgreich, die Metadatenzuordnung aber ausdrücklich offen. Kein zweiter Import-POST und keine automatische Wiederholung nach ungewisser Metadatenantwort. Dieser Browser-Folgeschritt ist bisher **nicht dauerhaft auftragssicher**: Browserende zwischen Core-Erfolg und Metadaten-POST kann die Zuordnung auslassen. Deshalb ist MT.META6 insgesamt **nicht abgeschlossen**; die spätere serverseitig dauerhafte Zuordnung, die tatsächliche VDR-`info`-Übernahme und die Nachkorrektur bereits importierter Andor-Folgen bleiben offen. Das Produkt darf keine native `info`-Übernahme behaupten, solange der Core/Agent nur das bisherige NFO-/Dateinamen-`info` schreibt.

**MT.META6.5 – End-to-End-Abnahme:** Quelle → technische Vorschau → eindeutige Metadatenbestätigung → Import → VDR-`info` → native VDR-Liste → Suite-Recording-Cache → Change-Feed → Browser/TV-App. Tests prüfen zwei aufeinanderfolgende Episoden, fehlende TMDb-Verbindung, verzögertes oder verlorenes Änderungsereignis, identische Importantworten, doppelte Quellnamen und Wiederanlauf ohne zweite Aufnahme. Separat protokollieren: SOURCE, UNIT, INTEGRATION, CI und REAL-yaVDR; kein installierter Erfolg aus einem Commit oder CI-Status ableiten.

**Status:** Entwurf/Analyse; die in MT.META5 ausschließlich visuelle TMDb-Zuordnung ist noch **keine** produktive Metadatenübernahme. Das Nachrüsten bestehender Aufnahmen und die neue automatisierte Auslösung der Aufnahmeaktualisierung sind **noch nicht implementiert oder auf dem Host installiert**. Ein nötiger Backend-Build muss separat benannt werden; keine Behauptung „nur Frontend“ für Backend-/VDR-Metadatenänderungen.

**Repo-Prüfung und UI-Fortschritt am 28.10.2026:** PR #556 hat die bestätigte `.duplicate`-Löschung nun zusätzlich als unmittelbar sichtbare Listenaktion (statt ausschließlich nach Klick auf die markierte Quellenzeile in der Detailkarte). Bestehende Benutzerbestätigung, Core-/Agent-Prüfung und Markergrenzen bleiben unverändert. Eine Laufzeitfunktion ordnet den Media-Import-Hauptreiter nach „Aufnahmen“ ein, selbst wenn die bestehende DOM-Reihenfolge abweicht; `index.html` hatte diese Reihenfolge bereits. Der vorhandene JS-Kompositionstest wurde um eine Laufzeit-DOM-Umordnung und die Listenaktion ergänzt. **Diese Änderungen sind quellseitig, nicht auf dem realen yaVDR abgenommen.** Die native VDR-`info`-Korrektur und die auftragsdauerhafte TMDb-Zuordnung **sind weiterhin offen**.

**Verifizierte MT.META6-Codekette:** `web/frontend/modules/media-import.js` friert die Kandidatenauswahl nur in `attempt.metadataSelection` ein und sendet über `submitMediaImport` weiterhin keinen Metadatenparameter. `api/rest/src/PublicApiRuntime.cpp::parseMediaImportAdmissionBody` akzeptiert nur `sourceId` und optionale `videoDecision`, die unveränderliche Operation `MediaImportOperationAdmissionService` speichert nur diese Auswahl. `addons/media-tools/include/MediaToolsAgentExecutor.h` verwendet ein eigenes striktes Payload-/Versionsformat; der Agent übergibt den Import an die bestehende `vdr-suite-media-import`-CLI und `addons/media-tools/src/MediaImport.cpp::executeRemuxImport` schreibt die native `info` derzeit aus NFO beziehungsweise Quelldateinamen. Zur vollständigen Erfüllung von MT.META6.1–6.5 muss eine einzige kompatible, versionierte Metadaten-Auswahl über alle diese vorhandenen Schichten geführt und nach Recording-Readback an den bestehenden Assignment-Service gebunden werden. Der Kompatibilitäts- und Restart-Test dieses durchgängigen Vertrags darf nicht durch einen isolierten Frontend-POST ersetzt werden.

**MT.META3 Implementierung:** Die vorhandene, authentifizierte Importvorschau enthält nun ausschließlich größenbegrenzte optionale Bildbytes aus geprüften, direkt neben der Quelle liegenden `<Dateistamm>.poster.jpg/jpeg/png/webp` und `<Dateistamm>.fanart.jpg/jpeg/png/webp`. Die Bilddatentypen JPEG/PNG/WebP werden wie beim bestehenden Recording-Artwork durch Erweiterung und Dateisignatur geprüft, direkt aus dem Agent lesend transportiert und im Browser nur mit kontrolliertem Bilddatenschema angezeigt. Die Recording-Artwork-Route ist auf bereits indexierte VDR-Aufnahmen beschränkt und nicht direkt für noch unimportierte Quellen nutzbar; daher wird der bestehende Import-Preview-Contract ergänzt, **kein neuer Dienst, keine Datenbank und kein separater Bild-API-Endpunkt**. Maximal 256 KiB je Bild; generische `poster.jpg`/`fanart.jpg` gelten nur für einen Quellordner mit genau einem unmarkierten Video, niemals ungeprüft für gemeinsame Serienordner. Keine Dateioperation bei fehlenden, zu großen oder falsch formatierten Bildern. Geräte-/yaVDR-Screenshots und tatsächliche Bilddarstellung sind separat abnahmebedürftig.

**MT.META4 Implementierung (Quellcode):** Die vorhandenen autorisierten relativen Quellordner werden im Frontend als Serien-/Ordnergruppen angeboten. Explizite `Staffel N`/`Season N`-Verzeichnisse sowie `S01E02`-Kennungen erlauben einen Staffelfilter und die Anzeige einer Episodenkennung; unbekannte oder widersprüchliche Angaben bleiben unzugeordnet. Gleichnamige Dateien aus verschiedenen Verzeichnissen behalten getrennte Quell-IDs. Wechsel zwischen Gruppe/Staffel setzt alte Auswahl und Vorschau zurück und öffnet auf dem Handy wieder die Medienliste; danach erfolgt ausschließlich die vorhandene technische Einzelvorschau. `.pc_encode`/`.skipped`/`.duplicate` bleiben im tatsächlichen Ordner als Marker sichtbar und werden nicht als unmarkierte Importkandidaten behandelt. Keine neue Metadaten-Persistenz, API oder Importoperation; Smartphone-/yaVDR-Screenshotabnahme weiterhin separat.

**MT.META5 Implementierung (Quellcode, 28.10.2026):** Die bestehende Media-Import-Detailkarte bietet eine ausschließlich auf ausdrücklichen Klick ausgelöste, vorläufige TMDb-Kandidatensuche über die vorhandenen backendgebundenen und CSRF-geschützten VDR-Suite-Metadatenrouten für Filme, Serien, Staffeln und Folgen. Ein Titelgleichklang wählt niemals automatisch ein Ergebnis: Kandidaten bleiben getrennt, enthalten bestätigte Provider-/Typ-/TMDb-Identität, und eine Folge mit widersprüchlicher SxxExx-Kennung wird nicht als passend ausgewählt. Ausschließlich ein vom Benutzer ausdrücklich ausgewählter Kandidat kann als **flüchtige Online-Vorschau** mit Text und optionalem TMDb-Poster erscheinen. Poster von Film-/Episodenkandidaten nutzen den bereits vorhandenen serverseitig begrenzten und zwischengespeicherten `settings/series-artwork/candidate-image`-Leseweg; lokale Poster/Fanart bleiben unabhängig sichtbar. Fehlende Zugangsdaten, fehlende Rechte, fehlende/ungültige Treffer oder offline Betrieb beeinflussen die lokale NFO-/FFprobe-Vorschau nicht. Veraltete Antworten bei Quellen-/Backend-/Ansichtswechsel werden verworfen. Die TMDb-Auswahl ist **keine persistente Zuordnung**, keine automatische Medienidentität, kein Fanart-Download und fließt **nicht** in NFO, Importoperation oder VDR-`info` ein; die Metadatenübernahme gehört zu MT.META6. Vorhandener Import/Marker/Readback unverändert. Gezielte Frontend-Kompositions- und C++-Bildrouten-Regressionen sind eingecheckt; deren Ausführung und erfolgreiche Ergebnisse müssen am tatsächlichen Produkt-SHA nachgewiesen werden. **REAL-VDR: NOT_VERIFIED**, keine Installation und kein Dienstneustart.

**Status (27.10.2026):** MT.META1 ist quellseitig und in der CI bestätigt. **MT.META2 Code/CI PASS:** Lokale NFO-Texte (Kurzbeschreibung, Beschreibung, Jahr und Genre), Titel und optional ermittelte FFprobe-Laufzeit werden über den vorhandenen Agent-/Core-/Public-v1-Vorschauweg in der Desktop- und Handy-Detailkarte angezeigt; technische Werte bleiben Bestandteil der Vorschau. Am exakten Produkt-/Test-SHA `af97d1d30169cdd96dd05c48e2d8d20f0b830f62` ist [VDR-Suite CI #40586551656](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40586551656) mit sechs erfolgreichen Jobs abgeschlossen (Frontend, Fast Regression, Architektur, Make-Audit, Docs und Packaging). **REAL-VDR sowie echte Handy-/Desktop-Screenshotabnahme bleiben NOT_VERIFIED**; CI ist keine installierte Runtime. MT.META3 und MT.META4 sind quellseitig ergänzt; MT.META5 ist quellseitig ergänzt, aber noch nicht abgenommen; MT.META6 bleibt geplant. **MT.META4 Frontend/Architektur PASS** am Produkt- und Test-SHA `5317f743b09895fc1bf581f1bc7610178f2167cd`: [VDR-Suite CI #40597902322](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40597902322), relevante Jobs `frontend-regression-test`, `architecture-check` und `make-test-audit` erfolgreich. Die übrigen CI-Jobs und die echte Host-/Geräte-Abnahme sind separate Prüfstufen. Eine Metadatenvorschau importiert, markiert, kodiert oder löscht keine Quelle automatisch.


### REAL-yaVDR-Rückmeldung vom 29.10.2026 – S01E05 und neue UI-Befunde

Der Betreiber zeigt im laufenden yaVDR-Suite-Frontend fünf Andor-Aufnahmen
S01E01–S01E05 unter „Neu aufgenommen“; S01E05 („Die Axt vergisst ihr Werk“)
wurde neu importiert. Gleichzeitig meldet die Media-Import-Operation
`outcome_unknown` und die bereits vorhandene Aufnahme erscheint in der
Importquelle nicht dauerhaft als importiert markiert. Die sichtbare
Suite-Aufnahme beweist allein keinen abgeschlossenen Core-Operations-Readback.
Bei der Diagnose zuerst die konkrete Operation und ihren Agent-Ergebnisbeleg
gegen die vorhandene `reconcileRecordingCache`-Kette prüfen:
`resultReference`, `resultEvidence`, `completedAt`, Cache-Refresh-Zeit,
Backend-/Recording-Identität und `succeeded`-CAS. Keinen zweiten Import
der Folge starten und keinen Erfolg aus Dateinamengleichheit ableiten.

Weitere bestätigte Produktbefunde: Der Quellenkatalog liefert keine
dauerhafte aufnahmebestätigte Statuskennung für gewöhnliche Dateien; beim
Auswählen läuft wieder die technische Vorschau. Das blockierte Entfernen
bereits importierter Quelldateien ist deshalb kein bloßes CSS-Problem:
Ein ungewisser Auftrag belegt die Core-/Frontend-Mutationssperre. Es muss
eindeutig erklärt und über bestehende journalgestützte Resultate aufgelöst
werden. Ein aktives Löschen muss die tatsächlich bestätigte VDR-Aufnahme
unverändert lassen und nach Core-ACK die Inventarliste sichtbar aktualisieren.

Für MT.META6 sind S01E01–S01E05 als Bestandsaufnahmen **ohne erneuten
Videoimport** nachzukorrigieren: echte VDR-`info`-Texte, Zuordnung von
Serie/Staffel/Episode, Beschreibung und Artwork über die bestehenden
Recording- und Metadaten-Owner. Die aktuelle Online-Vorschau ist kein
Nachweis für diese Persistenz. Die quellseitig ergänzte manuelle
`metadata/assign`-Aktion für exakt bestätigte Dubletten korrigiert zunächst
nur Suite-eigene Metadaten. VDR-`info` und persistentes
Core-Operationsresultat bleiben getrennte offene Slices.

`.pc_encode` markiert weiterhin eine zurückgestellte Quelldatei. Als
kleine UI-Verbesserung wurde am 29.10.2026 eine sichtbare ausdrückliche
Freigabeaktion ergänzt. Nach bestätigter Core-Freigabe wird ausschließlich
die eindeutige, wieder unmarkierte Quelle neu geprüft und die bereits
vorhandene lokale H.264-Umkodierungswahl geöffnet. Weder der Release-Klick
noch die technische Vorschau startet einen Videokonvertierungsauftrag.
Keine neue Worker-Architektur.

**Abnahme:** Die genannten Änderungen sind aktuell GitHub-Quellstand in
PR #556. Der UI-Quelltext wurde syntaktisch mit V8 geparst. Die vollständigen
Frontend-/C++-Regressionen und der reale yaVDR-End-to-End-Lauf sind für
diesen Stand **nicht ausgeführt / nicht bestätigt**; weder Agent-Dienste noch
produktive Dateien wurden durch den Repository-Commit verändert.

#### MT.REC6 – S01E05: dauerhafte Recording-Reconciliation (29.10.2026)

**REAL-yaVDR read-only evidence:** `op_44214ce6c1401c69f4f226b1e5b6ffd3`
steht seit 03:01:39 UTC auf `outcome_unknown`, Command
`cmd_879ab78fc8a8cdadf5df064e` meldet
`waiting_reconciliation / accepted_by_executor / reconcile_only` mit
Aufnahme-Pfad-Evidence
`media-import-path-sha256-v1:61ab18e5e075148f5179381e6fa75e370e799af86839fe109af0d055821e9abc`.
Der Cache `default` wurde erst danach (03:10:19–03:10:24 UTC) erfolgreich
aktualisiert, hat aber weiterhin die Operation nicht als Erfolg reconciliert.

Der Backend-Recording-Cache zeigt S01E05 mit dem darstellungsbezogenen
`path=/Serien/Star Wars-Andor/.../2026-10-29.04.01.1-0.rec` und der
absoluten `backend_native_id=/srv/vdr/video/Serien/Star Wars-Andor/.../2026-10-29.04.01.1-0.rec`.
Auf yaVDR zeigen `/srv/vdr/video` und `/srv/vdr/video.00` real auf dasselbe
Verzeichnis. Unabhängiges Nachrechnen des vollständigen SHA256-Inputs ergibt
einen **exakten Treffer nur für** `/srv/vdr/video.00/Serien/Star Wars-Andor/.../2026-10-29.04.01.1-0.rec`.
Die alte Funktion `exactRecordingIdentity` hashte ausschließlich
`VdrRecording.path`, das hier nur cache-relativ vorliegt; damit kann ein
korrektes Agent-Ergebnis nie erfolgreich reconciliert werden.

**Fix in PR #556:** Die existierende Core-Fence prüft jetzt in derselben
belegten Recording-Snapshot-Reihe den bisherigen `path`, die tatsächliche
`backendNativeId` und, für lokal verfügbare komplette `.rec`-Pfade,
die per Filesystem aufgelöste kanonische `backendNativeId`. Keine Suche
nach gleich klingenden Titeln und kein neues Resultat aus einer ungeprüften
Aufnahme. Backend, eindeutige Recording-Identität, journalgesicherte
Command-/Source-Evidence und jüngerer `ready`-Cache bleiben verbindlich.
Der bestehende Readback-Notice-/Cleanup-Pfad wird wieder nutzbar, ohne
Quelldateien ungeprüft zu löschen.

Regression `core/agent/tests/test_backend_agent_command_delivery.cpp`
erstellt isoliert einen synthetischen `video -> video.00`-Symlink,
ein relatives `recording.path` und die exakte physische Agent-Evidence.
Der Core muss diesen Fall nach aktuellem Cache-Readback einmalig nach
`succeeded` überführen; falsches Recording/Backend, doppelte Treffer und
veralteter Cache dürfen nicht bestätigen.

**Status:** GitHub-Commit vorhanden; noch kein Backend-Build und keine
produktive Installation/REAL-yaVDR-Abnahme. Dieser Fix wird erst mit dem
**Core-Daemon**, nicht bloß dem Media-Tools-Frontend oder Importer,
wirksam. Bereits vorhandene S01E05 nicht neu importieren.

#### MT.REC6 – dauerhafte Quellenanzeige und Dubletten-Löschen (29.10.2026, Code)

**Implementierter Quellvertrag:** Der bestehende `RootOwnedMediaImportInventory`
erkennt eine vollständige, zum Importtitel passende VDR-`.rec`-Aufnahme direkt
bei der regulären Quellinventur mit `confirmedMediaImportDuplicate(plan)`.
Die Aussage `recording_present` wird beim nächsten Scan erneut aus den
vorhandenen Dateien abgeleitet; sie ist **kein** zusätzlicher DB-Status und
behauptet nicht, dass die alte Core-Operation bereits `succeeded` ist.
Beschädigte, abweichende oder ersetzte Zielaufnahmen ergeben
`unverified`. Das Inventar startet hierfür keine Video-/FFprobe-Prüfung
und löst keinen Import, keine Markierung und keine Löschung aus.

**Vorhandener Datenpfad:** `MediaImportSourceLabel.recordingPresent`
→ Agent-Source-Observation `sourceStatuses` (parallel zu IDs/Labels/Foldern)
→ `BackendAgentMediaImportCatalog` (nur live Agent, gleicher Snapshot,
max. 100 Einträge) → Public v1 `recordingStatus` mit derselben
Cursor-/Revision-Gültigkeit → Media-Import-Listenansicht. Alte
Agenten/Clients ohne Status bleiben nutzbar und zeigen weiter `unverified`
statt einen Import zu erfinden. Die UI zeigt „Bereits importiert –
VDR-Aufnahme vorhanden“ direkt neben jeder passenden Quelle und bietet
einen „Bereits importiert“-Filter.

**Weniger Prüfungen:** Beim Anklicken einer bereits als
`recording_present` gemeldeten Quelle fordert das Frontend nicht erneut
eine technische FFprobe-Vorschau an; es verwendet die bereits bestätigte
Inventarbeobachtung. Nur bei einer explizit erzwungenen Neuprüfung, bei
unbekannten Quellen und für tatsächliche neue Importe ist die reguläre
technische Vorschau weiterhin erforderlich.

**Dubletten-Löschung:** Eine unmarkierte, im Scan bestätigte Dublette
erhält unmittelbar in ihrer Listenzeile den Button „Dublette aus
Importquelle löschen“. Der Button wählt ausschließlich die konkrete
opaque `sourceId` und verwendet nach Benutzerbestätigung das bereits
vorhandene `delete_duplicate`-Core/Agent-Verfahren mit erneuter
Quell-/Zielidentitätsprüfung. Es gibt **keinen** direkten PHP-/JS-Unlink,
keine neue Rolle/DB/Agenten-Gate und keine automatische Löschung. Erst die
`succeeded`-Operation mit erfolgreicher erneuter Quelle-Erfassung
entfernt den Eintrag aus der Liste und meldet „gelöscht“. Bereits
`.duplicate`-markierte Quellen behalten denselben funktionierenden
Listenbutton.

**Tests eingecheckt:** `test_local_media_import_inventory.cpp`
(belegtes Recording / unpassender Titel / nach Löschung kein Status),
`test_backend_agent_lifecycle.cpp` (Agent→Catalog),
`test_public_media_import_source_list.cpp` (Ausgabe, ungültiger Status,
Snapshot-Cursor nach Statuswechsel),
`test_media_import_composition.js` (erneutes Öffnen ohne Preview/POST;
explizites Löschen mit genau einer Mutation, Neuscan).
Außerdem gilt der zuvor implementierte MT.REC6-Fix für
`outcome_unknown` bei `/srv/vdr/video` ↔ `/srv/vdr/video.00`.

**Abnahmestatus:** Commit in PR #556, **noch keine behauptete** erfolgreiche
vollständige C++-/Frontend-Testausführung, kein bestätigter
Core-/Agent-/Web-Runtime-Build und keine Installation auf yaVDR.
Zur Wirksamkeit müssen die aktualisierten Runtime-Binaries einschließlich
Daemon und Backend-Agent sowie die Frontend-Staging-Dateien zusammen aus
einem exakt geprüften Stand installiert werden. **REAL-yaVDR**: Prüfen,
dass S01E01–S01E05 beim Laden bereits erkannt sind, Anklicken ohne
Preview-Operation funktioniert, einmaliges Löschen eines zuvor separat
freigegebenen Test-Duplikats die Quelle entfernt und die VDR-Aufnahme
unverändert lässt. Die fünf Andor-Quelldateien nicht ungefragt löschen.
Die native Metadaten-Übernahme bleibt **MT.META6**, nicht MT.REC6.

## MT.SCAN – Start-Scan mit sichtbarem Prüfstatus und manueller Priorität (31.10.2026)

**Vor MT.ENC umsetzen und real abnehmen.** Ziel: Nach Start der Suite bzw.
des Media-Tools-Add-ons zeigt die Oberfläche sofort die verfügbaren
Importordner, Filme und Serien. Während der technischen Prüfung werden
Quellen schrittweise als „Noch nicht technisch geprüft“, „Prüfung läuft“,
„Direktimport möglich“, „Umkodierung nötig“, „Dublette erkannt“ oder
„Prüfung momentan nicht möglich“ angezeigt. Der Benutzer kann jederzeit
eine bestimmte Folge wählen; diese Prüfung hat sofort Vorrang vor der
Hintergrundverarbeitung. **Keine Datei wird allein durch einen Lesescan
importiert, umbenannt, markiert oder gelöscht.** Nur die bestehende
ausdrücklich autorisierte Core-Mutation darf Dateien ändern.

### MT.SCAN1 – Sofort benutzbare Liste und gestaffelte Prüfung beim Öffnen (Code)

- Die vorhandenen Agent-Quellbeobachtungen und Public-v1-Quellenliste
  werden weiterverwendet. `Media Import` wird sofort geöffnet; der
  bereits existierende Verzeichnisscan lädt die Liste automatisch ohne
  vorherigen Benutzerklick.
- Nach erfolgreichem Quellenabruf beginnt die Browseransicht einzelne
  **lesende** technische Vorschauen nacheinander zu laden. Aktueller
  Prüfstatus und erledigte/offene Anzahlen erscheinen in der Medienliste.
- Pro Backend existiert derzeit im Core nur **ein transienter
  Vorschau-Slot**. Daher darf der Hintergrund-Scanner weder parallele
  GET-Stürme noch wechselnde Pending-IDs auslösen. Er bearbeitet eine
  Quelle bis zum Ergebnis, pausiert bei einer manuell gewählten Folge
  und setzt danach die übrigen fort. UI-Wechsel verwirft alte Antworten.
- Bei Scanfehlern bleibt die zuletzt erfolgreiche Quellenliste benutzbar.
  Die Beobachtung ist kein Import-/Lösch-/Freigabeversprechen.
- Quellcode erster Teilslice: `web/frontend/modules/media-import.js`,
  `web/frontend/tests/test_media_import_composition.js`. **Noch nicht
  auf realem yaVDR getestet oder als vollständig abgenommen.**

### MT.SCAN2 – Prüfungen schon beim Server-/Agent-Start (Quellcode implementiert; REAL-VDR-Abnahme offen)

- Den **bereits vorhandenen** Agent-Start und dessen periodische
  Importquellen-Beobachtung verwenden. Technische Vorprüfungen sollen
  künftig ohne geöffneten Browser laufen, ohne den Agent-Command-Poll zu
  blockieren. Kein neuer unabhängiger Scanner, Dienst oder Schreibjob.
- Prüfstatus pro tatsächlicher Quelle/Quellgeneration im serverseitigen
  Katalog ablegen. Nach Anmeldung erhält die Oberfläche bereits
  geprüfte/nicht geprüfte Ergebnisse; späterer Browserwechsel und
  Agent-Resynchronisierung zeigen den richtigen Status, keine erfundene
  Fertigmeldung.
- Ein interaktiver Preview-Auftrag rückt vor die Hintergrundqueue. Die
  aktuellen single-slot-Preview-Semantiken entweder kontrolliert
  koordinieren oder um genau diese Priorisierung erweitern. Ein
  automatischer Hintergrundauftrag darf weder interaktive
  Vorschauen aushungern noch eine bereits angenommene Mutation verdrängen.

**Implementierung 31.10.2026:** `BackendAgentMediaImportCatalog` startet aus
dem vorhandenen, authentifizierten Agent-Quellensnapshot selbständig die
nächste lesende Einzelvorschau und fordert sie über die *bestehende*
`previewSourceId`-Antwort der Agent-Beobachtung an. Geprüfte Ergebnisse
werden pro Backend/Agent-Generation und opaker Source-ID im Core-Speicher
vorgehalten. Eine manuelle Auswahl überschreibt die gerade angeforderte
Quelle sofort; die Agent-Antwort des verdrängten Films ist kein Ergebnis.
Bereits importierte und Rectools-markierte Quellen überspringt die
Hintergrundprüfung. Kein neuer Dienst, kein neuer Endpoint, keine
Datenbank-Tabelle und keine automatische Import-/Marker-/Löschaktion.

`core/agent/tests/test_backend_agent_media_import_catalog.cpp` prüft
Reihenfolge, Priorität, Ergebniscache und Quellenänderung;
`core/agent/tests/test_backend_agent_lifecycle.cpp` prüft dieselbe
Reihenfolge über die echte authentifizierte Agent-HTTP-Route.
**Quellcode-Stand ist nicht gleich REAL-VDR-Abnahme.** Ohne vorhandene
Browser-Anmeldung laufen Quellscan und Vorschau im serverseitigen
Heartbeat weiter; nach Anmeldung verwendet die vorhandene Frontend-UI
die Core-Preview-Antworten. Fortschritt wird weiterhin im geöffneten Tab
sichtbar; ein dauerhaft gespeicherter Scanbericht über Daemon-Neustarts
ist nicht behauptet.

### MT.SCAN3 – yaVDR-Abnahme

**Quellcode-/Testabgleich 11.11.2026 (kanonisches `main`, keine neue Hostabnahme):** Die nachstehenden Funktionen sind **nicht mehr erst zu implementieren**, sondern existieren bereits in der Produktkomposition:

- `web/frontend/modules/media-import.js::loadSources` öffnet die bereits verfügbare Quellenliste, überführt ausschließlich vollständige gültige Public-v1-Inventare in den sichtbaren Zustand, beginnt danach `startBackgroundChecks` und erhält bei HTTP 503/teilweiser/staler Antwort die zuletzt erfolgreiche Liste. Eine reine Quellenaktualisierung erzeugt keinen Import-POST.
- `sourceTechnicalLabel`/`updateBackgroundDisplay` zeigen pro Quelle ungeprüft, laufend, geprüft, Dublette oder nicht prüfbar sowie die erledigten/offenen Zähler. `runBackgroundCheck` hält jeweils nur eine read-only Core-Vorschau offen; die manuelle Selektion besitzt Vorrang und alte Generationen/Antworten werden nicht übernommen. Ein zeitlich begrenztes Preview-GET für einen ausstehenden *Lesecheck* ist **kein Polling eines laufenden Importauftrags**; der Operation-SSE-Vertrag bleibt getrennt.
- `core/agent/src/BackendAgentHttpServer.cpp` ruft `BackendAgentMediaImportCatalog::nextPreviewRequest` im bereits vorhandenen authentifizierten Quellen-Beobachtungsweg auf; die Agent-Seite startet also technische Prüfungen auch ohne offenen Browser. `core/agent/tests/test_backend_agent_media_import_catalog.cpp` enthält Tests für Priorität, stale Resultate, Quellwechsel, Transientfehler, Quellgrenzen und das Wiederfinden gültiger bereits geprüfter Quellen nach Katalog-Neuerstellung über die bestehende SQLite. Dies ist **kein** Nachweis eines kompletten über Neustarts dauerhaften Scanberichts.
- `web/frontend/tests/test_media_import_composition.js` enthält bereits reale DOM-/API-Kompositionsregressionen für sichtbare Liste vor Hintergrundprüfung, den geprüften Status, priorisierte manuelle Vorschau, keine automatischen Mutations-POSTs, Tabwechsel/Caching, temporären Agent-Ausfall und erhaltene Quellenlisten bei HTTP 503, vollständige/partielle Scans und Drei-Spalten-/Handy-Navigation. Die Implementierung verwendet `web/frontend/style.css` mit drei Desktopspalten, zwei Spalten bis 1100 px und drei per Schalter zugänglichen Vollbreitenansichten bis 700 px.

**Bewertung:** `MT.SCAN1=INTEGRIERT`; `MT.SCAN2=INTEGRIERT`; `MT.SCAN3=SOURCE/CONTRACT_COVERED_REAL_HOST_PENDING`. Für diese Bestandsfunktionen **kein weiterer Worker, Scanner, SSE-Ersatzkanal oder grundloser Neuaufbau**. Der reale yaVDR-Startzustand, die konkrete Agent-Unterbrechung und eine manuelle Prioritätsauswahl sind durch diesen reinen Repository-Abgleich nicht beobachtet worden. Nur diese realen Sichtfälle – ohne Datei-Mutation, ohne erneuten Import und ohne Serverneustart – stehen zur abschließenden SCAN3-Abnahme aus.

- Start der Suite und Start/Wechsel des Media-Import-Tabs getrennt
  prüfen; kein manuelles `Verzeichnis scannen` als Voraussetzung.
- Sichtbare Anzahl ungeprüfter/geprüfter Quellen, technische Status-
  aktualisierung und Auswahlpriorität mit zwei oder mehr Quellen
  bestätigen; alte Antworten nach Auswahl/Tabwechsel nicht übernehmen.
- Kein ungebetener Import, keine automatische Dubletten-Löschung,
  keine bereits bestandenen Andor-Aufnahmen mutieren. Nach Login, Agent-
  Störung, Rescan und spätem Öffnen werden echte Daten angezeigt.
- Kein CI-Warten, kein unnötiger Komplettbau; für reines Frontend
  gezieltes JS-Update, für spätere Backend-Änderungen den vorhandenen
  schnellen inkrementellen Build nutzen.

### MT.I3 / MT.ENC3 – Distributionsneutraler Ausführungsvertrag (09.11.2026)

**Verbindliches Ziel:** Media Import, Metadaten-Writer, langlaufende Core-Operationen,
SSE und Encoding benutzen **einen gemeinsamen Linux-/VDR-Produktpfad**.
yaVDR/Ubuntu ist das real geprüfte Referenzsystem; MLD und openSUSE dürfen
nicht durch hart kodierte yaVDR-Pfade ausgeschlossen werden. Unterschiede in
Werkzeugen, VDR-Videoverzeichnis, Init/Diensten, Paketmanager, Benutzer/Gruppe
und VDR-/FFmpeg-ABI bleiben Host-/Distributionskonfiguration. Das ist ein
**Querschnittsvertrag innerhalb MT.I3 und MT.ENC3**, keine neue Phase,
kein eigener Importer, keine zweite Queue und keine neue Sicherheitsfreigabe.

**Aktueller Quellbefund:** `addons/media-tools/src/MediaImport.cpp` verwendet
`/usr/bin/ffmpeg`, `/usr/bin/ffprobe`, `/usr/bin/vdr` und festes
`libx264 -preset medium -crf 23 -threads 2`. Der native `info`-Writer
arbeitet bereits mit übergebenem VDR-Root, nicht mit festem
`/srv/vdr/video.00`; die Foundation-S01E05–E08-`info`-Dateien wurden
am 09.11.2026 auf realem yaVDR vollständig gelesen (4/4, T/S/D vorhanden).
Der einzige erfolgreiche Hostbefund ist **keine** Freigabe für MLD/openSUSE.
Außerdem nutzt der aktuelle Software-Shrink-Pfad in
`addons/media-tools/src/MediaShrink.cpp` `libx265 -crf 27`, während
MT.ENC3 den gewünschten Rectools-Ausgangswert CRF 23 aufführt: diese
Abweichung vor einer Übernahme der Settings ausdrücklich auflösen, nicht
durch einen stillen Defaultwechsel.

**Nächste Slices in verbindlicher Reihenfolge:**

1. **MT.I3.1 – Read-only Portabilitätsinventar:** Alle vom laufenden
   Media-Import-/Shrink-Worker tatsächlich verwendeten FFmpeg-/FFprobe-/VDR-
   Programm- und Videowurzelpfade und ihre vorhandenen Config-/Stage-Owner
   ermitteln. Installiertes yaVDR-Verhalten dokumentieren; für MLD/openSUSE
   keine Pfade oder ABI-Werte erraten. Nur Quellprüfung und synthetische Tests,
   kein Eingriff in den Host.
2. **MT.I3.2 – Ein backendgebundenes Werkzeugprofil:** Den vorhandenen
   Worker mit den nachgewiesenen Programmpfaden für FFmpeg, FFprobe und
   VDR-Indexbildung versorgen. Die Quelle ist die vorhandene vertrauenswürdige
   Backend-/Agent-Hostkonfiguration, **nicht** ein Browser-POST, frei
   editierbare FFmpeg-Kommandozeile oder ein zweiter Launcher. Bestehende
   yaVDR-Defaults funktionieren ohne Konfigurationszwang. Fehlende Werkzeuge
   zeigen einen konkreten Fehler, statt neue Add-on-Sperren einzuführen.
   Keine Shell-Interpolation, bestehende gepinnte Ausführung erhalten.
3. **MT.ENC3.1 – Typisierte Encoderwerte:** Erst auf dieser Grundlage die
   geplanten backendbezogenen CRF-/Preset-/Audio-/Hardwarewerte in den
   **bestehenden** Core-Auftrag und Worker einhängen. Nur unterstützte Werte,
   kein freies `argv`; einen bei Auftragsstart fixierten Snapshot verwenden,
   keine Änderung eines laufenden Encodes. Import-H.264 und Shrink-H.265
   getrennt behandeln. Vor dem Ändern des Shrink-Standards CRF 27 vs.
   gewünschtem CRF 23 bewusst entscheiden.
4. **MT.I3.3 – Portabilitätsregression:** Synthetische Hosts mit
   unterschiedlichen existierenden Binär-, VDR- und Servicepfaden prüfen;
   identisches Import-/SSE-/Readback-Verhalten, kein zweiter Import und
   kein Wegfall der inkrementellen Build-/Stage-Verträge.
   **REAL-Status getrennt:** yaVDR geprüft; MLD/openSUSE erst nach
   tatsächlicher Host-/ABI-/Init-/Paketabnahme als unterstützt kennzeichnen.

**MT.I3.1 – Read-only Quellinventar (09.11.2026, SOURCE PASS):**
- `MediaImport.cpp`: FFprobe an mehreren Vorprüf-/Dauer-/Codecstellen;
  FFmpeg bei Remux/Reencode; VDR `--genindex` nach dem Encode – alle
  drei derzeit als feste `/usr/bin/...`-Programme. Der bisherige
  `runRequired`-Fehlerklassifikator vergleicht ebenfalls exakt diese
  Pfadstrings; MT.I3.2 muss auch diese Klassifikation auf einen
  Werkzeugtyp statt einen hart kodierten Pfad umstellen.
- `MediaShrink.cpp`: eigene fest kodierte FFprobe-, FFmpeg- und
  VDR-`--genindex`-Aufrufe sowie VAAPI-Gerätestandard
  `/dev/dri/renderD128` (nicht universell vorhanden). Software-H.265
  nutzt derzeit CRF 27; dies ist noch **kein** freigegebener Wechsel auf
  den geplanten CRF-23-Wert.
- `MediaCheckFfmpeg.cpp`: bereits vorhandenes Vorbild eines
  konstruktorgebundenen `trustedExecutable` mit direktem `execv`
  ohne Shell. Dessen Vertrag gezielt wiederverwenden, nicht duplizieren.
- `addons/media-tools/src/main.cpp` und
  `PinnedMediaImportAgentExecutor.cpp`: der Import-CLI-Einstieg erzeugt
  einen `NativeProcessRunner`; der Agent startet den geprüften
  Handler über seinen gepinnten Dateideskriptor. Die neue Werkzeugwahl
  darf diese geprüfte Handleridentität und die Auftragsevidenz nicht
  ersetzen oder per Benutzer-POST steuerbar machen.
- `tools/addons/install_media_tools.sh` und
  `configure_media_tools_paths.sh`: yaVDR-/systemd-spezifische
  Host-/Installationsschicht mit Beispielpfaden
  `/srv/video/Filme`, `/srv/vdr/video.00`. **Nicht** als allgemeine
  Linux-/MLD-/openSUSE-Defaults übernehmen.

**Nächster Code-Slice ist MT.I3.2**, zunächst mit einem kleinen
testbaren Werkzeugprofil für FFmpeg/FFprobe/VDR im vorhandenen
Import-/Shrink-Executor und unverändert funktionierenden yaVDR-Defaults.
Synthetische Pfadvarianten zuerst testen; echte andere Distributionen
bleiben bis zur Hostabnahme NOT_VERIFIED. Dieses Quellinventar
hat keine Host-, Dienst- oder Medienänderung ausgelöst.

**MT.I3.2 – gemeinsame Werkzeugpfade (09.11.2026, quellseitig implementiert;
REAL-VDR noch nicht installiert):**
- Ein `MediaToolExecutables`-Profil mit den bestehenden Defaults
  `/usr/bin/ffmpeg`, `/usr/bin/ffprobe`, `/usr/bin/vdr` wird am
  `ProcessRunner` typisiert abgefragt. Die Import- und Shrink-Aufrufe
  einschließlich VDR-`--genindex` verwenden dieselbe Auswahl.
  Der Import-Fehlercode unterscheidet FFmpeg/Index nach Werkzeugtyp,
  unabhängig vom Installationspfad.
- Die bereits geschützte, backend-lokale
  `/etc/vdr-suite/backend-agent.conf` akzeptiert optional
  `MEDIA_FFMPEG_PATH`, `MEDIA_FFPROBE_PATH`, `MEDIA_VDR_PATH`
  als absolute normalisierte Programmdateipfade. Fehlende Schlüssel
  verändern nichts. Agent-Importvorschau, gepinnter Worker und
  HEVC-Shrink-Runner erhalten den gleichen bei Agent-Start geladenen
  Stand. Der Agent übergibt dem gepinnten Import-Handler nur diese
  drei Werte in dessen bewusst bereinigter `fexecve`-Umgebung,
  **nicht** in einem Browser- oder Core-Auftrag.
- Gezielte C++-Regressionen mit synthetischen alternativen
  Werkzeugpfaden, normalem Import, Shrink-Stage, unverändertem
  Fehlercode, Defaultwerten und ungültigen Konfigurationswerten
  sind ergänzt. **yaVDR-Quelltest vom 09.11.2026: PASS**:
  `MEDIA_IMPORT_CPP_TEST=PASS checks=43`,
  `MEDIA_SHRINK_CPP_TEST=PASS checks=27`,
  `test_backend_agent_client passed`,
  `OPENAPI_DRIFT=PASS paths=59 methods=73` und
  `MT_I3_2_BUILD=PASS` für Import, Shrink und Agent.
  Der isolierte Agent-Test benötigt seinen eigenen Command-State:
  behoben in `6f626d6964`; OpenAPI-Audit nachgeführt bis `62251721cb`.
  **Live-Installation und echte Agent-/Host-Abnahme stehen noch aus**.
  Keine Produktionsdatei wurde verändert; MLD/openSUSE weiterhin
  NOT_VERIFIED. Die bestehende
  Handler-Pinning-/Approval-Grenze, SSE und Import-Berechtigung bleiben
  unverändert.
- **Keine neue Einstellungspflicht:** yaVDR nutzt unverändert die
  bisherigen Pfade. Wenn eine andere Distribution die Werkzeuge
  anders installiert, setzt der lokale Administrator nur die
  vorhandene Agent-Konfiguration passend; es gibt keinen separaten
  Media-Tools-Daemon oder zweiten Importer.

**MT.ENC3.1 – Typisierte Encoding-Einstellungen (10.11.2026, installiert; reale Encode-Abnahme offen):**
- H.264-Import: optional backendlokale Werte `MEDIA_IMPORT_H264_CRF` (Default 23)
  und `MEDIA_IMPORT_H264_PRESET` (Default medium). H.265-Shrink separat:
  `MEDIA_SHRINK_H265_CRF` (**Default bleibt 27**) und
  `MEDIA_SHRINK_H265_PRESET` (medium). CRF 0–51 und nur fest bekannte
  libx264/libx265-Presets. Kein frei editierbares Browser-`argv`.
- Die vorhandene geschützte `backend-agent.conf` bleibt einzige Quelle.
  `MediaEncodingSettings` wird als typisierte Wertkopie an das vorhandene
  `MediaToolExecutables`-Profil gebunden. Der Agent lädt es beim Start,
  übergibt es via bereinigter Umgebung an den **gleichen gepinnten** Importer
  und direkt an den vorhandenen Shrink-Runner. Beide Worker frieren die
  Werte zu Auftragsbeginn ein; Konfigurationsänderungen erfordern für neue
  Jobs einen Agent-Neustart, laufende Jobs bleiben unverändert.
- **VDR-Kompatibilität:** Es entstehen ausschließlich H.264 Main / HEVC Main
  mit 8-Bit `yuv420p` in unverändertem MPEG-TS. Nach Umkodierung prüfen
  FFprobe-Readback (Main und 8-Bit), den tatsächlichen MPEG-TS-Container
  und die Audio-Streams (AAC, AC-3, E-AC-3, MP2 bzw. kein Ton) sowie die
  vorhandene VDR-`--genindex`-Prüfung vor Veröffentlichung. Quell-Remux
  bleibt wie zuvor. Das VDR-Ausgabeplugin/der TV-Client muss den Codec selbst
  unterstützen: ein gültiger Index alleine beweist noch keine Wiedergabe.
- Software bleibt Default. Die bestehenden Shrink-Hardwarepfade werden nur
  benutzt, wenn explizit gewählt. Für VAAPI ist der tatsächliche
  `MEDIA_VAAPI_RENDER_DEVICE`-Pfad pro Backend zu konfigurieren, nicht
  automatisch `/dev/dri/renderD128` zu unterstellen. QSV und NVENC sind
  ausschließlich bereits vorhandene, hostabhängige FFmpeg-Wege.
  Gleiche Zahlen bei Hardware-QP/Global-Quality/CQ und Software-CRF
  bedeuten **nicht** identische Bildqualität.
- **CRF 23 vs. CRF 27 bewusst geklärt:** Ein Wechsel des produktiven
  Software-HEVC-Standards auf 23 ist **nicht** freigegeben; 27 bleibt.
  Vor einer späteren Änderung braucht es reale Bild-/Größenvergleiche.
  Gezielte C++-Tests für Defaults, alternative Einstellungen, FFmpeg-
  Argumente, Hardware-Fallback-Snapshot, inkompatibles Profil und
  fehlerhafte MPEG-TS-/Audio-Readbacks sind ergänzt. Kein neuer Core-Auftrag,
  Importer, Poller, Berechtigungs-Gate oder Eingriff in produktive Aufnahmen.
- **Abnahme getrennt (yaVDR-Rueckmeldung 09.–10.11.2026):**
  **SOURCE=IMPLEMENTED** (bestehender Agent/Importer/Shrink-Runner und
  Ausgabe-Readback); **INSTALL=PASS** (MT_ENC31_INSTALL=PASS,
  Worker-Freigaben konsistent, Agent-Neustart erfolgt, kein Daemon-/VDR-Neustart,
  zentraler Cache erhalten); **CONFIG-NAMES=VERIFIED** (alle vier
  Encoder-Konfigurationsnamen in der laufenden Backend-Agent-Binaerdatei);
  **C++-REGRESSION=NOT_SEPARATELY_VERIFIED** (kein vollstaendiger neuer
  Testlauf aus den vorliegenden Hostdaten ableitbar);
  **REAL-H264-ENCODE=NOT_VERIFIED**, **REAL-HEVC-SHRINK=NOT_VERIFIED**,
  **PLAYBACK=NOT_VERIFIED**. VDR-Index allein beweist keine Wiedergabe.
- **MT.ENC3-Webfrontend-Vorbereitung:** Unter
  `Einstellungen → Media Tools → Encoding` fehlt weiterhin die
  verständliche backendbezogene Bedienoberfläche. Zunächst bestehenden
  Administrations-/Konfigurationsweg auf sichere Wiederverwendung prüfen;
  `backend-agent.conf` bleibt die einzige Wertquelle. Kein Browser-FFmpeg-
  `argv`, keine neue Konfigurationstabelle, kein automatischer Encode.
  Nur hardware- und clientseitig real abgenommene Optionen anbieten.
- Nächster technischer Slice nach erfolgreicher Hostabnahme: **MT.I3.3**
  Portabilitätsregression und tatsächliche MLD-/openSUSE-Hosttests.

**MT.I3.3 – distributionsübergreifende Regressionen (09.11.2026, Testcode):**
- Neuer Offline-Einstieg `make test-media-tools-i33-portability`, eingebunden
  in das bestehende `test-install-staging`; ausschließlich Python-
  Standardbibliothek und vorhandene Install-/Stage-Funktionen. Keine neue
  Installationsarchitektur, kein C++-Rebuild und keine Hostdienst-Änderung.
- Synthetische `ID=ubuntu`, `ID=mld`, `ID=opensuse` und
  `ID=opensuse-leap`: vorhandenen read-only Host-Profiler und den
  realen `install_media_tools.sh profile`-CLI-Einstieg vergleichen;
  Service-/VDR-ABI-/Nutzer-/Quellpfade bleiben ausdrücklich NOT_VERIFIED.
  Ungültige IDs und fremde `os-release`-Symlinks werden abgewiesen.
- Dieselbe bereits versiegelte MT.I4-Teststage auf vier **wegwerfbaren**
  simulierten Root-Dateisystemen installieren, mit bestehendem
  `media_tools_install_preflight.inspect` auf Byte-/Modus-Übereinstimmung
  prüfen und die idempotente Wiederholung ausführen. Agent-Konfiguration
  einschließlich alternativem FFmpeg-Pfad, Quellledger, systemd-Drop-in,
  backendgebundene Freigabe, SQLite, Quellfilm und VDR-Aufnahme müssen
  byteidentisch erhalten bleiben. Nur synthetische Root-Pfade, keine
  Paketverwaltung oder Dienste.
- Vorhandene, weiterhin relevante isolierte Testverträge (nicht verdoppelt):
  `test-addon-media-import`, `test-addon-media-shrink`,
  `test-addon-media-check`, `test-addon-media-check-api`,
  `test-addon-media-import-api-admission`,
  `test-media-tools-agent-dispatch`,
  `test-addon-media-import-ui`, sowie die bisherigen
  `test_install_media_tools_payload.py`-Rollback-/Pin-Fälle. Sie
  prüfen Teile der Encoder-, VDR-Readback-, API-, Agent-, Fehler- und
  Auftragsverträge. Ein echter Daemon-Neustart während eines sieben-
  teiligen Stapels, verlorene Antwort und Reconciliation müssen
  weiterhin separat live beziehungsweise mit gezielten Core-Fixtures
  abgenommen werden; synthetische OS-Dateien belegen das **nicht**.
- **yaVDR-Ausführung am 09.11.2026: PASS (5/5 isolierte Tests, 0.804 s)**,
  `make test-media-tools-i33-portability` bei HEAD
  `f3c2cb84be3c476844fa85b09d2b6b760c07f58b`.
  Eine zunächst fehlende Bindung der wiederverwendeten Fixture-Methode
  `refresh_seal` wurde mit `f3c2cb84b` korrigiert und der vollständige
  Test danach erfolgreich wiederholt. Der Test verwendete nur temporäre
  synthetische Roots; kein Hostdeploy, Dienstneustart, Medien- oder DB-Backup.
  Der ergänzende C++-/Frontend-/Core-Recovery-Testlauf steht noch aus.
  Die echte yaVDR-Abnahme der bisherigen Import-/EVENT5-Funktionen
  bleibt bestehen; eine MT.ENC3.1-Encoder-/TV-Wiedergabeabnahme ist
  davon ausdrücklich nicht abgedeckt.

| Distribution | Isolierte Testmatrix | Live-/Paket-/VDR-Abnahme für MT.I3.3 |
| --- | --- | --- |
| yaVDR/Ubuntu | **PASS**, einschließlich synthetischer `ID=ubuntu`-Fixture (09.11.2026) | 4/4 bisherige Import-Readbacks bestätigt; MT.ENC3.1-Encode/Wiedergabe und spezifischer I3.3-Live-Neustarttest offen |
| MLD 6.6 | Synthetischer `ID=mld`-Testfall **PASS auf yaVDR** | NOT_VERIFIED; reale Agent-Unit/Init, Paketbesitz, FFmpeg/VDR-ABI/Hardware erforderlich |
| openSUSE | Synthetische `ID=opensuse`/`opensuse-leap`-Testfälle **PASS auf yaVDR** | NOT_VERIFIED; reale Unit, Paketmanager, Dienstnutzer, FFmpeg/VDR/Hardware erforderlich |

**Status MT.I3.3: PASS für yaVDR einschließlich echtem Daemon-Neustart
und 4/4 VDR-bestätigten Sammelimporten; PARTIAL für die
distributionsübergreifende Gesamtmatrix (MLD/openSUSE Live NOT_VERIFIED).**
Synthetische Portabilitätsregression, Core-SQL-Recovery, native
C++-Agent-Wiederanlaufregression und Frontend-/Public-v1-Verträge jeweils
PASS auf yaVDR. Kein automatisches Übertragen der yaVDR-Pfade auf andere
Distributionen.

**MT.I3.3 – vorhandene Frontend-Verträge auf yaVDR bestätigt
(09.11.2026, SOURCE/FRONTEND PASS):** Nutzerseitig ausgeführt:
`make test-addon-media-import-ui` meldete
`A7.5 real web composition and media import lifecycle: PASS`;
`node web/frontend/tests/test_shared_browser_sse.js` meldete
`test_shared_browser_sse passed`;
`node web/frontend/tests/test_media_check_composition.js` meldete
`MEDIA_CHECK_API_V1_FRONTEND=PASS HEVC_STAGE_PUBLISH_CLIENT=PASS`.
Keine Dienste neu gestartet, keine produktiven Aufträge verändert.
Dies bestätigt Frontend-Kompositions- und SSE-Verträge, **keinen**
realen Core-/Agent-/VDR-Neustart.

**MT.I3.3 – Core/Agent-SQL-Regressionsnachtrag (09.11.2026, Testcode):**
- Neuer `make test-media-tools-i33-core-recovery`-Test liest die
  **tatsächliche** SQL-Auswahl aus `DaemonRuntime.cpp` und die beiden
  produktiven Unique-Index-Statements aus
  `BackendAgentMediaImportAssignment.cpp`; keine kopierte Scheduler-
  Implementierung oder neue Queue.
- Fünf isolierte SQLite-Verträge: sieben angenommene Folgen werden erst
  nach bestätigtem `succeeded` jeweils um eine Folge freigegeben;
  Datenbank schließen/neu öffnen erhält diese Zuordnung; Fehler,
  `outcome_unknown` und abgelaufene Fristen blockieren weitere
  Zuweisungen; voneinander unabhängige Actor-/Batch-Scope werden
  getrennt behandelt; ein verlorener POST-Response kann wegen des
  tatsächlichen Core-Idempotenz-Unique-Index nicht dieselbe Operation
  erneut reservieren; doppelte Import- oder Readback-Command-Zuordnungen
  verletzen die vorhandenen Agent-Unique-Indizes.
- **SOURCE/UNIT: PASS auf yaVDR (09.11.2026, 5/5 in 0.035 s)**;
  `make test-media-tools-i33-core-recovery` wurde vom Nutzer ausgeführt.
  Die SQL-Verträge verwenden synthetische Tabellen/Operationen und
  simulieren einen Datenbank-Wiederanlauf. Sie starten weder den
  tatsächlichen Daemon noch den Agent und bestätigen keine echten
  VDR-Aufnahmen. Ein vollständig durchlaufener Core-/Agent-
  Integrationstest samt echten Dienstneustarts und MLD-/openSUSE-
  Hostabnahmen bleibt separat offen.

**MT.I3.3 – native C++-Agentregression (09.11.2026, UNIT/NATIVE PASS
auf yaVDR):** `make test-media-tools-i33-agent-recovery` bündelt
bestehende native Canonical-Bridge-, Agent-Dispatch-, wirksame
Autorisierungs- und Backend-Agent-Runtime-/Wiederanlauf-Tests.
Der Testblock lief zunächst beim Autorisierungstest wegen fehlendem
`TMPDIR` ab; das Make-Ziel wurde mit `da80f36ecda2bd5564baa0e7333ce34609603e87`
korrigiert. Anschließend bestätigte die Nutzer-Shell
`test-media-tools-effective-authorization` und den gebauten, ausgeführten
`test_backend_agent_media_import_runtime` mit
`MT_I33_NATIVE_AGENT=PASS`. Auch Canonical-Bridge und Agent-Dispatch
waren zuvor erfolgreich. Der root-only Runtime-Test verwendet
einen separaten temporären `/run/vdr-a6-runtime-XXXXXX`-Baum und
`/bin/true` als nicht schreibenden Handler; darunter werden echte
C++-Agentzustände, wiederholte Receipts, persistiertes `starting`,
`outcome_unknown` und Idempotenz getestet.
**Keine produktiven Aufnahmen, keine Service- oder Daemon-Neustarts:**
Die tatsächliche yaVDR-Live-Recovery-Abnahme bleibt bewusst offen.

**MT.I3.3 – yaVDR-Live-Preflight (09.11.2026, READ_ONLY PASS):**
Der Nutzer hat bei laufendem `vdr-suite-daemon.service` und
`vdr-suite-backend-agent.service` den vorhandenen aktiven
`mutation_operations`-Bestand nur gelesen: `media.import` =
131 `succeeded`, 4 `failed_before_dispatch`, 2
`failed_verified`, 3 `outcome_unknown`; keine Operation im
Zustand `accepted`, `queued`, `dispatching` oder
`executed_unverified`. Die ausgewiesenen drei offenen Operationen
sind somit ausschließlich **ungeklärte Ergebnisse**, nicht der
Nachweis dreier laufender Importprozesse.
Es erfolgte kein Neustart und kein Datenbank-Backup.
**Altbestand abgeklärt:** Der anschließende read-only-Check ergab
für alle drei `outcome_unknown` Einzelimporte vom **29.10.2026**
(08:04, 08:48, 14:52 lokale Zeit), Backend `default`;
**keiner ist Teil eines Sammelimports**. Nutzerbestätigung: alte
Aufträge. Unverändert lassen; kein Blocker für MT.I3.3, kein Grund für
weitere Altfall-Diagnostik.
**REAL-VDR-RESTART PASS auf yaVDR (09.11.2026):** Neuer, während der
Verarbeitung angenommener Stapel
`mb1_ed6d4633eedef31354f94a62828a2ce5` überstand den echten
Daemon-Neustart (13:10:23 CET); erster Auftrag danach um 13:12:18
`succeeded`, zweiter um 13:12:20 `dispatching`; Agent-PID blieb
unverändert. Bei der anschließenden read-only-Datenbankabnahme waren
**alle vier `succeeded`**. Die drei alten `outcome_unknown`-Einzelaufträge
vom 29.10. blieben unverändert.

**MT.EVENT5 / MT.I3.3 – Anzeige-Fix für Importprotokoll-Titel
(09.11.2026, TEST / INSTALL / LIVE-SICHTPRÜFUNG PASS auf yaVDR):** Echter yaVDR-
Sammelimport `mb1_ed6d4633eedef31354f94a62828a2ce5` nach Daemon-
Restart 13:10:23 weitergelaufen: erster Auftrag 13:12:18 `succeeded`,
zweiter 13:12:20 `dispatching`; Agent-PID unverändert. In Home und
Aufnahmen sind fertige Medien korrekt angekommen. Spätere read-only-
Abnahme: **vier von vier `succeeded`**; Titel aus bestehenden
Metadatensnapshots: True Blood – Strange Love, Blut geleckt,
Sie gehört zu mir, Flucht aus dem Drachenhaus. Das Importprotokoll zeigte nach Quellenentfernung
und Browser-Neuladen kryptische `scan_...`-IDs statt Filmtiteln.
Optionaler Titel kommt nun aus dem **schon vorhandenen unveränderlichen
Metadatensnapshot der selben Core-Operation**, wird in der actor- und
backendgebundenen Public-v1-Leseantwort angezeigt und im Frontend
für Historie, Wiederaufnahme und Stapelzeilen verwendet. Kein neuer
TMDb-Lookup, keine zusätzliche Persistenz, Queue oder neue Berechtigung.
Ohne Snapshot: `Filmname nicht mehr verfügbar` statt einer Quell-ID.
**Quell-/API-Regression PASS:** `make test-addon-media-import-ui` und
`make test-phase69-public-operation-resource` ausgeführt.
**yaVDR INSTALL PASS:** Gezielter Daemon-Build mit
`VDR_SUITE_SERVER_VERSION=git-56cc0453a499`, inkrementeller
Root-eigener `.build/obj`-Cache beibehalten, Daemon-Binary und
`/usr/share/vdr-suite/web/frontend/modules/media-import.js`
installiert; Daemon neugestartet, Agent und VDR nicht neugestartet.
Kein Datenbank-Backup. Bestehender Installer `install_daemon.py`
konnte initial den alten Git-markerlosen Daemon nicht identifizieren
(`INSTALLED_SOURCE_ID_UNKNOWN`); einmaliger Legacy-Install mit
Rückfallkopie `/root/vdr-suite-titlefix.BEEos5XS` abgeschlossen:
`MT_TITLE_INSTALL=PASS`. Neues Binary trägt nun Git-Quellmarker.
**LIVE-SICHTPRÜFUNG PASS (09.11.2026):** Screenshot des neugestarteten Webfrontends zeigt im Importprotokoll für True Blood S01E01–S01E04 die korrekten Serien-/Folgentitel und jeweils `VDR-Aufnahme bestätigt`; auch Foundation S03E03–S03E10 wird mit lesbaren Titeln bestätigt angezeigt. Die vorherigen `scan_...`-Quellkennungen erscheinen dort nicht mehr als Filmtitel. Damit ist der Titel-Anzeigefix nach Reload abgenommen.

**Abnahmegrenze:** Eine erfolgreiche Build-/Mockprüfung auf Ubuntu macht MLD
oder openSUSE nicht automatisch unterstützt. MT.EVENT4/MT.ENC4 bleiben für
echte lange Aufträge, Browser-/Agent-Reconnect und VDR-Readback zuständig.
Kein Installations-, Dienst- oder Medienänderungsautomatismus durch
Portabilitätsprüfung.

## MT.ENC – Hintergrund-Umkodierung, Wiederanmeldung und Encoder-Einstellungen (31.10.2026, geplant)

**Produktanforderung:** Ein bewusst gestarteter VDR-Encode läuft serverseitig weiter,
wenn der Benutzer den Webreiter schließt, sich abmeldet oder später auf einem
anderen Gerät anmeldet. Die Suite zeigt dann denselben Auftrag mit Fortschritt
und tatsächlichem Ergebnis wieder an; sie darf **keinen zweiten Encode**
starten. Bei Abbruch, Agent-/Host-Neustart oder ungewissem Ergebnis ist der
dauerhafte Istzustand korrekt anzuzeigen statt automatisch erneut zu starten.
Das ist ein Akzeptanzziel, **noch keine zugesicherte Produktfunktion**.

**Iststand (Quellprüfung 31.10.2026):**

- `Auf dem VDR umkodieren` startet FFmpeg serverseitig über die vorhandene
  Core-Operation und den Backend-Agent-Import-Executor. Ein bloßer
  Browser-/Tabwechsel stoppt FFmpeg nicht unmittelbar.
- Der Agent begrenzt den laufenden Import-Handler in
  `PinnedMediaImportAgentExecutor.cpp` derzeit auf **maximal 600 Sekunden**
  und beendet ihn nach Fristablauf. Längere Filme sind deshalb nicht
  zuverlässig als Hintergrund-Encode nutzbar.
- `GET /api/v1/operations/{operationId}` liest einen dauerhaften
  Operationsstatus. Eine öffentliche Operationen-Liste existiert bislang
  nicht; `web/frontend/modules/media-import.js` verwaltet aktive Aufträge
  im Speicher des Browsers und löscht sie bei `clearSession()`.
  Eine neue Anmeldung entdeckt daher einen vorher begonnenen Encode
  **nicht automatisch**.
- `addons/media-tools/src/MediaImport.cpp` verwendet beim lokalen Encode
  fest `libx264`, `CRF 23`, `preset medium`, `threads 2`; keine vom
  Benutzer speicherbaren Media-Import-Encoderwerte.
- `Auf PC umkodieren` führt **keine** Kodierung in VDR-Suite aus: Diese
  Aktion parkt die Quelldatei als `.pc_encode`. Ein externer PC verarbeitet
  sie eigenständig und legt die neue Datei für den normalen Import zurück.

### MT.ENC1 – Langer serverseitiger Encode und robuster Abschluss

Den **bestehenden** Import/FFmpeg-Executor verwenden, keine zweite Queue und
keinen separaten HandBrake-Worker hinzufügen. Das feste 600-s-Limit durch
eine zu langen Filmdateien passende, klar definierte Encode-Laufzeit ersetzen;
dauerhafte Core-Operation, Fortschritt, echte FFmpeg-Fehler und Endzustände
trennen. Kein pauschales Dienstneustarten oder automatisches erneutes
Ausführen eines möglicherweise schon begonnenen Jobs. Die laufende Verarbeitung
bleibt vom Browser unabhängig. Akzeptanz: Ein realer Encode über 10 Minuten
läuft nach Tabwechsel/Logout weiter und endet nach VDR-Readback korrekt.

**MT.ENC1 Quellcode-Stand (01.11.2026, reale Encode-Abnahme offen):**
Die bestehende Core-Operation, Agent-Zuweisung, der gepinnte Import-Handler
und der FFmpeg-Läufer haben bei ausdrücklich gewähltem lokalen Video-Encode
nun eine 24-Stunden-Obergrenze statt der uneinheitlichen 9-/10-/15-Minuten-
Grenzen. Gewöhnliche Remux-/Marker-Aufträge behalten ihre kurzen Fristen.
Während eines laufenden Encodes erneuert derselbe Agent die existierende
Heartbeat-Lease, ohne den dauerhaften Importbefehl nochmals zu pollen oder
auszuführen. Ein Transportfehler beendet FFmpeg nicht automatisch; der
nächste Renewal-Versuch bleibt möglich.

Der vorhandene Import-Worker übermittelt feste Fehlerkategorien wie
`ffmpeg_failed`, `ffmpeg_timeout`, `vdr_index_failed` oder
`worker_signaled` über das vorhandene Agent-Ergebnis an dieselbe Core-Operation.
`GET /api/v1/operations/{operationId}` projiziert diese nur lesend und
akteursgebunden als optionales `failureCode`; Media Import zeigt daraus einen
verständlichen Grund. Nach einem Agent-Neustart mitten in der Verarbeitung
ist `execution_interrupted` ausdrücklich **keine** Behauptung, dass ein
System-Shutdown oder Absturz sicher nachgewiesen wurde. Keine automatische
Wiederaufnahme oder Wiederholung; ausstehendes Recording-Readback bleibt
maßgeblich. Fortschritt bleibt zunächst phasenbasiert und ohne erfundene
Prozent-/ETA-Angaben.

**Noch offen:** Quellcode-/Integrationstests und echte Hardwareabnahme mit
einem ausdrücklich freigegebenen >10-Minuten-Encode. Wiederentdeckung nach
Anmeldung ist weiterhin MT.ENC2; echte stufen-/bytegenaue Fortschrittswerte
sind noch nicht vorhanden.

### MT.ENC2 – Auftrag nach Wiederanmeldung wiederfinden

Bestehende dauerhafte Core-Operationen als einzige Auftragsidentität nutzen.
Für denselben berechtigten Benutzer/Backend die offenen und letzten
Media-Import-Operationen **lesend** wieder auffindbar machen; die
bestehende Operation-ID und der Core-Status sind maßgeblich, nicht
`localStorage` oder ein neuer Browser-Jobstore. Anzeige in
`Media Import → Laufende / letzte Aufträge` mit Film, Encoding-Modus,
tatsächlichem Fortschritt (soweit vom Server bekannt), Startzeit,
Endzustand und eindeutigem Fehler. Wiederanmeldung, neues Gerät und
Agent-Transportfehler dürfen keinen zweiten POST auslösen. Bei
`outcome_unknown` den ungewissen Zustand wahrheitsgemäß anzeigen.

### MT.ENC3 – Encoding-Einstellungen mit Rectools-Defaults

Als noch offene MT.ENC3-Oberfläche in `Einstellungen → Media Tools → Encoding`
backendbezogene Einstellungen anbieten, standardmäßig **ohne Einrichtungszwang**.
Die produktive Speicherung bleibt bis dahin ausschließlich in der bestehenden
backendlokalen Agent-Konfiguration; keine zweite Settings-Verwaltung.
Werte werden bei **Auftragsstart** übernommen; spätere Änderungen wirken
nicht rückwirkend auf laufende Aufträge:

| Einstellung | VDR-Suite-Produktstandard (Rectools als Referenz) | Einordnung |
| --- | --- | --- |
| Import-Video | H.264 / libx264 | lokale, inkompatible Videoquelle |
| H.264-Qualität | CRF **23** | niedriger = bessere Qualität/größere Datei |
| H.264-Preset | **medium** | Geschwindigkeit / Kompression |
| H.265 für Shrink | CRF **27**, Preset **medium** | Produktstandard; Rectools-Referenz 23 bewusst nicht übernommen |
| Hardwarebeschleunigung | **none** (CPU) | später optional VAAPI, QSV, NVENC abhängig von realer Verfügbarkeit |
| Audio-Normalisierung | **aus** | nicht ungefragt Tonspuren verändern |
| Bestätigung vor Encode | **ein** | keine heimliche Videoneukodierung |

Die bestehenden `-threads 2` sind ein fest implementierter Importwert,
kein bereits vorhandener Rectools-Default. Bewusst nur unterstützte
Parameter als Auswahl anbieten; keine freien FFmpeg-Befehlszeilen.
Backend/Worker führen die gespeicherten Werte aus, nicht der Browser.
Akzeptanz: Standardprofil direkt nutzbar; geänderte Werte werden beim
nächsten Auftrag real an FFmpeg übergeben und nach erneutem Login korrekt
angezeigt. PC-`.pc_encode` bleibt externe Übergabe, kein lokaler Encode.

### MT.ENC4 – Echte yaVDR-Abnahme

**Nachtrag 11.11.2026 (nach den unten stehenden früheren Fehlbefunden):** Der Maintainer hat das aus fünf Modulen bestehende Produktions-Playback-Bundle wiederhergestellt. `BUNDLE_SYNTAX=PASS`, `HTTP_READBACK=PASS` und `PAGE_BUNDLE=PASS` wurden gemeldet; danach bestätigte er den praktischen Wiedergabe-/Seek-Test mit `Pass!`. Damit ist der unten beschriebene unvollständige Live-Bundle-Stand überholt. Die in diesem Abschnitt belegten H.264-/HEVC-Import-/Android-Tests bleiben erhalten. Samsung-/VIDAA-spezifische Wiedergabe ist dadurch nicht automatisch getestet.


**Host-Bestandsaufnahme 10.11.2026 (read-only; keine neue Encode-Abnahme):**
- Checkout /home/yavdr/vdr-suite auf main, HEAD == origin/main ==
  efd0f6769250cb0b75c15911cc5ecd0bb833f2d8. Die drei unversionierten
  Dateien cleanup-branches.py, x.recordingStatus und { bleiben unberuehrt.
- VDR 2.8.3 laeuft. `vdrctl status` bestaetigt softhddevice=disabled;
  das Ausgabeplugin nicht ungefragt aktivieren. Eine reale Wiedergabe
  muss spaeter mit einem tatsaechlich genutzten VDR-Suite-Client erfolgen.
- FFmpeg/FFprobe 8.0.1 enthalten libx264 und libx265 sowie angezeigte
  VAAPI-/QSV-Encoder. GPU: Intel UHD Graphics 605. vainfo fehlt.
  Vorhandensein eines Encoders belegt keine funktionierende Hardware-Kodierung.
- Videowurzel: /srv/vdr/video -> /srv/vdr/video.00.
  Kein Unterschied zwischen User-Checkout und origin/main beim Readback.
- Die vier Encoder-Schluessel erschienen nicht im lesenden grep der
  angegebenen Agent-Konfigurationsdatei; gueltige Defaults sind damit
  weder nachgewiesen noch widerlegt.
- True Blood S01E12 wurde erfolgreich DIREKT importiert. VDR-Sichtbarkeit,
  H.264 High/yuv420p/AC-3 in MPEG-TS, FFprobe und nativer Index sind
  ein Direktimport-Beleg, kein Encoding-Beleg.
- **Abnahmegrenzen:** Die unten dokumentierten synthetischen Host- und
  Worker-Tests belegen reale H.264-/HEVC-Umkodierung und native Indizes.
  Offen bleiben die tatsächliche Core-/Agent-Produktoperation,
  Client-Bild/Ton/Seek, Langläufer und Bestätigung geänderter
  Agent-Konfigurationswerte. Produktive Originale bleiben unangetastet.
- **MT.ENC4 H.264 Host-Smoke PASS (10.11.2026, rückgemeldete Shell-Ausgabe):**
  Künstliche achtsekündige MPEG-4-/AAC-Datei unter
  `/var/tmp/mt-enc4-h264.AleXVV/source.mkv` mit dem installierten FFmpeg
  real nach H.264 Main / yuv420p / AAC / MPEG-TS umkodiert.
  FFprobe-Readback PASS; `vdr --genindex` erzeugte einen nichtleeren
  nativen Index (ca. 1,6 KB) zur Testaufnahme mit ca. 985 KB TS.
  `MT_ENC4_H264_HOST_SMOKE=PASS` und `SOURCE_UNVERAENDERT=PASS`.
  Dieser isolierte Hosttest ist **kein** Media-Tools-Worker-/Core-/Agent-
  End-to-End-Test und **kein** Client-Wiedergabenachweis.
  Der erste Versuch lief durch einen zu streng formulierten
  FFprobe-Shell-Stringvergleich vor dem Indexschritt aus; der unmittelbar
  folgende JSON-Readback und native Indexlauf bestätigten den Host-Smoke.
- **MT.ENC4 isolierter Media-Tools-Worker PASS (10.11.2026, yaVDR):**
  Der vorhandene C++-Importer wurde auf `main` mit `addon-media-import`
  gebaut. `plan` erkannte die künstliche MPEG-4-Quelle (1.876.169 Byte)
  als umkodierungsbedürftig. Die explizit ausgelöste lokale H.264-
  Umkodierung (`CRF=23`, `preset=medium`) unter der ausschließlich
  synthetischen Wurzel `/var/tmp/mt-enc4-worker.yDZ7gF` meldete
  `IMPORT_STATUS=COMMITTED`; FFprobe-Readback der veröffentlichten
  `00001.ts`: H.264 Main/yuv420p, AAC, MPEG-TS, Dauer 8,04 s.
  Native `index` (~1,6 KB) und `info` (101 B) vorhanden;
  Quelle mit Original bytegleich; keine produktiven Aufnahmen verändert.
  `MT_ENC4_ISOLATED_WORKER=PASS`, `WORKER_VDR_INDEX=PASS`.
  Die Ursache der ersten Readback-Pause war der von der CLI ausgegebene
  quotierte `RECORDING`-Pfad; der Folgetest entfernte die Quotes und
  bestätigte die fertige Aufnahme **ohne erneute Umkodierung**.
  Die CLI meldet korrekt `SUITE_RECONCILIATION=NOT_CONNECTED`:
  damit sind Core-/Agent-End-to-End, SSE und Playback **nicht** abgenommen.
- **MT.ENC4 Core-/Agent-Vertragstests PASS (10.11.2026, yaVDR):**
  `test-addon-media-import` bestand 47 C++-Prüfungen;
  `test-addon-media-import-api-admission`,
  `test-media-tools-canonical-agent-bridge` und
  `test-media-tools-agent-dispatch` bestanden ebenfalls auf
  `33c41f9f5b788680f9cd09021f3aa16c8e26ba06`.
  `MT_ENC4_CORE_AGENT_CONTRACTS=PASS`; kein Dienstneustart,
  kein Datenbank-Backup und kein Eingriff in produktive Aufnahmen.
  **REAL_CORE_AGENT_IMPORT=NOT_TESTED**,
  **TV_PLAYBACK=NOT_TESTED**, **HEVC_SHRINK=NOT_TESTED**.
- **MT.ENC4 HEVC-Shrink-Stage PASS (10.11.2026, yaVDR):**
  Bestehender C++-Shrink-Worker gegen künstliche MPEG-2-/MP2-VDR-Aufnahme
  unter `/var/tmp/mt-enc4-hevc.NRDoOI` ausgeführt, ausschließlich
  `plan` und `stage`; keine Veröffentlichung. `libx265` mit
  produktivem CRF-27-Default, Preset medium, HEVC Main / MPEG-TS,
  20,00 s und nativer Index geprüft. Original 10.328.344 Byte,
  Candidate 2.095.636 Byte (79,7 % Ersparnis bei diesem Testmuster).
  Original-TS, Index und info bytegleich geblieben.
  `MT_ENC4_HEVC_WORKER_STAGE=PASS`, `HEVC_PUBLICATION=NEIN`,
  `REAL_CORE_AGENT_IMPORT=NOT_TESTED`, `TV_PLAYBACK=NOT_TESTED`.
- **MT.ENC4 HEVC-Publish PASS (10.11.2026, yaVDR):**
  Derselbe zuvor geprüfte Kandidat wurde ohne erneute Umkodierung mit
  `vdr-suite-media-shrink publish` ausschließlich unter
  `/var/tmp/mt-enc4-hevc.NRDoOI/video/MT-ENC4-Test_HEVC/` veröffentlicht.
  `MT_ENC4_HEVC_ISOLATED_PUBLISH=PASS`, unabhängiger FFprobe-
  Readback `HEVC_PUBLISHED_READBACK=PASS`, 20,00 s, TS rund 2,0 MiB,
  nativer Index 4,0 KiB, `info` 68 B.
  Originalaufnahme und Originalindex per Hash vor/nach der
  Veröffentlichung unverändert, `HEVC_PUBLICATION` nur im Testbaum.
  Die Ausgabe `SUITE_RECONCILIATION=REQUIRED` ist ein explizit
  noch offener Core-Aufnahmeabgleich und kein Erfolgsbeleg für Core.
- **MT.ENC4 HEVC-Core-/Agent-Tests PASS (10.11.2026, yaVDR):**
  `test-addon-media-shrink-core-review`: PASS (inklusive P7-Backup-Testfixture);
  `test-addon-media-shrink-core-admission`: PASS;
  `test-addon-media-shrink-agent-executor`: PASS
  (`STAGE_CRASH_REPLAY_FENCED=YES`, `RECONCILIATION_REQUIRED=YES`,
  `ORIGINAL_PRESERVED=YES`);
  `test-addon-media-shrink-command-fence`: PASS;
  `test-addon-media-shrink-mutation-api`: PASS
  (`BACKEND_SCOPE=PASS`, `OPERATION_READ=PASS`, `FAIL_CLOSED=YES`).
  Zusammen `MT_ENC4_HEVC_CORE_AGENT_CONTRACTS=PASS` auf
  `3c0d6150791665ebdaf927e1860f667a8f0d5c8f`.
  Vorhandener `vdr-suite-daemon` (PID 1891) und
  `vdr-suite-backend-agent` (PID 4794) sind `active`.
  In `/etc/vdr-suite/backend-agent.conf` sind keine expliziten
  Encoderoptionen gesetzt: Default H.264 CRF 23/medium,
  HEVC CRF 27/medium. Die Quellliste enthält
  `local-media-import` von `/srv/video/Filme` nach
  `/srv/vdr/video.00`. Keine Produktionsdatei verändert.
- **MT.ENC4 reales Importmedium READY (11.11.2026, yaVDR):**
  Eine Kopie der bereits synthetisch geprüften MPEG-4-Testquelle wurde
  in die installierte Importwurzel als
  `/srv/video/Filme/MT-ENC4-CoreAgent-Test-20261111-060450.mkv`
  eingebracht (ca. 1,8 MiB). `cmp` gegen die Testquelle PASS;
  `MT_ENC4_CORE_AGENT_SOURCE=READY`. Dies ist nur Quellbereitstellung;
  **IMPORT_AUSGEFUEHRT=NEIN** und VDR-Aufnahmen noch unverändert.
- **MT.ENC4 echter Core-/Agent-Import: UI/Recording PASS (11.11.2026):**
  Screenshots des laufenden yaVDR-Webfrontends zeigen den ausdrücklich
  bestätigten lokalen MPEG-4→H.264-Import für
  `MT-ENC4-CoreAgent-Test-20261111-060450`. Zunächst
  `Import läuft`, danach `outcome_unknown`, anschließend
  **`Import abgeschlossen ✓` und `Die Aufnahme ist im VDR vorhanden`**.
  Derselbe Testtitel erscheint unmittelbar unter `Neu aufgenommen`
  sowie in Recording-Detail/Player mit sichtbarem FFmpeg-Testbild
  und Wiedergabedauer etwa 8 Sekunden. Das ist ein realer
  Core-/Agent-/VDR-Inventar-Erfolg, kein bloßer Worker-Exit-Code.
  Der Codepfad `MediaImportOperationFence::reconcileRecordingCache`
  erlaubt ausdrücklich `outcome_unknown`→`succeeded` erst nach
  frischem, passend gefencetem nativen VDR-Inventar; der beobachtete
  Zwischenzustand belegt daher **nicht automatisch einen Fehler**.
  Ob UI-Erfolg unmittelbar per SSE oder per `Vorgang prüfen` kam,
  beweisen Screenshots allein nicht. Auch Audio und Seek nicht
  unabhängig bestätigt; Playeranzeige `0 min` bei acht Sekunden
  war dagegen ein reproduzierbarer Anzeige-Rundungsfehler.
- **MT.ENC4 H.264 produktives Core-/Agent-Readback PASS (11.11.2026):**
  Die über das Webfrontend importierte Testaufnahme
  `/srv/vdr/video.00/MT-ENC4-CoreAgent-Test-20261111-060450/2026-11-11.06.07.1-0.rec`
  enthält `00001.ts` (1.014.072 Byte), `index` (ca. 1,6 KiB),
  `info` (123 Byte), jeweils Eigentümer `vdr`. FFprobe:
  H.264 Main, yuv420p, AAC-LC, MPEG-TS, **8,04 Sekunden**.
  Vollständiger FFmpeg-Video-/Audio-Decode `VIDEO_AUDIO_DECODE=PASS`.
  Das aktive Daemon-Journal belegt um **06:08:13**:
  `Media Import readback verified: backend=default, operations=1`.
  `MT_ENC4_REAL_MEDIA_CHECK=PASS`; kein VDR-/Agent-/Daemon-Neustart
  und keine bestehenden Medien verändert. Damit ist der **reale
  H.264-Webfrontend→Public-v1→Core→Agent→Worker→VDR-Readback-Pfad
  abgenommen**. Die vorherige `outcome_unknown`-Phase wurde
  für genau eine Operation durch frischen nativen Aufnahme-Readback
  bestätigt; kein zusätzlicher Import nötig.
- **MT.ENC4 Frontend-Fixes installiert (11.11.2026, yaVDR):**
  `recordings2-shared.js` formatiert Aufnahmen unter 60 s in
  Sekunden statt `0 min`; Media-Import erklärt `outcome_unknown`
  als möglichen späteren VDR-Readback desselben Auftrags ohne Retry.
  Beide fokussierten Node-Tests bestanden
  (`recordings2 modular runtime ok`,
  `A7.5 real web composition and media import lifecycle: PASS`).
  Beide JS-Dateien wurden ausschließlich unter der tatsächlich
  konfigurierten `VDR_SUITE_FRONTEND_ROOT=/usr/share/vdr-suite/web/frontend`
  aktualisiert. **`MT_ENC4_FRONTEND_INSTALL=PASS`,
  `HTTP_READBACK=PASS` für beide Dateien** auf dem direkten
  Daemon-Port 18080; kein Build, kein Dienstneustart.
  Ein Browser-Neuladen/visuelles Nachmessen der `8 s`-Anzeige
  wurde noch nicht gesondert gemeldet.
- **Anzeige `Größe unbekannt` eingegrenzt, nicht als Media-Fehler werten:**
  Die tatsächliche TS hat 1.014.072 Byte, also **weniger als
  1 MiB**. Der vorhandene RESTfulAPI-Mapper übernimmt ausschließlich
  das ganzzahlige `filesize_mb` in `sizeMb`; der Renderer
  bezeichnet `sizeMb <= 0` als unbekannt. Dies ist mit auf null
  quantisierter Größe der künstlich kurzen Aufnahme vereinbar,
  **aber nicht der Nachweis**, dass der installierte RESTfulAPI-
  Provider tatsächlich null geliefert hat. Kein neuer Datei-Scan,
  Größe nicht raten und keine produktive Backend-Änderung.
- **MT.ENC4 HEVC-Direktimport im echten Suite-Produktpfad PASS
  (11.11.2026, yaVDR):** Die bereits isoliert erzeugte synthetische
  HEVC-TS wurde unverändert als
  `/srv/video/Filme/MT-ENC4-HEVC-Playback.ts` bereitgestellt
  (`HEVC_PLAYBACK_SOURCE=READY`). Der anschließend durch Media Import
  angelegte VDR-Zielordner
  `/srv/vdr/video.00/MT-ENC4-HEVC-Playback/2026-11-11.06.39.1-0.rec`
  enthält TS, nativen `index` und `info`.
  Unabhängiger FFprobe-Readback: **HEVC Main, yuv420p,
  MP2-Audio, MPEG-TS, 20,000 s, 2.095.636 Byte**.
  Der vollständige FFmpeg-Decode von Video und Audio bestand
  (`HEVC_FULL_DECODE=PASS`, `HEVC_NATIVE_INDEX=PASS`).
  Wichtigster Core-Beleg ist das aktive Daemon-Journal um
  **06:39:24 Uhr**:
  `Media Import readback verified: backend=default, operations=1`.
  Damit ist auch **HEVC-Direktimport → Core/Agent → nativer
  VDR-Aufnahmeabgleich** im realen yaVDR erfolgreich getestet.
  Keine neue HEVC-Umkodierung und kein neuer Import nötig.
- **MT.ENC4 Android-Webbrowser: HEVC-Aufnahme mit Bild/Ton
  und vollständiger ruckelfreier Wiedergabe PASS (11.11.2026):**
  Maintainer-Screenshot der echten VDR-Suite-Recording-Wiedergabe
  zeigt das synthetische Testbild, laufende Wiedergabe bei ca. 3/20 s,
  20-s-Zeitleiste sowie Pause/Stop/±10/±60-Steuerungen.
  Maintainer meldet ausdrücklich **Ton hörbar** und **Video läuft
  bis zum Ende ohne Ruckeln**. Dies ist ein positiver realer
  Android-Webplayer-Bild-/Ton-/Kontinuitätstest der aus einer
  **HEVC/MP2-VDR-Quelle** importierten Aufnahme.
  **Nicht als nativer Browser-HEVC- oder MP2-Decoder-Nachweis
  fehlinterpretieren:** `web/frontend/recordings2-playback.js`
  deklariert H.264/AAC-Clientcodecs, und
  `MediaPresentationSelector` kann für unpassende Quellcodecs
  H.264/AAC-Adaptation verwenden. Das auf dem Host tatsächlich
  gewählte Session-Profil/Transcoding wurde nicht separat gemessen.
  Der Screenshot zeigt die Seek-Steuerung, aber keine
  **bestätigte erfolgreiche Seek-Aktion**.
- **MT.ENC4 Android-HEVC-Seek-Regressionsbefund (11.11.2026,
  Screenshot 10:10 Uhr):** Bei pausierter, sichtbar gepufferter
  20-s-Wiedergabe scheitert `+10`/Seek ausdrücklich mit
  `Seek fehlgeschlagen: media_session_not_active`;
  Playerzeit (~0:02) und Suite-Timeline (~0:05/0:20)
  sind währenddessen nicht gleich. Kein Medienimport- oder Codecfehler.
  Produktcode-Ursache: `RecordingMediaSessionRuntime::reapInactive`
  beendet kontinuierliche Sessions nach Worker-EOF mit
  `recording_stream_worker_exited`. Bei sehr kurzen Aufnahmen
  können die gesamten fMP4-Bytes bereits im Browserpuffer liegen,
  während `RecordingMediaSessionController::seekSession` die
  inzwischen `ended`-Session mit `media_session_not_active`
  zurückweist. Der genaue terminal_reason im installierten Host
  wurde nicht unabhängig aus der DB gelesen; der Codepfad erklärt
  den beobachteten Fehler.
- **Gezielte Seek-Reparatur auf `main`, Host-Abnahme offen
  (11.11.2026):** `web/frontend/api/session-frontend-sync.js`
  behandelt ausschließlich den expliziten Seek-Fehler
  `media_session_not_active` durch Wechsel zum vorhandenen
  HLS-Kompatibilitätsowner mit **neuer, autorisierter Session
  am ursprünglich gewünschten absoluten Sekundenwert**.
  Pausierter Zustand bleibt durch `autoPlay:false` erhalten;
  sonstige Seek-Fehler bleiben unverändert sichtbar.
  Keine Änderung an Server-Reaper, Aufnahme, Encoder, Source-Bytes
  oder Berechtigungen; kein automatischer Neuimport.
  `test_phase65d2_recording_playback_controls.js` enthält
  kurze 20-s-Session-EOF-Regression und Negativfall bei anderem
  Fehlercode. **Host-Abnahme der fokussierten Node-Regressionen
  PASS (11.11.2026):** `test_phase65d2_recording_playback_controls.js`,
  `test_phase65d2_recording_fallback_restart_seek.js` und
  `test_phase65d_timeline_continuity_progressive.js` bestanden
  auf yaVDR nach Fast-forward von `main`.
  **WICHTIGE KORREKTUR DER INSTALLATIONSABNAHME:** Der manuelle
  Host-Befehl installierte irrtümlich nur die Quell-Datei
  `web/frontend/api/session-frontend-sync.js` direkt über die
  produktive **zusammengesetzte** Datei
  `/usr/share/vdr-suite/web/frontend/api/session-frontend-sync.js`.
  `HTTP_READBACK=PASS` bestätigte deshalb lediglich diese
  **unvollständige** Dateikopie, nicht das produktive Bundle!
  `mk/live-remote.mk` erzeugt dieses Bundle in exakter Reihenfolge
  aus `session-frontend-sync.js`, `live-replacement-cleanup.js`,
  `recording-fallback-restart-seek.js`,
  `recording-fallback-controls.js`,
  `recording-time-input-mask.js`. In der verkürzten
  Produktion fehlte insbesondere der HLS-Owner mit
  `startAtAbsolute`; der neue Fehler beim Android-Seek führte
  am 11.11. um 10:30 Uhr zu `Bereit zum Starten` und
  `Aufnahme abspielen` statt Fortsetzung.
  **`MT_ENC4_SEEK_TESTS=PASS` gilt für die Quelldateien,
  produktiver Browser-Seek = FAIL / nicht abgenommen.**
  Die manuell gelieferte Datei muss gemäß bestehender
  `install-live-remote-frontend`-Rezeptur aus genau diesen
  fünf Dateien atomar wiederhergestellt, syntaktisch geprüft,
  mit direktem HTTP- und Seiten-Bundle-Readback verifiziert und
  erst dann am Android-Client erneut getestet werden.
  Kein vollständiges Install-/Build-/Service-Update notwendig.
- **Weiterhin offen:** Client-Seek unter realer Last nach obigem
  Frontend-Fix, Wiedergabe auf Samsung-Tizen/Hisense-VIDAA mit
  Bild, Ton, Seek und jeweils tatsächlichem Client-Profil;
  H.264-Browser-Audio-/Seek-Bestätigung, SSE-Ereigniszustellung,
  Langläufer und Agent-Unterbrechungen. Beide
  Core-Reconciliations (H.264 06:08:13, HEVC 06:39:24) sind
  bestätigt. Keine Encoder-Smokes oder Imports wiederholen;
  nur bestehende Aufnahmen im jeweiligen Client abspielen.

Inkrementell bauen, vorhandene Runtime-Artefakte wiederverwenden,
kein neues Branch/PR, keine DB-Backups oder unnötigen Aktivierungsprüfungen.
Tests für >600-s-Verarbeitung, Browser schließen, Ab-/Anmeldung, neues
Gerät, Agent-Neustart/unklares Ergebnis, Einstellungen und Defaults,
korrektes Recording-Readback sowie nachfolgende Quellenaktualisierung.
Auf dem echten yaVDR mit einem **eigens bestätigten** Testmedium abnehmen;
bereits importierte Andor-Aufnahmen und Quellen nicht ungefragt verändern.

### Phasen und harte Abnahmekriterien — in dieser Reihenfolge

| Phase | Auftrag / betroffene Bestandsmodule | Fertig erst, wenn … |
| --- | --- | --- |
| **MT.REC0 — REAL-VDR-Ursachen beheben** | Bestehenden Stand in PR #556 und auf yaVDR unterscheiden; Scan-Unterordner, stale Rescan-Anzeige, 24 normal + 6 markiert, S02E12-Andor-Vorschau/HTTP-/Agent-Antwort, irreführendes `unavailable`, schiefen Modulreiter, falsch aktive/gesperrte Buttons gezielt reproduzieren. In der Konsole wurde vorher eine **andere** Folge `S01E01.skipped.mkv` gefunden; das erklärt **nicht** die ausgewählte **S02E12** und darf nicht als deren Ursache ausgegeben werden. Tests für den tatsächlich gewählten Quell-ID-/Dateibeleg. | Reproduzierbare Ursache jeder Blockade, klare Browsermeldung, funktionierende Einzelvorschau eines normalen Kandidaten, Markierungen korrekt klassifiziert; fokussierte JS-/API-/Agent-Tests, kein pauschaler Build/Restart. |
| **MT.REC1 — Rectools-Status + Auto-Dublette** | Marker aus Rectools übernehmen; bestehende `LocalMediaImportInventory`, `MediaImportDecisionMarker`, Core-Operation und Agent-Readback nutzen. Eindeutige Dubletten automatisch als `.duplicate` markieren; **auf ausdrückliche Nutzeranforderung vom 28.10.2026 zusätzlich einen separaten Löschbutton ausschließlich für bestätigte `.duplicate`-Quellmedien anbieten**. Keine automatische Löschung. | `skipped/pc_encode/duplicate` korrekt; automatische Markierung ohne erneuten Import; Löschaktion mit Bestätigung, Source-ID-/Inode-/Größen-/mtime-Abgleich und erneuter Prüfung der vorhandenen vollständigen VDR-Aufnahme; TS-/Index-/VDR-Original erhalten; echter Hosttest nur an einer ausdrücklich freigegebenen Wegwerf-Dublette. |
| **MT.REC2 — Automatische Codec-Verzweigung** | `inspectImportPreview`/`ffprobe`/bestehende Codecentscheidung für jeden ausgewählten Film; ohne Umkodierung Direktimport, bei Umkodierungsbedarf ausschließlich VDR- oder externe-PC-/HandBrake-Bearbeitung; bestehende Marker `.pc_encode`; keine neue PC-Ausführungsengine. | H.264/HEVC/MPEG-2 ohne Encode-Wahl, prüfbedürftiger Codec mit klarer Wahl und verifizierbarer Verarbeitungsfolge; niemals heimlich reencodieren. |
| **MT.REC3 — Mockup-Layout** | `web/frontend/modules/media-import.js`, vorhandene Styles/Navigation und Tests gezielt auf beschriebenes 3-Spalten-Dashboard umstellen; echte Bibliothek/Episoden/Vorschau, Badges und passende Aktionen, volle Desktop-Breite, responsive. | Screenshot-/Browservergleich mit Mockup, Navigation am richtigen Ort, kein Leerraum/Formularfriedhof, bedienbar mit Maus/Tastatur und auf schmalem Gerät. |
| **MT.REC4 — Verständlicher Batch + Fehlertexte** | Geprüfte Auswahl, Batch-Zusammenfassung und Markerstatus kompakt integrieren; nur geeignete Filme zu ausdrücklich bestätigten Imports, nötige Reencodes gesondert anbieten, automatische Dublettenmarkierung getrennt; klare Fehler-/Fortschrittstexte. | Gemischte 24+6-Situation verständlich, keine Doppelimporte, keine veralteten Zähler/technischen Texte, reproduzierbarer Pending-/Abbruch-/Wiederanlaufzustand. |
| **MT.REC5 — Gesamt-Abnahme auf echtem yaVDR** | Exakte Code-/Test-SHA und dann freigegebene installierte Runtime getrennt prüfen: gewöhnliche Folge, echte Dublette, alle drei Rectools-Marker, externe/VDR-Encode-Entscheidung, Stapel, VDR-Aufnahme-Readback, Navigation/Responsivität. Bestehende Install-/Rollback-Artefakte **wiederverwenden**, kein unnötiger neuer Daemon-Build. | Nachweis `UNIT/CONTRACT`, `PRODUCTION-INTEGRATION`, `CI`, `REAL-VDR` **jeweils separat**; produktive Änderungen erst bei abgestimmter tatsächlicher Testdatei und Host-Freigabe. |

**Quellcode-Fortsetzung (27.10.2026, PR #556):** Root- und Unterordner-Scan auf dem bestehenden Agent-Inventar, wahrheitsgemäße unmarkierte Quellzahlen, wiederholbare Vorschau, diagnostische Fehlercodes (Quelle/FFprobe/Ziel/Speicher), echtes responsives Drei-Spalten-Dark-Layout und automatische Rectools-`.duplicate`-Markierung ohne manuellen Button implementiert. Auch Stapel-Dubletten werden **seriell** durch die bestehende Core-/Agent-Markeroperation samt bestätigtem Readback verarbeitet; andere Filme benötigen weiterhin eine explizite Importbestätigung. `.skipped`/`.pc_encode` bleiben bestehende Rectools-Zustände; Video-Encode-Auswahl nur nach FFprobe-Befund. Fokus-Regressionsfälle für verzögerte Agent-Vorschau und zwei automatische Dubletten ergänzt. **Nicht geschlossen:** Konkrete S02E12-Hostursache, Integration/Build auf finalem HEAD, Desktop-/TV-Browser-Screenshot und tatsächlicher Import auf dem installierten yaVDR sind unbewiesen; keine Dienste, Aufnahmen, Grants oder Pakete verändert. Ein früherer Fund von `S01E01.skipped.mkv` ist kein Befund zu `S02E12`.

---

**Aktueller MT.HA-Plan:** [MT.HA.0–8 Hardware-, Distributions- und Launchpad-Abnahme](../../docs/planning/addon-platform-roadmap.md#mtha--manuelle-hardwareabnahme-bis-zur-launchpad-freigabe-26102026). Der erste reale Hostbefund zeigt bereits vorhandene Media-Tools-Worker, Root-Approval/Quellkonfiguration und Agent-Drop-in auf manuell installierter yaVDR-Suite. Deshalb **keine unbeaufsichtigte Erstinstallation**: zuerst Byte-/Stage-Provenienz, bereits bestehenden Lifecycle und DB-/WAL-/Backup-Grenze prüfen. Die Hardwareabnahme ist noch PENDING.\n\n## Fortschritt und Nachweise (25.10.2026)

| Slice | Tatsächlich erreicht | Noch nicht nachgewiesen |
| --- | --- | --- |
| MT.I0 | [Belegte Quell-/Hostmatrix](INSTALLATION-BASELINE.md) für Stages, Handler-Trust, Installer, `debian/`, feste Pfade/Build-Abhängigkeiten und bewusst unbekannte Distributionswerte; Dokumentationscommit `7a143a1d`. | Keine neue read-only-Abfrage des produktiven Hosts; keine Laufzeit- oder Distributionsfreigabe. |
| MT.I1 | **QUELLE/STAGING/CI ABGESCHLOSSEN:** Der vorhandene Composer ergänzt dieselbe Suite+Media-Tools-Stage um eine rollen-/versionsbezogene Datei- und Hash-Inventur; read-only `--verify-stage` mit unabhängig geprüftem Handler-SHA256. Sechs isolierte Stage-Regressionen (unveränderter Input, Vollständigkeit, mutierter/resealed Worker, falscher Approval-Digest) und alle vier GitHub-Actions-Jobs sind erfolgreich: `media-tools-trust-gate`, `frontend-install-staging`, `frontend-regression-test`, `addon-platform`, Code-/Test-SHA `bfa8b6574831f923d8864c03d8bae523a72fa9aa`, [Actions #40139416273](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40139416273). Drei zuvor überholte Agent-/Runtime-Test-Fixture-Stagingannahmen wurden in zwei Tests an den bereits bestehenden `libexec`-Metadatenbegleiter angepasst (`7440d648`, `98fffdb4`, `bfa8b657`); produktiver Agent und Stage-Vertrag dafür unverändert. | **REAL-VDR/INSTALL NOT_VERIFIED**: kein produktives Deployment, kein Nutzer-/Grant-/Dateirechte-Eingriff. Paketgenerator `~cpppreview1` und `debian/` verwenden das neue Inventar noch nicht; identische `.deb`-Paketpayloads, Migrationen und Update-/Rollback-Verhalten sind MT.I2/MT.I4 und ausdrücklich **nicht** MT.I1-abgenommen. Stage-Seal ist keine digitale Signatur und ersetzt keine unabhängige Zulassung. |
| MT.I2 – quellseitig implementiert, CI-/Host-Gates getrennt | **Ein vorhandener Installer/Deploy**: `prepare` prüft und verwendet vorhandene Worker/Stage; `check`/`verify` lesen Stage-Seal, acht Add-on-Dateien, Hashes/Versionen, vorhandene Konfiguration und Debian-Dateibesitz. `install_media_tools.sh` verlangt vor yaVDR-Erstinstallation eine ausdrückliche Zustimmung und den **exakten, vorhandenen Suite-only-Stand**, verwahrt geprüfte Stages in neuem root-privatem Verzeichnis, legt ein SQLite-Backup an, veröffentlicht nur neue Konfigurations-/Runtime-Dateien und **erteilt keine Grants, startet keine Dienste**. Ein gescheiterter Datei-Deploy setzt die Suite-Stage anhand des verifizierten Backups zurück; Dienste bleiben gestoppt. Für bereits installierte Media Tools gibt es einen **offline** `live-update`/`live-rollback` im bisherigen CLI: exakte vorherige/neue versiegelte Gesamtstages, gleicher Dateibaum und Datei-/Verzeichnismodi, Prüfung auf bestehende Paket- und Fremdeigentümer, stillstehende Suite/Agent-Dienste, exklusiver Wartungslock, explizite Bestätigung, neue root-private Backups von Stages und SQLite sowie Rückweg über den bestehenden `install_runtime_stage.py`-Deploy. Bei Datenbank-/WAL-Drift wird ein nachträgliches automatisches Rollback verweigert statt Nutzerzustand zu überschreiben. Isolierte Tests prüfen Vorwärts-/Rückwärtsdeployment, provoziertes Scheitern und Wiederherstellung, verweigerte Live-Aufrufe ohne Bestätigung und Stage-Drift. | **CI PASS (4/4)** auf Code-HEAD `f4b9c9dac2fb8fde6dc2804bfc2d97b345ae512f`: `frontend-install-staging`, `addon-platform`, `media-tools-trust-gate`, `frontend-regression-test` ([Actions #40159699229](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40159699229)). **Kein Test auf produktivem yaVDR**, keine Root-/Dateirechte-Reparatur, kein Dienststart und keine wirkliche Paketmigration. Wenn bestehender Suite-Stand/Package Ownership/Agentzustand den Offline-Updatevertrag nicht erfüllen, stoppt das Werkzeug; **MLD und openSUSE nicht unterstützt**. Die echte Hostabnahme benötigt eine separate, ausdrücklich genehmigte Ausführung. |
| MT.I3–MT.I5, MT.UX0–UX7 | Keine Umsetzung in diesem Arbeitsschritt. | Distributionsadapter, `.deb`-Parität, Live-/UI-Abnahme offen. |

**Komponentenänderung:** MT.I1: `stage_media_tools_runtime.py`/Stage-Test. MT.I2: `media_tools_install_preflight.py`, `media_tools_runtime_transaction.py` (synthetische und ausdrücklich autorisierte Offline-Host-Transaktion), bestehendes `install_media_tools.sh` und erweiterte isolierte Tests. `install_runtime_stage.py` bleibt der einzige Suite-Runtime-Dateideployer; keine neue Queue, Dienste, Grants oder Protokolle.
**Zusätzliche Laufzeitprozesse, Protokolle, Agenten und Importpipelines: 0.**
Eine Stage-/Paketprüfung ist weder ein Freigabenachweis noch ein Anlass für
Root-Berechtigungen oder Host-Reparaturen.

## MT.I2 → MT.UX: bewusster Installationsstopp

**Keine Installation vor Abschluss der gewünschten UX-Arbeit.**
Der verifizierte MT.I2-Code-/CI-Stand, die konkret fehlende yaVDR-Hostabnahme,
bestehender Debian-/Dateibesitz, Rollback-/Backuppfade, Service-/Grantgrenzen
und spätere Wiederaufnahmeschritte stehen verbindlich in
[INSTALLER-RESUME.md](INSTALLER-RESUME.md).
MT.I3–I5 sind nicht stillschweigend freigegeben; der Phasenwechsel
erlaubt keine `sudo`-/`systemctl`-/Rechte-/Aufnahmeaktionen.

## MT.UX-Stand beim Einstieg (Quellenänderungen, CI noch zu prüfen)

| Slice | Fortschritt | Für Slice-Abnahme noch erforderlich |
| --- | --- | --- |
| **UX0** | Modulnavigation `Aufnahmen → Media Import → Genres`; bestehendes `hidden/disabled` und Capability-/Session-Gate unverändert, gezielter Regressionstest. | Frontend-CI, reale Berechtigung im Zielsystem separat. |
| **UX1 – 100-Filme-Grenze implementiert, Testabnahme offen** | Nutzerentscheidung vom 25.10.2026: **höchstens 100 Filme pro Importdurchlauf**, im Alltag meist 5–10. Der bestehende rekursive, symlink-freie Scanner bietet maximal 100 geprüfte Source-IDs; erkennt er darüber hinaus geeignete Quellen oder erreicht die weiterhin bestehende 4096-Eintrags-Sicherheitsgrenze, wird `partial=true` gemeldet. Agent, Core und Public v1 akzeptieren maximal 100 beobachtete Quellen pro Snapshot; die vorhandenen 32er-API-Seiten sammeln diese maximal 100 im Webfrontend, ohne einen neuen Explorer/Jobdienst. Der bestehende Batch-Import weist eine Auswahl über 100 zurück, die UI erläutert die Grenze bei abgeschnittenen Treffern und ermöglicht nach dem Durchlauf einen erneuten Scan. Scanner, Quelle/Grants, Readback und Originalschutz bleiben unverändert. | **Noch nicht abgenommen:** Fokussierte C++-/Frontend-Tests mit 100/101 Fällen, 4096-Entry-Teilmengen und Backend-/Grant-Revalidierung müssen am exakten Code-SHA tatsächlich laufen; CI und REAL-VDR nicht nachgewiesen. **Eine Fortsetzung über 100 Filme ist ausdrücklich nicht mehr vorgesehen**. Das 4096-Sicherheitslimit bleibt bewusst bestehen und muss als unvollständiger Scan angezeigt werden, statt Dateien als vollständig erfasst auszugeben. |
| **UX2 – Einfaches Verzeichnismanagement im Webfrontend (lokaler Agent, Quellcode)** | Der vorhandene Administrator-Login schützt die Media-Tools-Einstellungen. Die Oberfläche lädt die aktuelle Quellliste, erlaubt Hinzufügen/Entfernen, Anzeigenamen und Standard-Reihenfolge und speichert sie direkt über das bestehende Core-HTTP-Gate mit dessen CSRF/role.admin-Prüfung. Der lokale Root-Daemon ersetzt die vorhandene root-kontrollierte TSV-Liste atomar unter dem bestehenden Import-Wartungslock; VDR-Zielordner und übrige Dienste bleiben unverändert, Agent aktualisiert den Scan beim normalen nächsten Zyklus. Keine Shell-Ausgabe im Browser, kein neuer Dienst, keine neuen Rollen, keine neue Datenbank oder Restart-Pflicht. | **Code-/Integrationsabnahme offen:** C++-/API-/Agent-/REAL-VDR-Test stehen noch aus. Der lokale Schreibweg verweigert abweichende Remote-Backends bzw. einen Daemon ohne nötige Schreibrechte, statt eine falsche Konfigurationsdatei zu verändern. Die Bedienbarkeit im Web ist in der laufenden Modulkomposition ohne Server-Mutation am echten VDR geprüft. |
| **UX3 – real-relative folders, search and selection source candidate** | Der Scanner liefert aus der geprüften Importwurzel relative Ordner pro Quelle; bei mehreren belegten Importwurzeln trennt eine virtuelle Anzeigenamen-Ebene gleichnamige Unterordner sicher. Agent `sourceFolders`, Core-Katalog, Public v1 mit Cursorbindung und Frontend-Breadcrumbs verwenden dieselbe opaque Quelle; keine browsergelieferten absoluten Pfade. Ordnernavigation, sofortige Kindordner, freie Suche über alle geprüften Ordner, Importierbar/Markiert-Filter und maximal 100 Quellen umfassende Checkbox-Mehrfachauswahl sind implementiert. Teil- und Inkompatibilitätsfälle verwerfen aktuelle Auswahl; nur sichtbare geprüfte IDs gelangen nach expliziter Bestätigung zur vorhandenen sequentiellen Operation. | **Abnahme offen:** übergreifender C++-/Agent-/Core-/Public-/Browser-Build und Isolations-/Unicode-/Traversal-/Backendwechseltests samt REAL-VDR am großen Integrationsgate. Keine neuen Scanner-/Importdienste oder Browser-Dateisystemoperationen. |
| **UX4** | Filmnamen statt technischer IDs, echte rein lesende Backend-Vorschau (Codec, Speicher, Dublette, VDR-Ziel). UX4.3: Die Vorschau entscheidet **technische VDR-Importeignung**, nicht die Wiedergabe auf einem konkreten TV. Bei geeignetem Video wird der bestätigte Remux-Import direkt angeboten. Nur wenn die Videospur nicht kopiert werden kann, erscheinen VDR-H.264-Umkodierung nach Einzelfreigabe, PC/HandBrake-Markierung oder Überspringen. Fehlende/abgelehnte Vorschau sperrt den Direktimport. | Code und Browser-Regressionsfälle vorhanden; exakter CI-HEAD gesondert zu prüfen. Echtes yaVDR-Import-/Wiedergabe-Readback weiterhin ausstehend. Keine stillschweigende Videoumkodierung, keine automatische Freigabe oder Original-Löschung. |
| **UX5 – UX5.1–UX5.3 Quellcode** | Die vorhandene Mehrfachauswahl löst nun zuerst pro ausgewähltem Film die echte **lesende** Public-v1-/Core-/Agent-Vorschau aus. Nur ein verifiziertes `ready` **ohne** Videoumkodierungsbedarf wird in der separaten Stapelbestätigung angeboten; VP9/andere Umkodierungsfälle, Dubletten, Platzmangel, `pending`/Backend-Fehler bleiben als namentlich bezeichnete Einträge zurückgestellt. Ein ausdrücklich bestätigter Stapel verwendet anschließend **nur** die bestehende sequentielle Core-Operation mit eigenen Idempotency-Keys, erneuter Quellen-/Rechteprüfung, Recording-Readback, sicherem Stopp bei ungewissen Ergebnissen und sichtbaren Statuszeilen. `Einzeln prüfen` führt zurück zur bestehenden Film-/HandBrake-Entscheidung, ohne automatische Umkodierung. Kein weiterer Dienst, Speicher oder Protokoll. | Final-HEAD-Frontend-Integration/CI und später REAL-VDR abnehmen. Ein beobachteter Speicherwert pro Film garantiert nicht die Kapazität für den gesamten Stapel; jeder Folgeauftrag prüft den Speicher erneut. Kein Nachtlauf ohne gesonderte Freigabe. |
| **UX6** | **SOURCE IMPLEMENTED (11.11.):** Eine bestehende Hero-HEVC-Plan/Stage/Review/Publish-Implementierung; Panel im vorhandenen Aufnahmeaktionsbereich. Ergänzt um `⋯` je Aufnahmekarte, lesende backendbezogene Capability-Prüfung und direkte Navigation in genau denselben Hero-Owner. | Isolierter Karten-zu-Hero-Test inklusive bestehendem Stage/Review/Publish-Pfad PASS. **Offen:** kompletter Node-/CI-Frontendgraph auf finalem SHA, echte yaVDR-/Mobile-Kartenaktion und visuelle Abnahme; keine produktive Encode-/Publish-Operation ohne Freigabe. |
| **UX7** | Responsive Scan-/Vorschau- und Fehlerhinweis-Stile ergänzt, technische Tokens in Details verschoben. | End-to-End-Abnahme des ganzen benutzerfreundlichen Workflows, 32+-Quellen, große Bäume, Keyboard, responsive und reale Backend-Capability. |

**Wichtig:** Diese Tabelle trennt bereits vorhandenen, mitbenutzten Code von neu
implementierten Teilverbesserungen. Ein UX-Slice wird **erst nach seinen
expliziten Abnahmen** geschlossen. Niemand soll den begrenzten Quellenkatalog
als vollständigen Dateisystem-Explorer oder erfolgreiches Live-Scanning
verkaufen.

## Verbindliche Produktentscheidung

**Rectools-einfacher Import als Default, Explorer als Zusatz.**

- Ein Administrator hinterlegt pro Backend eine oder mehrere **zugelassene Importwurzeln**, mit lesbarem Namen und optionaler Standardquelle. Der einfache Einstieg zeigt die Standardquelle und genau zwei Wege: **„Verzeichnis scannen“** oder **„Explorer öffnen“**.
- **„Verzeichnis scannen“** liest rekursiv **alle Unterverzeichnisse** der gewählten Quelle; ein Scan löst **keinen Import** aus. Danach stehen normale, zurückgestellte, übersprungene und doppelte Dateien verständlich getrennt in der Trefferliste.
- Nutzer wählen alle geeigneten Dateien, einen Ordner oder einzelne Einträge, sehen eine **Importvorschau** und starten erst dann den bestätigten Auftrag. Optionale Neukodierung braucht eine explizite, zum Auftrag passende Zustimmung.
- Der Explorer nutzt dieselbe erlaubte Dateiliste, nur mit Ordnernavigation, Breadcrumbs, Filtern und Mehrfachauswahl. Keine unbeschränkte Dateisystemnavigation durch den Browser und keine zweite Importengine.
- Navigation: **„Media Import“ direkt rechts von „Aufnahmen“**, `Home` bleibt zuerst. Nur sichtbar mit wirksamer Backend-/Akteur-Capability.
- **„Aufnahme verkleinern (HEVC)“** gehört in **Aufnahmen → Aufnahmeaktionen**, sowohl bei den Aufnahmedetails als auch im `⋯`-Menü der Detailkarten, sofern die Aufnahme geeignet und die Funktion freigegeben ist. Kein eigener Hauptreiter.
- Technische Token (`sourceId`, Freigabe-Revision, Handler-Digest) erscheinen nur unter „Technische Details“, niemals als Hauptbeschriftung.

### Importquelle auf VDR-Aufnahmeordner abbilden (quellseitig umgesetzt, Abnahme offen)

- **Relative Ordnerhierarchie übernehmen:** Eine konfigurierte Importwurzel entspricht dem VDR-Aufnahme-Root, **nicht** einem zusätzlichen Unterordner. Beispiel `Importroot/actions/Matrix.mkv` → logischer VDR-Ordner `actions/Matrix`, darunter die native datierte `.rec`-Aufnahme. `Importroot/Film.mkv` → `Film` unmittelbar im VDR-Root. Beliebig tiefe Serienstrukturen wie `Serien/Andor/Staffel 1/Folge01.mkv` erhalten ihre relativen Ordner.
- **Kein Schreiben beim Scan:** Scan und Vorschau bleiben rein lesend. Erst der bestätigte Import legt fehlende **gültige VDR-Ordner** im bestehenden, autorisierten Zielroot an; existierende Zielordner werden wiederverwendet. Keine leeren Ordner auf Vorrat; keine unkontrollierten Pfade/Symlinks, kein Umgehen von Staging, Duplikatschutz oder atomarer Veröffentlichung.
- **Vorhandene Semantik verwenden:** `web/frontend/recordings2-move-editor.js` nutzt bereits VDR-Ordner als Ziel der bestehenden Recording-MOVE-Aktion; laut `docs/development/recording-folder-genre-management.md` wurde das reale Anlegen beim Move abgenommen. Der Importer verwendet denselben logischen VDR-Ordnervertrag, **nicht** den MOVE-Endpunkt für noch nicht existierende Aufnahmen und keine zweite Ordnerverwaltung.
- **VDR-Quellenabgleich:** VDR `recording.h/recording.c` verwenden `FOLDERDELIMCHAR='~'` im logischen Aufnahmenamen und die physische Ordnerhierarchie im Videoverzeichnis. `folders.conf` listet hingegen vordefinierte Timer-Zielordner und ist **nicht** erforderlich, damit importierte Aufnahmen unter ihrem Pfad erscheinen. Gültige VDR-Namens-/Kodierungsregeln und die bestehende Aufnahmeaktualisierung samt autoritativem Readback sind zu prüfen; `folders.conf` nicht eigenmächtig bearbeiten.
- **Behobene Quellcode-Lücke (MT.UX4.1):** `addons/media-tools/src/MediaImport.cpp` bildet die tatsächlich relative Quellhierarchie unterhalb von `videoRoot` ab; eine Importwurzel erzeugt keinen zusätzlichen Zielordner. Unzulässige VDR-Verzeichnisnamen und Symlinks werden zurückgewiesen. Fehlende VDR-Unterordner entstehen erst nach erfolgreichem FFmpeg-/Index-Preflight unmittelbar vor der atomaren Aufnahmeveröffentlichung; bei Fehlern werden nur selbst angelegte, weiterhin leere Verzeichnisse zurückgenommen. Die Frontend-Vorschau bezeichnet die vom Agenten gelieferte relative **Quellordner**-Information ausdrücklich als solche. **MT.UX4.2 quellseitig umgesetzt:** Ein ausgewählter Film erhält über denselben bestehenden Importplan und die authentifizierte Agent-/Core-/Public-v1-Kette eine ausschließlich lesende Vorschau für Backend-Titel, Quellordner, tatsächlichen relativen VDR-Zielordner, Größe, Codec, freien und erforderlichen Speicherplatz sowie Dubletten- und Umkodierungsstatus. Die Oberfläche zeigt sofort „Film wird geprüft“, verarbeitet `202 pending` und verwirft verspätete Antworten bei Filmwechsel, Filterung oder Scan. `ready`, `duplicate`, `insufficient`, `unavailable` und Berechtigungsfehler erhalten verständliche Texte; die Import- und Umkodierungsfreigabe bleibt ausdrücklich separat. Keine zweite Import-Engine und keine Schreibwirkung durch die Vorschau. **Regressionen ergänzt:** Importer/Codec/verschachtelte Ziele, Agent-Lease und Quellen-ID, Public-v1-Antworten, Browser-Race/Pending/Fehler, paginierte Quellliste. **MT.UX4.3:** Erst eine aktuell erfolgreiche, rein lesende Backend-Diagnose schaltet die direkte Importaktion frei. Kopierfähige Videos benötigen nur die gewöhnliche Importbestätigung; sonst erscheint ausschließlich die Wahl lokaler H.264-Umkodierung, PC-/HandBrake-Delegierung (`.pc_encode`) oder Überspringen (`.skipped`). Die bereits vorhandene Core-Einzelfreigabe `videoDecision=local` und die sicheren Markierungsoperationen bleiben unverändert. Die Audioprüfung/-anpassung läuft wie bisher beim Import unabhängig vom Video-Codec. Die externe Umkodierung selbst findet außerhalb der Suite statt; eine zurückgelegte **neue** kompatible Datei wird beim nächsten Quellenscan normal erkannt, während das markierte Original geschützt bleibt. Bei Dublette, Platzmangel, nicht erfolgreicher oder verzögerter Vorschau kein Direktimport; eine PC-Markierung ist auch bei ungenügendem lokalem Staging-Speicher möglich; `skip` bleibt als ausdrücklich bestätigte separate Markierung möglich. Keine automatische Importausführung, kein Prüfversprechen zur konkreten TV-Wiedergabe. **Teststatus:** Quellcode und Tests sind eingecheckt; C++-/Frontend-/Staging-Ergebnisse gelten nur für den exakt zugehörigen GitHub-Actions-SHA und müssen als grün verifiziert werden. Real-VDR-/Hardware-Abnahme bleibt ausdrücklich offen.
- **Abnahme:** Root-Film, vorhandener und neuer Zielordner, mehrere Ebenen, gleicher Filmname in verschiedenen Unterordnern, VDR-unzulässige Ordnernamen, Symlink-/Traversal-Angriff, Duplikate, fehlgeschlagene Veröffentlichung ohne Phantom-Recording, passende Vorschau und sichtbare VDR-Aufnahme nach Readback. Produktive Dateisystemtests nur nach separater Freigabe.


## Belegter Ist-Zustand – nicht mit Unterstützung verwechseln

- `vdr-rectools` Branch `develop`: `vdr-rectools import` scannt den konfigurierten `IMPORT_DIR`; `var/www/html/explorer.php` ist eine getrennte Dateiansicht. Die Bedienidee ist die Vorlage, **nicht** die alten potenziell löschenden/ungeschützten Arbeitswege.
- Bekannter **einzelner** yaVDR/Ubuntu-Host: Backend `default`, Importwurzel `/srv/video/Filme`, Videoziel `/srv/vdr/video.00`. Das sind **keine** Standardwerte für openSUSE/MLD oder jedes Ubuntu.
- Live-Read-only-`plan` erkannte eine Andor-MKV mit HEVC ohne notwendigen Re-Encode. Der bisherige Browser zeigte anfangs null gewöhnliche und sechs markierte Quellen, obwohl tiefere Serienordner vorhanden waren. Das beweist keinen fertigen produktiven Import.
- Die Source enthält bereits Scanner, C++-Importer, M1.R-Entscheidungen, Suite Core/Agent-Operationen und HEVC-Vorprüfung/Stage/Publication im Recording-Hero. Bestehende Implementierungen wiederverwenden statt neu erfinden.
- `tools/addons/install_media_tools.sh` ist derzeit ein **yaVDR-/systemd-spezifischer Adapter**. Versiegelte Stages über `stage_media_tools_runtime.py` und `install_runtime_stage.py` existieren. Ein geprüftes openSUSE- oder MLD-Installprofil **existiert hiermit noch nicht**.

## Phase MT.I — zuerst eine einfache, distributionsbewusste und paketierbare Installation

**Architekturentscheidung:** Eine gemeinsame geprüfte Datei-/Build-Staging-Basis für manuelle Installation **und** spätere Ubuntu/Debian-`.deb`-Pakete. **Nicht** zwei Installationsimplementierungen pflegen. Das Paket und der manuelle Installer verwenden dieselben Worker, geprüften Artefakte, Zielpfad-/Config-Kontrakte und denselben versiegelten Inhaltsstand; nur Deployment- und Paketmanageradapter unterscheiden sich.

| Slice | Konkrete Umsetzung | Abnahme |
| --- | --- | --- |
| **MT.I0 – Installationsvertrag prüfen** | Bereits vorhandene Stage/Installer, paketierte Dateien, C++-festkodierte Pfade, Add-on-Zulassung, Service-/Benutzer-/Config-/State- und VDR-Video-Pfade vollständig inventarisieren; Ist/gewünschte Variabilität unterscheiden. Belegtes yaVDR-Referenzprofil; openSUSE/MLD: unbekannte Werte explizit als unbekannt markieren. | Host-/Quellmatrix mit getrenntem Status für Build, manuelle Installation, Paket und echte Laufzeitprüfung. Keine Host-Mutation. |
| **MT.I1 – Gemeinsame paketierbare Stage** | Bestehendes `make stage-install-runtime` und `stage_media_tools_runtime.py` gezielt wiederverwenden/verbessern. Deklarative Datei-/Versionliste mit Hashes, Binär-/Metadaten-/Config-/State-Trennung und vertrauenswürdiger Stage-Seal-Prüfung. `DESTDIR`/`PREFIX` **nur** anbieten, wenn tatsächliche Runtime- und Handlerpfade korrekt mitziehen; sonst sichtbare feste Grenze statt vorgetäuschter Portabilität. | Derselbe geprüfte Build-/Staging-Inhalt ist Eingang sowohl für manuelles Deployment als auch für spätere Paketierung. Keine abweichenden parallelen Dateibäume. |
| **MT.I2 – Einfacher manueller Installer/Updater** | Kleine Bedienoberfläche/CLI mit `prepare`, read-only `check`, ausdrücklichem `install/update`, `verify` und dokumentiertem Rollback. Konfigurierbare, **tatsächlich unterstützte** Werte: VDR-Videoziel, freigegebene Importroots, Account/Gruppe, Dienste, Install-/Config-/State-/Web-Ziele. Bestehende manuelle/paketierte Dateien, versiegelte Builds und Versionen erkennen und wiederverwenden; keine blind wiederholten Kompilierungen. | yaVDR-First-Install und Update mit Backups/Rollback, keinem heimlichen Quell-/DB-Verlust, keiner Dienstaktivierung/Grantvergabe durch bloße Installation. |
| **MT.I3 – Distributionsadapter/Hostprüfung (Quellcode)** | yaVDR/Ubuntu als verifizierter Ausgangspunkt. openSUSE/MLD nur anhand später geprüfter Host-/Maintainerdaten konfigurieren: Dienste/Init, reale VDR-Pfade, ABI, Gruppen/Nutzer, Abhängigkeiten und Deployment-Orte. Keine behaupteten Defaults oder stilles `systemctl` auf fremden Plattformen. | Mit synthetischen Nicht-Produktions-Roots und expliziten Beispielkonfigurationen testbar; echter Support für SUSE/MLD erst nach Host-Verifikation. Fehlende Angaben stoppen vor produktiven Änderungen. |
| **MT.I4 – Ubuntu/Debian-Paketbau aus genau derselben Stage (Quellcode)** | An vorhandenes `debian/`- und Add-on-Package-Build-System anschließen, keine zweite Installlogik. Erst paketierbare Stage + Manifest, dann deklarative Paketdateiliste, `dpkg-deb`-Build/Contents, Architektur/ABI-Abhängigkeiten, optional getrennte Pakete für Suite/Core und Media Tools, Conffiles und Upgrade-Migration. Paket-Skripte dürfen Add-on nicht automatisch freischalten, Originalmedien nicht verändern und keine Host-Verzeichnisrechte korrigieren. **Manuell installierte Dateien niemals ungeprüft mit dpkg überlagern.** | Abgleich `.deb`-Inhalt gegen dieselben Stage-Hashes; isolierter Erstinstall-/Upgrade-/Purge-Test. Später PPA/CI möglich, **nicht** als schon eingerichtet behaupten. |
| **MT.I5 – Anleitung und Release-Gates (Dokumentation)** | Eine dokumentierte Methode für manuelle Ubuntu/yaVDR-Installation und Paketerzeugung aus dem gleichen Stand, saubere Konfigurationsvorlage für alternative Systeme, klarer Rollback und Abnahme-Checkliste. | Anwender können Installpfade und Dienste nachvollziehen; kein unnötiges Neu-Build pro Doku-Commit; Live-Installation erst nach eigener Freigabe. |

**MT.I3–I5 Stand 25.10.2026 – Nachweise sind voneinander getrennt:**

- **I3:** `install_media_tools.sh profile ABSOLUTE_ROOT` liest ohne Hoständerung nur eine ausdrücklich angegebene Root-Umgebung und klassifiziert deren `os-release`. Ubuntu liefert einen **Referenz-Adapter-Hinweis**, keine positive Prüfung von Agent, ABI, Pfaden, Datenbank, Paketbesitz oder vorhandenen Freigaben. Die existierende yaVDR-Erstinstallation lehnt nicht nachgewiesene Nicht-Ubuntu-Hosts nun bereits vor dem Build-/Servicepfad ab. **MLD/openSUSE: nicht unterstützt**, bis Init, Pfade, VDR-ABI und Paket-/Benutzervertrag real geprüft werden. Die vorhandene `configure_media_tools_paths.sh` bleibt nur für Ubuntu/systemd.
- **I4:** `build-media-tools-stage-deb` baut aus dem **exakt gleichen**, vorher unabhängig freigegebenen und vollständig versiegelten `stage-with-media-tools`-Baum ein separates, **inaktives** `vdr-suite-addon-media-tools`-Paket. Es enthält ausschließlich die 8 in der Stage gebundenen Add-on-Dateien; der Builder prüft nach `dpkg-deb --extract` SHA256, Modus und vollständigen Dateisatz. Ein `preinst`-Guard verweigert vor dem Entpacken kollidierende manuelle oder fremd paketierte Dateien und symlinked Zielverzeichnisse. Keine Maintainer-Aktivierung, keine State-/Approval-/Config-Dateien, kein `systemctl`, keine erneute C++-Kompilation. **Nicht** gleichzusetzen mit freigegebener Migration bestehender Manuallayouts oder der Integration in die vorhandene dreiteilige `debian/`-Quellpaket-/PPA-Pipeline; diese separaten Upgrade-/Purge- und ABI-Abnahmen bleiben offen.
- **I5:** Der Installations-/Paketierungsvertrag und die **nicht mutierende** Release-/Rollack-Checkliste stehen in `INSTALLER-RESUME.md` sowie `README.md`. Ein `CI=PASS` bestätigt nur den isolierten Codepfad am genauen HEAD. Vor REAL-VDR: tatsächlichen Host, Altzustand, Dateibesitz, Agent-Service, Storage, VDR-Ausgabe und vollständige sichere Rollback-Stage erneut prüfen. **Keine Installation, keine Paketmigration und keine Rechteänderung** durch diesen Slice.

**Kernformel:** `ein Build → versiegelte Stage → [manueller Adapter ODER Ubuntu-.deb]`. Benutzer-/Root-Rechte, Dienstverwaltung, Konfiguration, lokale Freigabe, Paketbesitz und Add-on-Aktivierung bleiben unabhängig. **Keine Änderungen an bereits bestehenden Eigentümern/Dateimodi**, insbesondere kein rekursives `chmod`/`chown`.

## Phase MT.UX — Bedienoberfläche nach der Installationsbasis

| Slice | Konkrete Umsetzung | Abnahme |
| --- | --- | --- |
| **MT.UX0 – Navigation/Grundzustand** | `Media Import` direkt rechts von `Aufnahmen` in `web/frontend/index.html`. Vorhandene Modul-/Session-/Backend-Capability-Prüfung nicht entfernen. | Korrekte Reihenfolge, ohne wirksame Capability weiterhin ausgeblendet; Frontend-Tests grün. |
| **MT.UX1 – Einfacher rekursiver Nur-Lese-Scan** | Standardimportroot und rekursive Unterordner innerhalb der bestehenden 4096-Eintrags-Sicherheitsgrenze scannen; **höchstens 100 geprüfte Filme pro Durchlauf**. Mehr als 100 Filme werden nicht zur Auswahl angeboten; bei Überschreitung oder begrenztem Scan deutlich informieren, anschließend erneut scannen lassen. Kein unbeschränkter Import und keine Dateisystemänderung beim Scannen. | 5–10 Filme ohne unnötige Bedienungsschritte, exakt 100 unterstützt, 101. Quelle nicht als importierbare Auswahl; >4096-Einträge bzw. weitere Filme ehrlich als unvollständig gemeldet; keine Quelle verändert. |
| **MT.UX2 – Administrierbare Quellen** | Geschützter Adminbereich: aktuelle Quellen laden, Name/Pfad ändern, hinzufügen, entfernen, Standard festlegen, einmal speichern. Direktes atomisches Update der lokalen Backend-Konfiguration durch den bestehenden Daemon, Agent übernimmt beim normalen nächsten Scan. Der Offline-Konfigurator bleibt optionales Wartungswerkzeug, kein Teil der Browser-Bedienung. | Geplanter C++/Produktions- und Real-VDR-Abnahmetest; entfernte Backend-Rechner sind in diesem bewusst kleinen Slice nicht beschreibbar. |
| **MT.UX3 – Explorer als Erweiterung** | Backend-relativer Ordnerpfad pro geprüfter Source-ID, echte Breadcrumbs und verschachtelte Ordnerübersicht, Suche, Statusfilter und bestätigte Mehrfachauswahl; der Import erhält nur opaque IDs. Der 100-Filme-Deckel bleibt erhalten. | Full-stack-Build/CI und produktiver Integrations-/Sicherheitstest noch offen. Keine Herkunftsordner aus Titeln erraten. |
| **MT.UX4 – Importvorschau/Entscheidungen (UX4.1–UX4.3 Quellcode)** | UX4.1 erhält die vorhandene Ordnerhierarchie der Importquelle im VDR-Ziel, erzeugt Unterordner erst beim bestätigten Import und behält den no-overwrite-Schutz bei. UX4.2 fragt **nur den ausgewählten Film** über den vorhandenen Public-v1-/Core-/Agent-Quellenweg ab: vom tatsächlichen `inspectImportPreview`, `ffprobe` und der produktiven Speicherplatzberechnung kommen VDR-Unterordner, Video-Codec, Dateigröße, freier Platz, benötigter Platz inkl. Reserve, Dublettenstatus und Umkodierungsbedarf. Die Agent-Anfrage und Antwort bleiben flüchtig, auf Source-ID/Backend/Lease begrenzt und nur lesend; der Browser zeigt bis zur nächsten Agent-Meldung sichtbar »Prüfung läuft«. Vorhandene `Importieren`/`HandBrake`/`Überspringen`/`Markierung aufheben`-Operationen bleiben separat autorisiert. | **Offen:** zielgerichtete C++-Build-/API-/Agent-Integration/CI-Prüfung des finalen SHAs und spätere REAL-VDR-Abnahme. Keine automatische Import- oder Re-Encode-Freigabe, keine Originaländerung. UX4.3 leitet aus der tatsächlichen Nur-Lese-Vorschau den einfachsten VDR-Importweg ab; technisch notwendiger Re-Encode bleibt ausdrücklich bestätigt, und für copyfähiges HEVC ist eine freiwillige, einzelne H.264-Umkodierung mit separater Bestätigung zulässig (Direktimport bleibt Standard). PC-/Skip-Entscheidungen verwenden weiter die bestehenden Backend-Markierungen, nicht die Importpipeline. |
| **MT.UX5 – Stapel- und Auftragsansicht (5.1–5.3 Quellcode)** | `Ausgewählte/Alle sichtbaren Filme prüfen` liest höchstens 100 Backend-Vorschauen, trennt importfähige, umkodierungsbedürftige, doppelte und ausstehende Filme und zeigt eine konkrete Ergebnisliste. `Geeignete Filme jetzt importieren` ist eine **zweite**, bestätigte Aktion. Nur passende Filme gehen einzeln über die bisherigen Core-Operationen; Status/Readback und sicherer Stopp bleiben sichtbar. Zurückgestellte Filme können einzeln erneut geprüft und nach ausdrücklicher Entscheidung lokal umkodiert oder mit `.pc_encode` für extern markiert werden. | Keine Doppelimporte, auch nicht bei ungewissem Ergebnis; keine Quelle durch die Stapelvorschau verändert; Check der exakten finalen Actions-Revision und später realer VDR-Test offen. Nachtlauf nicht Teil des Slice. |
| **MT.UX6 – Shrink in Aufnahmeaktionen** | **Quellseitig implementiert:** Bestehender Hero-HEVC-Plan/Stage/Review/Publish-Owner wird unter `recordings2-actions` verwendet; Aufnahmekarten besitzen zusätzlich ein `⋯`-Menü mit lesender Capability-Prüfung und direkter Navigation zum selben Owner. Kein zweiter Worker, Auftrag oder freier FFmpeg-Aufruf. | **ISOLATED PASS** für Karten-zu-Hero- und Plan/Stage/Publish-Vertrag. **REAL-VDR und Mobile/Browser bleiben NOT_VERIFIED**; Originalerhalt und tatsächliche Veröffentlichung bei einem neuen Live-Test nicht erneut behauptet. |
| **MT.UX7 – Einfachheit/Abnahme** | Technische Dropdowns erst nach nachgewiesener Parität entfernen, responsive und tastaturbedienbar, verständliche Fehler, klare leere/teilweise Scans. Isolierte UI/Agent/Core-Kontrakttests und separate read-only-Hostabnahme. | Alltag ohne Tokenkenntnis: `Quelle → Scan → Auswahl → Vorschau → bestätigen`. Keine Rechte-, VDR- oder Aufnahmeänderung durch Leseaktionen. |

## Architektur-Leitplanke: kein weiterer Backend-Agenten-Ausbau für UX/Installation

**KISS ist ein Abnahmekriterium, nicht nur ein Stilwunsch.** Die neue Installations- und UX-Phase ist **keine Erlaubnis**, neue Agenten, Worker-Dienste, Kontrollprotokolle, parallele Job-Queues, Rechte-/Freigabesysteme oder eine zweite Importpipeline zu bauen.

- **Installation und Paketierung** sind Build-/Staging-/Deployment-Aufgaben, **kein Backend-Agent-Feature**. Insbesondere kein Agent zur Selbstinstallation, Paketverwaltung oder Reparatur der Live-Rechte.
- **Scan/Explorer/Vorschau** sollen die vorhandene Backend-lokale Inventarisierung, Source-IDs und die bestehende Public-v1-/Core-/Agent-Komposition **wiederverwenden**. Für eine reine UI-Änderung niemals einen weiteren Agenten-Layer einführen. Wo lesende Information tatsächlich fehlt, zuerst die vorhandene Read-API gezielt ergänzen statt ein neues Protokoll zu entwerfen.
- **Import/Shrink** bleiben bei der bestehenden autorisierten Operation/Agent-Ausführung; diese vorhandene Sicherheitsgrenze darf nicht umgangen werden. Aber es gibt **keinen zusätzlichen Agenten** und keinen separaten Executor allein für UX.
- Eine neue Runtime-Komponente oder ein neues Wire-Protokoll ist **nicht Teil der beauftragten Slices**. Falls ein konkreter Slice ohne sie nachweisbar unmöglich ist: belegten Engpass, einfachste Alternative und Auswirkungen vorlegen und **vor Implementierung ausdrückliche neue Genehmigung** einholen.
- Pro Slice ist im Review kurz zu dokumentieren: **Welche bestehenden Module genügen? Wie viele zusätzliche Laufzeitprozesse/Protokolle werden eingeführt?** Zielwert: **0**. Verständliche Benutzerdialoge statt Backend-Agent-Details.
- Kein stilles `chmod`/`chown` auf produktiven Bestandsdateien und kein ungefragter Neustart als „Workaround“. Fehlerursache zuerst am bestehenden Owner beheben.

## Umsetzungsvorgaben

- **Beginnen bei MT.I0**, nicht mit einem neuen generischen Installationsframework oder einem unüberprüften `make install`.
- Je Slice vorhandene produktive Owner und Tests prüfen; nur die kleinste fehlende Funktion ergänzen. Kein zweiter C++-Importer, keine neue Rechteverwaltung/Operationstabelle.
- Keine `AGENTS.md`-Datei ohne ausdrücklichen Nutzerauftrag ändern.
- Build, `.deb`, Quellcommit und realer yaVDR-Test sind **verschiedene** Nachweise. Benutzer- und Systemdaten, vorhandene Eigentümer/Rechte, Aufnahmen, DB, Dienste und andere Add-ons bleiben bei Doku/Offline-Tests unverändert.
- Die Roadmap ist **keine Autorisierung**, Pakete zu installieren, VDR neu zu starten, Dateien zu importieren oder Aufnahmen zu verkleinern.

### MT.ENC3.2 – Encoding-Konfiguration (14.11.2026)

- Vier bestehende Backend-Agent-Konfigwerte, keine zweite Settings-Datenbank und kein freies FFmpeg-Argument.
- Die neue Webkarte ist von `settings-media-transcode.js` (Wiedergabe) getrennt. H.264-Import: Software-CRF 19/23/27, Default 23. HEVC-Shrink: Software-CRF 23/27/30, Default 27. Jeweils Software-Preset fast/medium/slow, Default medium. Bereits vorhandene abweichende valide Werte bleiben sichtbar und speicherbar.
- Neue H.264-Aufträge frieren das neu gelesene Profil vor dem Pinned-Executor ein; HEVC liest es vor dem unwiderruflichen Stage-Claim ein. Laufende FFmpeg-Aufträge werden nicht verändert. Speichern startet keine Konvertierung und erfordert keinen Agent-/VDR-Neustart.
- Die fixen VDR-Workerbedingungen (MPEG-TS, Main/8 Bit, yuv420p, GOP 50, kompatibles Audio, nativer Index, Größen-/Laufzeitprüfung, FFprobe-Readback, Originalerhalt) bleiben unverändert.
- VAAPI/QSV/NVENC bieten in dieser Karte keine Pseudo-CRF-Profile; ihre bereits implementierten Workerwerte bleiben bestehen. Der x265-Software-Fallback nutzt die Softwareeinstellung.
- Getrennte Belegstufen: SOURCE implementiert; ISOLATED TEST PASS erst nach echten Node-/C++-Ausgaben; REAL yaVDR PASS erst nach realem Speichern, Neustart-Readback, frischem Auftrag, VDR-/Playback-/SSE-Abnahme. Historisch veröffentlichte HEVC-Aufnahmen nicht erneut behandeln.
