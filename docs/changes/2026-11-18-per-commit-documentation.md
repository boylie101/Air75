# Separate documentation for each change commit

Documentation-only change; existing Git history is preserved.

The single NKRO audit mixed upstream integration, several feature commits,
historical findings and later test results. It has been replaced by a short
index and individual change notes under `docs/changes/`.

Each note identifies the relevant commit or pending status, explains its change,
and records validation and remaining limits. A shared architecture reference
avoids repeating the full transport explanation. The preceding audit, including
the ACK fix's then-pending documentation, is retained as a frozen archive.

`Agents.md` now requires a separate note for each substantive project-specific
change commit. Use a stable date/topic filename and prepare the note alongside
its code. Add known commit IDs to the index without amending existing history.

Validation: relative Markdown links and referenced commits were checked, the
archived content was compared with the pre-split audit, whitespace checks passed,
and the existing RF source/test files were verified unchanged. Firmware checks
from the preceding ACK change remain its validation; no new builds are needed
for this documentation-only reorganisation.

[Change index](../AIR75_V2_NKRO_AUDIT.md)
