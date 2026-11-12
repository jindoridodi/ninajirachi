import sys
import math
import unittest
import subprocess
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.face_geometry import display_geometry, render_header, OUTPUT
from tools.render_face_preview import RasterCanvas
from preview.face_preview import PRESETS, draw, expression_geometry


class FaceGeometryTest(unittest.TestCase):
    def test_expression_geometry_matches_firmware(self):
        root = Path(__file__).resolve().parents[1]
        cases = PRESETS + [
            {**PRESETS[0], "eyeWidth": 1, "eyeOpen": 1, "eyeOffsetX": -60,
             "eyeOffsetY": 60, "mouthWidth": 100, "mouthHeight": 100},
            {**PRESETS[0], "eyeWidth": 100, "eyeOpen": 100, "eyeOffsetX": 60,
             "eyeOffsetY": -60, "mouthWidth": 1, "mouthHeight": 1},
        ]
        keys = ("eyeWidth", "eyeOpen", "eyeOffsetX", "eyeOffsetY", "mouthWidth", "mouthHeight")
        with tempfile.TemporaryDirectory() as folder:
            executable = Path(folder) / "face-layout"
            subprocess.run(["c++", "-std=c++11", str(root / "tests/face_layout_test.cpp"),
                            "-o", str(executable)], check=True)
            data = "".join(" ".join(str(p[k]) for k in keys) + "\n" for p in cases)
            result = subprocess.run([str(executable)], input=data, text=True, capture_output=True, check=True)
        self.assertEqual(len(result.stdout.splitlines()), len(cases))
        for preset, line in zip(cases, result.stdout.splitlines()):
            eyes, mouth = expression_geometry(display_geometry(), preset)
            expected = [value for part in (eyes, mouth) for point in part for value in point]
            self.assertEqual([int(value) for value in line.split()], expected, preset["name"])

    def test_no_expression_draws_a_dot_above_the_face(self):
        for preset in PRESETS:
            with self.subTest(expression=preset["name"]):
                canvas = RasterCanvas()
                draw(canvas, preset)
                self.assertIsNone(canvas.image.crop((0, 0, 960, 60*4)).getbbox())

    def test_firmware_geometry_matches_preview(self):
        self.assertEqual(OUTPUT.read_text(), render_header())

    def test_reference_silhouette_and_eye_proportions(self):
        g = display_geometry()
        self.assertEqual(g["outline"][0], g["outline"][-1])
        self.assertEqual(len(set(y for x, y in g["outline"][:25])), 1,
                         "reference has a flat top, not a diamond apex")
        xs, ys = zip(*g["outline"])
        self.assertGreater((max(xs)-min(xs)) / (max(ys)-min(ys)), 1.8)
        x, y, width, height = g["eyes"][0]
        self.assertAlmostEqual(height / width, 240 / 135, delta=0.1)
        self.assertGreater(max(y for x, y in g["mouth"]), g["mouth"][0][1],
                           "mouth lobes curve down")

    def test_face_fits_inside_round_display(self):
        g = display_geometry()
        self.assertLessEqual(max(math.hypot(x - 119.5, y - 119.5)
                                 for x, y in g["outline"]) + g["outlineWidth"] / 2, 118)

    def test_idle_renders_with_valid_canvas_options(self):
        canvas = RasterCanvas()
        draw(canvas, PRESETS[0])
        self.assertEqual(canvas.image.getpixel((69*4, 111*4)), (0, 0, 0))
        self.assertEqual(canvas.image.getpixel((120*4, 136*4)), (255, 255, 255))
        self.assertEqual(canvas.image.getpixel((120*4, 176*4)), (255, 255, 255))

    def test_tongue_is_optional_and_visible(self):
        normal, tongue = RasterCanvas(), RasterCanvas()
        draw(normal, PRESETS[0])
        draw(tongue, {**PRESETS[0], "tongueOut": 12})
        self.assertEqual(normal.image.getpixel((120*4, 164*4)), (255, 255, 255))
        self.assertEqual(tongue.image.getpixel((120*4, 164*4)), (0, 0, 0))
        self.assertEqual(tongue.image.getpixel((113*4, 160*4)), (0, 0, 0))
        self.assertEqual(tongue.image.getpixel((120*4, 170*4)), (255, 255, 255))

    def test_openness_control_changes_the_render(self):
        from PIL import ImageChops
        opened, closed = RasterCanvas(), RasterCanvas()
        draw(opened, PRESETS[0])
        draw(closed, {**PRESETS[0], "eyeOpen": 3})
        self.assertIsNotNone(ImageChops.difference(opened.image, closed.image).getbbox())


if __name__ == "__main__":
    unittest.main()
