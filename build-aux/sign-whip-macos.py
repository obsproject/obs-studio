#!/usr/bin/env python3
"""Sign a staged WHIP app, then notarize, staple, verify, and archive it.

Credentials stay in Keychain. Signing may run separately while notarization
credentials are being configured; only `release` produces a distributable archive.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import struct
import subprocess
import tempfile


MACHO = {bytes.fromhex(s) for s in ("feedface", "feedfacf", "cefaedfe", "cffaedfe", "cafebabe", "bebafeca", "cafebabf", "bfbafeca")}


def run(*args):
    result = subprocess.run([str(a) for a in args], capture_output=True, check=True)
    return result.stdout


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_json(path, data):
    path.write_text(json.dumps(data, indent=2) + "\n")


def code_paths(app):
    binaries, bundles = [], [app]
    for root, dirs, files in os.walk(app, followlinks=False):
        for name in dirs:
            path = Path(root) / name
            if not path.is_symlink() and path.suffix in (".app", ".framework", ".xpc", ".plugin", ".bundle"):
                bundles.append(path)
        for name in files:
            path = Path(root) / name
            if path.is_symlink():
                continue
            with path.open("rb") as stream:
                if stream.read(4) in MACHO:
                    binaries.append(path)
    return sorted(binaries), sorted(bundles, key=lambda p: (-len(p.parts), str(p)))


def entitlements(path):
    result = subprocess.run(["codesign", "-d", "--entitlements", ":-", str(path)], capture_output=True)
    if result.returncode and b"code object is not signed at all" in result.stderr:
        return {}
    result.check_returncode()
    data = result.stdout
    result = plistlib.loads(data) if data.strip() else {}
    if result.get("com.apple.security.get-task-allow"):
        raise ValueError(f"Debug entitlement is forbidden: {path}")
    return result


def unsigned_hash(path, temporary):
    # Remove signatures on a scratch copy only. This proves the code, including
    # the patched libdatachannel, did not change while replacing signatures.
    copy = temporary / "unsigned-code"
    shutil.copyfile(path, copy)
    result = subprocess.run(["codesign", "--remove-signature", str(copy)], capture_output=True)
    if b"code object is not signed at all" not in result.stderr:
        result.check_returncode()
    data = bytearray(copy.read_bytes())
    # codesign can grow __LINKEDIT's VM allocation to fit the CMS signature;
    # --remove-signature leaves that allocation unchanged. Normalize only this
    # field, retaining all code/data, imports, UUIDs, and other load commands.
    def normalize(offset):
        magic = bytes(data[offset:offset + 4])
        if magic not in (bytes.fromhex("cffaedfe"), bytes.fromhex("feedfacf")):
            raise ValueError(f"Expected a 64-bit Mach-O slice: {path}")
        endian = "<" if magic == bytes.fromhex("cffaedfe") else ">"
        count = struct.unpack_from(endian + "I", data, offset + 16)[0]
        command = offset + 32
        for _ in range(count):
            kind, size = struct.unpack_from(endian + "II", data, command)
            if kind == 0x19 and data[command + 8:command + 24].rstrip(b"\0") == b"__LINKEDIT":
                data[command + 32:command + 40] = bytes(8)
            command += size
    magic = bytes(data[:4])
    if magic in (bytes.fromhex("cafebabe"), bytes.fromhex("cafebabf")):
        count = struct.unpack_from(">I", data, 4)[0]
        wide = magic == bytes.fromhex("cafebabf")
        slices = []
        for i in range(count):
            header = 8 + i * (32 if wide else 20)
            offset, size = struct.unpack_from(">QQ" if wide else ">II", data, header + 8)
            normalize(offset)
            slices.append(bytes(data[offset:offset + size]))
        return hashlib.sha256(b"".join(slices)).hexdigest()
    normalize(0)
    return hashlib.sha256(data).hexdigest()


def verify(app, team):
    run("codesign", "--verify", "--deep", "--strict", "--verbose=2", app)
    binaries, bundles = code_paths(app)
    for path in binaries + bundles:
        run("codesign", "--verify", "--strict", path)
        result = subprocess.run(["codesign", "-d", "--verbose=4", str(path)], capture_output=True, check=True)
        details = result.stderr.decode()
        if (f"TeamIdentifier={team}\n" not in details or "Authority=Developer ID Application:" not in details
                or "runtime" not in details or "Timestamp=" not in details):
            raise ValueError(f"Missing Developer ID, team, hardened runtime, or timestamp: {path}")
        entitlements(path)


def sign(stage, identity, team):
    if not identity.startswith("Developer ID Application:") or not identity.endswith(f"({team})"):
        raise ValueError("An explicit Developer ID Application identity for the expected team is required")
    app = stage / "OBS.app"
    manifest = stage / "whip-build.json"
    metadata = json.loads(manifest.read_text())
    if metadata.get("platform") != "Darwin" or not metadata.get("patch_sha256"):
        raise ValueError("Missing patched macOS build provenance")
    binaries, bundles = code_paths(app)
    if not binaries or not any("datachannel" in p.name for p in binaries):
        raise ValueError("Patched WebRTC runtime is missing")
    with tempfile.TemporaryDirectory(prefix="obs-whip-sign-") as temp:
        temporary = Path(temp)
        original = {p: entitlements(p) for p in binaries + bundles}
        if not original[app].get("com.apple.security.cs.disable-library-validation"):
            raise ValueError("OBS application entitlements are missing")
        before = {p: unsigned_hash(p, temporary) for p in binaries}
        for path in binaries + bundles:
            args = ["codesign", "--force", "--sign", identity, "--timestamp", "--options", "runtime",
                    "--generate-entitlement-der"]
            if original[path]:
                plist = temporary / "entitlements.plist"
                plist.write_bytes(plistlib.dumps(original[path]))
                args += ["--entitlements", plist]
            run(*args, path)
        verify(app, team)
        for path in binaries:
            if unsigned_hash(path, temporary) != before[path]:
                raise ValueError(f"Code changed beyond its signature: {path}")
        for path, expected in original.items():
            if entitlements(path) != expected:
                raise ValueError(f"Entitlements changed: {path}")
    record = {
        "schema": 1, "identity": identity, "team_id": team,
        "signed_at": datetime.now(timezone.utc).isoformat(),
        "manifest_sha256": sha256(manifest), "manifest": metadata,
        "hardened_runtime": True, "secure_timestamp": True,
        "entitlements_preserved": True, "unsigned_code_unchanged": True,
        "code_sha256_without_signature": {str(p.relative_to(app)): h for p, h in before.items()},
        "notarization": {"status": "not_submitted"},
    }
    write_json(stage / "macos-signing.json", record)
    print(f"Signed and verified {app}: {len(binaries)} Mach-O files, {len(bundles)} bundles", flush=True)


def release(stage, profile, output, evidence, team):
    app = stage / "OBS.app"
    record = json.loads((stage / "macos-signing.json").read_text())
    if record["team_id"] != team or record["manifest_sha256"] != sha256(stage / "whip-build.json"):
        raise ValueError("Signing record does not match the staged build")
    verify(app, team)
    signature = subprocess.run(["codesign", "-d", "--verbose=4", str(app)], capture_output=True, check=True)
    cdhash = re.search(r"^CDHash=(\w+)$", signature.stderr.decode(), re.MULTILINE).group(1)
    with tempfile.TemporaryDirectory(prefix="obs-whip-verify-") as temp:
        binaries, _ = code_paths(app)
        current = {str(p.relative_to(app)): unsigned_hash(p, Path(temp)) for p in binaries}
        if current != record["code_sha256_without_signature"]:
            raise ValueError("Code changed after signing")
    evidence.mkdir(parents=True, exist_ok=True)
    submission = evidence / "submission.json"
    auth = ["--keychain-profile", profile]
    if submission.exists():
        result = json.loads(submission.read_text())
        if result.get("app_cdhash") != cdhash:
            raise ValueError("Saved submission belongs to another app signature; use a new evidence directory")
    else:
        upload = evidence / "notarization.zip"
        run("ditto", "-c", "-k", "--keepParent", app, upload)
        # Save the ID immediately, so an interrupted wait never loses a submission.
        result = json.loads(run("xcrun", "notarytool", "submit", upload, *auth, "--output-format", "json"))
        result["app_cdhash"] = cdhash
        write_json(submission, result)
    submission_id = result["id"]
    print(f"Notarization submission: {submission_id}", flush=True)
    result = json.loads(run("xcrun", "notarytool", "wait", submission_id, *auth, "--output-format", "json"))
    write_json(evidence / "result.json", result)
    run("xcrun", "notarytool", "log", submission_id, *auth, evidence / "notarization-log.json")
    if result.get("status") != "Accepted":
        raise ValueError(f"Apple did not accept this app: {result.get('status')}; see {evidence}")
    run("xcrun", "stapler", "staple", app)
    run("xcrun", "stapler", "validate", app)
    verify(app, team)
    gatekeeper = subprocess.run(["spctl", "--assess", "--type", "execute", "--verbose=2", str(app)],
                               capture_output=True, check=True)
    assessment = gatekeeper.stderr.decode()
    if "source=Notarized Developer ID" not in assessment:
        raise ValueError(f"Unexpected Gatekeeper assessment: {assessment}")
    (evidence / "gatekeeper.txt").write_text(assessment)
    record["notarization"] = {"id": submission_id, "status": "Accepted", "stapled": True,
                              "gatekeeper": "Notarized Developer ID",
                              "verified_at": datetime.now(timezone.utc).isoformat(),
                              "log": json.loads((evidence / "notarization-log.json").read_text())}
    write_json(stage / "macos-signing.json", record)
    notice = stage / "WHIP-BUILD.txt"
    lines = notice.read_text().splitlines(keepends=True)
    lines[0] = "Unofficial OBS WHIP test build. Developer ID signed and Apple-notarized; ticket stapled and Gatekeeper verified.\n"
    notice.write_text("".join(lines))
    output.parent.mkdir(parents=True, exist_ok=True)
    # Apple's tar preserves the stapled ticket and framework links. Verify the
    # extracted distribution too; a valid staging tree alone is insufficient.
    pending = output.with_name(output.name + ".pending")
    run("/usr/bin/tar", "-czf", pending, "-C", stage, ".")
    with tempfile.TemporaryDirectory(prefix="obs-whip-extract-") as temp:
        run("/usr/bin/tar", "-xzf", pending, "-C", temp)
        unpacked = Path(temp) / "OBS.app"
        verify(unpacked, team)
        run("xcrun", "stapler", "validate", unpacked)
        run("spctl", "--assess", "--type", "execute", "--verbose=2", unpacked)
    pending.replace(output)
    write_json(evidence / "package.json", {"file": output.name, "sha256": sha256(output),
                                          "bytes": output.stat().st_size, "signing": record})
    print(f"Notarized, stapled, and verified archive: {output}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("sign", "release", "all"))
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--identity", default=os.environ.get("MACOS_SIGNING_IDENTITY"))
    parser.add_argument("--team-id", required=True)
    parser.add_argument("--keychain-profile", default=os.environ.get("MACOS_NOTARY_PROFILE"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--evidence", type=Path)
    args = parser.parse_args()
    if args.command in ("sign", "all") and not args.identity:
        parser.error("--identity is required for signing")
    if args.command in ("release", "all") and not all((args.keychain_profile, args.output, args.evidence)):
        parser.error("release requires --keychain-profile, --output, and --evidence")
    if args.command in ("sign", "all"):
        sign(args.stage.resolve(), args.identity, args.team_id)
    if args.command in ("release", "all"):
        release(args.stage.resolve(), args.keychain_profile, args.output.resolve(), args.evidence.resolve(), args.team_id)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        # Commands take Keychain profile names, never credential values.
        raise SystemExit(f"Command failed: {error.cmd}\n{(error.stderr or b'').decode(errors='replace')}") from error
