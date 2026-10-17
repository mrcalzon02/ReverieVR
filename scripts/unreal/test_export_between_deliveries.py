"""Ordinary CPython tests for portable Unreal export file safety, no UE install needed."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zlib


HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("bd_unreal_export", HERE / "export_between_deliveries.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ExportSafetyTests(unittest.TestCase):
    def test_names(self):
        self.assertTrue(module.valid_filename("101draft_texture_0"))
        for invalid in ("..", "../../secrets", "bad/file", "", "x.png"):
            self.assertFalse(module.valid_filename(invalid))

    def test_catalog(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            location = root / "docs/project/BETWEEN_DELIVERIES_UNREAL_CATALOG.json"
            location.parent.mkdir(parents=True)
            products = [
                {"supplier_id": str((i // 4 + 1) * 101),
                 "folder_label": "item" + str(i), "design_name": "Product" + str(i)}
                for i in range(24)
            ]
            location.write_text(json.dumps({"products": products}), encoding="utf-8")
            self.assertEqual(24, len(module.load_catalog(root)))
            products[8]["folder_label"] = "../unsafe"
            location.write_text(json.dumps({"products": products}), encoding="utf-8")
            with self.assertRaises(ValueError):
                module.load_catalog(root)

    def test_texture_dimensions_and_hash(self):
        with tempfile.TemporaryDirectory() as temp:
            target = Path(temp) / "sample.png"
            def chunk(kind, data):
                return (struct.pack(">I", len(data)) + kind + data
                        + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff))
            raw = (b"\x00" + b"\x00" * (1024 * 4)) * 1024
            original = (b"\x89PNG\r\n\x1a\n"
                        + chunk(b"IHDR", struct.pack(">II", 1024, 1024)
                                + b"\x08\x06\x00\x00\x00")
                        + chunk(b"IDAT", zlib.compress(raw))
                        + chunk(b"IEND", b""))
            target.write_bytes(original)
            result = module.validate_export(target, "texture")
            self.assertEqual((1024, 1024), (result["width"], result["height"]))
            self.assertEqual(64, len(result["sha256"]))
            target.write_bytes(original[:-5])
            with self.assertRaises(ValueError):
                module.validate_export(target, "texture")
            corrupt = bytearray(original)
            corrupt[-18] ^= 0x40  # corrupt an IDAT or footer byte
            target.write_bytes(corrupt)
            with self.assertRaises(ValueError):
                module.validate_export(target, "texture")
            target.write_bytes(b"not a png" + b"a" * 66)
            with self.assertRaises(ValueError):
                module.validate_export(target, "texture")

    def test_bad_mesh_not_accepted(self):
        with tempfile.TemporaryDirectory() as temp:
            target = Path(temp) / "bad.fbx"
            target.write_bytes(b"garbage" * 40)
            with self.assertRaises(ValueError):
                module.validate_export(target, "mesh")


if __name__ == "__main__":
    unittest.main()
