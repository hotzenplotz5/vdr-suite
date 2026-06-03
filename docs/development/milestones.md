# VDR-Suite – Milestones

This file keeps tag and milestone history out of `docs/development/current-status.md`.

## Existing milestone tags

### v0.1-phase0

Project foundation and documentation.

### v0.2-phase1-core

Core architecture and persistence.

### v0.3-phase1-workflow

Workflow layer.

### v0.4-phase2-worker

Worker lifecycle.

### v0.5-phase2-queue

Queue-style processing.

### v0.6-phase3-dashboard

Job dashboard service.

### v0.7-phase4-recording-dashboard

Recording dashboard service.

### v0.8-phase5-dashboard-facade

Dashboard facade.

### v0.9-phase6-json

Dashboard JSON serialization.

### v1.0-phase6-cli

Dashboard CLI application.

### v1.1-phase6-rest-skeleton

Initial REST API foundation and project structure.

### v1.2-phase7-router

API router.

### v1.3-phase7-jobs-api

Jobs API route.

### v1.4-phase7-recordings-api

Recordings API route.

### v1.5-phase7-metadata-api

Metadata API route.

### v1.6-phase8-daemon-foundation

Daemon foundation.

### v1.7-phase8-vdr-adapter-foundation

External VDR integration foundation.

### v1.8-phase8-vdr-config

VdrConfig architecture.

### v1.9-phase8-vdr-adapter-interface

IVdrAdapter abstraction layer.

### v1.10-phase8-vdr-adapter-factory

VdrAdapterFactory implementation.

### v1.11-phase8-vdr-mock-backend

Mock backend implementation.

### v1.12-phase8-vdr-backend-architecture

Backend-independent VDR backend architecture.

### v1.13-phase8-restfulapi-architecture

RESTfulAPI integration architecture.

### v1.14-phase8-http-abstraction

HTTP abstraction layer.

### v1.15-phase8-restfulapi-adapter

RESTfulAPI VDR adapter foundation.

### v1.18-phase8-vdr-event-domain-object

VDR event domain object.

### v1.19-phase8-vdr-event-adapter-architecture

VDR event adapter architecture.

### v1.42-phase8-library-first-vdr-architecture

Media platform comparison and Library First VDR architecture.

## Current verified work after v1.42

The repository continued with additional Phase 8 work after the last documented milestone tag.

Verified current work includes:

- PollingService interface
- PollingService implementation
- VdrSnapshot
- VdrSnapshotBuilder
- modular Makefile include conversion
- VDR test target extraction into `mk/vdr-tests.mk`
- duplicate VDR test target cleanup pending as Phase 8.80

## Tagging rule

After code phases, create a milestone tag when the build has been verified.

Documentation-only split commits may remain untagged unless they mark a larger architectural milestone.
