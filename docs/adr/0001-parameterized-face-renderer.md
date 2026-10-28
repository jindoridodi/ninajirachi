# Use a parameterized face renderer with a desktop preview

The wearable renders its face from named expression parameters instead of maintaining separate image assets for every expression. A desktop face preview is used to tune and inspect those presets at 240×240; the wearable runtime remains a compact renderer, with bitmap assets reserved for optional decorative effects that cannot be drawn simply.
