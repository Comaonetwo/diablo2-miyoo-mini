# Contributing

Miyoo Mini and Mini Plus playtesting, bug reports, small fixes, tests and documentation are welcome. The current gameplay scope is single-player Acts I–IV.

## Coordinate before starting

Check GitHub Issues for an existing report or a linked pull request. Comment on the issue with the scope you intend to take before starting a substantial change. Maintainers can mark it as in progress or point you to work already under way. Link your PR to the issue and update the issue if you hand the work back. This also applies when a coding agent does the work.

A useful first contribution is an Act IV performance report on a physical device. For small code tasks, ask which open issue is available before changing targeting, animations or rendering. Those areas may already have local changes for the next release.

## Build and check

Start with [BUILDING.md](BUILDING.md). It describes the pinned dependencies, how to build from a fork checkout or a release source archive, and how to install the ARM executables. The compiler does not need game files. For an actual console session, follow the game data setup in [README.md](README.md) and use your own game files. Keep original game files, personal saves and runtime logs out of commits and release archives.

Fork the repo, create a branch for your change, and open a PR against the project's main branch. Explain how to reproduce the problem, what changed and which relevant checks you ran. For a rendering or input change, include the display or control settings and say whether you tested on an actual device. A focused regression check is enough for a small fix.

## Report a bug or performance issue

Include port version, device, Onion version, location, relevant settings, steps, and expected versus observed behavior. For frame drops, describe the room and activity and whether the slowdown is sustained or occurs during a transition. For an Act IV comparison, keep the character, route, world zoom, lighting and camera mode consistent. Attach a short clip or relevant lines from `diablo2.log` if they help. Remove personal information and avoid posting saves publicly unless they are purpose-built test saves.

## Current test priorities

| Area | Suggested contribution | Coordination |
| --- | --- | --- |
| Act IV frame drops | Record repeatable routes and device settings, ideally with the matching log excerpt | Open for independent playtesting |
| Act II packs and interiors | Report an exact spawn or room where frame pacing drops | Open for independent playtesting |
| Door and nearby NPC selection | Check both choices from several approach directions and note whether manual cursor interaction works | Maintainer has a candidate fix. Coordinate before editing |
| Animation glitches | File the class or monster, equipment, area and a short clip where possible | Open for reproduction. Coordinate before editing shared animation code |
