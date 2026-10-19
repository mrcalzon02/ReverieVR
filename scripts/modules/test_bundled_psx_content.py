#!/usr/bin/env python3
"""Check the actual checked-in Breakwater and Omega Front asset selections."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location("bundled_psx",HERE/"build-bundled-psx-content.py")
tool=importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)

class BundledPSXContentTest(unittest.TestCase):
    def test_sources_are_real_repository_assets(self):
        for game in tool.GAMES:
            with self.subTest(game=game):
                root,files=tool.source_files(game)
                self.assertTrue(files)
                self.assertTrue(all((root/rel).is_file() for rel in files))
                self.assertFalse(any("source/" in rel.as_posix() for rel in files))
        root,files=tool.source_files("breakwater-battery")
        self.assertEqual(len(files),5)
        self.assertEqual({p.name for p in files},set(tool.BREAKWATER))
        root,files=tool.source_files("omega-front")
        self.assertGreaterEqual(len(files),len(tool.OMEGA_SHARED))
    def test_builds_actual_shipped_content_in_separate_game_scopes(self):
        with tempfile.TemporaryDirectory() as temp:
            roots=Path(temp);hashes=set()
            for game in tool.GAMES:
                with self.subTest(game=game):
                    out=roots/(game+".rvpack")
                    result=tool.build(game,out,1,"0.1.0")
                    self.assertGreater(result["assets"],0)
                    self.assertNotIn(result["sha256"],hashes)
                    hashes.add(result["sha256"])
                    with zipfile.ZipFile(out) as pack:
                        names=pack.namelist()
                        self.assertTrue(all(name.startswith(
                            "assets/native/"+tool.GAMES[game][0]+"/")
                            for name in names if name.startswith("assets/")))
            self.assertEqual(len(hashes),2)
    def test_unapproved_game_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,"Only shipped"):
                tool.build("classic-chess",Path(temp)/"bad.rvpack",1,"0.1.0")

if __name__=="__main__":unittest.main()
