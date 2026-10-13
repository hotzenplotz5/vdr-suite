# Tools — Repository-Prüfungen und Operator-Hilfen

## Navigation

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
- [Entwickler-Onboarding](../docs/development/developer-onboarding.md)
- [Dokumentationsstandards](../docs/development/documentation-standards.md)
- [Installation und Staging](../docs/development/install-runtime-deployment-reliability.md)

## Rolle dieses Verzeichnisses

`tools/` enthält zahlreiche Python-/JavaScript-Hilfen für Quellcode-, Architektur-, Dokumentations-, Build- und Integrationsprüfungen. **Nicht jedes Skript ist ein ungefährlicher Checker.** Manche Werkzeuge schreiben Dateien, generieren Artefakte, verändern einen Checkout oder werden nur im Rahmen einer getrennt autorisierten Runtime-Operation verwendet. Der Dateiname allein ist kein Freibrief zur Ausführung.

## Konkrete, lesende Prüfeinstiege

| Werkzeug | Zweck |
| --- | --- |
| [`check_docs.py`](check_docs.py) | Überschriften und relative Links unter `docs/` |
| [`check_doc_indexes.py`](check_doc_indexes.py) | Dokumentationsindex-Konventionen |
| [`check_doc_reachability.py`](check_doc_reachability.py) | Erreichbarkeit kanonischer Doku-Seiten |
| [`check_public_v1_openapi.py`](check_public_v1_openapi.py) | Begrenzter Quell-/OpenAPI-Driftcheck für geprüfte Public-v1-Routen |
| [`check_phase69_public_api_inventory.py`](check_phase69_public_api_inventory.py) | Historische/aktuelle API-Pfadinventur mit expliziten Public-v1-Grenzen |
| [`audit_make_test_inventory.py`](audit_make_test_inventory.py) | Inventarisierung der Make-Testziele |

Die zugehörigen Tests liegen unter [`tests/test_public_v1_openapi.py`](tests/test_public_v1_openapi.py), weitere Testziele unter [`mk/maintenance-tests.mk`](../mk/maintenance-tests.mk). Der vorhandene CI-Dokumentationsjob verwendet `make test-docs`. Ein Commit allein belegt **keinen** abgeschlossenen CI-Lauf; der neue Driftguard erfasst derzeit bewusst nur die auditierten OpenAPI-Operationen.

## Deploy- und schreibende Helfer

[`install_runtime_stage.py`](install_runtime_stage.py) sowie [`test_install_runtime_stage.py`](test_install_runtime_stage.py) gehören zur versiegelten Runtime-Staging-/Deployment-Grenze; den Workflow beschreibt der [verbindliche Installationsvertrag](../docs/development/install-runtime-deployment-reliability.md). Solche Tools sind **nicht** als pauschale Reparaturbefehle für ein laufendes yaVDR-System gedacht. Vor Verwendung Quell-SHA, Make-Ziel, Ressourcen, Seiteneffekte, Eigentümer und konkrete Freigabe prüfen.

## Codec-Referenztest (externes Testmaterial)

[`codec_reference_compare.py`](codec_reference_compare.py) vergleicht einen **explizit ausgewaehlten** MPEG-TS-Aufnahmefall mit einem oeffentlichen Big-Buck-Bunny-Referenzencode. Der Standardfall ist HEVC Main 1080p24 gegen den bereits dokumentierten Medicus-II-Fall. Weitere, **nicht automatisch geladene** Referenzprofile sind H.264 High 1080p24 und MPEG-2 480p24. Die URLs und Originalgroessen sind im Skript fest verankert (Quelle: [LaFibre.info](https://lafibre.info/tv-numerique-hd-3d/comparer-h-264-vp9-av1/msg1006867/)); ihre aktuelle Erreichbarkeit und die identischen ffprobe-Metadaten werden auf dem Zielsystem vor dem Vergleich geprueft. Ein nicht erreichbarer oder veraenderter Download fuehrt zu einem klaren STOP, nicht zu einem anderen Download.

Als vorhandener Terminal-Benutzer ausfuehren (auch bestehende Root-Shell erlaubt); das Tool selbst fuehrt **keinen Benutzerwechsel und keine Berechtigungs- oder Eigentuemer-Aenderung** durch. Keine Service-Operation, kein VDR-Indexzugriff, kein Speichern in `/tmp`, keine Aenderung von Quellaufnahmen. Alle Referenzdownloads und Remux-Samples liegen in einem einzigen privaten, automatisch entfernten Arbeitsverzeichnis auf dem angegebenen Scratch-Dateisystem (standardmaessig `/home/yavdr`). Vorab muss genuegend freier Platz mit 15 % bzw. mindestens 4 GiB Reserve vorhanden sein. Der Download ist groessen- und zeitbegrenzt (HEVC ca. 138 MB). Ein Unterbruch kann nur Dateien im ausschliesslich neu erzeugten Arbeitsverzeichnis betreffen. Start ohne `sudo`.

```bash
python3 tools/codec_reference_compare.py --list
python3 tools/codec_reference_compare.py --reference bbb-hevc-1080p24 --known medicus
python3 tools/codec_reference_compare.py --reference bbb-hevc-1080p24 --known solo
python3 tools/codec_reference_compare.py --reference bbb-h264-1080p24 --known angel
python3 tools/codec_reference_compare.py --reference bbb-h264-1080p24 --known 1917
```

Weitere Quell-TS-Dateien koennen mit `--source /absoluter/pfad/00001.ts` mehrfach angegeben werden; das ist kein rekursiver Gesamtscan. `--skip-remux` deaktiviert den zusaetzlichen temporaren fMP4-Vergleich. Die Auswertung protokolliert Codec, Profil, Pixelformat, Frameraten, PTS-/DTS-Statistik und **exakt identische benachbarte dekodierte Frames** an zwei Positionen. Das ist nicht gleichbedeutend mit effektiven Display-fps oder einem Beweis fuer einen Codierungsfehler. Unterschiedliche Film- und Bewegungsszenen duerfen nicht als identische Bildinhalte verglichen werden. DTS (Decode Time Stamps) wird vom Audio-Codec DTS getrennt behandelt; ein H.264/AAC-Referenzclip kann beispielsweise nicht die DTS-5.1-Audioverträglichkeit von `1917` beweisen. Die vollstaendigen Checks pro realem Client benoetigen spaeter eigene Playback-Acceptance.

Fokussierter Test ohne Medien/Netzwerk: `python3 -m unittest discover -s tests -p test_codec_reference_compare.py`.


## Medicus II: Fuenf-Stellen-Originalanalyse ohne Downloads

Das eigenstaendige Skript [\`analyze_hevc_frame_cadence.py\`](analyze_hevc_frame_cadence.py) liest **nur** die MPEG-TS-Originalaufnahme \`Historienfilm/Der_Medicus_II/2026-05-18.19.00.1-0.rec/00001.ts\`. Es waehlt fuenf auseinanderliegende, je 10 Sekunden lange Intervalle aus der vom **ersten TS-Segment** gemeldeten Dauer (ungefaehr 2 %, 25 %, 50 %, 75 %, 95 %). Bei mehrteiligen VDR-Aufnahmen wird damit nicht automatisch die gesamte Aufnahme abgedeckt. Der Container muss eine plausible Dauer melden; ungueltige oder ueberlappende Testfenster fuehren zu einem klaren STOP.

Pro Zeitfenster werden durch **FrameMD5 exakte dekodierte Pixelgleichheiten** zwischen benachbarten Bildern ermittelt. Ausgegeben werden Anzahl, Anteil, Zeitverteilung und Laengen von Wiederholungsserien sowie separate Paket-PTS-/DTS-Abstaende. Das hilft zu unterscheiden, ob sich identische Bilder in einem Standbild konzentrieren oder regelmaessig ueber bewegte Szenen verteilen. Es beweist allein **keine** falsche Encoder-Einstellung oder reale Panel-framerate. Die ffprobe-Suchintervalle koennen bei MPEG-TS an benachbarten Keyframes beginnen und sind deshalb nicht zwingend framegenau mit den FFmpeg-Fenstern identisch.

Das Skript schreibt **keine** Aufnahme- oder Mediendateien, laedt nichts herunter und erstellt keine MediaSession. Es arbeitet in der vorhandenen Shell-Identitaet, auch bei Root, und enthaelt weder \`sudo\` noch \`su\`, \`runuser\`, \`chown\` oder \`chmod\`. Es unterbricht keine Dienste. Pro Fenster sind maximal 270 Frames, 150 s FFmpeg- und 45 s ffprobe-Laufzeit erlaubt. Ein Start aus einer interaktiven SSH-Shell soll in einer isolierten Subshell mit Verifikation eines unveraenderlichen GitHub-Commits erfolgen.

Fokussierter Test ohne Medien/Netzwerk: \`python3 -m unittest discover -s tests -p test_hevc_frame_cadence.py\`.

## Back

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
