#!/usr/bin/env python3
"""Apply only this fork's WHIP changes to an upstream OBS ref for compatibility builds."""

import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
# Last OBS upstream merge in this fork. Keep protocol changes separate from
# unrelated OBS development when testing stable and prerelease versions.
UPSTREAM_BASE = "cffa83ba5"
WHIP_FILES = ["plugins/obs-webrtc/obs-webrtc.cpp", "plugins/obs-webrtc/whip-output.cpp",
              "plugins/obs-webrtc/whip-output.h", "plugins/obs-webrtc/whip-utils.h"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ref", required=True)
    parser.add_argument("--destination", type=Path, required=True)
    args = parser.parse_args()
    destination = args.destination.resolve()
    if destination.exists():
        parser.error("destination must not exist; existing worktrees are preserved")
    subprocess.run(["git", "-C", str(ROOT), "fetch", "--no-tags", "https://github.com/obsproject/obs-studio.git", "tag", args.ref], check=True)
    revision = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "FETCH_HEAD"], text=True).strip()
    patch = subprocess.check_output(["git", "-C", str(ROOT), "diff", "--binary", UPSTREAM_BASE, "--", *WHIP_FILES])
    subprocess.run(["git", "-C", str(ROOT), "worktree", "add", "--detach", str(destination), revision], check=True)
    subprocess.run(["git", "-C", str(destination), "apply", "--check", "--ignore-space-change", "-"], input=patch, check=True)
    subprocess.run(["git", "-C", str(destination), "apply", "--ignore-space-change", "-"], input=patch, check=True)
    for name in ("build-whip-dependency.py", "build-whip.py"):
        shutil.copy2(ROOT / "build-aux" / name, destination / "build-aux" / name)
    shutil.copytree(ROOT / "build-aux/whip-dependency", destination / "build-aux/whip-dependency")
    shutil.copytree(ROOT / "test/whip", destination / "test/whip")
    subprocess.run(["git", "-C", str(destination), "submodule", "update", "--init", "--recursive", "--jobs", "4"], check=True)
    print(f"Prepared {args.ref} ({revision}) with the current WHIP patch at {destination}")


if __name__ == "__main__":
    main()
