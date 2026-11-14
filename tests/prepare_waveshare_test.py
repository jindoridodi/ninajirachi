import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from prepare_waveshare import FILES, PREFIX


class PrepareWaveshareTest(unittest.TestCase):
    def test_fresh_install_adapts_only_spi_transfer_and_preserves_license(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "tools").mkdir()
            target = root / "firmware/src/waveshare"
            target.mkdir(parents=True)
            script = root / "tools/prepare_waveshare.py"
            script.write_bytes((Path(__file__).resolve().parents[1] / "tools/prepare_waveshare.py").read_bytes())
            archive = root / "demo.zip"
            original = b"/* vendor permission and license */\n    vspi->transfer(pData, Len);\n"
            with ZipFile(archive, "w") as zipped:
                for name in FILES:
                    zipped.writestr(PREFIX + name, original if name == "DEV_Config.cpp" else b"vendor")
            subprocess.run([sys.executable, str(script), str(archive)], check=True)
            self.assertEqual((target / "DEV_Config.cpp").read_bytes(),
                             original.replace(b"transfer(pData", b"writeBytes(pData"))
            self.assertEqual((target / "LCD_1in28.cpp").read_bytes(), b"vendor")
            # An unexpected upstream change must fail before replacing installed files.
            with ZipFile(archive, "w") as zipped:
                for name in FILES:
                    zipped.writestr(PREFIX + name, b"changed upstream")
            failed = subprocess.run([sys.executable, str(script), str(archive)],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.assertNotEqual(failed.returncode, 0)
            self.assertEqual((target / "LCD_1in28.cpp").read_bytes(), b"vendor")


if __name__ == "__main__":
    unittest.main()
