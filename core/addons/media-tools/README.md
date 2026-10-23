# Suite Core's Media Tools adapters

This subtree is the **Suite Core-owned integration boundary** for Media Tools (Rectools), not the optional add-on executable. It holds Media Import and HEVC Shrink admission, operation fences, durable shrink review persistence and maintenance checks/fencing. These classes reuse canonical Suite Core authentication, exact actor/backend grants, administrator desired state, SQLite operation identity, Agent generation and audit requirements.

General add-on registration, provider trust, administration and policy classes stay in `core/addons/`. Optional import/read-only Check/non-destructive HEVC Shrink implementations and isolated worker tests live under `addons/media-tools/`. Disabled packaging metadata stays under `modules/rectools/`. No runtime is activated by directory organization. No repair/smart_repair, cut, marks, rename or move.
