#!/usr/bin/env python3
"""Offline regression tests for the non-invasive S9 sampler; no phone required."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("sampler", HERE / "s9-session-sampler.py")
sampler = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sampler)

BATTERY = """Current Battery Service state:
  AC powered: false
  USB powered: false
  Wireless powered: false
  status: 3
  level: 79
  voltage: 3965
  temperature: 366
"""


class BatteryTests(unittest.TestCase):
    def test_android_battery_units(self):
        value = sampler.parse_battery(BATTERY)
        self.assertEqual(value["battery_pct"], 79)
        self.assertEqual(value["battery_temp_c"], 36.6)
        self.assertEqual(value["voltage_mv"], 3965)
        self.assertEqual(value["battery_status"], 3)
        self.assertEqual(value["usb_power"], "false")

    def test_missing_fields_are_unknown_not_zero(self):
        value = sampler.parse_battery("garbled\n")
        self.assertIsNone(value["battery_temp_c"])
        self.assertIsNone(value["battery_pct"])

    def test_thousand_battery_temperatures_parse_without_drift(self):
        for tenths in range(-500, 501):
            actual = sampler.parse_battery(f"temperature: {tenths}")["battery_temp_c"]
            self.assertEqual(actual, round(tenths / 10.0, 1))

    def test_invalid_temperature_does_not_crash(self):
        self.assertIsNone(sampler.parse_battery("temperature: unavailable")["battery_temp_c"])

    def test_negative_temperature_is_preserved(self):
        self.assertEqual(sampler.parse_battery("temperature: -15")["battery_temp_c"], -1.5)

    def test_dnd_and_radio_unmodified(self):
        self.assertNotIn("settings", sampler.__dict__["run"].__code__.co_names)
        self.assertNotIn("svc", sampler.__dict__["run"].__code__.co_names)


class ThermalTests(unittest.TestCase):
    def test_android_thermal_status(self):
        self.assertEqual(sampler.parse_thermal("Thermal Status: 2\n"), 2)
        self.assertEqual(sampler.parse_thermal("mStatus: 3\n"), 3)

    def test_unavailable_thermal_not_invented(self):
        self.assertIsNone(sampler.parse_thermal("no thermal service"))


class EndToEndTests(unittest.TestCase):
    def test_one_sample_creates_csv_and_json_without_modifying_device(self):
        commands = []
        def fake_command(adb, *args):
            commands.append(args)
            if args == ("devices",):
                return "List of devices attached\nFAKE123\tdevice\n"
            if args == ("shell", "dumpsys", "battery"):
                return BATTERY
            raise AssertionError(args)
        def fake_optional(adb, *args):
            commands.append(args)
            if args == ("shell", "dumpsys", "thermalservice"):
                return "Thermal Status: 1"
            if args == ("shell", "pidof", sampler.PACKAGE):
                return "1234\n"
            raise AssertionError(args)
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(sampler.shutil, "which", return_value="/mock/adb"), \
             patch.object(sampler, "command", side_effect=fake_command), \
             patch.object(sampler, "optional", side_effect=fake_optional):
            stats = sampler.run("adb", Path(tmp), "Home 60Hz baseline", 0, 60)
            self.assertEqual(stats["samples"], 1)
            self.assertEqual(stats["thermal_max_status"], 1)
            self.assertIsNone(stats["battery_delta_pct"])
            self.assertFalse(stats["charging_detected"])
            self.assertEqual(stats["battery_temperature_max_c"], 36.6)
            self.assertIn("power in watts", stats["metrics_not_measured"])
            self.assertEqual(len(list(Path(tmp).glob("*.csv"))), 1)
            self.assertEqual(len(list(Path(tmp).glob("*.json"))), 1)
            rows = list(Path(tmp).glob("*.csv"))[0].read_text().splitlines()
            self.assertEqual(len(rows), 2)
            self.assertIn('"samples": 1', list(Path(tmp).glob("*.json"))[0].read_text())
            self.assertEqual(len(commands), 4)

    def test_repeated_same_label_does_not_overwrite_results(self):
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(sampler.shutil, "which", return_value="/mock/adb"), \
             patch.object(sampler, "command", side_effect=lambda adb, *a: "List of devices attached\nS9\tdevice\n" if a == ("devices",) else BATTERY), \
             patch.object(sampler, "optional", return_value=""):
            sampler.run("adb", Path(tmp), "repeat", 0, 60)
            sampler.run("adb", Path(tmp), "repeat", 0, 60)
            self.assertEqual(len(list(Path(tmp).glob("*.csv"))), 2)
            self.assertEqual(len(list(Path(tmp).glob("*.json"))), 2)

    def test_rejects_no_android_device(self):
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(sampler.shutil, "which", return_value="/mock/adb"), \
             patch.object(sampler, "command", return_value="List of devices attached\n"):
            with self.assertRaisesRegex(RuntimeError, "Exactly one authorized"):
                sampler.run("adb", Path(tmp), "test", 0, 60)

    def test_rejects_ambiguous_android_devices(self):
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(sampler.shutil, "which", return_value="/mock/adb"), \
             patch.object(sampler, "command", return_value="List of devices attached\na\tdevice\nb\tdevice\n"):
            with self.assertRaisesRegex(RuntimeError, "found 2"):
                sampler.run("adb", Path(tmp), "test", 0, 60)


if __name__ == "__main__":
    unittest.main(verbosity=2)
