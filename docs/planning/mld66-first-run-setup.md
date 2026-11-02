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
| **F2.1 – Lokalen Installationsfall erkennen** | Nach normalem Admin-Claim vorhandene Backend-/SuiteBridge-Fakten lesend prüfen: genau ein lokaler VDR oder bereits konfigurierte/mehrere Backends; keine neue harte Paketabhängigkeit. Bestehende funktionierende Konfiguration wird nicht verändert. | Auf frischem Einzel-VDR wird eindeutig `setup_required` erkannt; auf bestehender Installation `already_configured`; bei unklarer Topologie kein automatischer Eingriff. | **OFFEN** |
| **F2.2 – Agent automatisch einmalig registrieren** | Im von der Installation kontrollierten lokalen privilegierten Einrichtungskontext die vorhandene Agent-Konfiguration aus tatsächlichem Backend-/Control-Plane-Kontext nutzen; vorhandenes Enrollment-Werkzeug aufrufen, geschütztes Paket an den bereits vorgesehenen Ort legen, vorhandene Identität niemals überschreiben. Ein einmaliger Setup-Schritt nach erfolgreichem Admin-Claim, kein offenes anonymes Setup-API. | Derselbe Backend-ID-/Datenbank-/Konfigurationskontext, einmalige Registrierung, korrekt geschützte Identity, keine ausgegebenen Secrets und kein zweiter Agent bei Wiederholung. | **OFFEN** |
| **F2.3 – Dienststart und Kanalbeobachtung** | Vorhandenen Agent-Dienst nach erfolgreichem Enrollment einmalig starten; lokale Senderbeobachtung auf die tatsächlich verwendete MLD-`channels.conf` konfigurieren und Provider-Fakten prüfen. Erst bei Online-/Backend-Health-Readback den vorhandenen Autostart-Helfer nutzen. Keine pauschalen Neustarts von VDR oder Daemon. | Agent online, gültige Lease, passende `BackendAgentRecord`, aktueller `channels`-Cursor, Autostart nach Hostneustart; kein neuer Systemdienst. | **OFFEN** |
| **F2.4 – Web-Setup-Abschluss und Wiederaufnahme** | First-Admin-Webablauf um tatsächlichen lokalen Backend-Fortschritt ergänzen. Kein technischer Fragebogen; ggf. ein verständlicher Fehler mit `Erneut prüfen`, ohne doppelte Registrierung oder erneute Kontoanlage. Bereits eingerichtete Systeme kommen direkt zur normalen Oberfläche. | Ein Setup-Ablauf im Browser; Fehler lassen sich ohne Konsole erneut prüfen; Authentifizierung und bestehende Admin-Session bleiben erhalten. | **OFFEN** |
| **F2.5 – Distribution und Paketierung** | Bereits vorhandene Dateien/Tools über gemeinsame Runtime-/Package-Contracts installieren; nur Service-User, VDR-ABI, Senderpfad und privilegierten lokalen Setup-Auslöser im MLD-Paket anpassen. Ubuntu/yaVDR mit bestehendem Agent bleibt unverändert. | MLD-/Ubuntu-Paketinhalt, Rechte, Bootverhalten und Upgrade ohne Identity-/DB-Verlust geprüft; kein Debian-Skript in MLD kopiert. | **OFFEN** |
| **F2.6 – Echte Abnahme mit skippy** | Einen installierten, SHA-identischen MLD-Testkandidaten nach Installationsfreigabe gemeinsam testen. Danach F1.4 Live-TV mit Start, Senderwechsel, Bild/Ton und Stop. | `FIRST_ADMIN=PASS`, `LOCAL_BACKEND_SETUP=PASS`, `AGENT_ONLINE=PASS`, `CHANNEL_OBSERVATION=PASS`, `LIVE_TV=PASS` auf echter MLD-Installation; Fehler getrennt dokumentieren. | **OFFEN** |

## Umsetzungslinien / Nicht-Ziele

1. **Erstbestandsaufnahme vor Mutation:** Die vom Installer verwaltete lokale Instanz und ihr Dienstkonto müssen tatsächlich bekannt sein. Keine automatische Registrierung auf unklaren Mehrbackend-/Remote-Installationen.
2. **Keine neue Sicherheits- oder Architektur-Schicht:** vorhandene administrative Enrollment- und Authentifizierungsfunktionen benutzen; falls deren Privileggrenze einen automatischen Browserauslöser nicht zulässt, zuerst den bestehenden installerseitigen Ausführungskontext nachweisen, statt einen zweiten öffentlichen HTTP-Endpunkt zu erfinden.
3. **Kein manueller Endnutzer-Workaround als Fertigmeldung.** Diagnose-Skripte sind nur Entwickler-/Testwerkzeuge.
4. **Isolierte Evidenz:** Quellcode-, CI-, Paket- und REAL-MLD-Ergebnisse getrennt berichten. Ein geändertes Frontend ist nicht gleich vollständige Auto-Einrichtung.
5. **Kein Media-Tools-Code, kein Main-Merge und keine Installation auf yaVDR oder MLD durch diese Roadmap.** F1.4 wird nach F2.3 wieder aufgenommen.

**Abhängigkeit:** Das automatische privilegierte Setup muss dort starten können, wo das MLD-Paket die Agent-Konfiguration und Service-Rechte tatsächlich verwaltet; genau dieser Paket-/Laufzeit-Auslöser ist bisher nicht nachgewiesen. F2.1/F2.2 dürfen daher nicht durch ein unautorisiertes Frontend-`systemctl` ersetzt werden.
