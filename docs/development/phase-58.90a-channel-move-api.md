# Phase 58.90a - Channel Move API

## Ziel

Diese Phase ergaenzt eine sichere Backend-Mutation zum Verschieben von VDR-Kanaelen.

Der Browser und das spaetere Drag-and-Drop-Frontend sollen channels.conf nicht direkt aendern. Stattdessen laeuft die Aenderung kontrolliert ueber VDR-Suite und den VDR-nativen SVDRP-Befehl MOVC.

## Neuer API-Endpunkt

POST /api/vdr/channels/move
POST /api/vdr/channels/actions/move

Beispiel:

{
  "backendId": "default",
  "sourceNumber": 19,
  "targetNumber": 18
}

Dry-Run:

{
  "backendId": "default",
  "sourceNumber": 7,
  "targetNumber": 3,
  "dryRun": true
}

## Architektur

ApiRouter
  -> VdrChannelMoveController
  -> VdrChannelMoveRequestParser
  -> BackendAccessPolicy
  -> VdrChannelMoveExecutionService
  -> VdrChannelMoveExecutorAdapterRegistry
  -> SvdrpChannelMoveExecutor
  -> svdrpsend MOVC <sourceNumber> <targetNumber>

## Sicherheit

- Backend-ID ist Pflicht, Default ist default.
- Read-only Backends werden ueber BackendAccessPolicy blockiert.
- Ungueltige Kanalnummern werden abgelehnt.
- Verschieben auf dieselbe Position wird abgelehnt.
- Dry-Run validiert nur und fuehrt kein svdrpsend aus.
- SVDRP wird mit Timeout ausgefuehrt.
- Nach erfolgreichem Move aktualisiert der Daemon den Channel-Snapshot.

## Validierung

Getestet wurde zuerst ein Dry-Run:

{
  "success": true,
  "dryRun": true,
  "backendId": "default",
  "sourceNumber": 7,
  "targetNumber": 3,
  "message": "dry-run: channel move command validated",
  "command": "MOVC 7 3"
}

Danach wurde ein reversibler Echt-Test durchgefuehrt:

MOVC 19 18
MOVC 18 19

Ergebnis:

- WDR HD Koeln wurde von Position 19 auf 18 verschoben.
- Danach wurde der Kanal von 18 zurueck auf 19 verschoben.
- Die urspruengliche Reihenfolge wurde wiederhergestellt.

## Naechste Phase

Phase 58.90b ergaenzt die UI:

- Sortiermodus ueber Button Kanäle sortieren
- Drag-Handle am Kanal mit linker Maustaste
- Touchpad- und spaeter Touch-taugliche Bedienung
- Drop-Ziel
- Sicherheitsdialog
- API-Aufruf an /api/vdr/channels/move
- Refresh der Kanalliste
