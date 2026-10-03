#!/usr/bin/env python3
"""Build the pinned WHIP dependency after configuring OBS, on any supported OS."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess

REVISION = "4e4f4892dccb2a57fe3a490d0c9d958de4244e74"
JUICE_REVISION = "5948a4162d37bc213d6051b67ee2876ccc5a99a6"
ROOT = Path(__file__).resolve().parent.parent


def run(*args, **kwargs):
    subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def capture(*args):
    return subprocess.check_output([str(arg) for arg in args], text=True, encoding="utf-8").strip()


def cache_values(build):
    return dict(re.findall(r"^([^/#\r\n][^:=\r\n]*):[^=\r\n]*=(.*)$", (build / "CMakeCache.txt").read_text(encoding="utf-8"), re.M))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--obs-build", required=True, type=Path)
    parser.add_argument("--jobs", type=int, default=min(4, max(1, (os.cpu_count() or 2) // 2)))
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    obs_build = args.obs_build.resolve()
    cache = cache_values(obs_build)
    build = obs_build / "whip-dependency"
    source = build / "source"
    install = build / "install"
    patch = ROOT / "build-aux/whip-dependency/libjuice-zero-tiebreaker.patch"
    if not source.exists():
        run("git", "clone", "--no-checkout", "https://github.com/paullouisageneau/libdatachannel.git", source)
        run("git", "-C", source, "checkout", "--detach", REVISION)
    if capture("git", "-C", source, "rev-parse", "HEAD") != REVISION:
        raise RuntimeError("Unexpected libdatachannel revision; checkout preserved")
    run("git", "-C", source, "diff", "--exit-code", "--", ".", ":!deps/libjuice")
    run("git", "-C", source, "submodule", "update", "--init", "--recursive", "--jobs", args.jobs)
    juice = source / "deps/libjuice"
    if capture("git", "-C", juice, "rev-parse", "HEAD") != JUICE_REVISION:
        raise RuntimeError("Unexpected libjuice revision")
    # Normalize the patch itself, including on checkouts using core.autocrlf.
    normalized_patch = build / "libjuice.patch"
    normalized_patch.write_bytes(patch.read_bytes().replace(b"\r\n", b"\n"))
    existing = capture("git", "-C", juice, "diff", "--binary")
    if existing:
        if existing.replace("\r\n", "\n").strip() != normalized_patch.read_text(encoding="utf-8").strip():
            raise RuntimeError("Unexpected libjuice edits; checkout preserved")
    else:
        run("git", "-C", juice, "apply", "--check", "--ignore-space-change", normalized_patch)
        run("git", "-C", juice, "apply", "--ignore-space-change", normalized_patch)
        for name in ("agent.c", "stun.c", "stun.h"):
            path = juice / "src" / name
            path.write_bytes(path.read_bytes().replace(b"\r\n", b"\n"))
    generator = ["-G", "Ninja" if platform.system() == "Darwin" else cache["CMAKE_GENERATOR"]]
    if cache.get("CMAKE_GENERATOR_PLATFORM"):
        generator += ["-A", cache["CMAKE_GENERATOR_PLATFORM"]]
    if cache.get("CMAKE_GENERATOR_TOOLSET"):
        generator += ["-T", cache["CMAKE_GENERATOR_TOOLSET"]]
    options = ["-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5", "-DCMAKE_INSTALL_LIBDIR=lib",
               "-DENABLE_WARNINGS_AS_ERRORS=OFF"]
    for key in ("CMAKE_PREFIX_PATH", "CMAKE_OSX_ARCHITECTURES", "CMAKE_OSX_DEPLOYMENT_TARGET"):
        if cache.get(key):
            options.append(f"-D{key}={cache[key]}")
    # Match obs-deps on Windows/macOS. Linux uses the distribution's OpenSSL.
    options += [f"-DUSE_MBEDTLS={'OFF' if platform.system() == 'Linux' else 'ON'}"]
    # Static Linux linkage keeps the patch inside obs-webrtc without requiring
    # users to replace their distribution's libdatachannel package.
    options += [f"-DBUILD_SHARED_LIBS={'OFF' if platform.system() == 'Linux' else 'ON'}"]
    run("cmake", "-S", source, "-B", build / "build", *generator, *options,
        "-DNO_WEBSOCKET=ON", "-DNO_TESTS=ON", "-DNO_EXAMPLES=ON", f"-DCMAKE_INSTALL_PREFIX={install}")
    parallel = ["--parallel", str(args.jobs)]
    if platform.system() == "Windows":
        parallel += ["--", "/p:CL_MPCount=1"]
    run("cmake", "--build", build / "build", "--config", "Release", "--target", "datachannel", *parallel)
    tests = build / "tests"
    run("cmake", "-S", ROOT / "test/whip", "-B", tests, *generator, *options, f"-DLIBJUICE_SOURCE_DIR={juice}")
    run("cmake", "--build", tests, "--config", "Release", "--target", "whip-ice-test", *parallel)
    run("ctest", "--test-dir", tests, "-C", "Release", "--output-on-failure")
    run("cmake", "--install", build / "build", "--config", "Release")
    if platform.system() == "Linux":
        # The pinned upstream package omits dependencies from its static export.
        config = install / "lib/cmake/LibDataChannel/LibDataChannelConfig.cmake"
        config.write_text("include(CMakeFindDependencyMacro)\nfind_dependency(Threads)\n"
                          "find_dependency(OpenSSL)\n" + config.read_text(encoding="utf-8"), encoding="utf-8")
    run("cmake", "-S", ROOT, "-B", obs_build, "-DENABLE_WEBRTC=ON", f"-DLibDataChannel_DIR={install}/lib/cmake/LibDataChannel")
    (build / "manifest.json").write_text(json.dumps({"libdatachannel": REVISION, "libjuice": JUICE_REVISION,
        "patch_sha256": hashlib.sha256(normalized_patch.read_bytes()).hexdigest(),
        "platform": platform.system(), "obs_commit": capture("git", "-C", ROOT, "rev-parse", "HEAD"),
        "whip_commit": os.environ.get("GITHUB_SHA", capture("git", "-C", ROOT, "rev-parse", "HEAD"))}, indent=2) + "\n")


if __name__ == "__main__":
    main()
