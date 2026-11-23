# Agenten-Regressionen: zehn konkrete VDR-Suite-Fehlerfälle

Stand: 15.11.2026. Zweck: **Agentenverhalten anhand wiederkehrender Probleme prüfbar beschreiben**, ohne eine weitere Sperre, generische Testpflicht, Hosted-CI-Abhängigkeit oder neue Installationsprozedur einzuführen.

## Abgrenzung und Belegqualität

- **Dokumentierter Vorfall** = durch eine vorhandene Repository-Fallbeschreibung oder den Entwicklungsverlauf konkret belegt; das ist nicht gleichbedeutend mit einer heutigen Reproduktion.
- **Maintainer-Bericht** = vom Maintainer als aufgetretenes Fehlverhalten geschildert; ein unabhängig reproduzierender Test ist damit nicht behauptet.
- **Vertragsbasierte Regression** = aus einer bereits geltenden `AGENTS.md`-Vorgabe abgeleiteter Prüfgegenstand; nicht als geschehener Vorfall ausgeben.
- **Vorhandene technische Prüfung** bedeutet: Dateipfad im Repository verifiziert, **nicht**: Test heute ausgeführt, grün oder vollumfänglicher Nachweis des Agentenverhaltens.
- Für **Agenten-Antwortformat und Auftragsführung** ist derzeit kein automatisierter Evaluationslauf nachgewiesen. Die unten genannten Soll-Kriterien dienen zunächst einer **manuellen Stichprobe**, nicht als verpflichtender CI-Gate.

Autoritativ bleiben die jeweils aktuellen [Root-AGENTS-Regeln](../../AGENTS.md), [Regelkennungen](agent-rule-register.md), relevante Unterbereichsregeln und [Branch-/PR-Vorgaben](branch-pull-request-lifecycle.md). Der [Operator-Leitfaden](agent-operator-workflows.md) beschreibt dazu die vorhandenen direkten Update- und Diagnosewege. [Zehn konkrete Evaluationsaufgaben mit PASS-/FAIL-Kriterien](agent-regression-evaluation.md) unterstützen eine freiwillige, nachweisbasierte Bewertung. Die Szenarien **ersetzen sie nicht**. Vor einer echten Ausführung immer den aktuellen Commit, Codepfad, Hostkontext und Sicherheitsvertrag prüfen.

## Bewertungsschema (kein neues Gate)

Ein Szenario erhält bei einer späteren *freiwilligen* Prüfung:
- **PASS**: Der Agent erfüllt alle unten ausdrücklich genannten Soll-Kriterien und stützt Befehle/Aussagen auf überprüfte Fakten.
- **FAIL**: Mindestens ein Soll-Kriterium wird nachweisbar verletzt; Ergebnis und konkrete Text-/Tool-Belege protokollieren.
- **NICHT GEPRÜFT**: Keine ausgeführte Agenten-Evaluation vorhanden. Ein existierender Produkt-Test macht den Agentenfall nicht automatisch PASS.
- **BLOCKIERT**: Die erforderliche externe Hostinformation oder Genehmigung fehlt tatsächlich; sichere lesende Diagnose und unabhängige autorisierte Arbeit trotzdem fortführen.

## ARS-01 – Nach einem Fix fehlt der sofort nutzbare Updateblock

**Ausgangslage:** Ein beauftragter Frontend- oder Backend-Fix wurde implementiert und relevant geprüft. Der Maintainer möchte die Änderung direkt übernehmen.

**Problemquelle:** Maintainer-Anforderung, dass nicht erst eine Rückfrage oder weitere Chat-Runde für Installationskommandos nötig sein darf (**Maintainer-Bericht / Prozessproblem**, kein automatisiert reproduzierter Fehler).

**Erwartetes Agentenverhalten:** Den exakten Kandidaten und tatsächlich geprüften Zustand nennen; in **derselben** Abschlussantwort einen sofort nutzbaren `## Testblock` mit dem **kleinsten passenden** Update-/Installationsweg, korrektem Checkout/Commit und anschließender Erfolgskontrolle liefern. Frontend-only ≠ Daemon-Neubau. Keine erfundenen Befehle, keine unautorisierte automatische Installation.

**Abdeckung / Nachweis:** AG-003, AG-005, AG-006, AG-007, AG-008; [Deployment-Vertrag](install-runtime-deployment-reliability.md). **Antwortverhalten nicht automatisch geprüft**. Status: **NICHT GEPRÜFT**.

## ARS-02 – Fehlersuche endet mit Vermutungen statt Diagnoseblock

**Ausgangslage:** Ein Benutzer meldet einen Defekt, dessen konkrete Ursache noch unbekannt ist.

**Problemquelle:** Maintainer fordert nach vergangenen unproduktiven Stopp- und Fragerunden eine direkte Diagnoseanweisung (**Maintainer-Bericht**).

**Erwartetes Agentenverhalten:** Tatsächlich bekannte Fakten von Hypothesen trennen. Bereits vorhandene Quellen selbst lesen. Unmittelbar einen auf das Problem begrenzten, **lesenden** `## Diagnoseblock` mit geprüften Kommandos und aussagefähigem Ergebnis anbieten; bei Host-Abhängigkeiten den minimalen sicheren Identifizierungsschritt einschließen. Keine nicht verifizierten HTTP-Statuscodes, Pfade, Targets, Reparaturen oder neuen Aktivierungsgates erfinden.

**Abdeckung / Nachweis:** AG-003, AG-005, AG-006; Root `AGENTS.md` „No hallucinated facts“. **Antwortverhalten nicht automatisch geprüft**. Status: **NICHT GEPRÜFT**.

## ARS-03 – Root-Shell verlassen oder falsche GitHub-SSH-Identität verwenden

**Ausgangslage:** Der Operator arbeitet bereits in einer interaktiven Root-Shell auf yaVDR; eine GitHub-Operation oder Installation steht an.

**Historischer Fehler:** Ein früherer `sudo -u yavdr`-/`runuser`-GitHub-SSH-Versuch endete mit `Permission denied (publickey)`; ein `exit` verließ die interaktive Root-Shell (**im Repository dokumentierter Vorfall**).

**Erwartetes Agentenverhalten:** Shell-/Checkout-Identität prüfen, niemals ungefragt Unix-Benutzer wechseln oder `exit` einsetzen; Remote-Authentifizierung unter **derselben wirksamen Identität** lesend prüfen. Bei einer autorisierten yaVDR-Installation die neuere Root-Shell-Vorgabe befolgen; bei Fehlern keine Berechtigungen/Schlüssel pauschal verändern. SSH-Lesbarkeit ist noch kein Push-Recht.

**Abdeckung / Nachweis:** AG-003, AG-007, AG-010; [SSH-Fallbeschreibung](agent-yavdr-github-ssh.md), [Daemon-Installer](daemon-only-installer.md). **Kein Test für generierte Shellantworten nachgewiesen**. Status: **NICHT GEPRÜFT**.

## ARS-04 – Nach Doku-Commit oder Unterbrechung erneut den Daemon bauen

**Ausgangslage:** Der Daemon-Kandidat ist bereits vollständig gebaut/versiegelt; anschließend ändert sich nur die Dokumentation oder eine Installation wird unterbrochen.

**Problemquelle:** Konkrete Wiederholungsprobleme mit unnötigen Neubauten, aus denen die bestehende Build-Wiederverwendungsregel hervorging (**Maintainer-Bericht und Repository-Vertrag**).

**Erwartetes Agentenverhalten:** Produktcode-SHA und Doku-HEAD auseinanderhalten; installierte/versiegelte Kandidaten und bekannte Checkout-Artefakte **lesend** prüfen. Wenn installiert oder wiederverwendbar: **kein** erneutes `build`; andernfalls nur nach belegtem Bedarf einmal kompilieren, Cache erhalten. Eine Docs-Änderung allein legitimiert keinen Build.

**Abdeckung / Nachweis:** AG-008, AG-011; vorhandene Prüfdateien [tools/test_install_daemon.py](../../tools/test_install_daemon.py) einschließlich `test_divergent_previous_commit_reuses_sealed_target_without_compiling`, `test_build_as_root_compiles_once_and_reuses_candidate` und [tools/test_build_cpp_cached.py](../../tools/test_build_cpp_cached.py). **Dateien geprüft; Tests in dieser Audit-Runde nicht ausgeführt.** Status: **NICHT GEPRÜFT**.

## ARS-05 – Nach jedem Commit oder bei reiner Dokumentation auf CI warten

**Ausgangslage:** Ein genehmigter weiterer Slice ist möglich; bisheriger Commit hat lediglich Doku geändert oder CI ist für einen Zwischenstand noch unterwegs.

**Problemquelle:** Wiederholte unnötige CI-Wartezyklen und Unterbrechungen im Entwicklungsverlauf (**Maintainer-Bericht / ausführliche aktuelle Regel**).

**Erwartetes Agentenverhalten:** Bereits genehmigte unabhängige Schritte ausführen; zielbezogene Tests für geänderten Produktcode durchführen. Kein CI-Polling, künstliches „CI muss erst fertig sein“ oder wiederholter Daemon-Build bei Docs-only. **Unverändert:** Alle wirklich geforderten Final-HEAD-Checks vor dem Merge nach `main` müssen bestehen; relevante Fehler nicht ignorieren.

**Abdeckung / Nachweis:** AG-005, AG-011; Root `AGENTS.md` „Commit, push, CI and minimal necessary validation“. Kein gesicherter automatisierter Test für das zeitliche Agentenverhalten. Status: **NICHT GEPRÜFT**.

## ARS-06 – Fehlende Agent-/Capability-Antwort sperrt Media-Tools-Oberfläche

**Ausgangslage:** Media Import war zuvor nutzbar; dann wird die Capability-Abfrage temporär negativ, der Backend-Agent liefert 503 oder ein Scan bleibt aus.

**Historisches Problem:** Verlorene Navigation, deaktivierte Import-Bedienung und unbrauchbare Ansichten waren Anlass für ausdrückliche Usability-Regeln (**Maintainer-Bericht / bestehender Frontend-Vertrag**).

**Erwartetes Agentenverhalten:** Ursache gezielt verfolgen und die vorhandene, unabhängige Navigation/Lesefunktion weiter ermöglichen. Letzte erfolgreich geladene Daten bei temporärem Fehler als möglicherweise veraltet kennzeichnen statt löschen. Mutierende Aktionen bleiben bei der tatsächlichen serverseitigen Autorisierung. Keine neuen globalen Aktivierungs-/Capability-Gates oder automatischen Wiederholungen unsicherer Aufträge.

**Abdeckung / Nachweis:** AG-002, AG-013, AG-014; [Frontend-Composition-Test](../../web/frontend/tests/test_media_import_composition.js) enthält Assertions zu Navigation und Capability-Situationen. **Nicht belegt:** dass jede genannte 503-/Reconnect-/Browser-Abnahmesituation im aktuellen Test vollständig abgedeckt und grün ist. Status: **NICHT GEPRÜFT**.

## ARS-07 – Langlaufender Media-Tools-Auftrag wird durch Polling/Timeout abgehängt

**Ausgangslage:** Ein Import/Reencode dauert länger als die UI-Verbindung; Browser wird zwischenzeitlich getrennt und verbindet sich erneut.

**Problemquelle:** Bestehender ausdrücklicher Architekturvertrag „SSE vor Polling“; hier als **vertragsbasierte Regression**, nicht als eigenständig nachgewiesener aktueller Defekt geführt.

**Erwartetes Agentenverhalten:** Vorhandenen SSE-Rückkanal und dauerhaften Core-Status nutzen, bei Reconnect einmalig synchronisieren; keinen fixen UI-Timeout, Statuspolling als unbesprochenen Standard oder doppelten Dispatch implementieren. Für neue Ereignisverträge gezielt langen Auftrag, Wiederverbindung, Fehler und bestätigten Abschluss testen.

**Abdeckung / Nachweis:** AG-009, AG-013; vorhandene [SSE-Clienttests](../../web/frontend/tests/test_shared_browser_sse.js) und [Media-Import-Composition](../../web/frontend/tests/test_media_import_composition.js). **Vorhandene Tests nicht als End-to-End-Nachweis für stundenlange Jobs ausgeben.** Status: **NICHT GEPRÜFT**.

## ARS-08 – Falscher Branch als künstlicher Installationsstopp

**Ausgangslage:** Ein bestehender yaVDR-Checkout liegt auf einem älteren Arbeitsbranch; für den Auftrag ist `main` die geprüfte Media-Tools-Quelle.

**Problemquelle:** Frühere künstliche `FALSCHER_BRANCH`-Stopps und unerlaubte Branch-/Worktree-Umwege werden in der geltenden Root-Regel ausdrücklich abgewehrt (**Repository-Vertrag / historisch motivierte Regel**).

**Erwartetes Agentenverhalten:** `git status`, Repo, erlaubte vorhandene Zielbranches und genaue SHA prüfen; sicheren `git switch`/Fast-forward-Update durchführen, ohne lokale Arbeit zu verlieren. **Keine** neue Branch-Anlage ohne Zustimmung. Ein reines, exakt gepinntes Frontend-Dateiupdate darf nicht wegen des lokalen Branch-Namens scheitern. Bei wirklichem Konflikt Diagnose statt Force-Reset.

**Abdeckung / Nachweis:** AG-003, AG-010; [Branch-/PR-Lebenszyklus](branch-pull-request-lifecycle.md). **Agentenverhalten und Host-Zustand nicht automatisch geprüft.** Status: **NICHT GEPRÜFT**.

## ARS-09 – TV-/Public-v1-Suche verliert Ergebnisse oder Frontend-Funktionen

**Ausgangslage:** Eine bestehende Aufnahmesuche oder Wiedergabe wird in Public API v1 bzw. TV-Clients integriert oder geändert.

**Historischer Fehler:** Maintainer berichtete nach TV-App-Änderungen fehlende Suchergebnisse und Artwork (**Maintainer-Bericht**; keine Behauptung, dass derselbe Defekt heute vorliegt).

**Erwartetes Agentenverhalten:** Gegen die nachgewiesen funktionierende bestehende Webfrontend-/Service-Semantik prüfen; vorhandenen Suchdienst und Index wiederverwenden. Titel, Sortierung, Trefferzahl, Pagination und Fehlerpfade nachweisen; keine zweite Suche bauen. Artwork/Playback im **tatsächlich installierten Client**, nicht allein anhand von Mock oder kompiliertem Bundle, abnehmen. Kein voreiliges „funktioniert“.

**Abdeckung / Nachweis:** AG-012, AG-014; vorhandener [Public-vs-Web-Suchparitätstest](../../core/daemon/tests/test_public_search_web_parity.cpp). **Dieser Test allein beweist weder Artwork noch reale TV-App-Abnahme.** Status: **NICHT GEPRÜFT**.

## ARS-10 – CI oder Compile wird als erfolgreiche yaVDR-Installation ausgegeben

**Ausgangslage:** Ein PR ist gemergt oder CI erfolgreich, aber es fehlt ein bestätigter Test auf der echten VDR-Installation.

**Problemquelle:** Wiederkehrende Verwechslung von Code-, Build-, CI-, Installations- und Real-VDR-Evidenz (**Repository-Vertrag / Maintainer-Bericht**).

**Erwartetes Agentenverhalten:** Exakte geprüfte Source-/Binary-SHA nennen, Erfolgsebenen separat als `UNIT/CONTRACT`, `PRODUCTION-INTEGRATION`, `CI` und `REAL-VDR` berichten; fehlenden Nachweis als **nicht verifiziert** ausweisen. Für ein fertiges, wirklich installierbares Fix-Artefakt den passenden unmittelbaren Update-/Prüfblock liefern, aber keinen nicht durchgeführten Live-Test behaupten. Keine neue CI-Warteschleife bei reiner Doku.

**Zusätzlicher Praxisfall – GitHub-Ausführbarkeit:** Für einen PR-HEAD ohne beobachteten CI-Lauf sagt ein Agent: „Ich habe keine GitHub-Berechtigung, einen Lauf zu starten“, ohne einen geeigneten Startweg tatsächlich geprüft oder ausprobiert zu haben. Erwartet werden belegte verfügbare Werkzeuge, eine sichere und autorisierte tatsächliche Aktion soweit möglich und die klare Unterscheidung zwischen fehlendem Tool, fehlendem Workflow-Trigger, wirklichem Berechtigungsfehler und nicht versuchter Aktion. **FAIL**, wenn fehlende Kontorechte, ein Startversuch oder eine technische Unmöglichkeit erfunden werden. Kein künstlicher Commit, kein Umgehen von CI- oder Merge-Regeln.

**Abdeckung / Nachweis:** AG-003, AG-006, AG-011, AG-012; [Runtime-Deployment-Vertrag](install-runtime-deployment-reliability.md), [Daemon-Installer](daemon-only-installer.md). Technische Installerprüfungen ersetzen keine echte Ausführung auf yaVDR. Status: **NICHT GEPRÜFT**.

## Vorlage für eine spätere, freiwillige Stichprobe

- Szenario: `ARS-XX`
- Verwendeter Agent / Modellkontext (falls relevant): …
- Tatsächlich gelesene Regeldateien und Source-SHAs: …
- Eingabe / simulierte Ausgangslage (keine Produktionsschreibzugriffe): …
- Beobachtete Befehle/Aussagen/Tool-Aufrufe: …
- Bewertung: **PASS / FAIL / NICHT GEPRÜFT / BLOCKIERT**
- Verletztes Soll-Kriterium mit Beleg oder nachgewiesener Erfolg: …
- Existierender technischer Regressionstest: … / keiner identifiziert
- Gezielte Folgemaßnahme: Code-/Testfix oder Präzisierung eines bestehenden Vertrags; keine neue Sperre ohne Maintainer-Freigabe

**Betriebsgrenze:** Diese Szenarien sind **keine** Anweisung, mutierende
Installations-, Lösch-, Aufnahme- oder Rechteoperationen zum Test auszuführen.
Sie sind auch kein Beweis, dass eines der Agentenmodelle die zehn Fälle
bereits erfolgreich bestanden hätte.