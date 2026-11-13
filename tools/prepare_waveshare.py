#!/usr/bin/env python3
"""Install the Waveshare demo drivers with the write-only SPI adaptation."""
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
    # Arduino 2.0.12 transfer(buffer, length) overwrites the input with RX data.
    old = b'vspi->transfer(pData, Len);'
    if sources['DEV_Config.cpp'].count(old) != 1:
        raise ValueError('Unexpected Waveshare SPI driver; write-only adaptation not applied')
    sources['DEV_Config.cpp'] = sources['DEV_Config.cpp'].replace(
        old, b'vspi->writeBytes(pData, Len);')
    for name, data in sources.items():
        (target / name).write_bytes(data)

if __name__ == '__main__':
    main()
