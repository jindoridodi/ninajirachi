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
        radius = preset["pupilRadius"]
        if radius > 0 and preset["eyeOpen"] > 3:
            import math
            for row in range(-radius, radius + 1):
                y = py + row
                if y < 92 - preset["eyeOpen"] // 2 + preset["eyelidHeight"]:
                    continue
                ratio = (y - 92) / (preset["eyeOpen"] / 2)
                if abs(ratio) > 1:
                    continue
                eye_span = int(preset["eyeWidth"] / 2 * math.sqrt(1 - ratio * ratio))
                pupil_span = int(math.sqrt(radius * radius - row * row))
                left, right = max(x - eye_span, px - pupil_span), min(x + eye_span, px + pupil_span)
                if left <= right:
                    canvas.create_rectangle(left, y, right + 1, y + 1,
                                            fill=preset["pupil"], outline="")
    canvas.create_oval(120 - preset["mouthWidth"] // 2, 174 - preset["mouthHeight"] // 2,
                       120 + preset["mouthWidth"] // 2, 174 + preset["mouthHeight"] // 2,
                       fill=preset["mouth"], outline="")
    if preset["effectRadius"]:
        radius = preset["effectRadius"]
        canvas.create_oval(120 - radius, 32 - radius, 120 + radius, 32 + radius,
                           fill=preset["effect"], outline="")


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
