#!/usr/bin/env python3
"""Render the actual desktop drawing without Tk. Pillow is needed only here."""

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from preview.face_preview import PRESETS, draw


class RasterCanvas:
    def __init__(self):
        from PIL import Image, ImageDraw
        self.image = Image.new("RGB", (960, 960), "black")
        self.painter = ImageDraw.Draw(self.image)

    def delete(self, _):
        self.painter.rectangle((0, 0, 960, 960), fill="black")

    def create_rectangle(self, *coords, fill, outline):
        self.painter.rectangle(tuple(v * 4 for v in coords), fill=fill)

    def create_oval(self, *coords, fill, outline):
        self.painter.ellipse(tuple(v * 4 for v in coords), fill=fill)

    def create_line(self, *coords, fill, width, joinstyle, capstyle):
        assert joinstyle in ("bevel", "miter", "round")
        assert capstyle in ("butt", "projecting", "round")
        points = list(zip(coords[::2], coords[1::2]))
        self.painter.line([(x*4, y*4) for x, y in points], fill=fill,
                          width=max(1, round(width*4)), joint="curve")
        if capstyle == "round":
            for x, y in (points[0], points[-1]):
                r = width / 2
                self.painter.ellipse(((x-r)*4, (y-r)*4, (x+r)*4, (y+r)*4), fill=fill)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--preset", choices=[p["name"] for p in PRESETS], default="idle")
    args = parser.parse_args()
    canvas = RasterCanvas()
    draw(canvas, next(p for p in PRESETS if p["name"] == args.preset))
    canvas.image.resize((720, 720)).save(args.output)
