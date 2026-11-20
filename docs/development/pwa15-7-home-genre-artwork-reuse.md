# PWA.15.7 – Home-Genre-Cover beim Wechsel wiederverwenden

Status: implementiert auf `main`; automatische Tests und echte Browser-Abnahme auf
yaVDR stehen noch aus.

## Reproduktion

Im vom Maintainer aufgenommenen Video bleibt die Home-Ansicht geöffnet.
Unter den Genre-Kacheln wechselt die **Home-Genre-Filmleiste** zwischen
Komödie und anderen Genres. Das bereits geladene Poster von „Ace Ventura“
wird nach Rückkehr zu Komödie erneut kurz durch einen Platzhalter ersetzt.

Dies betrifft `web/frontend/home-recording-discovery.js` und dessen
`random-genre`-Rail. Es ist **nicht** die separat geöffnete Recordings-2-
Aufnahmeordnerseite.

## Ursache und Fix

`renderRecordingRail('random-genre', ...)` versöhnt Karten anhand ihrer
Recording-ID, aber beim Genre-Wechsel wird die vorherige Rail ersetzt.
`renderState()` entfernt die Rails bereits vor dem neuen Server-Read. Die
alten `img`-Elemente gehen damit verloren.

PWA.15.7 merkt sich aus der vorhandenen Home-Genre-Leiste nur vollständig
geladene Bilder (`img.complete && img.naturalWidth > 0`) und ihre Karten.
Der In-Memory-Speicher ist auf **48 Karten** begrenzt und nach Backend
isoliert. Nach dem normalen erfolgreichen Genre-API-Read kann
`renderRecordingRail` die vorhandene Karte wiederverwenden und ihre
aktuelle Presentation über `updateRecordingCard` aktualisieren. Bei
unveränderter Poster-URL bleibt das identische Bild-DOM-Element bestehen;
eine neue URL/Revision tauscht das Cover gezielt aus. Fehlende oder
fehlerhafte Bilder durchlaufen weiter den regulären Ladeweg.

Ein Fehlerstatus der Genre-Leiste, Wechsel des ausgewählten Backends
oder Verlassen des Home-Moduls verwirft den Wiederverwendungsspeicher.
Es gibt keine neuen HTTP-Aufträge, Datenbankänderungen, Berechtigungen,
SSE-/Polling-Schleifen oder Daemon-Worker.

## Regression und reale Abnahme

Die bestehende `test_phase66_home_root_random_genre.js` prüft zusätzlich:
Komödie → Horror → Komödie mit exakt demselben „Ace Ventura“-Bildobjekt,
Aktualisierung einer geänderten Poster-Revision und Backend-Isolation.
Außerdem müssen die bestehenden Home-Frontend-Tests grün sein:

```sh
make test-phase66-recording-discovery-frontend
```

Die reale Browser-Abnahme ist separat: Home geöffnet lassen,
Komödie → Horror/Historienfilm → Komödie wechseln und beobachten, ob
„Ace Ventura“ ohne sichtbaren Platzhalter sofort wieder erscheint.
Der Browser-Network-Tab kann zusätzlich neue Image-Requests von
einem bloßen DOM-Neuaufbau unterscheiden.

## Installation

`mk/phase66-recording-discovery.mk` installiert die geänderte
`home-recording-discovery.js` als einzelne Frontend-Datei. Die
HTTP-Routing-Komposition hängt `home-recently-watched.js` serverseitig an;
deshalb wird **nur die primäre installierte Datei** ersetzt und danach
der bestehende PWA-Manifestgenerator `tools/build_pwa_shell.js`
erneut ausgeführt und geprüft. Weder ein C++-Build noch ein
Daemon-Neustart ist nötig.
