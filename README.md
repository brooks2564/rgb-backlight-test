# RGB Backlight Test

A Pebble app for the **Pebble Time 2 (Emery)** to interactively test the RGB backlight using the new `light_set_color_rgb888()` API introduced in SDK 4.9.169.

## Features

- Live R, G, B channel adjustment with instant backlight feedback
- Color swatch preview (approximate 64-color display)
- Touch **and** button controls

## Controls

| Input | Action |
|-------|--------|
| Slide left/right on a row | Adjust that channel (1px = 1 unit) |
| Tap a row | Select that channel |
| UP / DOWN (hold to repeat) | ±5 on selected channel |
| SELECT | Cycle selected channel (R → G → B) |
| BACK | Exit |

## Requirements

- Pebble Time 2 (Emery platform) — requires `PBL_RGB_BACKLIGHT`
- Pebble SDK 4.9.169+

## Build & Install

```bash
pebble build
pebble install --phone <watch-ip>
```
