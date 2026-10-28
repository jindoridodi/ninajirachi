import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from preview.face_preview import PRESETS, export_presets
from tools.generate_face_presets import render


class PreviewEditorTest(unittest.TestCase):
    def test_edited_presets_export_for_firmware(self):
        presets = json.loads(json.dumps(PRESETS))
        presets[0]["eyeOpen"] = 35
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "presets.json"
            export_presets(presets, path)
            loaded = json.loads(path.read_text())["presets"]
            self.assertEqual(loaded[0]["eyeOpen"], 35)
            self.assertIn("kExcitedPreset", render(loaded))
            old = path.read_text()
            presets[0]["eyeOpen"] = -5
            with self.assertRaises(ValueError):
                export_presets(presets, path)
            self.assertEqual(path.read_text(), old)


if __name__ == "__main__":
    unittest.main()
