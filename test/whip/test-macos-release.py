#!/usr/bin/env python3
"""Check release metadata preservation and refusal of unnotarized packages."""

import importlib.util
import copy
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
        self.manifest = {"whip_commit": "original", "obs_commit": "original-obs-base", "patch_sha256": "patched-dependency"}
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
            release.signing.write_json(target / "whip-build.json", self.receipts[archive.name]["manifest"])
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

    def make_rebuild(self):
        rebuild = {"run": "new-ci", "whip_commit": "corrected-whip", "artifacts": []}
        for package in self.packages[:2]:
            name = package["file"]
            path = self.evidence / name.removesuffix(".tar.gz") / "package.json"
            receipt = json.loads(path.read_text())
            receipt["signing"]["manifest"]["whip_commit"] = rebuild["whip_commit"]
            self.receipts[name] = receipt["signing"]
            release.signing.write_json(path, receipt)
            rebuild["artifacts"].append({"name": package["artifact"] + "-notarized", "id": len(rebuild["artifacts"]),
                                         "digest": "artifact-digest", "url": "artifact-api-url"})
        return rebuild

    def test_rebuild_requires_explicit_provenance(self):
        self.make_rebuild()
        with self.assertRaisesRegex(ValueError, "Original build provenance changed"):
            release.prepare(self.original, self.signed, self.evidence, "Smoke results")

    def test_rebuild_cannot_replace_obs_base_or_patched_dependency(self):
        rebuild = self.make_rebuild()
        path = self.evidence / self.packages[0]["file"].removesuffix(".tar.gz") / "package.json"
        receipt = json.loads(path.read_text())
        for field in ("obs_commit", "patch_sha256"):
            with self.subTest(field=field):
                changed = copy.deepcopy(receipt)
                changed["signing"]["manifest"][field] = "unexpected-replacement"
                release.signing.write_json(path, changed)
                with self.assertRaisesRegex(ValueError, "Original build provenance changed"):
                    release.prepare(self.original, self.signed, self.evidence, "Smoke results", rebuild)

    def test_rebuild_preserves_previous_signed_release_and_non_mac_entries(self):
        rebuild = self.make_rebuild()
        previous = copy.deepcopy(self.packages)
        for package in previous[:2]:
            package["original_build_package"] = {"sha256": "original-unsigned-hash"}
            package["signing"] = {"notarization": {"id": "previous-accepted-submission"}}
        release.signing.write_json(self.original / "BUILD-PROVENANCE.json", {"run": "original-ci", "packages": previous})
        with patch.object(release.signing, "run", side_effect=self.fake_run), patch.object(release.signing, "verify"):
            release.prepare(self.original, self.signed, self.evidence, "Smoke results", rebuild)
        result = json.loads((self.signed / "BUILD-PROVENANCE.json").read_text())
        self.assertEqual(result["run"], "original-ci")
        self.assertEqual(result["packages"][2:], previous[2:])
        for old, new in zip(previous[:2], result["packages"][:2]):
            self.assertEqual(new["original_build_package"], old["original_build_package"])
            self.assertEqual(new["superseded_packages"], [old])
            self.assertEqual(new["manifest"]["whip_commit"], "corrected-whip")
            self.assertEqual(new["rebuild"]["run"], "new-ci")


if __name__ == "__main__":
    unittest.main()
