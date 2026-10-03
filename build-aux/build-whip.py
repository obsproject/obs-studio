#!/usr/bin/env python3
"""Build and stage OBS with the tested WHIP dependency and WebRTC enabled."""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True, cwd=ROOT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_whip")
    parser.add_argument("--version", required=True)
    parser.add_argument("--arch", default="x64" if platform.system() == "Windows" else platform.machine())
    parser.add_argument("--generator")
    parser.add_argument("--jobs", type=int, default=min(4, max(1, (os.cpu_count() or 2) // 2)))
    parser.add_argument("--configure-only", action="store_true")
    args = parser.parse_args()
    system = platform.system()
    build = args.build_dir.resolve()
    preset = f"windows-{args.arch}" if system == "Windows" else "macos" if system == "Darwin" else "ubuntu"
    options = [f"-DOBS_VERSION_OVERRIDE={args.version}-whip", "-DENABLE_WEBRTC=OFF", "-DENABLE_AJA=OFF",
               "-DENABLE_SCRIPTING=OFF", "-DCMAKE_BUILD_TYPE=RelWithDebInfo"]
    if args.generator:
        options += ["-G", args.generator]
    if system == "Windows":
        options += ["-A", args.arch]
    elif system == "Darwin":
        options += [f"-DCMAKE_OSX_ARCHITECTURES={args.arch}", "-DOBS_CODESIGN_IDENTITY=-", "-DENABLE_VIRTUALCAM=OFF"]
    else:
        presets = json.loads((ROOT / "CMakePresets.json").read_text())
        dependencies = next(p for p in presets["configurePresets"] if p["name"] == "dependencies")["vendor"]["obsproject.com/obs-studio"]["dependencies"]
        cef = ROOT / ".deps" / f'cef_binary_{dependencies["cef"]["version"]}_linux_{args.arch}'
        options += ["-DENABLE_BROWSER=ON", f"-DCEF_ROOT_DIR={cef}", "-DENABLE_RELOCATABLE=ON", "-DENABLE_PORTABLE_CONFIG=ON"]
    run("cmake", "--preset", preset, "-B", build, *options)
    run(sys.executable, ROOT / "build-aux/build-whip-dependency.py", "--obs-build", build, "--jobs", args.jobs)
    if args.configure_only:
        return
    # Windows capture targets share an x86 child build: serialize outer builds.
    parallel = ["--parallel", "1" if system == "Windows" else str(args.jobs)]
    if system == "Windows":
        parallel += ["--", f"/p:CL_MPCount={args.jobs}"]
    run("cmake", "--build", build, "--config", "RelWithDebInfo", *parallel)
    stage = build / "install"
    run("cmake", "--install", build, "--config", "RelWithDebInfo", "--prefix", stage)
    if system == "Windows":
        # Reconfiguring an existing OBS tree need not relink the frontend.
        dependency = build / "whip-dependency/install/bin/datachannel.dll"
        for runtime in (stage, build / "rundir/RelWithDebInfo"):
            shutil.copy2(dependency, runtime / "bin/64bit/datachannel.dll")
        (stage / "portable_mode.txt").touch()
    shutil.copy2(build / "whip-dependency/manifest.json", stage / "whip-build.json")
    (stage / "WHIP-BUILD.txt").write_text(
        "Unofficial OBS WHIP test build. Unsigned and not notarized.\n"
        "AJA and scripting are disabled; macOS virtual camera is disabled.\n"
        "Browser, WebSocket, H.264/Opus and patched WebRTC are enabled.\n"
        "Build success is not a live TURN/NAT interoperability test.\n")
    archive = build / f"obs-whip-{args.version}-{system.lower()}-{args.arch}"
    shutil.make_archive(str(archive), "zip" if system == "Windows" else "gztar", stage)


if __name__ == "__main__":
    main()
