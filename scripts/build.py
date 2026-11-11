#!/usr/bin/env python3
"""Build research targets from pinned public sources; never touches game data."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("target", choices=("probe", "desktop", "arm32", "console"))
parser.add_argument("--prepare", action="store_true", help="Fetch missing pinned public sources and apply local patches")
parser.add_argument("--cmake", default=os.environ.get("D2_CMAKE", shutil.which("cmake")))
parser.add_argument("--jobs", type=int, default=4)
args = parser.parse_args()
if not args.cmake:
    parser.error("Provide CMake 3.28+ with --cmake or D2_CMAKE")
if args.target in ("arm32", "console") and not os.environ.get("D2_ZIG"):
    parser.error("Set D2_ZIG to an absolute Zig 0.17.0 executable")
deps = json.loads((root / "dependencies.json").read_text())
needed = deps if args.target in ("desktop", "console") else ("engine", "storm")

def run(*argv, cwd=root):
    subprocess.run([str(a) for a in argv], cwd=cwd, check=True)

for name in needed:
    dep = deps[name]
    folder = root / dep["path"]
    if not folder.exists():
        if not args.prepare:
            parser.error(f"Missing {name}; use --prepare")
        folder.mkdir(parents=True)
        run("git", "init", folder)
        run("git", "remote", "add", "origin", dep["url"], cwd=folder)
        run("git", "fetch", "--depth", "1", "origin", dep["revision"], cwd=folder)
        run("git", "checkout", "--detach", "FETCH_HEAD", cwd=folder)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=folder, text=True).strip()
    if revision != dep["revision"]:
        parser.error(f"Wrong revision of {name}: {revision}; preserving checkout")
    patch = root / "patches" / (name + ".patch")
    if patch.exists():
        applied = subprocess.run(["git", "apply", "--reverse", "--check", str(patch)], cwd=folder, capture_output=True).returncode == 0
        if not applied:
            if not args.prepare:
                parser.error(f"Local {name} patch is missing or different; use --prepare on the pinned source")
            run("git", "apply", "--check", patch, cwd=folder)
            run("git", "apply", patch, cwd=folder)

cross = args.target in ("arm32", "console")
build = root / "work/build" / (args.target if cross else "probe")
options = ["-DCMAKE_BUILD_TYPE=Release", "-DD2_BUILD_DESKTOP=" + ("ON" if args.target == "desktop" else "OFF"),
           "-DD2_BUILD_CONSOLE=" + ("ON" if args.target == "console" else "OFF")]
if cross:
    options += ["-DCMAKE_TOOLCHAIN_FILE=" + str(root / "scripts/arm32.cmake"),
                "-DCMAKE_C_FLAGS_RELEASE=-O2 -DNDEBUG -g0", "-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG -g0",
                "-DCMAKE_EXE_LINKER_FLAGS=-static -Wl,--strip-all"]
run(args.cmake, "-S", root, "-B", build, *options)
run(args.cmake, "--build", build, "--parallel", args.jobs)
if not cross:
    run(args.cmake, "--build", build, "--target", "test")
