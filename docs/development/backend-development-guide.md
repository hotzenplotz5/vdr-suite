# Backend Development Guide

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Architecture Map](architecture-map.md)

---

## Purpose

This guide explains the backend development direction for VDR-Suite.

---

## Core Building Blocks

- BackendNode
- BackendRegistry
- SnapshotCache
- SnapshotAccessService
- SnapshotChangeFeed
- VDR Adapters

---

## Development Rules

- Keep backend-neutral abstractions in the domain layer.
- Keep backend-specific behavior inside adapters.
- Prefer snapshot-based reads.
- Preserve capability-based routing.
- Add tests for new backend behavior.

---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
