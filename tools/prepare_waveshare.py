#!/usr/bin/env python3
"""Install the required unmodified drivers from the official Waveshare demo ZIP."""
import argparse
from pathlib import Path
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
PREFIX = 'ESP32-S3-Touch-LCD-1.28-Demo/Arduino/examples/ESP32-S3-Touch-LCD-1.28-Test/'
FILES = ('DEV_Config.cpp', 'DEV_Config.h', 'GUI_Paint.cpp', 'GUI_Paint.h',
         'LCD_1in28.cpp', 'LCD_1in28.h', 'Debug.h', 'fonts.h', 'font12.cpp',
         'QMI8658.cpp', 'QMI8658.h')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('zip', type=Path)
    args = parser.parse_args()
    target = ROOT / 'firmware/src/waveshare'
    with ZipFile(args.zip) as archive:
        sources = {name: archive.read(PREFIX + name) for name in FILES}
    for name, data in sources.items():
        (target / name).write_bytes(data)

if __name__ == '__main__':
    main()
