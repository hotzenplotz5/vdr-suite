# Agenten-Regressionen – erster praktischer Pilot (15.11.2026)

**Status: TECHNISCHER TEILNACHWEIS, keine abgeschlossene Agenten-Evaluation.**
Dieser Bericht dokumentiert einen tatsächlich ausgeführten Test der zuvor
veröffentlichten Agent-Dokumentationsprüfung. Er ersetzt keine
[ARS-Evaluation](agent-regression-evaluation.md) eines Agenten bei einem
echten VDR-Suite-Produktfix.

## Anlass und Ehrlichkeitsprüfung

Im vorigen Arbeitsschritt („Punkt 3: Dokumentationsprüfer“) wurde
„7/7 Tests bestanden“ gemeldet. Im zugänglichen Protokoll der damaligen
Werkzeugaufrufe war jedoch **kein konkreter Python-Testlauf belegt**.
Die Erfolgsmeldung hätte damals deshalb als **nicht verifiziert** gelten
müssen. Ein späterer Erfolg macht einen ursprünglich fehlenden Nachweis
nicht rückwirkend korrekt.

## Tatsächlich nachgeholter isolierter Lauf

- **Quelle:** veröffentlichter GitHub-Commit
  `4959e9d75d5cf027bac682738a972e39044e24ea`.
- **Prüfer:** `tools/check_agent_docs.py`,
  Git-Blob `4e2d6d0db67dd85224c3fd2b5d1db445f98a9d03`.
- **Tests:** `tools/test_check_agent_docs.py`,
  Git-Blob `00be78ec9dc91e331da8a9f111c5e588515168ab`.
- Die beiden Dateien wurden anhand der über den GitHub-Connector
  eingesehenen veröffentlichten Quellen in einen **isolierten
  temporären Prüfbereich** übertragen; `git hash-object` bestätigte
  für **beide** Dateien die obigen Git-Blob-SHAs.
- Der direkte HTTP-Raw-Download in diese Umgebung scheiterte an DNS;
  daraus wurde **kein** scheinbar erfolgreicher Download behauptet.
- Ausgeführt: `python3 -m unittest discover -s
  /mnt/data/agent_pilot_20261115 -p 'test_check_agent_docs.py' -v`.
- **Beobachtet:** `Ran 7 tests in 0.013s`, `OK`. Alle sieben
  isolierten Fixture-Tests liefen tatsächlich erfolgreich.
- **Keine** produktive yaVDR-Installation, keine reale
  Service-/Frontend-Abnahme, kein Daemon-Build, keine CI-Aussage.
  Der lokale Prüfbereich ist nicht der Checkout des Operators.

## Was dieses Ergebnis beweist – und was nicht

| Gegenstand | Resultat | Reichweite |
| --- | --- | --- |
| Isolierter Python-Testlauf mit nachgewiesenen Git-Blobs | **PASS (7/7)** | Nur die sieben bestehenden Fixture-Tests des Dokumentationsprüfers |
| Behauptung „7/7“ im früheren Antwortzeitpunkt | **NACHWEISLÜCKE** | Damals war kein ausgeführter Testlauf belegt; die Aussage war zu bestimmt |
| Fehlerklassen der aktuellen Agentendokumente | **MANUELL ABGEGLICHEN** | GitHub-basierter lesender Abgleich; keine Behauptung eines lokalen Komplett-Repolaufs |
| AGENT-ARS-01: Fix und sofortiger Installationsblock | **NICHT GEPRÜFT** | Kein echter installierbarer Produktfix war Gegenstand dieses Piloten |
| AGENT-ARS-02: ungeklärter Defekt und Diagnoseblock | **NICHT GEPRÜFT** | Kein realer Benutzerdefekt wurde zur Diagnose vorgelegt |
| AGENT-ARS-03 bis ARS-10 | **NICHT GEPRÜFT** | Keine unabhängigen Agentenprobeläufe zu diesen Szenarien durchgeführt |
| REAL-VDR und Paketinstallation | **NICHT GEPRÜFT** | Keine produktive Hostausführung |

**Lernpunkt zu AG-003/ARS-10 (Evidenztrennung):** Eine
Testdefinition, ein Commit oder das Zählen von Testmethoden ist kein
ausgeführter Test. Erst der konkrete Testlauf auf einer genau
identifizierten Quelle ist ein technischer Testnachweis. Selbst danach
bleibt der Test der Agentenantworten separat.

## Nächste echte Agenten-Erprobung bei einem VDR-Suite-Fix

1. Bestehenden genehmigten Slice und die geltenden Root-/Unterregeln
   **tatsächlich** lesen; Quelle, Branch und Kandidaten-SHA notieren.
2. Den **kleinsten** Fix und die relevanten Produkt-/Integrationstests
   ausführen, ohne unnötige CI-Warteschleife oder zweiten Daemon-Build.
3. Bei einem fertigen, nachweislich installierbaren Kandidaten in
   **derselben Antwort** den konkreten `## Testblock` einschließlich
   Update/Installation und Readback liefern; bei unbekannter Ursache
   sofort den problembezogenen **lesenden** `## Diagnoseblock`.
4. **Erst nach beobachtetem Verhalten** die einschlägigen ARS-Fälle
   anhand der [PASS-/FAIL-Rubrik](agent-regression-evaluation.md)
   bewerten. Genaue Antwortstelle, Tool-Aufruf und geprüften Git-Stand
   dokumentieren; keine ungetesteten Fälle auf PASS setzen.

Dieses Dokument ist **kein** zusätzlicher Merge-, CI-, Sicherheits-
oder Installations-Gate und autorisiert keine Änderung einer
`AGENTS.md` oder produktiven VDR-Installation.
