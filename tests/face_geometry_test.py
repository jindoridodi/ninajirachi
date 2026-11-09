import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.face_geometry import display_geometry, render_header, OUTPUT
from tools.render_face_preview import RasterCanvas
from preview.face_preview import PRESETS, draw


class FaceGeometryTest(unittest.TestCase):
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

    def test_idle_renders_with_valid_canvas_options(self):
        canvas = RasterCanvas()
        draw(canvas, PRESETS[0])
        # Center of left eye and area below mouth: blue and black respectively.
        self.assertEqual(canvas.image.getpixel((69*4, 95*4)), (0, 0, 238))
        self.assertEqual(canvas.image.getpixel((120*4, 160*4)), (0, 0, 0))

    def test_openness_control_changes_the_render(self):
        from PIL import ImageChops
        opened, closed = RasterCanvas(), RasterCanvas()
        draw(opened, PRESETS[0])
        draw(closed, {**PRESETS[0], "eyeOpen": 3})
        self.assertIsNotNone(ImageChops.difference(opened.image, closed.image).getbbox())


if __name__ == "__main__":
    unittest.main()
