# Media Tools add-on ownership

Root `../../AGENTS.md`, `../../modules/AGENTS.md`, and
`../../modules/rectools/AGENTS.md` security constraints apply.

- Follow the maintainer-authorized branch and scope for the current work.
  A review branch name is not a permanent restriction after merge. Do not
  create an unapproved parallel workstream, repository or duplicate import
  implementation.
- Add-on-specific code lives here; Suite-owned execution authorization,
  authenticated dispatch and durable lifecycle remain in Core/Agent.
- No arbitrary filesystem paths, implicit source deletion, shell
  interpolation, legacy repair, cut/marks or unreviewed shrink operations.
- Nothing here can self-register an executable command or grant a user right.
- No automatic live install, VDR restart or recording mutation.

- The C++17 import implementation was moved here from `modules/rectools/` without a second copy; keep its functional and security tests intact.
