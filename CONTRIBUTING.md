# Contributing

Miyoo Mini and Mini Plus playtesting, bug reports, small fixes, tests and documentation are welcome. The current gameplay scope is single-player Acts I–IV.

## Coordinate before starting

Check GitHub Issues for an existing report or a linked pull request. Comment on the issue with the scope you intend to take before starting a substantial change. Maintainers can mark it as in progress or point you to work already under way. Link your PR to the issue and update the issue if you hand the work back. This also applies when a coding agent does the work.

## First task for a coding agent

Use the [open contribution task](https://github.com/Comaonetwo/diablo2-miyoo-mini/issues/1) as your brief. Pick one opportunity in single-player Acts I–IV and work it through to a focused PR:

- **Performance.** Look for a costly code path or a reproducible slowdown in any act. Measure a baseline, implement a fix and compare the same scene and settings. Preserve gameplay behavior and visual output. For caching changes, report the memory cost. Label desktop and device results separately.
- **Gameplay.** Find a reproducible bug through testing or code review. Establish the expected behavior from game data or another reliable reference, then fix the cause and run a focused regression check. Describe the trigger and what now works.

Start from current main in a fork and check existing issues and PRs. Coordinate larger changes before implementing them. A local reproduction, check or benchmark can support a PR while physical device testing is pending. State what you tested and what still needs a device.

## Build and check

Start with [BUILDING.md](BUILDING.md). It describes the pinned dependencies, how to build from a fork checkout or a release source archive, and how to install the ARM executables. The compiler does not need game files. For an actual console session, follow the game data setup in [README.md](README.md) and use your own game files. Keep original game files, personal saves and runtime logs out of commits and release archives.

Fork the repo, create a branch for your change, and open a PR against the project's main branch. Explain how to reproduce the problem, what changed and which relevant checks you ran. For a rendering or input change, include the display or control settings and say whether you tested on an actual device. A focused regression check is enough for a small fix.

## Report a bug or performance issue

Include port version, device, Onion version, location, relevant settings, steps, and expected versus observed behavior. For frame drops, describe the room and activity and whether the slowdown is sustained or occurs during a transition. For a performance comparison, keep the character, route, world zoom, lighting and camera mode consistent. Attach a short clip or relevant lines from `diablo2.log` if they help. Remove personal information and avoid posting saves publicly unless they are purpose-built test saves.
