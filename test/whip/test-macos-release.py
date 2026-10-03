#!/usr/bin/env python3
"""Check release metadata preservation and refusal of unnotarized packages."""

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("release", ROOT / "build-aux/update-whip-macos-release.py")
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class ReleaseMetadata(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.original, self.signed, self.evidence = (root / n for n in ("original", "signed", "evidence"))
        for path in (self.original, self.signed, self.evidence):
            path.mkdir()
        self.packages, self.receipts = [], {}
        self.manifest = {"whip_commit": "original", "patch_sha256": "patched-dependency"}
        for platform in ("darwin-arm64", "darwin-x86_64", "windows-x64", "linux-x86_64"):
            name = f"obs-whip-test-{platform}.tar.gz"
            entry = {"file": name, "sha256": "original-hash", "bytes": 1,
                     "artifact": platform, "manifest": self.manifest}
            self.packages.append(entry)
            if platform.startswith("darwin"):
                archive = self.signed / name
                archive.write_bytes(b"signed-package")
                record = {"team_id": "TEAM", "manifest": self.manifest,
                          "notarization": {"status": "Accepted", "stapled": True}}
                receipt = {"file": name, "sha256": release.signing.sha256(archive),
                           "bytes": archive.stat().st_size, "signing": record}
                folder = self.evidence / name.removesuffix(".tar.gz")
                folder.mkdir()
                release.signing.write_json(folder / "package.json", receipt)
                self.receipts[name] = record
        release.signing.write_json(self.original / "BUILD-PROVENANCE.json", {"run": "original-ci", "packages": self.packages})
        (self.original / "SHA256SUMS.txt").write_text("".join(f"original-hash  {p['file']}\n" for p in self.packages))

    def fake_run(self, *args):
        if args[:2] == ("/usr/bin/tar", "-xzf"):
            archive, target = args[2], Path(args[4])
            release.signing.write_json(target / "whip-build.json", self.manifest)
            release.signing.write_json(target / "macos-signing.json", self.receipts[archive.name])
        return b""

    def test_windows_linux_and_original_build_provenance_preserved(self):
        with patch.object(release.signing, "run", side_effect=self.fake_run), patch.object(release.signing, "verify"):
            names = release.prepare(self.original, self.signed, self.evidence, "Actual smoke results")
        self.assertEqual(len(names), 2)
        result = json.loads((self.signed / "BUILD-PROVENANCE.json").read_text())
        self.assertEqual(result["run"], "original-ci")
        for old, new in zip(self.packages, result["packages"]):
            if "-darwin-" in old["file"]:
                self.assertEqual(new["original_build_package"], old)
                self.assertEqual(new["manifest"], old["manifest"])
                self.assertNotEqual(new["sha256"], old["sha256"])
            else:
                self.assertEqual(new, old)
                self.assertIn(f"original-hash  {old['file']}\n", (self.signed / "SHA256SUMS.txt").read_text())

    def test_unnotarized_package_cannot_prepare_release(self):
        first = self.packages[0]["file"]
        path = self.evidence / first.removesuffix(".tar.gz") / "package.json"
        receipt = json.loads(path.read_text())
        receipt["signing"]["notarization"]["status"] = "In Progress"
        release.signing.write_json(path, receipt)
        with self.assertRaisesRegex(ValueError, "Notarization and stapling"):
            release.prepare(self.original, self.signed, self.evidence, "Smoke results")
        self.assertFalse((self.signed / "BUILD-PROVENANCE.json").exists())


if __name__ == "__main__":
    unittest.main()
