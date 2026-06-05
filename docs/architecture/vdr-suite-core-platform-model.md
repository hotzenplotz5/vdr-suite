# VDR-Suite Core Platform Model

## Purpose

This document summarizes the emerging long-term core platform architecture of VDR-Suite.

It connects the architecture decisions from:

- ADR-0010: Library First VDR Architecture
- ADR-0011: VDR Source Model
- ADR-0012: Source Capability Model
- ADR-007: Platform API Strategy

The goal is to keep the architecture VDR-centered while preparing VDR-Suite for multiple VDR instances, remote VDR-Suite federation, permissions, future frontends, VDR plugins and other external consumers.

---

## Core Principle

VDR remains the primary backend domain.

VDR-Suite should not become a generic media server that merely happens to support VDR.

Instead, VDR-Suite should become a modern VDR-centered media platform.

The platform should expose VDR strengths through a modern architecture:

- recordings
- Live TV
- timers
- channels
- EPG
- metadata
- future OSD and remote-control functionality
- runtime diagnostics
- backend capabilities
- future multi-VDR access
- future AI-assisted capabilities

The guiding question is:

```text
Which VDR-Suite capabilities should be reusable by other programs?
```

This is different from building a special integration for each frontend, app or plugin.

---

## Future AI Capability Strategy

Artificial intelligence is considered a future platform capability.

AI should not be treated as a frontend feature or as a ChatGPT-specific integration.

Instead, AI should follow the same architectural principles as other platform capabilities.

Possible future AI capability areas include:

```text
AI Analysis
AI Classification
AI Search
AI Assistant
```

Potential use cases:

```text
Recording content analysis
Automatic summaries
Chapter detection
Scene detection
Metadata enrichment
Artwork generation
Speaker recognition
Person recognition
Sports event extraction
News topic extraction
Semantic search
Natural-language search
Storage recommendations
Recording cleanup suggestions
```

Examples:

```text
Show me all recordings about elections.

Show me all crime movies set in Hamburg.

Summarize this documentary.

Which recordings can safely be deleted?
```

The architecture should remain provider-neutral.

VDR-Suite must not become coupled to a specific AI vendor.

Conceptually:

```text
AI Capability
        ↓
AI Provider Boundary
        ↓
OpenAI
Ollama
LM Studio
Local Models
Future Providers
```

This follows the same philosophy already used for:

- backend adapters
- stream providers
- capability providers

The primary goal is to preserve architectural freedom.

Future AI features should be able to use:

- cloud-hosted providers
- local inference servers
- fully offline models
- future provider implementations

without changing the surrounding platform architecture.

AI is therefore considered a future platform capability rather than a frontend feature.

---

## Platform Consumer Model

VDR-Suite should expose reusable platform capabilities.

Consumers may include:

- VDR-Suite Web UI
- VDR-Suite desktop clients
- VDR-Suite mobile clients
- VDR OSD integration
- VDR plugins
- API clients
- automation tools
- monitoring tools
- external services
- third-party VDR management tools

These consumers use VDR-Suite capabilities.

VDR-Suite does not become a special integration layer for one consumer.

[Content unchanged below]