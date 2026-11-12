#!/usr/bin/env python3
"""Generate the Arduino preset header from the desktop preview's JSON source."""

import argparse
import json
from pathlib import Path
import sys
import os
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "shared" / "expression_presets.json"
OUTPUT = ROOT / "firmware" / "src" / "face" / "ExpressionPresets.generated.h"
COLOR_KEYS = ("background", "face", "eye", "mouth")
METRIC_BOUNDS = {
    "eyeWidth": (1, 100), "eyeOpen": (1, 100),
    "eyeOffsetX": (-60, 60), "eyeOffsetY": (-60, 60),
    "mouthWidth": (1, 100), "mouthHeight": (1, 100), "tongueOut": (0, 20),
}


def color(value: str) -> str:
    if (not isinstance(value, str) or len(value) != 7 or not value.startswith("#") or
            any(digit not in "0123456789abcdefABCDEF" for digit in value[1:])):
        raise ValueError(f"expected #RRGGBB color, got {value!r}")
    return f"0x{value[1:].upper()}"


def render(presets: list[dict]) -> str:
    names = [preset.get("name") for preset in presets]
    if names != ["idle", "curious", "excited"]:
        raise ValueError("expected idle, curious, excited in order")
    for preset in presets:
        for key, (low, high) in METRIC_BOUNDS.items():
            value = preset[key]
            if type(value) is not int or not low <= value <= high:
                raise ValueError(f"{key} must be an integer from {low} to {high}")
    rows = ["#pragma once", "", "namespace nina {", ""]
    for preset in presets:
        name = preset["name"]
        enum_name = name.title()
        colors = ", ".join("{" + color(preset[key]) + "}" for key in COLOR_KEYS)
        metrics = ", ".join(str(preset[key]) for key in METRIC_BOUNDS)
        rows.extend((
            f"constexpr ExpressionPreset k{enum_name}Preset = {{",
            f"    ExpressionName::{enum_name},",
            f"    {{{colors}}},",
            f"    {{{metrics}}}",
            "};",
            "",
        ))
    rows.extend((
        "inline const ExpressionPreset& expressionPreset(ExpressionName name) {",
        "  switch (name) {",
    ))
    for preset in presets:
        enum_name = preset["name"].title()
        rows.append(f"    case ExpressionName::{enum_name}: return k{enum_name}Preset;")
    rows.extend((
        "  }",
        "  return kIdlePreset;",
        "}",
        "",
        "inline const ExpressionPreset& idlePreset() { return kIdlePreset; }",
        "",
        "}  // namespace nina",
        "",
    ))
    return "\n".join(rows)


def export_presets(presets: list[dict], path: Path, *, source: Path = SOURCE, output: Path = OUTPUT) -> None:
    header = render(presets)  # Validate before replacing any existing file.
    payload = json.dumps({"presets": presets}, indent=2) + "\n"
    canonical = path.resolve() == source.resolve()
    if canonical:
        path = source.resolve()
    with tempfile.TemporaryDirectory(dir=path.parent) as source_dir:
        staged_source = Path(source_dir) / "presets.json"
        staged_source.write_text(payload)
        if not canonical:
            os.replace(staged_source, path)
            return
        with tempfile.TemporaryDirectory(dir=output.parent) as header_dir:
            staged_header = Path(header_dir) / "new.h"
            staged_header.write_text(header)
            backup = None
            had_header = output.exists()
            if had_header:
                descriptor, backup_name = tempfile.mkstemp(dir=output.parent, suffix=".backup.h")
                backup = Path(backup_name)
                try:
                    with os.fdopen(descriptor, "wb") as stream:
                        stream.write(output.read_bytes())
                except BaseException:
                    backup.unlink()
                    raise
            try:
                os.replace(staged_header, output)
                try:
                    os.replace(staged_source, path)
                except OSError as error:
                    try:
                        if had_header:
                            os.replace(backup, output)
                        else:
                            output.unlink()
                    except OSError as rollback_error:
                        recovery = f"Previous header saved at {backup}" if backup else "Remove the generated header"
                        backup = None
                        raise OSError(f"Export failed; rollback failed. {recovery}.") from rollback_error
                    raise error
            finally:
                if backup and backup.exists():
                    backup.unlink()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    payload = json.loads(SOURCE.read_text())
    presets = payload["presets"]
    expected = render(presets)
    if args.check:
        return 0 if OUTPUT.exists() and OUTPUT.read_text() == expected else 1

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=OUTPUT.parent) as folder:
        staged = Path(folder) / "header.h"
        staged.write_text(expected)
        os.replace(staged, OUTPUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
