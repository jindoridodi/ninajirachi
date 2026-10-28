#!/usr/bin/env python3
"""Generate the Arduino preset header from the desktop preview's JSON source."""

import argparse
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "shared" / "expression_presets.json"
OUTPUT = ROOT / "firmware" / "src" / "face" / "ExpressionPresets.generated.h"


def color(value: str) -> str:
    if not isinstance(value, str) or len(value) != 7 or not value.startswith("#"):
        raise ValueError(f"expected #RRGGBB color, got {value!r}")
    int(value[1:], 16)
    return f"0x{value[1:].upper()}"


def render(presets: list[dict]) -> str:
    rows = ["#pragma once", "", "namespace nina {", ""]
    for preset in presets:
        name = preset["name"]
        enum_name = name.title()
        colors = ", ".join(color(preset[key]) for key in (
            "background", "face", "eye", "eyelid", "pupil", "mouth", "effect"
        ))
        metrics = ", ".join(str(int(preset[key])) for key in (
            "faceRadius", "eyeWidth", "eyeOpen", "eyelidHeight", "pupilRadius",
            "pupilOffsetX", "pupilOffsetY", "mouthWidth", "mouthHeight", "effectRadius"
        ))
        rows.extend((
            f"constexpr ExpressionPreset k{enum_name}Preset = {{",
            f"    ExpressionName::{enum_name}, {{{colors}}}, {{{metrics}}}",
            "};",
            "",
        ))
    rows.extend((
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
    expected = render(payload["presets"])
    if args.check:
        return 0 if OUTPUT.exists() and OUTPUT.read_text() == expected else 1

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(expected)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
