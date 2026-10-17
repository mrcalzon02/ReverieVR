"""Privately recover Between Deliveries SoundWave audio with Unreal Editor 5.7.

Run with -ExecutePythonScript in the restored original .uproject. No Unreal
packages are modified and no exported audio is admitted to the public repo.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import wave

SOURCE_FOLDERS = {"music": "/Game/Sounds/Music", "ui": "/Game/Sounds/Ui"}


def check_wav(path):
    """Require a complete PCM WAV and record actual decoded frame metadata."""
    if not path.is_file() or path.stat().st_size < 44:
        raise ValueError("Missing or too-small WAV export")
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    with wave.open(str(path), "rb") as reader:
        channels, sample_bytes, rate, frames, compression, _ = reader.getparams()
        if compression != "NONE" or not (1 <= channels <= 8) or not (1 <= sample_bytes <= 4):
            raise ValueError("Unsupported WAV channel/sample encoding")
        if not (8000 <= rate <= 192000) or frames <= 0:
            raise ValueError("Invalid WAV sample rate or duration")
        remaining = frames
        while remaining:
            count = min(remaining, 32768)
            if len(reader.readframes(count)) != count * channels * sample_bytes:
                raise ValueError("Truncated WAV sample payload")
            remaining -= count
    return {"size_bytes": path.stat().st_size, "sha256": digest.hexdigest(),
            "channels": channels, "sample_rate_hz": rate,
            "sample_width_bits": sample_bytes * 8, "frames": frames,
            "duration_seconds": round(frames / rate, 4)}


def export_audio(unreal, output):
    """Export only real SoundWave objects; keep failures and rights explicit."""
    records, failures = [], []
    for category, folder in SOURCE_FOLDERS.items():
        objects = sorted(set(map(str, unreal.EditorAssetLibrary.list_assets(folder, True, False))))
        if not objects:
            failures.append({"source_folder": folder, "error": "No assets found"})
        seen = set()
        for object_path in objects:
            if not object_path.startswith(folder + "/"):
                continue
            try:
                asset = unreal.EditorAssetLibrary.load_asset(object_path)
                if asset is None:
                    raise ValueError("Unreal asset cannot be loaded")
                if not isinstance(asset, unreal.SoundWave):
                    continue  # SoundCues are not the original recorded samples.
                name = str(asset.get_name())
                # Preserve the Unicode Unreal object name in source metadata,
                # but turn typography such as the apostrophe in
                # People’s_Front_Rising into a portable Windows-safe filename.
                safe_name = re.sub(r"[^A-Za-z0-9_-]", "_", name)
                if (not 1 <= len(safe_name) <= 120 or
                        safe_name.strip("_-") == ""):
                    raise ValueError("Unsafe SoundWave name")
                if safe_name.lower() in seen:
                    raise ValueError("Ambiguous duplicate SoundWave filename")
                seen.add(safe_name.lower())
                target = output / category / (safe_name + ".wav")
                target.parent.mkdir(parents=True, exist_ok=True)
                status = "existing - untouched"
                if not target.exists():
                    task = unreal.AssetExportTask()
                    task.object = asset
                    task.filename = str(target)
                    task.exporter = unreal.SoundExporterWAV()
                    task.automated = True
                    task.prompt = False
                    task.replace_identical = False
                    if not unreal.Exporter.run_asset_export_task(task):
                        raise ValueError("Unreal WAV exporter failed")
                    status = "exported"
                details = check_wav(target)
                records.append({"source_unreal_object": object_path, "kind": "soundwave",
                                "category": category, "file": target.relative_to(output).as_posix(),
                                "status": status, **details,
                                "rights_status": "owner-confirmed"})
            except (OSError, ValueError, wave.Error, RuntimeError) as failure:
                failures.append({"source_unreal_object": object_path, "error": str(failure)})
    return {"status": "owner-confirmed original audio recovery; Android packaging not yet verified",
            "records": records, "failures": failures,
            "files_validated": len(records)}


def main():
    import unreal  # Available only in a compatible Unreal Editor session.
    root = Path(__file__).resolve().parents[2]
    output = Path(os.environ.get("BETW_DELIVERIES_EXPORT_DIR",
                                 str(Path.home() / "BetweenDeliveries_UnrealExports"))).expanduser().resolve()
    if output == root or root in output.parents:
        raise ValueError("Private exports cannot be placed inside the public Git checkout")
    output.mkdir(parents=True, exist_ok=True)
    result = export_audio(unreal, output / "audio")
    destination = output / "audio_manifest.json"
    temp = destination.with_suffix(".json.tmp")
    temp.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    temp.replace(destination)
    print("Between Deliveries audio recovery: {} WAVs, {} failures".format(
        result["files_validated"], len(result["failures"])))
    if result["failures"] or not result["files_validated"]:
        raise RuntimeError("Audio export incomplete; inspect private audio_manifest.json")


if __name__ == "__main__":
    main()
