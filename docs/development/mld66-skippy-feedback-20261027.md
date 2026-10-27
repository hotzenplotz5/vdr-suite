# MLD 6.6: Rückmeldungen skippy, 27.10.2026

Branch: `work/mld66-skippy-feedback-20261027`, ab `main` `cc6cc4678fb10a4376d931c21fbf2ae14e2d7038`.
Dies ist ein **separater MLD-Teststand**, keine allgemeine Freigabe, kein MLD-Paket und kein nachgewiesener Hardwaretest. `main` und der bisherige Integrationsbranch bleiben unverändert.
Verbindlicher Slice-Plan für diesen Testbranch: [Phase MLD.F1](../planning/mld66-feedback-phase-f1.md).

Referenz: [MLD-Kompatibilitätsinventar](mld66-compatibility-inventory.md),
[Public-v1-Suchtimer-Vertrag](../../api/search-timers.md),
[Medienwiedergabevertrag](frontend-playback-integration-contract.md).

## Rückmeldungen und prüfbare Änderungen

| Rückmeldung | Branch-Stand | Abnahme auf MLD |
| --- | --- | --- |
| Admin kann Fernbedienung steuern | Vom Tester vorläufig bestätigt; hier unverändert | Nicht erneut als Branch-Fix ausgeben |
| Kanallogos | Vom Tester bestätigt; hier unverändert | Kein neuer Test nötig |
| Timer-Vor- und Nachlauf | MLD-Paket fehlt laut Tester noch | Erst mit passender paketierter Version testen |
| Suchtimer-Anlage: „public API resource temporarily unavailable“ | **Quell-Diagnose erweitert; REAL-MLD offen**: Die möglichen 503-Vorbedingungen sind jetzt unterscheidbar | MLD-Provider/Agent/Bridge-Version und einen vorhandenen Operationstatus lesend prüfen; kein CREATE mit neuem Schlüssel, solange Ausgang unklar |
| Vollständiger Suchtimer-Editor und direkte Aktivierung | **TEILWEISE**: Optionen wieder sichtbar; Public-v1 unterstützt noch kein verlustfreies Full-Field-SAVE und keine Browser-Aktivierung | Nur inaktive Textsuche speicherbar; native epgsearch-Generation-/Revision-CAS und Browser-Autorisierung plus ID-/ETag-Readback fehlen für direkte Aktivierung |
| Live-Vorschau `live_backend_authority_required` | **OFFEN**: Quellpfad verlangt eine gültige Agent-/Backend-Instanz vor Provider-Auswahl | Backend-ID/Agent-Onlinezustand und installierte Bridge-/Live-Provider-Fähigkeit auf MLD prüfen, danach Live-Wiedergabe |
| Berechtigungsauswahl | **IM BRANCH IMPLEMENTIERT**: Sichtbare Optionen berücksichtigen bereits wirksame Grants pro ausgewähltem Backend. Globale Administratorrolle blendet normale Rechte aus; unabhängig autorisierte Add-on-Rechte bleiben getrennt | Rolle und Backend wählen; doppelte Standardrechte verschwinden; Adminrechte und Berechtigungsmutationen bleiben unverändert servergeprüft |
| Aufnahme abspielen | **OFFEN**: MLD-MediaSession, Aufnahmequelle, FFmpeg und Browser-Profil noch nicht zur Fehlermeldung korreliert | Eine nicht vertrauliche Beispielaufnahme wählen, nachvollziehbaren Session-/Playback-Fehler und verwendeten Medienpfad diagnostizieren; keine Aufnahme löschen/verändern |
| Plugin-/Installationsübersicht | Vorläufige Funktionsmatrix unten | Effektiv installierte MLD-Rezepte, ABI und Fähigkeiten gegen lokale Paketmetadaten prüfen |
| Suchtimer-Treffervorschau zeigt Kanal-ID | **IM BRANCH IMPLEMENTIERT**: Vorschautreffer werden mit derselben Backend-Kanalliste in Klartext aufgelöst; bei fehlender Kanalliste bleibt die Originalkennung als Fallback sichtbar | Einen bekannten Treffer mit Sender-ID anzeigen und Klarnamen prüfen; Kanallistenfehler muss die Treffervorschau weiterhin erlauben |

**Zusatz F1.1 (27.10.2026):** Nach HTTP 202 mit einer gültigen
dauerhaften Operation-ID kann der Browser jetzt die actor-gebundene
`GET /api/v1/operations/{operationId}`-Route über
„Anlagevorgang prüfen“ lesen. `dispatching` wird ausdrücklich nicht
mit `succeeded` verwechselt; bei ausstehendem/unbekanntem Ergebnis
wird **kein** zweiter CREATE ausgelöst. Isolierter UI-DOM-Test inklusive
zweimaligem Status-Readback bestanden, MLD-Runtime weiterhin offen.

**F1.3 Quellabschluss (Branch-Arbeitsstand):** Ein Suchtimer wird über
die vorhandene angemeldete Browser-Route mit erweiterten Optionen direkt
aktiv oder inaktiv angelegt; der Browser-Readback vergleicht Native-ID,
Status und tatsächlich beobachtbare Einstellungen. Der Backend-Adapter
weist Anfragen für ein anderes als sein konfiguriertes Backend vor dem
Schreiben ab. Real-MLD-Abnahme und kompiliertes CI-Build sind noch offen.
Siehe [Phase MLD.F1](../planning/mld66-feedback-phase-f1.md).

**Aktueller F1.1–F1.3-Befund:** Siehe [Phase MLD.F1](../planning/mld66-feedback-phase-f1.md) für die eingecheckten Diagnose-/Editor-Änderungen und den noch offenen aktivierenden Native-CAS-Pfad. 

**Test-Evidenz:** JS-V8-Modultest der Kanalnamensauflösung (inklusive bestehender Workflow-Assertions) und DOM-Mock der Berechtigungsauswahl (inklusive bestehender Mutationstests) bestanden. Der vollständige Preview-Fetch-Pfad wurde noch nicht end-to-end getestet. Noch **nicht bestätigt:** nativer Node-/CI-Lauf des Branch-HEAD, MLD-Kompilierung und Paketierung, reale MLD-Installation sowie reale VDR-Operationen.

## Was gehört zur VDR-Suite-Installation?

Die folgenden Zeilen beschreiben **Funktionen**, keine verifizierten MLD-Paketnamen. „Optional“ heißt, dass die Suite ohne diese Spezialfunktion lauffähig sein kann; welche MLD-Rezepte tatsächlich verwendet werden, muss die MLD-Paketierung festlegen.

| Baustein | Wozu? | Einordnung |
| --- | --- | --- |
| VDR | Tuner, Kanäle, EPG, native Timer und Aufnahmen | Basis |
| VDR-Suite Daemon und Webfrontend | Oberfläche, Konten/Berechtigungen, API und Wiedergabe-Orchestrierung | Basis der Suite |
| VDR-Suite Backend-Agent | Beobachtung und gebundene Ansteuerung eines VDR-Backends | Benötigt, wo der produktive Backend-/Live-Weg ihn verwendet |
| VDR SuiteBridge | Native VDR-Fähigkeiten und kontrollierter Agent-Transport | Abhängig vom aktiven Integrationsweg; ABI passend zum MLD-VDR |
| RESTfulAPI | Privater VDR-Datenprovider für bestehende Integrationen | Abhängig vom aktiven Integrationsweg; **nicht** automatisch öffentliche Client-API |
| epgsearch | Native epgsearch-Suchtimer | Erforderlich für epgsearch-Suchtimer |
| TVScraper | Zusätzliche Film-/Serieninformationen und Artwork | Optional |
| Kanallogos | Logos im EPG und in Senderlisten | Optionale Ressourcen |
| OSDTeletext / Videotext-Anbindung | Videotextansicht | Optional; separates generisches `teletext`-Plugin nicht pauschal voraussetzen |
| Streamdev bzw. anderer unterstützter Live-Provider | Live-TV-Zulieferung | Nur wenn der konkret installierte Suite-Live-Weg dies nutzt |
| FFmpeg / ffprobe | Prüfung/Aufbereitung von Medienformaten | Für die jeweils verwendete MediaSession-/Transcode-Funktion erforderlich |

Keine pauschale Plugin-Nachinstallation als Fehlerbehebung: Erst ermitteln, welche Fähigkeiten der konkrete MLD-Build nutzt. Fremde private Provider dürfen nicht als Ersatz für die Suite-Authentifizierung offen im Netz erreichbar sein.

## Test-Reihenfolge für die MLD-Abnahme

1. Exakten installierten Git-/Paketstand des Daemons, Frontends, Agenten, SuiteBridge und der relevanten VDR-Plugins festhalten; Quelle der Webdateien mit Branch-HEAD vergleichen.
2. Zuerst **lesend** im Browser kontrollieren: Kanallogos, Suchtimer-Treffer mit Klarnamen sowie rollen- und backendabhängige Berechtigungsauswahl.
3. Suchtimer-CREATE-Fehler zunächst nur an vorhandener Operation/Backend-Readback diagnostizieren. Keine Wiederholung der unklaren Anlage und kein breit treffender aktiver Suchbegriff als Test.
4. Agent-/Backend-Zuordnung für Live-TV und MediaSession-/Codecpfad für eine einzelne Aufnahme getrennt diagnostizieren.
5. Timer-Vor-/Nachlauf erst nach bestätigter MLD-Paketaktualisierung mit kontrolliertem Test prüfen.

Produktive Änderungen an Timern, Aufnahmen, Rechten, Diensten oder MLD-Paketen werden durch diese Dokumentation **nicht** ausgelöst.
