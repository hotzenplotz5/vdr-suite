# VDR Live Reimplementation Study

## Goal

Evaluate how much code can be eliminated when a Live-like frontend is implemented on top of VDR-Suite.

## Traditional Architecture

A traditional VDR Live implementation typically contains:

- VDR integration logic
- timer access logic
- recording access logic
- EPG access logic
- HTTP handling
- state synchronization
- permission handling
- UI rendering

## VDR-Suite Architecture

The frontend consumes:

- /api/vdr/overview
- /api/recordings
- /api/jobs
- /api/runtime/summary
- future event endpoints
- future capability endpoints
- future permission endpoints

The frontend therefore focuses primarily on presentation.

## Expected Benefits

- significantly reduced frontend complexity
- reusable backend services
- consistent behavior across clients
- easier testing
- easier maintenance

## Long-Term Goal

Create a small Live-like reference implementation and compare:

- code size
- architectural complexity
- maintenance effort
- testing effort

This serves as a practical demonstration of the value of the VDR-Suite backend architecture.
