# Branch- und Pull-Request-Lebenszyklus

**Gültig ab 18.10.2026 für neu begonnene Arbeiten.** Diese Regel gilt für
VDR-Suite und die zugehörige Entwicklung am Fork
[`vdr-plugin-web`](https://github.com/hotzenplotz5/vdr-plugin-web).
Sie ändert keine laufenden Systeme, PPA-Pakete oder bestehenden PRs.

## Neue Arbeiten: direkt von `main`

1. Repository und **exakten aktuellen** `main`-HEAD prüfen, dann jeden neuen
   Arbeitsbranch grundsätzlich **von `main` desselben Repositories** anlegen.
   Keine neuen Arbeitsbranches von älteren Feature-, Packaging- oder Fixbranches.
2. Ein klar begrenztes Thema auf **einem** Branch mit **einem PR nach `main`**
   weiterführen. Für bloße Nacharbeiten keinen neuen Folgebranch erzeugen.
3. Eine echte technische Abhängigkeit kann ausnahmsweise einen gestapelten
   PR erfordern, aber **nur nach ausdrücklicher Freigabe**. Parent-PR,
   Basis-SHA, Grund und Plan für die spätere Rückführung auf `main`
   dokumentieren. Vor dem endgültigen Merge den integrierten Stand testen.
4. Direkte Änderungen an `main` nur bei ausdrücklicher Maintainerfreigabe
   **und nur soweit Repository-Branchschutz dies erlaubt**; Branchschutz
   nicht umgehen.

## Bestandsschutz: bereits laufende Branches

**Alle vor dieser Regel bereits begonnenen, tatsächlich laufenden Arbeiten
sind von der neuen Startbasis-Regel ausgenommen.**

- Ihre Branches und bereits vorhandene Abhängigkeitsketten dürfen
  unverändert weiterbearbeitet und abgeschlossen werden.
- **Kein Zwang zu Rebase, Force-Push, Umbenennung, Neuanlage,
  sofortigem Merge oder Schließung.**
- Weiterarbeit möglichst im vorhandenen aktiven Branch, ohne weitere
  unnötige Tochter-/Folgebranches. Neue technische Ausnahme nur nach
  ausdrücklicher Absprache samt dokumentierter Abhängigkeit.
- Vor dem letztlichen Merge nach `main` Konflikte, doppelte Änderungen,
  die tatsächliche Integrationsbasis und die passenden Tests prüfen.
- Alter allein ist **kein** Löschgrund. Geschlossene, aber *nicht gemergte*
  PRs und Branches mit einzigartigen Commits bleiben zur Sichtung erhalten.

Der Bestandsschutz gilt **nur für bereits laufende Arbeiten** und ist keine
dauerhafte Freigabe für neue verkettete Branches.

## Verbindlicher Abschluss eines PR

1. Vor dem Merge: Dateidiff und Abhängigkeiten sowie nötige CI/Tests
   auf dem **exakten Kandidaten** prüfen. Packaging- und Laufzeitrisiken
   separat beachten.
2. Nach dem Merge bestätigen: PR-Status **Merged / Closed**, tatsächlicher
   Merge-Commit und neuer `main`-HEAD; relevante Post-Merge-CI prüfen.
   Ein Merge schließt den PR automatisch.
3. Quellbranch erst dann löschen, wenn **keine Referenz mehr benötigt wird**:
   andere offene/gestapelte PRs, laufende Arbeit, Launchpad-Rezepte,
   GitHub Actions, Deployment-/Build-Automation, Tags und Releasepflege
   prüfen. Verbraucher zuerst kontrolliert auf die richtige Quelle umstellen.
4. Nach Freigabe den nicht mehr benötigten Branch löschen. GitHubs
   **Automatically delete head branches** nur aktivieren, wenn dadurch
   keine benötigten Launchpad-/Build-Referenzen ungültig werden.
   Geschützte `main`-, Upstream-, Release- und Wartungsbranches ausnehmen.
5. Status getrennt melden: `PR=MERGED/CLOSED` und
   `BRANCH=DELETED` **oder** `BRANCH=RETAINED (Grund)`.
   Falls das verbundene Werkzeug keine Branch-Löschung unterstützt,
   keine Löschung behaupten; genauen manuellen Schritt nennen.

## Nachweis der Paket-Provenienz

Ein Git-Merge ist **kein** Nachweis, dass eine schon veröffentlichte
`.deb`-Datei neu gebaut wurde. Für Builds, besonders `vdr-suite` und
`vdr-plugin-web`, Quelle/Commit-SHA, Recipe, Ubuntu-Serie, Architektur,
Paketversion, Patchserie, Dateimanifest und VDR-ABI festhalten.
Vor Branch-Löschung alle Launchpad-Recipe-Referenzen prüfen. HbbTV-Patches
dürfen nicht erneut auf bereits in `main` integrierte identische
HbbTV-Quellen angewendet werden. Bereits veröffentlichte Pakete
bleiben historische Artefakte ihrer ursprünglichen Quellen; neue Builds
benötigen ihre eigene Paket- und Installationstest-Abnahme.

## Übergangsbereinigung der alten Branches

Alte Branches **nicht pauschal löschen**. Pro Branch PR/Merge-Status,
unintegrierte Commits und externe Referenzen prüfen. Ergebnis als
`ACTIVE/GRANDFATHERED`, `MERGED/DEPENDENCY`,
`MERGED/DELETE-READY` oder `UNMERGED/REVIEW` dokumentieren.
Löschung nur für `MERGED/DELETE-READY` nach Freigabe; andere Fälle
bleiben erhalten, bis ihr Zweck bzw. ihre Abhängigkeit geklärt ist.
