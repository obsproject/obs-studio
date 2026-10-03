#!/usr/bin/env python3
"""Replace only verified Mac assets in an existing WHIP release.

Defaults to preparation and validation. --publish explicitly uploads the prepared
archives, checksums, provenance, and notes, then verifies public downloads.
"""

import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import urllib.request

SPEC = importlib.util.spec_from_file_location("signing", Path(__file__).with_name("sign-whip-macos.py"))
signing = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(signing)


def gh(*args):
    return subprocess.check_output(["gh", *map(str, args)])


def prepare(original, signed, evidence, notes):
    provenance = json.loads((original / "BUILD-PROVENANCE.json").read_text())
    updated = copy.deepcopy(provenance)
    replacements = {}
    for package in updated["packages"]:
        if "-darwin-" not in package["file"]:
            continue
        name = package["file"]
        archive = signed / name
        receipt = json.loads((evidence / name.removesuffix(".tar.gz") / "package.json").read_text())
        record = receipt["signing"]
        status = record["notarization"]
        if status.get("status") != "Accepted" or not status.get("stapled"):
            raise ValueError(f"Notarization and stapling are required: {name}")
        if record["manifest"] != package["manifest"]:
            raise ValueError(f"Original build provenance changed: {name}")
        if receipt["file"] != name or receipt["sha256"] != signing.sha256(archive) or receipt["bytes"] != archive.stat().st_size:
            raise ValueError(f"Signed package does not match its receipt: {name}")
        with tempfile.TemporaryDirectory(prefix="obs-whip-publish-") as temp:
            signing.run("/usr/bin/tar", "-xzf", archive, "-C", temp)
            unpacked = Path(temp)
            if json.loads((unpacked / "whip-build.json").read_text()) != package["manifest"]:
                raise ValueError(f"Embedded build provenance changed: {name}")
            if json.loads((unpacked / "macos-signing.json").read_text()) != record:
                raise ValueError(f"Embedded signing record differs: {name}")
            app = unpacked / "OBS.app"
            signing.verify(app, record["team_id"])
            signing.run("xcrun", "stapler", "validate", app)
            signing.run("spctl", "--assess", "--type", "execute", "--verbose=2", app)
        # Keep every original artifact field as a separate, complete snapshot.
        package["original_build_package"] = copy.deepcopy(package)
        package["sha256"] = receipt["sha256"]
        package["bytes"] = receipt["bytes"]
        package["signing"] = record
        replacements[name] = receipt["sha256"]
    if len(replacements) != 2:
        raise ValueError("Expected exactly two Mac packages")
    lines = (original / "SHA256SUMS.txt").read_text().splitlines(keepends=True)
    seen = set()
    for index, line in enumerate(lines):
        old_hash, name = line.strip().split(maxsplit=1)
        name = name.lstrip("*")
        if name in replacements:
            expected = next(p["sha256"] for p in provenance["packages"] if p["file"] == name)
            if old_hash != expected:
                raise ValueError(f"Original checksum and provenance disagree: {name}")
            lines[index] = f"{replacements[name]}  {name}\n"
            seen.add(name)
    if seen != set(replacements):
        raise ValueError("Mac checksum entries are missing")
    signing.write_json(signed / "BUILD-PROVENANCE.json", updated)
    (signed / "SHA256SUMS.txt").write_text("".join(lines))
    (signed / "MACOS-VALIDATION.txt").write_text(notes)
    return sorted(replacements)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--signed", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--validation-notes", type=Path, required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--repo", default="steveseguin/obs-studio")
    parser.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    files = prepare(args.original, args.signed, args.evidence, args.validation_notes.read_text())
    release = json.loads(gh("release", "view", args.tag, "-R", args.repo, "--json", "body,isPrerelease,assets"))
    with tempfile.TemporaryDirectory(prefix="obs-whip-current-") as temp:
        gh("release", "download", args.tag, "-R", args.repo, "-p", "BUILD-PROVENANCE.json", "-p", "SHA256SUMS.txt", "-D", temp)
        for name in ("BUILD-PROVENANCE.json", "SHA256SUMS.txt"):
            if (Path(temp) / name).read_bytes() != (args.original / name).read_bytes():
                raise ValueError("Public release metadata changed since download; refresh and review it first")
    body = release["body"]
    marker = "unsigned and not notarized"
    if marker not in body:
        raise ValueError("Release notes no longer match the expected unsigned-release handoff")
    body = body.replace(marker, "with Developer ID signed and Apple-notarized Mac apps; Windows packages remain unsigned")
    body = body.replace(
        "**The public Mac archives remain ad-hoc-signed and unnotarized pending Apple notarization credentials.**",
        "**The Mac archives are now Developer ID signed and Apple-notarized, with stapled tickets and verified Gatekeeper acceptance.**")
    body += "\n### Mac signing and runtime validation\n\n" + args.validation_notes.read_text().strip() + "\n\n"
    body += ("The original CI binaries and patched dependency are retained. `BUILD-PROVENANCE.json` keeps each "
             "original Mac artifact under `original_build_package` and records signing/notarization separately "
             "under `signing`. Windows and Linux archives and provenance entries are unchanged.\n")
    notes = args.signed / "release-notes.md"
    notes.write_text(body)
    print(f"Prepared {args.tag}: {', '.join(files)}")
    if not args.publish:
        return
    original_others = {a["name"]: a for a in release["assets"] if a["name"] not in files + ["BUILD-PROVENANCE.json", "SHA256SUMS.txt"]}
    gh("release", "upload", args.tag, "-R", args.repo, "--clobber",
       *[args.signed / name for name in files + ["BUILD-PROVENANCE.json", "SHA256SUMS.txt"]])
    gh("release", "edit", args.tag, "-R", args.repo, "--notes-file", notes,
       "--prerelease=" + str(release["isPrerelease"]).lower(),
       "--latest=" + str(not release["isPrerelease"]).lower())
    current = json.loads(gh("release", "view", args.tag, "-R", args.repo, "--json", "assets,isPrerelease"))
    others = {a["name"]: a for a in current["assets"] if a["name"] in original_others}
    # downloadCount may change while this script runs; IDs/dates/sizes must not.
    for name, before in original_others.items():
        for key in ("id", "size", "updatedAt"):
            if others[name][key] != before[key]:
                raise ValueError(f"Unexpected change to non-Mac asset: {name}")
    results = []
    for name in files + ["BUILD-PROVENANCE.json", "SHA256SUMS.txt"]:
        url = next(a["url"] for a in current["assets"] if a["name"] == name)
        digest = hashlib.sha256()
        with urllib.request.urlopen(url) as response:
            while chunk := response.read(1024 * 1024):
                digest.update(chunk)
        if digest.hexdigest() != signing.sha256(args.signed / name):
            raise ValueError(f"Public download checksum mismatch: {name}")
        results.append({"url": url, "sha256": digest.hexdigest()})
    signing.write_json(args.signed / "public-verification.json", results)
    print("Verified all replaced public downloads; non-Mac assets remain unchanged")


if __name__ == "__main__":
    main()
