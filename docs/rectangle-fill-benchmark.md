# Rectangle-fill host comparison

Install the official demo sources with `python3 tools/prepare_waveshare.py /path/to/ESP32-S3-Touch-LCD-1.28-Demo.zip`, then run `sh tests/run_surface_benchmark.sh`. The script compiles both implementations and the same vendor rasterizer with `-O2`, draws 200 idle frames for each, checks their framebuffer bytes match, and prints elapsed times. The vendor path uses the original `Paint_ClearWindows` calls; the direct path uses `WaveshareSurface`. No framebuffer is added to the firmware.

On Windows with portable GCC 16.2.0, one run measured 165,030 microseconds for the vendor path and 66,210 microseconds for direct rows. These are desktop results for this workload, not physical FPS or power measurements. Measure display transfer, render timing, and current on the assembled wearable under #15.
