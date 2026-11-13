#!/usr/bin/env python3
"""A 240x240 desktop preview for the parameterized Ninajirachi face."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.generate_face_presets import COLOR_KEYS, METRIC_BOUNDS, SOURCE, render, export_presets
from tools.face_geometry import display_geometry


ROOT = Path(__file__).resolve().parents[1]
PRESETS = json.loads((ROOT / "shared" / "expression_presets.json").read_text())["presets"]


def expression_geometry(geometry, preset):
    eyes = []
    for x, y, base_width, base_height in geometry["eyes"]:
        width = max(1, base_width * preset["eyeWidth"] // 36)
        height = max(1, base_height * preset["eyeOpen"] // 42)
        eyes.append((x + int((base_width-width)/2) + preset["eyeOffsetX"],
                     y + int((base_height-height)/2) + preset["eyeOffsetY"], width, height))
    mouth_y = geometry["mouth"][0][1]
    mouth = [(120 + (x-120)*preset["mouthWidth"]/42,
              mouth_y + (y-mouth_y)*preset["mouthHeight"]/14)
             for x, y in geometry["mouth"]]
    return eyes, mouth


def tongue_geometry(mouth, preset):
    x, y = mouth[len(mouth) // 2]
    tongue = preset.get("tongueOut", 0)
    return int(x) - 9, int(y) + 8, 18, max(1, tongue + 4), 9, tongue > 0


def draw(canvas, preset: dict) -> None:
    canvas.delete("all")
    canvas.create_rectangle(0, 0, 240, 240, fill=preset["background"], outline="")
    geometry = display_geometry()

    def stroke(points, color, width):
        canvas.create_line(*[value for point in points for value in point],
                           fill=color, width=width, joinstyle="round", capstyle="round")

    canvas.create_polygon(*[value for point in geometry["outline"] for value in point],
                          fill=preset["face"], outline="")
    stroke(geometry["outline"], preset["face"], geometry["outlineWidth"])

    def rounded_rect(x, y, width, height, radius, fill):
        radius = min(radius, width / 2, height / 2)
        canvas.create_rectangle(x + radius, y, x + width - radius, y + height,
                                fill=fill, outline="")
        canvas.create_rectangle(x, y + radius, x + width, y + height - radius,
                                fill=fill, outline="")
        for cx, cy in ((x, y), (x + width - 2 * radius, y),
                       (x, y + height - 2 * radius),
                       (x + width - 2 * radius, y + height - 2 * radius)):
            canvas.create_oval(cx, cy, cx + 2 * radius, cy + 2 * radius,
                               fill=fill, outline="")

    eyes, mouth = expression_geometry(geometry, preset)
    for x, y, width, height in eyes:
        rounded_rect(x, y, width, height, geometry["eyeRadius"], preset["eye"])
    x, y, width, height, radius, visible = tongue_geometry(mouth, preset)
    if visible:
        radius = min(radius, width / 2, height / 2)
        # Clip the tongue to the lower side of the current mouth curve.
        for column in range(width * 4):
            left = x + column / 4
            center = left + 0.125
            mouth_y = mouth[0][1]
            clipped_y = []
            for (ax, ay), (bx, by) in zip(mouth, mouth[1:]):
                if bx > ax and ax <= left + 0.25 and bx >= left:
                    for px in (max(ax, left), min(bx, left + 0.25)):
                        clipped_y.append(ay + (by-ay) * (px-ax) / (bx-ax))
            if clipped_y:
                mouth_y = max(clipped_y)
            dx = max(0, abs(center - (x + width / 2)) - (width / 2 - radius))
            inset = radius - (max(0, radius*radius - dx*dx)) ** 0.5
            top, bottom = math.ceil(max(y + inset, mouth_y) * 4) / 4, y + height - inset
            if top < bottom:
                # Pillow includes the right edge; keep strips in separate samples.
                canvas.create_rectangle(left, top, left + 0.249, bottom,
                                        fill="#000000", outline="")
    stroke(mouth, preset["mouth"], geometry["mouthWidth"])


def main() -> None:
    import tkinter as tk
    from tkinter import filedialog, messagebox
    from PIL import Image, ImageTk
    from tools.render_face_preview import RasterCanvas

    root = tk.Tk()
    root.title("Ninajirachi face preview")
    canvas = tk.Canvas(root, width=240, height=240, highlightthickness=0)
    canvas.pack()

    def render_preview(preset):
        raster = RasterCanvas()
        draw(raster, preset)
        image = ImageTk.PhotoImage(raster.image.resize((240, 240), Image.Resampling.LANCZOS))
        canvas.delete("all")
        canvas.create_image(0, 0, anchor="nw", image=image)
        canvas.image = image

    preset_by_name = {preset["name"]: preset for preset in PRESETS}
    selected = tk.StringVar(value=PRESETS[0]["name"])
    fields = tk.Frame(root)
    fields.pack()
    variables = {key: tk.StringVar() for key in (*METRIC_BOUNDS, *COLOR_KEYS)}
    status = tk.StringVar()

    def apply(*_):
        try:
            candidate = dict(preset_by_name[selected.get()])
            for key in METRIC_BOUNDS:
                candidate[key] = int(variables[key].get())
            for key in COLOR_KEYS:
                candidate[key] = variables[key].get()
            updated = [candidate if p["name"] == selected.get() else p for p in PRESETS]
            render(updated)
            preset_by_name[selected.get()].update(candidate)
            render_preview(candidate)
            status.set("")
            return True
        except (ValueError, KeyError) as error:
            status.set(str(error))
            return False

    def select(name):
        for key, variable in variables.items():
            variable.set(str(preset_by_name[name][key]))
        render_preview(preset_by_name[name])

    def export():
        if not apply():
            return
        filename = filedialog.asksaveasfilename(defaultextension=".json", initialdir=SOURCE.parent,
                                               initialfile=SOURCE.name)
        if filename:
            try:
                export_presets(PRESETS, Path(filename))
                status.set("Exported presets")
            except (OSError, ValueError) as error:
                messagebox.showerror("Export failed", str(error))

    tk.OptionMenu(root, selected, *preset_by_name, command=select).pack()
    for index, (key, bounds) in enumerate(METRIC_BOUNDS.items()):
        tk.Label(fields, text=key).grid(row=index, column=0, sticky="w")
        tk.Spinbox(fields, from_=bounds[0], to=bounds[1], textvariable=variables[key],
                   width=8, command=apply).grid(row=index, column=1)
    for index, key in enumerate(COLOR_KEYS):
        tk.Label(fields, text=key).grid(row=index, column=2, sticky="w")
        tk.Entry(fields, textvariable=variables[key], width=10).grid(row=index, column=3)
    fields.bind_all("<Return>", apply)
    tk.Button(root, text="Apply preview", command=apply).pack()
    tk.Button(root, text="Export presets JSON", command=export).pack()
    tk.Label(root, textvariable=status, wraplength=430).pack()
    select(selected.get())
    root.mainloop()


if __name__ == "__main__":
    main()
