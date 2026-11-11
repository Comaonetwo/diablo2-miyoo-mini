# Local release checks

Use the existing native build for iteration. Do not rebuild ARM and sanitizer
variants after every small edit. At the final source state, build the sanitizer
and console variants once. Native performance measurements must run without
compilers, the ARM emulator, or other benchmarks competing for the CPU.

Freeze shared headers before starting final variant builds. Do not edit class layouts while a compiler is still reading them: an overlapping build can finish an old-layout object after the header timestamp changes. If that happens, rebuild every dependent translation unit after all earlier builds stop, before running release checks.

Run `scripts/release_checks.py --profile renderer --cmake /path/to/cmake
--output work/release-N/checks.json` for changes confined to decoding/blitting.
The runner builds its selected directory, executes all fast CTests, compares sprite pixels against the scalar reference, and runs one heavy Diablo combat
smoke with sanitizers. `--no-build` is only for an already completed build.

Use `--profile gameplay` whenever simulation, saves, commands, inventory or
controls change. This also runs the potion, inventory/death/shop, QA-route and
cube scenarios. A render-only release does not repeat those five entire game
scenarios or the full mercenary-equipment interaction suite.

A passing result can be reused only with the same source/test fingerprint,
compiled test/game hashes, profile, sanitizer options, fixtures and private
archive metadata. Logs remain under `work/check-cache/`; the result explicitly
marks reuse. Failed runs are never reused. `--force` always executes the checks.
Archives are immutable local owner inputs; packaging separately verifies their
full hashes. Replacing an archive while preserving its size and timestamp
requires `--force`.

For ARM, run the changed arithmetic/boundary tests and one relevant scene once
against the final binary. Qiling runs must be sequential because they share a
root filesystem, framebuffer and input queue. Unsupported Linux calls must
return ENOSYS, not a false success (notably copy_file_range, syscall 391).
An emulator timeout is not a passing test and is not automatically an OOM.

Before SD installation, take a fresh backup of the installed DiabloIIResearch
files and fingerprint saves/configuration/logs. After installation, verify every
installed hash, protected hash and the exact state-file set. Keep these checks
for every release: they protect the user's progress. Never install test-mutated
QA saves over physical progress. Never touch other games or the automation.

Package and verify corresponding source and an asset-free update once, after
code checks pass. Do not create another full private game-data ZIP by default.
Source-only documentation changes do not require another gameplay run. A code
change after verification invalidates the result and requires relevant checks
again. Console FPS and physical stability require the user's console test.

For day-to-day work, read the latest HANDOFF and new feedback rather than
replaying historical release logs. Keep native experiments outside production
headers until measurements justify them. Do not print full emulator JSON or
compiler logs into the conversation: report counts, the important failure and
the saved log path. Use a single final verification pass after code settles;
repeat only the affected checks after a new change or an unresolved failure.

For measured render changes, use scripts/render_profile.py --output work/release-N/profile.json
with --mode compare-tiles or compare-shadows. It uses isolated fixture copies,
freezes both time and mutable rain, alternates old/new order per frame, and
requires full-frame hashes to match for every pair. Report median paired ratios
alongside phase shares; isolated helper speedups are not frame speedups. Device
logs now sample one frame in four and report draw/update/present plus nested
DCC decoding. Never add nested decode time to its parent rendering phase.
