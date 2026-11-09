#!/usr/bin/env python3
"""A 240x240 desktop preview for the parameterized Ninajirachi face."""

import json
from pathlib import Path
import os
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.generate_face_presets import COLOR_KEYS, METRIC_BOUNDS, SOURCE, OUTPUT, render


ROOT = Path(__file__).resolve().parents[1]
PRESETS = json.loads((ROOT / "shared" / "expression_presets.json").read_text())["presets"]


def export_presets(presets: list[dict], path: Path) -> None:
    header = render(presets)  # Validate before replacing any existing file.
    payload = json.dumps({"presets": presets}, indent=2) + "\n"
    descriptor, temporary = tempfile.mkstemp(dir=path.parent, suffix=".json")
    try:
        with os.fdopen(descriptor, "w") as stream:
            stream.write(payload)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    if path.resolve() == SOURCE.resolve():
        OUTPUT.write_text(header)


def draw(canvas, preset: dict) -> None:
    import math

    canvas.delete("all")
    canvas.create_rectangle(0, 0, 240, 240, fill=preset["background"], outline="")

    # Same geometry as LvglFace: rotated rounded-square outline, vertical eyes,
    # and two bottom-facing arcs. Legacy expression fields remain available for
    # animation, but the reference icon does not use pupils or a filled mouth.
    half, corner = 85, 28
    points = []
    for cx, cy, start in ((half - corner, half - corner, -90),
                          (-half + corner, half - corner, 0),
                          (-half + corner, -half + corner, 90),
                          (half - corner, -half + corner, 180)):
        for step in range(7):
            angle = math.radians(start + step * 90 / 6)
            x, y = cx + corner * math.cos(angle), cy + corner * math.sin(angle)
            points.extend((120 + (x - y) / math.sqrt(2),
                           120 + (x + y) / math.sqrt(2)))
    canvas.create_line(*points, points[0], points[1], fill=preset["face"], width=4,
                       joinstyle="curve")
    def rounded_rect(x, y, width, height, radius, fill):
        canvas.create_rectangle(x + radius, y, x + width - radius, y + height,
                                fill=fill, outline="")
        canvas.create_rectangle(x, y + radius, x + width, y + height - radius,
                                fill=fill, outline="")
        for cx, cy in ((x, y), (x + width - 2 * radius, y),
                       (x, y + height - 2 * radius),
                       (x + width - 2 * radius, y + height - 2 * radius)):
            canvas.create_arc(cx, cy, cx + 2 * radius, cy + 2 * radius,
                              start=0, extent=360, fill=fill, outline="")

    for x in (48, 152):
        rounded_rect(x, 43, 28, 86, 16, preset["face"])
    for x in (62, 114):
        canvas.create_arc(x, 104, x + 64, 168, start=200, extent=140,
                          style="arc", outline=preset["face"], width=11)
    if preset["effectRadius"]:
        canvas.create_oval(120 - 3, 29, 120 + 3, 35, fill=preset["effect"], outline="")


def main() -> None:
    import tkinter as tk
    from tkinter import filedialog, messagebox

    root = tk.Tk()
    root.title("Ninajirachi face preview")
    canvas = tk.Canvas(root, width=240, height=240, highlightthickness=0)
    canvas.pack()
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
            draw(canvas, candidate)
            status.set("")
            return True
        except (ValueError, KeyError) as error:
            status.set(str(error))
            return False

    def select(name):
        for key, variable in variables.items():
            variable.set(str(preset_by_name[name][key]))
        draw(canvas, preset_by_name[name])

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
    draw(canvas, preset_by_name[selected.get()])
    root.mainloop()


if __name__ == "__main__":
    main()
