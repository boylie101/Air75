# AGENTS.md — NuPhy Air75 V2 QMK Firmware

## 1. Project purpose and scope

This repository maintains QMK firmware for the **NuPhy Air75 V2** keyboard.

- GitHub repository: https://github.com/boylie101/Air75
- Original base: https://github.com/nuphy-src/qmk_firmware
- Official QMK upstream: https://github.com/qmk/qmk_firmware
- QMK documentation: https://docs.qmk.fm/

The original NuPhy fork is no longer actively maintained. The long-term objective is to keep this project continuously aligned with official QMK `master`, while retaining all Air75 V2-specific hardware support and functionality.

**The Air75 V2 is the only target keyboard for this project.** Do not spend time maintaining or adding support for other NuPhy keyboards.

## 2. Core principles

Prioritise the following, in order:

1. Preserve all existing keyboard functionality and behaviour.
2. Maintain compatibility with current official QMK.
3. Follow QMK conventions, APIs and coding standards wherever practical.
4. Keep changes focused, understandable and maintainable.
5. Document significant changes concisely.

Do not introduce unnecessary rewrites, abstractions or refactoring. Prefer incremental improvements that are easy to review and test.

## 3. Preserve existing functionality

**Existing functionality must not be removed, weakened or unintentionally changed. This is a fundamental project requirement.**

The examples below illustrate the kinds of behaviour that must be preserved; they are not an exhaustive list.

Preserve:

- The behaviour and output of every key and physical switch.
- All keymaps, layers, layer switching and modifier behaviour.
- All custom keymap functionality.
- Lighting behaviour, including which keys illuminate, when they illuminate and the colours used.
- Lighting modes, settings, effects and transitions.
- Wireless connectivity, pairing, connection indicators and related behaviour.
- Battery reporting and other hardware-specific features.
- Any other functionality present in the existing firmware.

For example:

- Holding Fn illuminates the active function layer, with particular colours assigned to specific keys.
- The keyboard is ANSI, but the operating systems are configured for UK ISO. The physical backslash key normally produces `#`; Fn+backslash must produce a backslash.
- The custom keymap provides Windows and Mac modes. Mac mode is used for Linux, not macOS.

These examples must not be treated as the complete specification. Discover and preserve all other existing behaviour in the repository.

### Implementation versus behaviour

The underlying implementation may be changed when necessary to maintain QMK compatibility, improve reliability or simplify maintenance.

However, changes to the implementation must not silently change the user-visible behaviour.

Before modifying existing functionality:

1. Inspect the relevant implementation and understand its purpose.
2. Identify dependencies and interactions with other parts of the firmware.
3. Establish which existing behaviours could be affected.
4. Preserve those behaviours in the revised implementation.
5. Explain any unavoidable behaviour changes and obtain explicit approval before proceeding.

Do not assume that functionality is unnecessary simply because it is non-standard or differs from upstream QMK.

## 4. Continuous alignment with upstream QMK

Official QMK `master` is the upstream source of truth. Do not treat the abandoned NuPhy fork as the authoritative source for future development.

At the start of each maintenance session:

1. Inspect the current repository, branch and working-tree state.
2. Check the configured Git remotes.
3. Check the latest official QMK `master`.
4. Identify how far the project has diverged from upstream.
5. Identify relevant upstream changes, including API changes, deprecated functionality, build-system changes and changes affecting the Air75 V2.

Regularly merge the latest official QMK `master` into the Air75 development branch in manageable, reviewable updates.

### Upstream Git integration strategy

Maintain a clear, traceable history consisting of official QMK upstream plus the additional Air75 V2 commits.

- Use genuine Git merges of official QMK `master` as the normal integration method. Preserve the original upstream commits and ancestry, together with all existing Air75 V2-specific commits.
- Do not substitute selective cherry-picking, manual copying or patch application for an upstream merge as the normal integration method.
- Do not rebase, squash or rewrite existing project history simply to make it appear cleaner.
- A full upstream merge includes changes to other keyboards. Retain those upstream changes as part of the integration; limit project-specific adaptations and maintenance to the Air75 V2.
- Resolve merge conflicts carefully, preserving Air75 V2 functionality and the purpose of the upstream changes.
- If a genuine merge is not possible or would introduce significant risks, explain the problem and obtain explicit approval before using an alternative integration strategy.

Fetch official `master` and record its exact commit before preparing the merge. Preparing a merge does not authorise creating its commit: use `git merge --no-commit --no-ff upstream/master` when preparing an integration for review, resolve conflicts, and run the required checks before requesting approval for the merge commit. The commit and push permission requirements in section 8 still apply.

After the approved merge is committed, verify that the fetched official `master` commit is an ancestor of the Air75 development branch:

```sh
git merge-base --is-ancestor upstream/master HEAD
git rev-list --left-right --count HEAD...upstream/master
```

The ancestry check must succeed, and the second count (upstream-only commits) must be zero. The Air75 branch should be ahead of upstream by its project-specific history, with no unintegrated commits from the fetched official `master`. Report the upstream commit integrated and the verification results; source equality alone does not establish Git integration.

When integrating upstream changes:

- Follow current QMK architecture and conventions where practical.
- Resolve conflicts carefully, paying particular attention to keyboard-specific code.
- Never blindly overwrite Air75 V2 implementations with upstream versions.
- Preserve all existing keyboard functionality.
- Avoid adding unrelated project-specific changes alongside the upstream merge.
- Keep upstream integration separate from unrelated feature changes where practical.

Flag relevant upstream changes even when the immediate task is unrelated. Do not automatically interrupt an unrelated task with a large upstream integration.

Do not allow the project to drift unnecessarily far behind official QMK.

## 5. Working practices and code changes

Before making changes:

- Inspect the current Git branch and working-tree status.
- Understand the existing code before modifying it.
- Check for uncommitted changes and preserve the user's work.
- Read relevant current QMK documentation and source code.
- Identify the smallest sensible change that achieves the objective.

During development:

- Prefer small, focused changes.
- Follow current QMK naming, formatting and architectural conventions.
- Avoid duplicating functionality already provided by QMK.
- Do not introduce unnecessary dependencies.
- Do not remove existing code or functionality simply because it appears redundant without first understanding its purpose.
- Explain significant design decisions and compatibility considerations.

Do not assume that previous development notes, remembered branch names or historical build results describe the current repository. The current repository and working tree are the source of truth.

## 6. Testing and verification

Testing is mandatory. Code must not be presented as ready to commit without the relevant checks having been run successfully.

### Before committing changes

Always run:

1. Relevant formatting checks.
2. Relevant static analysis and lint checks.
3. QMK compile checks for affected keyboard targets and keymaps.

Use the current QMK-recommended tools and procedures. Consult the current QMK documentation and repository tooling rather than relying on outdated commands.

Also run relevant QMK unit tests and other automated tests where applicable.

For changes affecting shared infrastructure, build configuration or multiple areas of the firmware, broaden the testing accordingly.

Do not claim that a check has passed unless it has actually been run.

### Reporting results

Clearly report:

- Which formatting checks were run and their results.
- Which static checks and lint checks were run and their results.
- Which compile targets and keymaps were built and their results.
- Which unit tests or other automated tests were run and their results.
- Any checks that could not be run, with an explanation.

Distinguish between automated verification and physical hardware testing. A successful compilation does not prove that the keyboard behaves correctly.

### Physical hardware testing

For changes affecting key mappings, layers, lighting, wireless functionality or other hardware behaviour, provide a concise, specific test procedure.

The procedure should identify what to test and the expected result. Make it easy for the user to verify the functionality on the physical keyboard.

Do not describe hardware functionality as verified unless it has actually been tested on hardware.

## 7. Documentation

Keep project documentation accurate and up to date.

Document significant:

- Air75 V2 hardware-specific behaviour.
- Non-standard QMK implementations.
- Intentional deviations from upstream QMK.
- Build and testing procedures.
- Hardware verification procedures.

Documentation must be **concise, clear and human-readable**.

For each substantive project-specific change commit, keep a separate, concise
note in `docs/changes/`, prepared alongside that commit. Use a stable filename
such as `YYYY-MM-DD-short-description.md` so the note can be included in the
commit it documents. Explain the problem, resulting behaviour, validation and
remaining limitations. Identify the commit in the index once its hash is known;
do not amend or rewrite existing history just to add a hash to its own note.

Keep overview documents as short indexes linking to these notes and shared
technical references. Do not append successive commit reports to one growing
audit document. Mark archived investigations as historical and leave them frozen.
Update the existing note for the same pending change rather than creating
duplicate reports. Other documentation should explain what a maintainer needs
without lengthy commentary or documenting every minor code change.

## 8. Git and branch management

Use focused branches for significant upstream updates and feature changes.

- Keep the main development branch aligned with official QMK `master` through genuine merges, preserving ancestry as described in section 4.
- Avoid mixing unrelated changes.
- Check the current branch and working-tree status before starting work.
- Never overwrite or discard existing uncommitted changes.
- Do not assume branch names or repository state from previous sessions.

### Commits and pushes

Codex may prepare changes, branches and commit messages.

**Explicit permission is required before:**

- Creating a Git commit.
- Pushing changes to GitHub.

Ask separately for permission to commit and to push. Permission to do one does not imply permission to do the other.

Before requesting permission to commit, report the changes made and the results of the required checks.

Never force-push, rewrite shared branch history or perform destructive Git operations without explicit permission.

## 9. Communication

Keep explanations concise, practical and technically accurate.

When proposing or completing work:

- Explain what changed and why.
- Highlight any potential impact on existing functionality.
- Report testing results accurately.
- Identify anything that still requires physical hardware verification.
- Explain upstream compatibility considerations where relevant.
- Flag risks or uncertainties rather than making assumptions.

The objective is a well-maintained, modern QMK implementation for the Air75 V2 that retains the functionality and behaviour the user relies on.
