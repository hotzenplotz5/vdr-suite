# Frontend Contribution Guide

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

Frontend projects should consume VDR-Suite APIs.

Principles:

- Backend first
- Stable REST APIs
- Platform independent architecture
- Reusable UI concepts

Potential frontends:

- Windows desktop
- Linux desktop
- Android
- iOS
- Web UI
- VDR OSD integration

Frontend code should avoid direct VDR communication.

Preferred flow:

Frontend -> VDR-Suite API -> Backend Services -> VDR

This keeps platform-specific code small and maintainable.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
