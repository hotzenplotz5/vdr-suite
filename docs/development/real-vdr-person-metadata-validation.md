# Real VDR Person Metadata Validation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Person API](person-api.md)

---

## Purpose

This document records the Phase 46.25 validation findings for real VDR person metadata sources.

The goal is to determine whether VDR-Suite can populate the existing person query API from real VDR, RESTfulAPI, TVScraper or scraper2vdr metadata.

This phase is intentionally a validation and architecture phase.

It does not add a new endpoint, database table or runtime importer.

---

## Validated Sources

The following source areas were reviewed:

- vdr-plugin-restfulapi recording export
- RESTfulAPI recording JSON structure
- RESTfulAPI scraper2vdr bridge
- TVScraper service interface
- scraper2vdr-compatible actor metadata structures

---

## RESTfulAPI Recording Export

The RESTfulAPI recording JSON structure exposes recording and event-adjacent metadata.

The current recording structure contains fields such as:

- number
- name
- file_name
- relative_file_name
- inode
- is_new
- is_edited
- is_pes_recording
- duration
- filesize_mb
- channel_id
- frames_per_second
- marks
- event_title
- event_short_text
- event_description
- event_start_time
- event_duration
- additional_media
- aux
- sync_action
- hash

Direct person fields are not present at the top level.

There are no direct recording fields such as:

- actors
- directors
- writers
- producers
- composers
- cast
- crew

Therefore VDR-Suite must not assume that plain RESTfulAPI recordings provide person metadata directly.

---

## RESTfulAPI Event Metadata Attached To Recordings

RESTfulAPI derives the event-adjacent recording fields from the VDR recording event information.

The exported fields are useful for title and description search.

They are not sufficient for deterministic person search.

The following fields may contain human-readable text that could mention people, but they are not structured person metadata:

- event_title
- event_short_text
- event_description
- aux

VDR-Suite should avoid treating these fields as authoritative actor or director data.

They may be used later only as weak textual evidence.

---

## RESTfulAPI Additional Media

RESTfulAPI can attach additional media to recordings through its scraper bridge.

The relevant path is:

    cRecording
    -> Scraper2VdrService::getMedia(recording, additionalMedia)
    -> additional_media

The additional media structure contains actor records.

An actor record contains:

- name
- role
- thumb

This is the first validated real person metadata path for recordings.

The source should be treated as scraper-provided metadata, not as native VDR metadata.

---

## scraper2vdr / TVScraper Bridge

RESTfulAPI looks for a scraper plugin in this order:

1. scraper2vdr
2. tvscraper

If either plugin is available and can identify the event or recording, RESTfulAPI can enrich the response with additional media.

This means VDR-Suite can potentially import actors from existing RESTfulAPI JSON without requiring a new RESTfulAPI endpoint.

The initial mapping is:

| RESTfulAPI additional_media field | VDR-Suite person field |
| --- | --- |
| actors[].name | originalName |
| actors[].role | characterName |
| actors[].thumb | provider/image evidence |
| scraper type | source |
| movie/series identifiers | providerReference candidate |

The actor role in this structure is the character role, not the VDR-Suite PersonRole enum.

Therefore RESTfulAPI additional_media actors should map to PersonRole::Actor.

The actor role text should map to characterName.

---

## TVScraper Modern Character Interface

The modern TVScraper service interface exposes richer character metadata.

It defines character types including:

- director
- writer
- actor
- guestStar
- crew
- creator
- producer
- showrunner
- musicalGuest
- host
- executiveProducer
- screenplay
- originalMusicComposer
- others

It also exposes a scraper video object for events or recordings.

That object can return characters through getCharacters(...).

This is the best long-term data source for complete cast and crew import.

Unlike the older RESTfulAPI additional_media actor array, this interface can represent directors, writers, producers and composers.

---

## Current Person Query API Status

The routed person query API exists:

    GET /api/persons
    GET /api/vdr/persons

The current router still supplies an empty PersonCollection.

Therefore the API currently validates query parameters and returns a stable JSON contract, but does not yet search real data.

Current person query pipeline:

    ApiRouter
    -> PersonController
    -> PersonSearchService
    -> PersonQueryMatcher
    -> PersonQueryResultJsonSerializer

---

## Validation Result

Native RESTfulAPI recording fields:

- Structured person data: no

RESTfulAPI recording event fields:

- Structured person data: no
- Weak textual evidence: yes

RESTfulAPI additional_media:

- Structured actors: yes
- Structured directors: not in the currently exported legacy additional_media actor structure

scraper2vdr-compatible service:

- Structured actors: yes

modern TVScraper service:

- Structured actors: yes
- Structured directors: yes
- Structured writers: yes
- Structured producers: yes
- Structured composers: yes
- Recording-aware lookup: yes
- Event-aware lookup: yes

---

## Architectural Decision

VDR-Suite should not parse names out of descriptions as the primary person search mechanism.

The recommended implementation order is:

1. Import actor data from RESTfulAPI recording additional_media.
2. Map additional_media actors to PersonRole::Actor.
3. Preserve movie or series IDs as provider reference candidates where available.
4. Add a later TVScraper adapter for the modern cScraperVideo character interface.
5. Use the modern TVScraper interface for directors, writers, producers and composers.

---

## Recording Search Implication

Recordings should become part of person search.

The first useful user-facing query should be:

    Which recordings contain actor X?

The initial implementation can search only actors from additional_media.

Later implementations can expand to:

- directors
- writers
- producers
- composers
- guest stars
- hosts
- recording-to-event inherited metadata
- multi-backend person search

---

## EPG Search Implication

EPG person metadata is also technically possible.

RESTfulAPI can retrieve scraper media for events as well as recordings.

However, EPG person search should be handled after recording person search because:

- recordings are the more valuable immediate user use case
- recordings have stable identity and paths
- recordings can later be indexed persistently
- EPG windows are transient and selective

---

## Out of Scope For This Phase

This phase does not implement:

- RESTfulAPI JSON parser changes
- Person import from additional_media
- TVScraper service adapter
- recording person index
- database persistence
- real VDR smoke test target
- API response changes
- frontend changes

---

## Recommended Next Phases

Recommended next implementation phases:

1. Phase 46.26 - Recording Additional Media Person Import
2. Phase 46.27 - Recording Person Search Result Model
3. Phase 46.28 - Actor Search Over Recordings
4. Phase 46.29 - TVScraper Character Adapter
5. Phase 46.30 - Director Search Over Recordings

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
- [Back to Person API](person-api.md)
