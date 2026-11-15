# VDR-Suite – AGENTS-Widerspruchsaudit (15.11.2026)

**Gegenstand:** Vollständiger Git-Tree `main` vom 15.11.2026
(`git/trees/main?recursive=1`, `truncated=false`, 4.424 Einträge).
Darin waren **zehn** `AGENTS.md`-Dateien enthalten. Jede wurde
inhaltlich über GitHub abgerufen und mit der Root-Vorgabe verglichen.
Der Snapshot ist dokumentiert; die Aussagen sind kein pauschaler Beweis,
dass alle künftigen Commits oder technischen Laufzeitwege konfliktfrei sind.

## Geprüfte Dateien und Snapshot-Blob-SHAs

| Datei | Blob-SHA beim Audit |
| --- | --- |
| `AGENTS.md` | `665e66d916d64308314bb481ae087bf11a41395a` |
| `addons/media-tools/AGENTS.md` | `caed3d33012abd7b32925361f8f759987dd6b497` |
| `core/addons/AGENTS.md` | `7feea874526d8165586e350f91e50633ff62da66` |
| `core/addons/media-tools/AGENTS.md` | `da5834cf7335b4a1616dfae5e2d3a9069c4fc662` |
| `debian/AGENTS.md` | `72f4a6f96c8dc785a2d34d78387faec3052b2c49` |
| `modules/AGENTS.md` | `57dcee6e9890c87da87453d4f8f571660d3b3068` |
| `modules/image/AGENTS.md` | `0e6982f2d800ad6d577f85f7b69133e17c468877` |
| `modules/music/AGENTS.md` | `d8e82009b5e393a45e35ad338b730f70685b5957` |
| `modules/rectools/AGENTS.md` | `b08eded54787baa37af914adb42201cc856f95b5` |
| `vdr-plugin-suite-bridge/AGENTS.md` | `07c7bb40d910b9f528ecbdb0974cc7cc43c99faa` |

Zusätzlich abgeglichen: `CONTRIBUTING.md`, der
[Branch-/PR-Lebenszyklus](branch-pull-request-lifecycle.md) und
[Daemon-Installer](daemon-only-installer.md).

## Befunde

| Status | Fund und tatsächliche Einordnung | Maßnahme |
| --- | --- | --- |
| **Widerspruch in angrenzender Doku** | `CONTRIBUTING.md` empfiehlt uneingeschränkt einen „fresh branch“. `docs/development/branch-pull-request-lifecycle.md` fordert bei neuen Arbeiten ebenfalls die Branch-Anlage von `main`. Die spätere Root-`AGENTS.md` verlangt jedoch ausdrückliche Maintainer-Genehmigung für **jeden neuen Branch**. | Contributor- und Branch-Lebenszyklus-Text klarstellen: **Falls** neue Branch-Anlage genehmigt, von aktuellem `main`; sonst auf bestehendem autorisiertem Branch arbeiten. |
| **Scope-Klärung, kein belegter Widerspruch** | `debian/AGENTS.md` fordert gewöhnliche Benutzerprivilegien für isolierte Paket-/Source-Builds; die Root-`AGENTS.md` fordert Root-Shell für beauftragte **yaVDR-Installationen**. Die Root-Datei lässt Clean-Paketbuilds ausdrücklich als separaten Prozess zu. | Bereich unterscheiden. Keine automatische Änderung von `debian/AGENTS.md`; beim konkreten Paket-/Host-Workflow beide lesen. |
| **Scope-Klärung** | `modules/rectools/AGENTS.md` fordert Speicherplatzprüfung für bestimmte Import-/Remux- und isolierte Paket-Arbeiten. Die Root-Datei verbietet **willkürliche pauschale** Speicherplatzreserven (z. B. feste GiB- oder Prozentgrenzen). | Nur belegte Ressourcenanforderungen des konkreten Vorgangs werten, keine neue pauschale Installationssperre ableiten. Eine technische Änderung bedarf eigenen Auftrags. |
| **Kein unmittelbarer Widerspruch** | `modules/AGENTS.md`, `core/addons/AGENTS.md` und die Media-Tools-Unterregeln verlangen bestehende Rechte-/Handler-/Grant-Prüfungen. Die Root-Regel „keine **neuen** Sperren“ hebt bestehende Autorisierung ausdrücklich **nicht** auf. | Vor jeder neuen oder verschärften Blockierung AG-002 einhalten; vorhandene Autorisierung nicht entfernen. |
| **Potenzielle Überbreite für spätere Features** | `modules/rectools/AGENTS.md` schließt mehrere Legacy-Funktionen strikt aus; `addons/media-tools/AGENTS.md` übernimmt diese Regeln. Ob das mit späteren ausdrücklich gewünschten Media-Tools-Funktionen kollidiert, hängt vom konkreten Auftrag und tatsächlichem Codepfad ab. | Bei der betreffenden neuen Funktion gezielt prüfen; keine Abschwächung oder eigenmächtige Sperre im Rahmen dieses Dokumentationsaudits. |
| **Bisherige Antwortregel unpräzise** | Root-`AGENTS.md` verlangte für jede Arbeitsantwort einen `## Testblock`, sagte aber nicht, dass nach **Fix** unmittelbar Update-/Installationsbefehle und bei **Fehlersuche** direkt lesende Diagnosekommandos zu liefern sind. | Root-Antwortregel präzisieren; nur belegte Befehle, kein unnötiger Nutzer-Rückfragezyklus, keine erzwungene neue Kompilierung. |
| **Kein direkter Konflikt** | `vdr-plugin-suite-bridge/AGENTS.md` verlangt unabhängigen Plugin-Build mit eigenem `DESTDIR`; Root-Installationsregeln unterscheiden bereits Projekt-Build, isolierten Paketbau und Live-Deployment. | Unverändert belassen. |

## Grenzen des Audits

- **Kein Produktcode** und keine bestehende Berechtigungs-/Installationslogik
  wurden für diesen Audit geändert oder auf einem realen yaVDR getestet.
- Kein zusätzlicher blockierender CI-Job und keine weitere Freigabeschleife
  wurden angelegt.
- Die Git-Tree-Inventur ist vollständig für den festgehaltenen Snapshot;
  eine spätere neue `AGENTS.md` muss in der nächsten Inventur ergänzt
  werden.
- Bei einer späteren Konfliktlösung die konkreten betroffenen Dokumente,
  den nachgewiesenen Auslöser und die aktuelle Maintainer-Genehmigung prüfen;
  nicht aus dem Audit automatisch Produktänderungen ableiten.
