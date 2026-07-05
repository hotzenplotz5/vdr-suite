# Phase 58.94c: RESTfulAPI Timer Conflict Discovery

## Ziel

VDR-Suite bindet die epgsearch-Timerkonfliktprüfung über RESTfulAPI an.

## Quelle

RESTfulAPI liefert Timerkonflikte über:

    /searchtimers/conflicts.json

Beispiel:

    {
      "check_advised": false,
      "conflicts": [
        "1783260840:14|84|11#12#14#13",
        "1783261500:13|33|11#12#14#13"
      ],
      "count": 2,
      "total": 2
    }

## VDR-Suite-Endpunkte

    /api/vdr/timer-conflicts/live
    /api/vdr/timers/conflicts/live

## Umsetzung

- VdrTimerConflict und VdrTimerConflictReport als Domain-Objekte
- RestfulApiTimerConflictMapper für RESTfulAPI-Konfliktzeilen
- RestfulApiVdrAdapter::getTimerConflictReport()
- VdrService::getTimerConflictReport()
- VdrSnapshotReadJsonSerializer::serializeTimerConflictReport()
- VdrController::getLiveTimerConflicts()
- ApiRouter-Routen für den Live-Konfliktbericht

## Architekturentscheidung

RESTfulAPI ist die primäre Quelle.
SVDRP PLUG epgsearch LSCC bleibt Gegencheck und möglicher Fallback für spätere Phasen.
