#!/usr/bin/env python3
"""Non-invasive, no-root, low-frequency Android VR endurance sampler.

Only battery temperature/level/voltage, charging state, OS thermal status and
process existence are sampled. This does NOT measure SoC temperature, GPU work,
frame presentation or headset comfort. Never changes phone settings.
"""
import argparse
import csv
import datetime as dt
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

PACKAGE = "io.github.mrcalzon02.reverievr"
FIELDS = ["elapsed_s", "utc", "battery_pct", "battery_temp_c", "voltage_mv",
          "battery_status", "usb_power", "ac_power", "wireless_power",
          "android_thermal_status", "app_pid_present"]


def parse_battery(output):
    values = {}
    for line in output.splitlines():
        if ":" in line:
            k, v = line.split(":", 1)
            values[k.strip().lower()] = v.strip()
    def integer(key):
        value = values.get(key, "")
        return int(value) if re.fullmatch(r"-?\d+", value) else None
    temp = integer("temperature")
    return {"battery_pct": integer("level"),
            "battery_temp_c": round(temp / 10.0, 1) if temp is not None else None,
            "voltage_mv": integer("voltage"),
            "battery_status": integer("status"),
            "usb_power": values.get("usb powered"),
            "ac_power": values.get("ac powered"),
            "wireless_power": values.get("wireless powered")}


def parse_thermal(output):
    match = re.search(r"(?im)^\s*(?:Thermal Status|mStatus)\s*:\s*(\d+)\s*$", output)
    return int(match.group(1)) if match else None


def command(adb, *args):
    result = subprocess.run([adb, *args], capture_output=True, text=True, timeout=12)
    if result.returncode:
        raise RuntimeError("ADB command failed: " + " ".join(args) + " (" + result.stderr.strip()[:200] + ")")
    return result.stdout.replace("\r", "")


def optional(adb, *args):
    try:
        return command(adb, *args)
    except (RuntimeError, subprocess.TimeoutExpired):
        return ""


def run(adb, out, label, minutes, interval):
    if not shutil.which(adb):
        raise RuntimeError("adb not found; install Android platform-tools or set ADB_BIN")
    devices = command(adb, "devices").splitlines()
    active = [line for line in devices if re.search(r"\tdevice\s*$", line)]
    if len(active) != 1:
        raise RuntimeError("Exactly one authorized connected Android device is required; found " + str(len(active)))
    out.mkdir(parents=True, exist_ok=True)
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    safe = re.sub(r"[^a-z0-9_-]+", "-", label.lower()).strip("-")[:48] or "vr"
    csv_path = out / (stamp + "_" + safe + ".csv")
    summary_path = out / (stamp + "_" + safe + ".json")
    duration = minutes * 60.0
    begin = time.monotonic()
    samples = []
    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS)
        writer.writeheader()
        sample_number = 0
        while True:
            elapsed = time.monotonic() - begin
            battery = parse_battery(command(adb, "shell", "dumpsys", "battery"))
            thermal = parse_thermal(optional(adb, "shell", "dumpsys", "thermalservice"))
            pid = optional(adb, "shell", "pidof", PACKAGE).strip()
            row = {"elapsed_s": round(elapsed, 1),
                   "utc": dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds"),
                   **battery, "android_thermal_status": thermal,
                   "app_pid_present": bool(pid)}
            writer.writerow(row)
            handle.flush()
            samples.append(row)
            sample_number += 1
            print("sample", sample_number, "at", row["elapsed_s"], "sec: battery",
                  row["battery_pct"], "%,", row["battery_temp_c"], "C, thermal", thermal, flush=True)
            if time.monotonic() - begin >= duration:
                break
            time.sleep(min(interval, max(0.0, duration - (time.monotonic() - begin))))
    levels = [s["battery_pct"] for s in samples if s["battery_pct"] is not None]
    temps = [s["battery_temp_c"] for s in samples if s["battery_temp_c"] is not None]
    stats = {"label": label, "samples": len(samples), "requested_minutes": minutes,
             "interval_seconds": interval, "battery_delta_pct": (levels[-1] - levels[0]) if len(levels) >= 2 else None,
             "battery_temperature_start_c": temps[0] if temps else None,
             "battery_temperature_end_c": temps[-1] if temps else None,
             "battery_temperature_max_c": max(temps) if temps else None,
             "charging_detected": any(s[k] == "true" for s in samples for k in ("usb_power", "ac_power", "wireless_power")),
             "thermal_max_status": max((s["android_thermal_status"] for s in samples if s["android_thermal_status"] is not None), default=None),
             "metrics_not_measured": ["GPU/SoC junction temperature", "power in watts", "GPU frame completion", "physical display presentation", "headset motion-to-photon latency"]}
    summary_path.write_text(json.dumps(stats, indent=2) + "\n", encoding="utf-8")
    print("CSV:", csv_path, "summary:", summary_path, flush=True)
    if stats["charging_detected"]:
        print("WARNING: charging detected; this run must not be compared to unplugged runs.", file=sys.stderr)
    return stats


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", required=True, help="Condition label e.g. home-90pct-brightness30")
    parser.add_argument("--minutes", type=float, default=60, help="Sampling duration, default 60")
    parser.add_argument("--interval", type=float, default=60, help="Sample interval in seconds, default 60")
    parser.add_argument("--output", type=Path, default=Path("validation/vr-perf"))
    args = parser.parse_args()
    if not 0 <= args.minutes <= 240 or not 10 <= args.interval <= 600:
        parser.error("minutes must be 0..240, interval must be 10..600")
    try:
        run(os.environ.get("ADB_BIN", "adb"), args.output, args.label, args.minutes, args.interval)
    except (RuntimeError, subprocess.TimeoutExpired, OSError) as exc:
        parser.exit(2, "ERROR: " + str(exc) + "\n")


if __name__ == "__main__":
    main()
