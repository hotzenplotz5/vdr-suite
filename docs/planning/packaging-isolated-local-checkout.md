# Lokaler Paketbau neben dem Codec-Branch

## Entscheidung

Die Codec-/TV-Entwicklung bleibt im vorhandenen Checkout `/home/yavdr/vdr-suite`. **Kein Merge in den Packaging-Branch und kein Branch-Wechsel im produktiven Checkout** nur zum Pakettesten. Für Packaging wird ein separater flacher Git-Clone unter `/home/yavdr/vdr-suite-packaging-test` als normaler Benutzer `yavdr` angelegt.

Der Checkout ist ausschließlich für Build, Tests und `DESTDIR`-Staging. Keine `sudo make install`-Aktionen, `apt install`-Experimente oder Neustarts von VDR/Daemon. Die Ubuntu-PPA-Abnahme erfolgt später in VM/CI, nicht über die produktive Installation.

## Sicherer Ersttest

1. Als `yavdr`, nicht `root`, anmelden.
2. Existiert das Verzeichnis bereits, **nicht** löschen, sondern Inhalt und Git-Status zuerst gesondert prüfen.
3. Vor dem Clone Speicherreserve prüfen: mindestens die größere Grenze aus 4 GiB und 15% des gesamten Dateisystems müssen *nach* einem konservativen 1-GiB-Testbudget frei bleiben. Vor kompilierenden Builds erneut mit einer eigenen Worst-Case-Schätzung prüfen.
4. Flachen Clone des Branches anlegen, HEAD mit der unmittelbar abgefragten Remote-Ref vergleichen.
5. Nur lesende bzw. temporäre Quellcode-Regressionsprüfungen ausführen:
   - `python3 tools/check_packaging_version.py`
   - `python3 tools/check_packaging_tracked_artifacts.py`
   - `python3 -m unittest discover -s tools -p 'test_packaging_*.py'`.
6. Nach Tests den Git-Status des **Packaging-Checkouts** prüfen, ursprünglichen Codec-Checkout unverändert lassen.

## Abgrenzung

- `git clone --depth 1 --single-branch --branch work/packaging-debian-ubuntu-install git@github.com:hotzenplotz5/vdr-suite.git /home/yavdr/vdr-suite-packaging-test` kopiert nur den Packaging-Branch in einen neuen Bereich. Nicht als root.
- Der SuiteBridge-Plugin-only-Paketbau steht unter `vdr-plugin-suite-bridge/BUILDING.md`; er braucht keine Daemon-Kompilierung.
- Eine wirkliche `.deb`-Erstellung benötigt Build-Depends, Verfügbarkeit von `dh_vdrplugin_depends`, ABI-Prüfung und einen isolierten Build. Nicht mit `apt install` auf dem Produktivhost vorbereiten.
- Für spätere Aktualisierung des Test-Clones zuerst `git status --porcelain`, `git branch --show-current`, `git fetch` und HEAD/Upstream vergleichen; keine pauschalen `reset --hard` oder rekursiven Löschaktionen.

Hinweis: Das ist ein Verfahren. Es **richtet nicht selbst** Verzeichnisse auf dem yaVDR-Rechner ein.
