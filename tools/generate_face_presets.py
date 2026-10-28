#!/usr/bin/env python3
"""Generate the Arduino preset header from the desktop preview's JSON source."""

import argparse
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "shared" / "expression_presets.json"
OUTPUT = ROOT / "firmware" / "src" / "face" / "ExpressionPresets.generated.h"
COLOR_KEYS = ("background", "face", "eye", "eyelid", "pupil", "mouth", "effect")
METRIC_BOUNDS = {
    "faceRadius": (1, 120), "eyeWidth": (1, 100), "eyeOpen": (1, 100),
    "eyelidHeight": (0, 100), "pupilRadius": (0, 40),
    "pupilOffsetX": (-60, 60), "pupilOffsetY": (-60, 60),
    "mouthWidth": (1, 100), "mouthHeight": (1, 100), "effectRadius": (0, 24),
}


def color(value: str) -> str:
    if not isinstance(value, str) or len(value) != 7 or not value.startswith("#"):
        raise ValueError(f"expected #RRGGBB color, got {value!r}")
    int(value[1:], 16)
    return f"0x{value[1:].upper()}"


def render(presets: list[dict]) -> str:
    names = [preset.get("name") for preset in presets]
    if names != ["idle", "curious", "happy", "surprised", "excited"]:
        raise ValueError("expected idle, curious, happy, surprised, excited in order")
    for preset in presets:
        for key, (low, high) in METRIC_BOUNDS.items():
            value = preset[key]
            if type(value) is not int or not low <= value <= high:
                raise ValueError(f"{key} must be an integer from {low} to {high}")
    rows = ["#pragma once", "", "namespace nina {", ""]
    for preset in presets:
        name = preset["name"]
        enum_name = name.title()
        colors = ", ".join("{" + color(preset[key]) + "}" for key in (
            "background", "face", "eye", "eyelid", "pupil", "mouth", "effect"
        ))
        metrics = ", ".join(str(int(preset[key])) for key in (
            "faceRadius", "eyeWidth", "eyeOpen", "eyelidHeight", "pupilRadius",
            "pupilOffsetX", "pupilOffsetY", "mouthWidth", "mouthHeight", "effectRadius"
        ))
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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    payload = json.loads(SOURCE.read_text())
    presets = payload["presets"]
    names = [preset.get("name") for preset in presets]
    required_names = ["idle", "curious", "happy", "surprised", "excited"]
    if names != required_names:
        raise ValueError(f"expected presets in order {required_names}, got {names}")
    expected = render(presets)
    if args.check:
        return 0 if OUTPUT.exists() and OUTPUT.read_text() == expected else 1

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(expected)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
