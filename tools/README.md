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

## Back

- [Projekt-README](../README.md)
- [Dokumentationsindex](../docs/index.md)
