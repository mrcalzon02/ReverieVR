#!/usr/bin/env python3
"""Reproduce Breakwater's curated mono PCM16 audio bank from verified originals.

Offline preparation only: Python 3, NumPy and FFmpeg/ffprobe. No runtime decoder
or downloaded content is required by the game. Original files are never edited.
The companion source_catalog.json is public-safe; private storage IDs are absent.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import wave

import numpy as np

RATE = 22050
LOOP_SECONDS = 6.0
CROSSFADE_SECONDS = 0.25
GROUPS = {
    "control": {"peak_dbfs": -12.0, "active_rms_dbfs": -22.0},
    "combat": {"peak_dbfs": -9.0, "active_rms_dbfs": -18.0},
    "loop": {"peak_dbfs": -18.0, "active_rms_dbfs": -24.0},
}


def amp(db: float) -> float:
    return 10.0 ** (db / 20.0)


def dbfs(value: float) -> float:
    return 20.0 * math.log10(max(value, 1e-12))


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def active_rms(samples: np.ndarray) -> float:
    """RMS of 20 ms blocks within 25 dB of the loudest block; silence excluded."""
    size = round(RATE * 0.020)
    total_energy = 0.0
    total_frames = 0
    blocks = []
    for i in range(0, len(samples), size):
        block = samples[i:i + size]
        energy = float(np.dot(block, block))
        blocks.append((energy, len(block)))
    best = max((math.sqrt(e / n) for e, n in blocks), default=0.0)
    gate = max(1e-7, best * amp(-25.0))
    for energy, frames in blocks:
        if math.sqrt(energy / frames) >= gate:
            total_energy += energy
            total_frames += frames
    return math.sqrt(total_energy / total_frames) if total_frames else 0.0


def faded(samples: np.ndarray, fade_in: float = 0.003,
          fade_out: float = 0.015) -> np.ndarray:
    result = samples.copy()
    for duration, front in ((fade_in, True), (fade_out, False)):
        frames = min(round(duration * RATE), len(result) // 3)
        if frames > 1:
            ramp = np.sin(np.linspace(0.0, math.pi / 2, frames)) ** 2
            if front:
                result[:frames] *= ramp
            else:
                result[-frames:] *= ramp[::-1]
    if len(result):
        result[0] = result[-1] = 0.0
    return result


def lowpass(samples: np.ndarray, cutoff: float) -> np.ndarray:
    taps = 129
    n = np.arange(taps, dtype=np.float64) - (taps - 1) / 2
    kernel = (2.0 * cutoff / RATE) * np.sinc(2.0 * cutoff * n / RATE)
    kernel *= np.hanning(taps)
    kernel /= np.sum(kernel)
    return np.convolve(samples, kernel, mode="same")


def retime(samples: np.ndarray, speed: float) -> np.ndarray:
    if speed == 1.0:
        return samples.copy()
    if speed <= 0:
        raise ValueError("Playback speed must be positive")
    # Explicit deterministic varispeed. Filter before faster playback to retain
    # headroom below the new Nyquist limit; slower playback interpolates linearly.
    source = lowpass(samples, RATE * 0.43 / speed) if speed > 1.0 else samples
    positions = np.arange(math.floor((len(source) - 1) / speed)) * speed
    return np.interp(positions, np.arange(len(source)), source)


def circular_loop(samples: np.ndarray) -> np.ndarray:
    """6 s output with a 250 ms equal-power crossfade across the cycle join.

    Read 6.25 s: blend the extra tail into the first 250 ms, then append the
    remaining first 6 s. The file boundary follows adjacent original samples;
    the crossfade transitions back to the head inside the loop, without silence.
    """
    n = round(LOOP_SECONDS * RATE)
    fade = round(CROSSFADE_SECONDS * RATE)
    source = samples.copy()
    if len(source) < 2 * fade:
        raise ValueError("A loop source must contain at least 500 ms")
    angle = np.linspace(0.0, math.pi / 2, fade)
    tail_gain, head_gain = np.cos(angle), np.sin(angle)
    base = source.copy()
    while len(source) < n + fade:
        join = source[-fade:] * tail_gain + base[:fade] * head_gain
        source = np.concatenate((source[:-fade], join, base[fade:]))
    source = source[:n + fade]
    join = source[n:n + fade] * tail_gain + source[:fade] * head_gain
    return np.concatenate((join, source[fade:n]))


def soft_peak_limit(samples: np.ndarray, ceiling: float) -> np.ndarray:
    """Smooth knee above half the ceiling, linear below it; no hard clipping.

    This is deliberate offline peak compression. A 50 ms envelope limiter was
    measured first, but broadband source peaks held it down and
    missed the specified loop RMS by up to 2.5 dB. The memoryless smooth knee
    reaches the requested active level while retaining the guaranteed bound.
    Its continuous slope also preserves an already crossfaded circular join.
    """
    knee = ceiling * .5
    magnitude = np.abs(samples)
    result = magnitude.copy()
    above = magnitude > knee
    result[above] = knee + (ceiling - knee) * np.tanh(
        (magnitude[above] - knee) / (ceiling - knee))
    return np.copysign(result, samples)


def normalize(samples: np.ndarray, group: str,
              circular: bool) -> tuple[np.ndarray, dict]:
    policy = GROUPS[group]
    # One PCM step of safety avoids rounding a sample above the specified bound.
    ceiling = (math.floor(amp(policy["peak_dbfs"]) * 32768.0) - 1) / 32768.0
    target = amp(policy["active_rms_dbfs"])
    before = active_rms(samples)
    if before <= 1e-10 or not np.all(np.isfinite(samples)):
        raise ValueError("Empty, silent or non-finite cue")
    linear_gain = target / before
    gain = linear_gain
    linear_peak = float(np.max(np.abs(samples))) * gain
    method = "linear gain"
    iterations = 0
    if linear_peak <= ceiling:
        result = samples * gain
    else:
        method = "linear gain with smooth-knee peak compression"
        lo, hi = 0.0, linear_gain * 2.0
        max_gain = linear_gain * 256.0
        while active_rms(soft_peak_limit(samples * hi, ceiling)) < target and hi < max_gain:
            hi = min(max_gain, hi * 2.0)
        for iterations in range(32):
            gain = (lo + hi) * .5
            result = soft_peak_limit(samples * gain, ceiling)
            measured = active_rms(result)
            if abs(dbfs(measured) - policy["active_rms_dbfs"]) < .01:
                break
            if measured < target:
                lo = gain
            else:
                hi = gain
    if not circular:
        result[0] = result[-1] = 0.0
    maximum = float(np.max(np.abs(result)))
    if maximum > ceiling + 1e-9:
        raise AssertionError("Limiter exceeded its safe sample peak")
    return result, {
        "active_rms_definition": "20 ms blocks within 25 dB of the loudest block",
        "target_active_rms_dbfs": policy["active_rms_dbfs"],
        "peak_ceiling_dbfs": policy["peak_dbfs"],
        "pregain_db": round(dbfs(gain), 4),
        "maximum_limiter_reduction_db": round(max(0.0, dbfs(
            float(np.max(np.abs(samples))) * gain / ceiling)), 4),
        "method": method,
        "limiter": "linear below half the ceiling; continuous-slope tanh knee above it; no hard clipping",
        "knee_db_below_ceiling": round(dbfs(.5), 4),
        "normalization_iterations": iterations + 1,
        "target_reached": abs(dbfs(active_rms(result)) - policy["active_rms_dbfs"]) < 0.15,
    }


@dataclass
class Clip:
    samples: np.ndarray
    sources: set[str]
    recipe: dict


class Builder:
    def __init__(self, source_dir: Path, synth_dir: Path, output_dir: Path,
                 catalog_path: Path):
        self.source_dir, self.synth_dir = source_dir, synth_dir
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.catalog = json.loads(catalog_path.read_text())
        self.sources = {s["key"]: s for s in self.catalog["sources"]}
        self.decoded: dict[str, np.ndarray] = {}
        self.outputs: list[dict] = []

    def load(self, key: str) -> np.ndarray:
        if key in self.decoded:
            return self.decoded[key]
        source = self.sources[key]
        root = self.synth_dir if source.get("location") == "synth" else self.source_dir
        path = root / source["filename"]
        if digest(path) != source["sha256"] or path.stat().st_size != source["bytes"]:
            raise ValueError(f"Source bytes do not match admitted original: {path.name}")
        info = json.loads(subprocess.check_output([
            "ffprobe", "-v", "error", "-select_streams", "a:0",
            "-show_entries", "stream=channels,sample_rate,bits_per_sample,codec_name",
            "-of", "json", str(path)]))["streams"][0]
        channels = int(info["channels"])
        if channels not in (1, 2):
            raise ValueError(f"Unexpected channel layout: {path.name}")
        # Arithmetic average avoids ffmpeg's +3 dB stereo-to-mono summation.
        pan = "pan=mono|c0=c0" if channels == 1 else "pan=mono|c0=0.5*c0+0.5*c1"
        decoded = subprocess.check_output([
            "ffmpeg", "-v", "error", "-nostdin", "-i", str(path),
            "-map", "0:a:0", "-af", pan, "-ar", str(RATE),
            "-c:a", "pcm_f64le", "-f", "f64le", "pipe:1"])
        samples = np.frombuffer(decoded, dtype="<f8").copy()
        if not len(samples) or not np.all(np.isfinite(samples)):
            raise ValueError(f"Invalid decoded source: {path.name}")
        samples -= float(np.mean(samples))
        self.decoded[key] = samples
        return samples

    def part(self, key: str, *, speed: float = 1.0, duration: float | None = None,
             start: float = 0.0, peak_focus: float | None = None,
             low: float | None = None, high: float | None = None,
             trim: bool = True, envelope: float | None = None) -> Clip:
        samples = self.load(key).copy()
        if start:
            samples = samples[round(start * RATE):]
        if high:
            samples -= lowpass(samples, high)
        if low:
            samples = lowpass(samples, low)
        samples = retime(samples, speed)
        if peak_focus:
            frames = round(peak_focus * RATE)
            lead = min(round(0.015 * RATE), frames // 4)
            begin = max(0, int(np.argmax(np.abs(samples))) - lead)
            samples = samples[begin:begin + frames]
        elif trim:
            useful = np.flatnonzero(np.abs(samples) >= max(1e-9,
                float(np.max(np.abs(samples))) * amp(-45)))
            if len(useful):
                begin = max(0, int(useful[0]) - round(0.004 * RATE))
                end = min(len(samples), int(useful[-1]) + round(0.020 * RATE) + 1)
                samples = samples[begin:end]
        if duration:
            samples = samples[:round(duration * RATE)]
        if len(samples) < 2:
            raise ValueError(f"Source selection too short: {key}")
        if envelope:
            samples *= np.exp(-np.arange(len(samples)) / RATE * envelope)
        if trim:
            samples = faded(samples)
        peak = float(np.max(np.abs(samples)))
        if peak > 1e-10:
            samples /= peak
        return Clip(samples, {key}, {"op": "source", "key": key,
            "start_seconds": start, "varispeed": speed,
            "duration_limit_seconds": duration, "peak_focus_seconds": peak_focus,
            "lowpass_hz": low, "highpass_hz": high,
            "trim_db_below_peak": -45 if trim else None,
            "edge_fades_ms": [3, 15] if trim else None,
            "exponential_decay_per_second": envelope,
            "component_peak_normalized": True})

    def mix(self, layers: list[tuple[Clip, float, float]],
            duration: float | None = None) -> Clip:
        count = max(round(t * RATE) + len(c.samples) for c, t, g in layers)
        if duration:
            count = round(duration * RATE)
        result = np.zeros(count, dtype=np.float64)
        sources = set()
        recipe = []
        for clip, at, gain in layers:
            begin = round(at * RATE)
            end = min(count, begin + len(clip.samples))
            if begin < end:
                result[begin:end] += clip.samples[:end - begin] * gain
            sources.update(clip.sources)
            recipe.append({"at_seconds": at, "gain": gain, "input": clip.recipe})
        return Clip(result, sources, {"op": "mix", "duration_seconds": duration,
                                    "layers": recipe})

    def emit(self, name: str, clip: Clip, group: str, description: str,
             loop: bool = False) -> None:
        samples = circular_loop(clip.samples) if loop else faded(clip.samples)
        samples, normalization = normalize(samples, group, loop)
        pcm = np.rint(samples * 32768.0).astype("<i2")
        path = self.output_dir / (name + ".wav")
        with wave.open(str(path), "wb") as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(RATE)
            output.writeframes(pcm.tobytes())
        actual = pcm.astype(np.float64) / 32768.0
        peak = float(np.max(np.abs(actual)))
        if peak > amp(GROUPS[group]["peak_dbfs"]) or np.any(np.abs(pcm.astype(np.int32)) >= 32767):
            raise AssertionError(f"Invalid peak or clipped PCM: {name}")
        if pcm.nbytes > 1_000_000:
            raise AssertionError(f"Decoded buffer exceeds one MB: {name}")
        metrics = {
            "sample_peak_dbfs": round(dbfs(peak), 4),
            "rms_dbfs": round(dbfs(float(np.sqrt(np.mean(actual * actual)))), 4),
            "active_rms_dbfs": round(dbfs(active_rms(actual)), 4),
            "dc_offset": round(float(np.mean(actual)), 9),
            "clipped_samples": 0,
        }
        if loop:
            metrics["loop_boundary_step_dbfs"] = round(dbfs(abs(float(actual[0] - actual[-1]))), 4)
            metrics["adjacent_step_p99_dbfs"] = round(dbfs(float(np.percentile(np.abs(np.diff(actual)), 99))), 4)
        self.outputs.append({
            "file": path.name, "group": group, "description": description,
            "source_keys": sorted(clip.sources),
            "licenses": sorted({self.sources[k]["license"] for k in clip.sources}),
            "sha256": digest(path), "bytes": path.stat().st_size,
            "sample_rate_hz": RATE, "channels": 1, "bits_per_sample": 16,
            "frames": len(pcm), "decoded_bytes": pcm.nbytes,
            "duration_seconds": round(len(pcm) / RATE, 6),
            "loop": loop,
            "circular_crossfade_seconds": CROSSFADE_SECONDS if loop else None,
            "circular_crossfade_frames": round(CROSSFADE_SECONDS * RATE) if loop else None,
            "recipe": clip.recipe, "normalization": normalization, "metrics": metrics,
        })


def build_bank(b: Builder) -> None:
    p, m, e = b.part, b.mix, b.emit
    # The 15 mechanical controls use genuine admitted NOX recordings.
    click = p("click", peak_focus=.115, high=100, low=7000)
    tick = p("click2", peak_focus=.085, high=200, low=6500)
    low_tick = p("click2", speed=.76, peak_focus=.125, high=100, low=2800)
    latch = p("latch", duration=.27, high=100, low=6000)
    key = p("handle", duration=.24, high=120, low=5500)
    e("ui_focus", tick, "control", "Short recorded physical switch tick for target focus.")
    e("ui_activate", click, "control", "Recorded physical switch confirmation.")
    e("ui_failure", m([(low_tick, 0, 1), (low_tick, .125, .8)]), "control",
      "Two low recorded switch knocks distinguish rejected activation.")
    e("equip", m([(key, 0, .65), (latch, .11, 1)]), "control", "Key and latch equipment handling.")
    e("reload_start", m([(p("metal", duration=.24, speed=.88, low=4500), 0, .6),
                           (key, .09, .8)]), "control", "Recorded hardware unlock and breech handling.")
    e("reload_ready", m([(latch, 0, 1), (click, .13, .6)]), "control", "Recorded latch seats and confirms ready.")
    e("menu_open", m([(key, 0, .6), (p("click2", speed=1.25, peak_focus=.09), .09, 1)]),
      "control", "Two rising hardware gestures for opening the command panel.")
    e("menu_back", m([(click, 0, .7), (low_tick, .085, .9)]), "control", "Descending pair for return/back.")
    e("station_move", m([(p("metal_drag", duration=.33, high=120, low=4000), 0, .65),
                           (latch, .12, .4)]), "control", "Owner metal handling plus latch for station movement.")
    e("support_arm", m([(click, 0, 1), (p("click2", speed=1.4, peak_focus=.09), .12, .85)]),
      "control", "Ascending double switch confirms support targeting armed.")
    e("support_disarm", m([(tick, 0, .8), (low_tick, .13, .85)]), "control",
      "Descending double switch confirms support targeting cancelled.")
    e("dry_fire", p("latch", peak_focus=.115, high=220, low=6500), "control", "Recorded latch dry click.")
    e("aim_limit", m([(tick, 0, .8), (tick, .065, .45)]), "control", "Brief paired stop-detent ticks.")
    e("cooldown", m([(low_tick, 0, .7), (low_tick, .20, .65)]), "control", "Spaced low ticks for a cooling weapon.")
    e("support_unavailable", m([(low_tick, 0, .9), (low_tick, .13, .7), (low_tick, .26, .5)]),
      "control", "Three descending knocks for unavailable support.")

    # Seven hardware-looped layers; replacements are honestly described as
    # derived engine/water Foley where no matching field recording exists.
    e("surf_loop", p("surf", start=3, high=60, low=7500, trim=False), "loop",
      "NOX recorded coastal sea ambience.", True)
    e("wind_loop", p("wind", start=3, high=60, low=5500, trim=False), "loop",
      "NOX recorded calm wind ambience.", True)
    e("night_loop", p("night", start=3, high=100, low=7500, trim=False), "loop",
      "NOX recorded night ambience.", True)
    e("boat_loop", p("engine", start=.4, speed=.76, high=45, low=1800, trim=False), "loop",
      "Boat machinery treatment derived from a NOX truck recording; not a boat field recording.", True)
    wa = p("water_walk_a", speed=.85, duration=.55, high=100, low=6500)
    wb = p("water_walk_b", speed=.90, duration=.55, high=100, low=6500)
    layers = [(p("surf", start=8, duration=6.25, low=4500, trim=False), 0, .5)]
    layers += [(wa if i % 2 == 0 else wb, i * 1.5, .30) for i in range(5)]
    e("rowing_loop", m(layers, duration=6.25), "loop",
      "Rhythmic water displacement assembled from NOX water footsteps and quiet surf; not an oar field recording.", True)
    aircraft = p("aircraft_engine", start=1, speed=1.18, high=55, low=2800, trim=False)
    phase = np.arange(len(aircraft.samples), dtype=np.float64) / RATE
    aircraft.samples *= .78 + .22 * np.sin(2 * math.pi * 14 * phase)
    aircraft.recipe = {"op": "amplitude_modulate", "hz": 14, "offset": .78,
                       "depth": .22, "input": aircraft.recipe}
    e("aircraft_loop", m([(aircraft, 0, 1),
        (p("wind", start=6, duration=8, high=400, low=4500, trim=False), 0, .12)]),
      "loop", "Propeller-like treatment derived from NOX car engine and wind; not an aircraft field recording.", True)
    e("traverse_loop", p("traverse_motor", start=.5, duration=2.8, speed=.88,
        high=110, low=3500, trim=False), "loop",
      "Turret traverse motor treatment derived from NOX electric-window machinery.", True)

    # Authored weapon signatures are retained and layered with the owner's
    # admitted grenade/metal exports. These are designed effects, not recordings
    # of real firearms. First-party baseline cue bytes are individually pinned.
    metal = p("metal_thwak", duration=.20, high=130, low=6500)
    boom = p("explosion", duration=.70, high=40, low=4500)
    short_boom = p("explosion", speed=1.30, duration=.34, high=50, low=3000)
    splash = p("splash", peak_focus=.85, high=100, low=6500)
    synth = lambda i: p("synth_" + str(i), trim=False)
    e("weapon_machine", m([(synth(5), 0, 1), (metal, .015, .16)]), "combat",
      "Authored machine-gun signature plus owner metal accent; designed firearm effect.")
    e("weapon_heavy", m([(synth(6), 0, 1), (short_boom, .025, .28), (metal, .01, .12)]), "combat",
      "Authored heavy-gun signature plus owner grenade and metal layers; designed firearm effect.")
    e("weapon_flak", m([(synth(7), 0, 1), (metal, .01, .22), (short_boom, .025, .20)]), "combat",
      "Authored flak signature plus owner mechanical and blast layers.")
    e("weapon_missile", m([(synth(8), 0, 1),
        (p("wind", start=8, duration=.42, high=200, low=5500, envelope=4), 0, .22)]), "combat",
      "Authored missile exhaust with a short recorded wind texture.")
    e("weapon_cannon", m([(synth(9), 0, 1), (boom, .015, .45), (metal, .025, .15)]), "combat",
      "Authored cannon signature plus owner grenade and breech metal; designed firearm effect.")
    e("impact_hull", m([(synth(10), 0, .7), (metal, 0, .8)]), "combat", "Authored hull hit with recorded owner metal strike.")
    e("impact_water", splash, "combat", "Real NOX water splash cut from a large water footstep/jump.")
    e("boat_destroyed", m([(synth(12), 0, .75), (boom, 0, .85), (splash, .12, .4)], .95), "combat",
      "Boat destruction composite: authored blast, owner grenade and NOX water splash.")
    e("gunboat_salvo", m([(synth(13), 0, 1), (short_boom, .025, .30), (metal, .02, .15)]), "combat",
      "Authored offshore gun report with admitted blast and mechanical layers.")
    e("infantry_volley", m([(synth(14), 0, 1), (metal, .02, .10)]), "combat",
      "Authored burst with a restrained recorded metal transient.")
    e("aircraft_bomb", m([(synth(15), 0, .8), (boom, 0, .90), (splash, .10, .22)], .95), "combat",
      "Bomb impact composite using authored report, owner grenade and NOX water.")
    e("aircraft_strafe", m([(synth(16), 0, 1), (metal, .015, .10)]), "combat",
      "Authored strafing burst with a restrained metal accent.")
    e("sam_launch", m([(synth(17), 0, 1),
        (p("wind", start=11, duration=.46, high=300, low=6000, envelope=4), 0, .20)]), "combat",
      "Authored SAM exhaust with recorded wind texture.")
    e("wave_start", m([(synth(18), 0, 1), (click, 0, .15)]), "control", "Authored wave-start signal with mechanical attack.")
    e("shore_collision", m([(synth(19), 0, .8), (metal, 0, .7), (wa, .09, .5)], .60), "combat",
      "Shore collision composite using authored impact, owner metal and NOX water.")
    for name, cue, description in [
        ("shop_purchase", 20, "Successful purchase"), ("wave_clear", 21, "Wave cleared"),
        ("victory", 22, "Campaign victory"), ("defeat", 23, "Battery defeat")]:
        e(name, m([(synth(cue), 0, 1), (click, 0, .10)]), "control",
          description + ": preserved authored notification with recorded switch transient.")
    e("armored_gun", m([(synth(24), 0, 1), (short_boom, .02, .32), (metal, .02, .15)]), "combat",
      "Authored armored-gun signature plus owner blast/metal layers.")
    e("armor_destroyed", m([(synth(25), 0, .85), (boom, 0, .85), (metal, .12, .5)]), "combat",
      "Authored armor rupture plus owner grenade and metal fragments.")
    e("sand_impact", p("synth_11", speed=1.5, duration=.22, high=250, low=4300, envelope=12), "combat",
      "Short filtered first-party procedural impact texture; no sand recording was available in the inspected library folders.")
    e("artillery", m([(synth(3), 0, 1), (boom, .03, .85)]), "combat",
      "Authored artillery impact layered with the owner's grenade recording.")
    e("airstrike", m([(synth(4), 0, 1), (short_boom, .05, .75),
                      (short_boom, .34, .65), (short_boom, .67, .55)]), "combat",
      "Authored support airstrike with three staggered owner-recorded blast accents.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", required=True, type=Path)
    parser.add_argument("--synth-dir", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--catalog", type=Path, default=Path(__file__).with_name("source_catalog.json"))
    args = parser.parse_args()
    builder = Builder(args.source_dir, args.synth_dir, args.output_dir, args.catalog)
    build_bank(builder)
    used = set().union(*(set(r["source_keys"]) for r in builder.outputs))
    total = sum(r["bytes"] for r in builder.outputs)
    manifest = {
        "schema": 1, "bank": "Breakwater normalized recorded/procedural bank",
        "format": "RIFF WAV, PCM signed 16-bit little-endian, mono, 22050 Hz",
        "preparation_script_sha256": digest(Path(__file__)),
        "input_catalog_sha256": digest(args.catalog),
        "toolchain": {"python": sys.version.split()[0], "numpy": np.__version__,
            "ffmpeg": subprocess.check_output(["ffmpeg", "-version"], text=True).splitlines()[0]},
        "normalization_groups": GROUPS,
        "loop_seconds": LOOP_SECONDS, "loop_crossfade_seconds": CROSSFADE_SECONDS,
        "total_wav_bytes": total,
        "sources": [s for s in builder.catalog["sources"] if s["key"] in used],
        "outputs": builder.outputs,
    }
    (args.output_dir / "audio_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Prepared {len(builder.outputs)} WAVs; {total:,} bytes; {len(used)} verified sources")
    for row in builder.outputs:
        stats = row["metrics"]
        print(f"{row['file']:<25} {row['duration_seconds']:>6.3f}s "
              f"peak {stats['sample_peak_dbfs']:>7.2f}  active RMS {stats['active_rms_dbfs']:>7.2f} "
              f"target {'PASS' if row['normalization']['target_reached'] else 'PEAK-LIMITED'}")
    if total > 3_000_000:
        raise SystemExit("Prepared bank exceeded the three MB budget")


if __name__ == "__main__":
    main()
