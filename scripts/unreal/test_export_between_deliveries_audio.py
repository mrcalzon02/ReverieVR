"""Host-only tests for private Between Deliveries SoundWave recovery."""
import hashlib
from pathlib import Path
import tempfile
import unittest
import wave
from types import SimpleNamespace

from export_between_deliveries_audio import check_wav, export_audio


def write_wav(path, frames=20):
    with wave.open(str(path), "wb") as writer:
        writer.setnchannels(2)
        writer.setsampwidth(2)
        writer.setframerate(22050)
        writer.writeframes(b"\0\1\2\3" * frames)


class FakeSoundWave:
    def __init__(self, name):
        self.name = name

    def get_name(self):
        return self.name


class FakeUnreal:
    SoundWave = FakeSoundWave

    def __init__(self, values, failures=None):
        self.values = values
        self.failures = failures or set()
        self.EditorAssetLibrary = SimpleNamespace(
            list_assets=lambda folder, *_: self.values.get(folder, []),
            load_asset=lambda path: FakeSoundWave(path.rsplit("/", 1)[-1]))
        self.AssetExportTask = lambda: SimpleNamespace()
        self.SoundExporterWAV = lambda: object()
        self.Exporter = SimpleNamespace(run_asset_export_task=self.do_export)

    def do_export(self, task):
        if task.object.name in self.failures:
            return False
        write_wav(Path(task.filename))
        return True


class AudioRecoveryTest(unittest.TestCase):
    def test_complete_wav_has_correct_audio_metadata_and_hash(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "a.wav"
            write_wav(source)
            actual = check_wav(source)
            self.assertEqual((actual["channels"], actual["sample_rate_hz"],
                              actual["sample_width_bits"], actual["frames"]),
                             (2, 22050, 16, 20))
            self.assertEqual(actual["sha256"], hashlib.sha256(source.read_bytes()).hexdigest())

    def test_truncated_sample_payload_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "a.wav"
            write_wav(source)
            source.write_bytes(source.read_bytes()[:-4])
            with self.assertRaises((ValueError, EOFError, wave.Error)):
                check_wav(source)

    def test_exports_only_soundwaves_and_preserves_existing_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = "/Game/Sounds/Music/Theme"
            fx = "/Game/Sounds/Ui/Coin"
            engine = FakeUnreal({"/Game/Sounds/Music": [src], "/Game/Sounds/Ui": [fx]})
            first = export_audio(engine, root)
            self.assertEqual(first["files_validated"], 2)
            self.assertFalse(first["failures"])
            filename = root / "music/Theme.wav"
            content = filename.read_bytes()
            second = export_audio(FakeUnreal(engine.values, {"Theme", "Coin"}), root)
            self.assertEqual(filename.read_bytes(), content)
            self.assertEqual([r["status"] for r in second["records"]],
                             ["existing - untouched", "existing - untouched"])

    def test_unsafe_names_and_failed_exports_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = "/Game/Sounds/Music"
            engine = FakeUnreal({folder: [folder + "/Bad Name", folder + "/Missing"]},
                                {"Missing"})
            outcome = export_audio(engine, Path(tmp))
            self.assertEqual(outcome["files_validated"], 0)
            self.assertEqual(len(outcome["failures"]), 3)  # Also missing Ui assets.

    def test_non_audio_objects_skipped_and_missing_folders_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            engine = FakeUnreal({"/Game/Sounds/Music": ["/Game/Sounds/Music/Cue"]})
            engine.EditorAssetLibrary.load_asset = lambda *_: object()
            outcome = export_audio(engine, Path(tmp))
            self.assertFalse(outcome["records"])
            self.assertEqual(len(outcome["failures"]), 1)


if __name__ == "__main__":
    unittest.main()
