# PolySecCCTVFirmware
PlatformIO project for the artnet control of the PolySec CCTV


## Use of this repo
This repo is built for the [platformIO](https://platformio.org/) ide and runs on a [lilygo poe esp32 board](https://www.tindie.com/products/ttgo/lilygor-ttgo-t-internet-poe-and-downloader-expansi/).


## Artnet adresses
- 1 - Laser control - [0] Off, [1] On, [2-255] Strobing Slow to Fast
- 2 - Pan
- 3 - Pan Fine
- 4 - Tilt
- 5 - Tilt Fine
- 6 - Pan/Tilt Speed
- 7,8,9 - RGB for led strip pixel 1

Repeating till

- 202,203,204 - RGB for led strip pixel 66
- 205,206,207 - RGB for led array pixel 1, top left

Repeating till

- 247,248,249 - RGB for led array pixel 15, bottom right

Line pixels are arranged:

66,65,64 ... 3,2,1

Array pixels are arranged:
```
1|6|7|12|13
2|5|8|11|14
3|4|9|10|15
```
