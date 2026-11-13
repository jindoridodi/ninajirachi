Copy the direct-driver source files from Waveshare's `ESP32-S3-Touch-LCD-1.28-Test` example into this directory before building the sketch. Keep their filenames unchanged: `DEV_Config.*`, `GUI_Paint.*`, `LCD_1in28.*`, `Debug.h`, `fonts.*`, and the required font source files.

This directory is intentionally not populated with a copied vendor package. The official example package remains the source of truth for its GC9A01A SPI, CST816S I2C, and QMI8658 board definitions.


Run `python3 tools/prepare_waveshare.py /path/to/demo.zip` from the repository root. It keeps the vendor notices and replaces the multi-byte SPI transfer with a write-only call; it rejects an unexpected driver instead of silently installing it.
