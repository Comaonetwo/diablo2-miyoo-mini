#!/usr/bin/env python3
"""Relink private diagnostic executable with function symbols for fatal PC/LR logs.
Run after the normal ARM CMake build with D2_ZIG set. Symbols are non-loadable;
keeping them adds file bytes, not another game framebuffer/cache allocation.
"""
from pathlib import Path
import os,shlex,subprocess
root=Path(__file__).resolve().parents[1];build=root/'work/build/console';args=shlex.split((build/'CMakeFiles/d2-desktop.dir/link.txt').read_text());args=[arg for arg in args if arg!='-Wl,--strip-all'];at=args.index('-o');dest=build/'d2-desktop';temp=build/'d2-desktop-symbols.tmp';args[at+1]=str(temp)
subprocess.run(args,cwd=build,check=True);temp.chmod(0o755);os.replace(temp,dest)
