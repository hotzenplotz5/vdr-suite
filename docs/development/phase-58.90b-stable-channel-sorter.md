# Phase 58.90b: Stable Channel Sorter

## Navigation

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)

---

Datum: 2026-07-04

## Ziel

Die Kanalsortierung im Webfrontend wurde als eigenständige Sortieroberfläche umgesetzt.

## Ergebnis

- Neues Modul: `Kanäle sortieren`
- Eigene Sortieransicht getrennt vom normalen Kanalbrowser
- Touch- und Desktop-taugliches Verschieben über Pointer Events
- Drag startet nur über den linken `↕`-Griff
- Normales Scrollen der Kanalliste bleibt auf Handy und Desktop möglich
- Kanalverschiebung nutzt die bestehende Move-API:
  - `POST /api/vdr/channels/move`
- Nach erfolgreichem Verschieben wird die Kanalliste neu geladen

## Bewusste Abgrenzung

Der experimentelle Fokus-Restore nach dem Verschieben wurde nicht übernommen.

Grund:
Der Fokus-Patch verursachte einen fehlerhaften Frontend-Zustand. Phase 58.90b enthält deshalb nur den getesteten stabilen Stand.

## Geänderte Frontend-Dateien

- `web/frontend/index.html`
- `web/frontend/app.js`
- `web/frontend/style.css`

## Nicht geändert

- `web/frontend/channel-browser.js`
- Backend/Daemon
- VDR-Adapter
- Move-API

## Teststatus

Manuell getestet:

- Handy:
  - Kanäle sortieren sichtbar
  - Scrollen funktioniert
  - Drag am linken Griff funktioniert
  - Kanalverschiebung funktioniert

- Desktop:
  - Kanäle sortieren sichtbar
  - Scrollen funktioniert
  - Drag am linken Griff funktioniert
  - Kanalverschiebung funktioniert

## Technische Entscheidung

Der Sortierer ist in `app.js` integriert, um zusätzliche statische JS-Ladeprobleme zu vermeiden.

Die normale Kanalliste bleibt unverändert.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
- [Back to Completed Phases](completed-phases.md)
