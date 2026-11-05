#!/usr/bin/env python3
"""Check the checked-in Sitka landmark register, not imagined map geometry."""
import unittest
from pathlib import Path
from shore_reality import load_site_catalog


class SitkaCatalogTest(unittest.TestCase):
    def test_checked_in_sites_are_identity_only(self):
        catalog = load_site_catalog(
            Path(__file__).resolve().parents[2] /
            "data/sitka/shore-reality.json")
        self.assertGreaterEqual(len(catalog), 10)
        self.assertEqual(catalog["crescent_harbor"]["presence"], "existing")
        self.assertEqual(catalog["oconnell_bridge"]["category"], "bridge")
        self.assertEqual(
            catalog["sitka_cold_storage_original"]["presence"], "demolished")
        self.assertEqual(
            catalog["herring_rock_original"]["presence"], "relocated")
        self.assertEqual(
            catalog["fishermans_quay_pyramid"]["presence"],
            "needs_current_verification")
        for site in catalog.values():
            self.assertEqual(site["geometry_state"], "not_acquired")


if __name__ == "__main__":
    unittest.main()
