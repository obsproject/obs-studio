#!/usr/bin/env python3
"""Release-gate regressions; no Apple credentials or macOS tools are required."""

import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("signing", ROOT / "build-aux/sign-whip-macos.py")
signing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(signing)


class ReleaseGates(unittest.TestCase):
    def test_ad_hoc_identity_rejected_before_touching_stage(self):
        with self.assertRaisesRegex(ValueError, "Developer ID Application"):
            signing.sign(Path("does-not-exist"), "-", "TEAM")

    def test_debug_entitlements_rejected(self):
        result = subprocess.CompletedProcess([], 0, signing.plistlib.dumps(
            {"com.apple.security.get-task-allow": True}), b"")
        with patch.object(signing.subprocess, "run", return_value=result):
            with self.assertRaisesRegex(ValueError, "Debug entitlement"):
                signing.entitlements(Path("app"))

    def attempt_release(self, apple_status, gatekeeper_fails=False, stale_submission=False):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            stage = root / "stage"
            stage.mkdir()
            (stage / "whip-build.json").write_text('{}\n')
            signing.write_json(stage / "macos-signing.json", {
                "team_id": "TEAM", "manifest_sha256": signing.sha256(stage / "whip-build.json"),
                "code_sha256_without_signature": {},
            })
            evidence = root / "evidence"
            if stale_submission:
                evidence.mkdir()
                signing.write_json(evidence / "submission.json", {"id": "old", "app_cdhash": "def"})
            commands = []

            def run(*args):
                commands.append(args)
                if args[:3] == ("xcrun", "notarytool", "submit"):
                    return b'{"id":"submission"}'
                if args[:3] == ("xcrun", "notarytool", "wait"):
                    return json.dumps({"id": "submission", "status": apple_status}).encode()
                if args[:3] == ("xcrun", "notarytool", "log"):
                    Path(args[-1]).write_text('{}')
                return b""

            def process(args, **kwargs):
                if args[0] == "spctl" and gatekeeper_fails:
                    raise subprocess.CalledProcessError(3, args, stderr=b"rejected")
                return subprocess.CompletedProcess(args, 0, b"", b"CDHash=abc\n")

            output = root / "release.tar.gz"
            with patch.object(signing, "verify"), patch.object(signing, "code_paths", return_value=([], [])), \
                    patch.object(signing, "run", side_effect=run), \
                    patch.object(signing.subprocess, "run", side_effect=process):
                with self.assertRaises((ValueError, subprocess.CalledProcessError)):
                    signing.release(stage, "profile", output, evidence, "TEAM")
            self.assertFalse(output.exists())
            self.assertFalse(output.with_name(output.name + ".pending").exists())
            return commands

    def test_rejected_notarization_cannot_staple_or_package(self):
        commands = self.attempt_release("Invalid")
        self.assertFalse(any(c[:2] == ("xcrun", "stapler") for c in commands))

    def test_gatekeeper_rejection_cannot_package(self):
        commands = self.attempt_release("Accepted", gatekeeper_fails=True)
        self.assertFalse(any(c[0] == "/usr/bin/tar" for c in commands))

    def test_saved_submission_cannot_be_reused_for_different_signature(self):
        commands = self.attempt_release("Accepted", stale_submission=True)
        self.assertFalse(commands)


if __name__ == "__main__":
    unittest.main()
