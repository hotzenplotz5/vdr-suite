# MLD 6.6 – temporärer Media-Tools-Laufzeittest (ohne BitBake-Änderung)

**Stand:** 25. November 2026 · **Quelle:** `main`.
Dieser Ablauf betrifft einen ausdrücklich ausgewählten **MLD-Testhost**.
Er installiert kein Add-on, importiert keine Aufnahme, vergibt keine
Berechtigungen automatisch und verändert keine produktive yaVDR-Installation.

## Belegte Ausgangslage

Skippys Diagnose vom 25.11.2026 ergab:

- Backend-Agent aktiv, aber `RUNNING_AGENT_OPT_IN=NO` und
  `UNIT_MEDIA_IMPORT_OPT_IN=NO`.
- Alle drei Worker installiert, beide Quellverzeichnisse vorhanden,
  `media-import.tsv` zuvor erfolgreich erstellt.
- Beide Root-Freigaben fehlen:
  `/etc/vdr-suite/addon-approvals/rectools.json` (lokaler Handler)
  und `rectools.<backend-id>.json` (Core-Erwartungswert).
- Zwölf `accountability_unavailable`-Heartbeat-Meldungen in 24 Stunden.
  Der genaue SQLite-Fehlercode wurde noch nicht gemeldet.

**Folgerung aus dem Quellcode:** Nur `--enable-media-import` erstellt
in `apps/agent/main.cpp` den `BackendAgentMediaImportRuntime` und
registriert die Media-Import-Kommandotypen. `backend-agent.conf`
braucht dafür keinen zusätzlichen Eintrag. Die installierten Binaries,
`COMMAND_TYPES` und eine Browser-Aktivierung ersetzen den Parameter
nicht. Ein Executor ohne gültige Root-Freigaben liefert noch keinen
nutzbaren Importkatalog; tatsächliche Operationen erfordern außerdem
bestehende Core-/Benutzerautorisierung.

## Schritt 1 – unveränderten Dienst prüfen

Zunächst den realen MLD-Startbefehl vor Ort lesen; dessen weitere
Argumente und Drop-ins müssen erhalten bleiben. **Keine vollständigen
Prozessargumente öffentlich posten**, falls eine Distribution vertrauliche
Parameter ergänzt.

```sh
set +e
set +u
set +o pipefail

systemctl cat vdr-suite-backend-agent.service
systemctl show vdr-suite-backend-agent.service -p ExecStart
```

Die vorhandene lesende Diagnose liegt unter
`tools/addons/diagnose_mld_media_tools_runtime.sh`. Sie wurde erweitert
und zählt im Daemon-Journal die SQLite-Rückgabecodes für
`accountability append ... failed: sqlite_rc=...`.
Diese Codes sind ein **Befund**, keine Autorisierung zu Datenbankänderungen.

## Schritt 2 – Startparameter nur für diesen Boot testen

`tools/addons/mld_media_tools_trial.sh` bietet drei Befehle:

- `check` – reine Leseprüfung, druckt keine Prozessargumente.
- `prepare` – erstellt ausschließlich
  `/run/systemd/system/vdr-suite-backend-agent.service.d/90-vdr-suite-media-import-trial.conf`
  mit `--enable-media-import` und lädt systemd neu. **Kein Dienstneustart.**
- `restore` – entfernt nur diesen exakt unveränderten Test-Drop-in
  und lädt systemd neu. **Kein Dienstneustart.**

`prepare` akzeptiert nur die belegte einfache Unit:
`/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf`.
Hat Pit weitere MLD-Startargumente eingebaut, verweigert das Hilfsskript
die Änderung. Dann die effektive Unit manuell prüfen und einen eigenen
`systemctl edit --runtime`-Drop-in erstellen, der **alle** Argumente erhält.
Nicht den produktiven `ExecStart` blind überschreiben.

Auf dem explizit freigegebenen MLD-Testhost, in einem tatsächlich vorhandenen
Suite-Checkout (Repo-Top-Level prüfen), lauten die Aufrufe:

```sh
set +e
set +u
set +o pipefail

sh tools/addons/mld_media_tools_trial.sh check
sh tools/addons/mld_media_tools_trial.sh prepare
sh tools/addons/mld_media_tools_trial.sh check
```

Der Betreiber entscheidet getrennt über einen Neustart. Ein solcher
Neustart unterbricht den Agenten und kann bereits freigegebene
Backend-Aufträge verarbeiten; daher **nicht automatisch** ausführen.
Wenn der Testhost bewusst freigegeben ist:

```sh
systemctl restart vdr-suite-backend-agent.service
sh tools/addons/mld_media_tools_trial.sh check
```

Erwartet nach einem erfolgreichen Restart:
`RUNNING_OPT_IN=YES`; die zuvor berichteten Freigabedateien fehlen
dadurch aber weiterhin. **Noch keinen Medienimport starten.**

## Schritt 3 – vorhandene Freigabeprozedur, nicht manuelle JSON-Dateien

Die geprüfte Implementierung kennt zwei getrennte Root-Einträge:
`rectools.json` für den lokalen Agenten und
`rectools.<BACKEND_ID>.json` für die erwartete Core-Handleridentität.
Der bestehende Issuer ist `tools/addons/admin_approval_issuer.py`.

Ein erstelltes MLD-Paketinventar *beschreibt* zwar
`approved_handler_sha256`; es autorisiert sich nicht selbst.
Vor dem Ausstellen der Einträge ist die **genaue tatsächlich installierte
Handler-Version 0.1.0 und deren SHA-256 aus einer unabhängig geprüften
Build-/Paketquelle** mit dem installierten Handler abzugleichen.
Die alte SHA aus Paket 13407 darf nicht ohne Vergleich für einen neueren
Nightbuild verwendet werden.

Nach dieser unabhängigen Prüfung und der gesonderten Entscheidung des
Host-Administrators kann der bestehende Issuer so benutzt werden
(`REVIEWED_SHA256` ist **kein** automatisch aus der Datei übernommener
Wert; der korrekte Backend-Name stammt aus der lokalen Konfiguration):

```sh
# NUR auf einem freigegebenen Testhost, mit überprüftem SHA-256!
# REVIEWED_SHA256 und BACKEND_ID müssen vorher explizit gesetzt werden.
python3 tools/addons/admin_approval_issuer.py issue \
  --root / --module rectools --version 0.1.0 \
  --approved-sha256 "$REVIEWED_SHA256" --origin manual --allow-live-root

python3 tools/addons/admin_approval_issuer.py issue \
  --root / --module rectools --version 0.1.0 \
  --approved-sha256 "$REVIEWED_SHA256" --origin manual \
  --backend-id "$BACKEND_ID" --core-expected-only --allow-live-root
```

`--origin manual` bezeichnet hier die vom Issuer unterstützte manuelle
Trust-Herkunft, **keinen** Ersatz für den MLD-Paketnachweis. MLD kann den
Issuer nur verwenden, wenn dessen Python-Abhängigkeiten auf dem Host
wirklich verfügbar sind; kein Nachbau der JSON-Struktur per Shell.

**Keine automatische Freigabe bei Installation oder systemd-Restart.**
Der Core muss anschließend den erwarteten Handler, eine frische
Agent-Veröffentlichung und die schon existierenden Nutzerrechte prüfen.
Erst dann darf skippy einen kleinen, entbehrlichen Testfilm wählen.
Es werden niemals Aufnahmen allein durch das Einlesen dieser Anleitung
oder einen Opt-in-Neustart gestartet.

## Schritt 4 – Rollback des Laufzeittests

Nach dem Versuch:

```sh
set +e
set +u
set +o pipefail

sh tools/addons/mld_media_tools_trial.sh restore
sh tools/addons/mld_media_tools_trial.sh check
```

Das entfernt **nur** das vom Test erzeugte /run-Drop-in; der gerade
laufende Agent bleibt bis zu einem gesonderten manuellen Neustart
unverändert. Bestehende Zustände, die der Test nicht angelegt hat,
werden nicht gelöscht. Root-Freigaben werden **nicht** stillschweigend
widerrufen: der Betreiber muss über die vorhandenen Issuer-Revoke-Befehle
separat entscheiden, wenn er den Versuch vollständig rückgängig machen
möchte.

## Accountability – getrennter Fehlerpfad

`BackendAgentLifecycleService::heartbeat()` liefert
`accountability_unavailable`, wenn das vorgeschaltete Audit-Ereignis
`agent.lease.renewed` nicht persistiert werden konnte. Der existierende
`AccountabilityEventRepository` schreibt bei SQLite-Schreibfehlern
`accountability append ... failed: sqlite_rc=N, sqlite_extended_rc=M`
ins **Daemon-Journal**, nicht ins Agent-Journal. Das erweiterte
Diagnoseskript zählt `SQLITE_BUSY_RC5`, `SQLITE_LOCKED_RC6`,
`SQLITE_CONSTRAINT_RC19` und andere Fehlerklassen anonymisiert.

Erst wenn die Codes vorliegen, ist eine konkrete Maßnahme begründbar.
`accountability_unavailable` allein beweist noch keinen SQLite-Lock.
Kein unbedachtes Datenbank-Repair, keine Löschung der Audit-Ereignisse
und kein Aufweichen der verpflichtenden Audit-Persistenz.

## Abnahme

Getrennt bestätigen: (1) Agent-Opt-in tatsächlich aktiv,
(2) lokale und Core-Freigabe vorhanden und geprüft,
(3) Quellenbeobachtung im Core verfügbar,
(4) Webfrontend zeigt die freigegebene Quelle,
(5) ausdrücklicher Testimport erzeugt eine gültige native VDR-Aufnahme
und wird per Core-Readback als abgeschlossen bestätigt.
Nur Schritt 1 und 2 genügen nicht für eine Media-Tools-Freigabe.
