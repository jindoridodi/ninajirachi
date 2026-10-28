#!/usr/bin/env python3
"""A 240x240 desktop preview for the parameterized Ninajirachi face."""

import json
from pathlib import Path
import tkinter as tk


ROOT = Path(__file__).resolve().parents[1]
PRESETS = json.loads((ROOT / "shared" / "expression_presets.json").read_text())["presets"]


def draw(canvas: tk.Canvas, preset: dict) -> None:
    canvas.delete("all")
    canvas.create_rectangle(0, 0, 240, 240, fill=preset["background"], outline="")
    canvas.create_oval(120 - preset["faceRadius"], 120 - preset["faceRadius"],
                       120 + preset["faceRadius"], 120 + preset["faceRadius"],
                       fill=preset["face"], outline="")
    for x in (72, 168):
        canvas.create_oval(x - preset["eyeWidth"] // 2, 92 - preset["eyeOpen"] // 2,
                           x + preset["eyeWidth"] // 2, 92 + preset["eyeOpen"] // 2,
                           fill=preset["eye"], outline="")
        canvas.create_rectangle(x - preset["eyeWidth"] // 2, 92 - preset["eyeOpen"] // 2,
                                x + preset["eyeWidth"] // 2,
                                92 - preset["eyeOpen"] // 2 + preset["eyelidHeight"],
                                fill=preset["eyelid"], outline="")
        px = x + preset["pupilOffsetX"]
        py = 92 + preset["pupilOffsetY"]
        canvas.create_oval(px - preset["pupilRadius"], py - preset["pupilRadius"],
                           px + preset["pupilRadius"], py + preset["pupilRadius"],
                           fill=preset["pupil"], outline="")
    canvas.create_oval(120 - preset["mouthWidth"] // 2, 174 - preset["mouthHeight"] // 2,
                       120 + preset["mouthWidth"] // 2, 174 + preset["mouthHeight"] // 2,
                       fill=preset["mouth"], outline="")
    if preset["effectRadius"]:
        radius = preset["effectRadius"]
        canvas.create_oval(120 - radius, 32 - radius, 120 + radius, 32 + radius,
                           fill=preset["effect"], outline="")


def main() -> None:
    root = tk.Tk()
    root.title("Ninajirachi face preview")
    canvas = tk.Canvas(root, width=240, height=240, highlightthickness=0)
    canvas.pack()
    preset_by_name = {preset["name"]: preset for preset in PRESETS}
    selected = tk.StringVar(value=PRESETS[0]["name"])
    tk.OptionMenu(root, selected, *preset_by_name, command=lambda name: draw(canvas, preset_by_name[name])).pack()
    draw(canvas, preset_by_name[selected.get()])
    root.mainloop()


if __name__ == "__main__":
    main()
