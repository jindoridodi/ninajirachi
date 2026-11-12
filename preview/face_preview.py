#!/usr/bin/env python3
"""A 240x240 desktop preview for the parameterized Ninajirachi face."""

import json
from pathlib import Path
import os
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.generate_face_presets import COLOR_KEYS, METRIC_BOUNDS, SOURCE, OUTPUT, render
from tools.face_geometry import display_geometry


ROOT = Path(__file__).resolve().parents[1]
PRESETS = json.loads((ROOT / "shared" / "expression_presets.json").read_text())["presets"]


def export_presets(presets: list[dict], path: Path) -> None:
    header = render(presets)  # Validate before replacing any existing file.
    payload = json.dumps({"presets": presets}, indent=2) + "\n"
    canonical = path.resolve() == SOURCE.resolve()
    with tempfile.TemporaryDirectory(dir=path.parent) as source_dir:
        staged_source = Path(source_dir) / "presets.json"
        staged_source.write_text(payload)
        if not canonical:
            os.replace(staged_source, path)
            return
        with tempfile.TemporaryDirectory(dir=OUTPUT.parent) as header_dir:
            staged_header = Path(header_dir) / "new.h"
            staged_header.write_text(header)
            backup = None
            had_header = OUTPUT.exists()
            if had_header:
                descriptor, backup_name = tempfile.mkstemp(dir=OUTPUT.parent, suffix=".backup.h")
                backup = Path(backup_name)
                try:
                    with os.fdopen(descriptor, "wb") as stream:
                        stream.write(OUTPUT.read_bytes())
                except BaseException:
                    backup.unlink()
                    raise
            try:
                os.replace(staged_header, OUTPUT)
                try:
                    os.replace(staged_source, path)
                except OSError as error:
                    try:
                        if had_header:
                            os.replace(backup, OUTPUT)
                        else:
                            OUTPUT.unlink()
                    except OSError as rollback_error:
                        recovery = f"Previous header saved at {backup}" if backup else "Remove the generated header"
                        backup = None
                        raise OSError(f"Export failed; rollback failed. {recovery}.") from rollback_error
                    raise error
            finally:
                if backup and backup.exists():
                    backup.unlink()



def expression_geometry(geometry, preset):
    eyes = []
    for x, y, base_width, base_height in geometry["eyes"]:
        width = max(1, base_width * preset["eyeWidth"] // 36)
        height = max(1, base_height * preset["eyeOpen"] // 42)
        eyes.append((x + int((base_width-width)/2) + preset["eyeOffsetX"],
                     y + int((base_height-height)/2) + preset["eyeOffsetY"], width, height))
    mouth_y = geometry["mouth"][0][1]
    mouth = [(120 + int((x-120)*preset["mouthWidth"]/42),
              mouth_y + int((y-mouth_y)*preset["mouthHeight"]/14))
             for x, y in geometry["mouth"]]
    return eyes, mouth


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
    tongue = preset.get("tongueOut", 0)
    if tongue > 0:
        x, y = mouth[len(mouth) // 2]
        rounded_rect(x - 9, y + 8, 18, tongue + 4, 9, "#000000")
    stroke(mouth, preset["mouth"], geometry["mouthWidth"])


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
