# PeanutButterGB

PeanutButterGB is a Gameboy emulator based off of [PeanutGB](https://github.com/deltabeard/Peanut-GB)
that is specifically designed to run on Wi-Fi Routers. I decided to make this project because why not.
PeanutButterGB uses the router's USB interface to output sound via ALSA and recieve joystick inputs
via the Linux USB joystick library. This project has been tested working on OpenWRT, however lots of
setup is required that will be documented Soon(tm).

Only the original Game Boy (DMG) is supported at this time, but preliminary work
has been completed to support Game Boy Color
(see https://github.com/deltabeard/Peanut-GB/issues/50).

This emulator is a work in progress and can be inaccurate (although it does pass
Blargg's CPU instructions and instruction timing tests). As such, some games may
run incorrectly or not run at all. Please seek an alternative emulator if
accuracy is important.

## Features

- Game Boy (DMG) Support
- Very fast
- MBC1, MBC2, MBC3, and MBC5 support
- Real Time Clock (RTC) support
- Serial connection support
- Can be used with or without a bootrom
- Allows different palettes on background and sprites
- Frame skip and interlacing modes (useful for slow LCDs)

## Caveats

- The LCD rendering is performed line by line, so certain animations will not
  render properly (such as in Prehistorik Man).
- Some games may not be playable due to emulation inaccuracy
  (see https://github.com/deltabeard/Peanut-GB/issues/31).
- MiniGB APU runs in a separate thread, and so the timing is not accurate. If
  accurate APU timing and emulation is required, then Blargg's Gb_Snd_Emu
  library (or an alternative) can be used instead.

### Screenshot

![Pokemon Blue - Main screen animation](/screencaps/PKMN_BLUE.gif)
![Legend of Zelda: Links Awakening - animation](/screencaps/ZELDA.gif)
![Megaman V](/screencaps/MEGAMANV.png)

![Shantae](/screencaps/SHANTAE.png)
![Dragon Ball Z](/screencaps/DRAGONBALL_BBZP.png)

Note: Animated GIFs shown here are limited to 50fps, whilst the emulation was
running at the native ~60fps. This is because popular GIF decoders limit the
maximum FPS to 50.

### Controls

| Action            | Keyboard   | Joypad |
|-------------------|------------|--------|
| A                 | z          | A      |
| B                 | x          | B      |
| Start             | Return     | START  |
| Select            | Backspace  | BACK   |
| D-Pad             | Arrow Keys | DPAD   |
| Repeat A          | a          |        |
| Repeat B          | s          |        |
| Normal Speed      | 1          |        |
| Turbo x2 (Hold)   | Space      |        |
| Turbo X2 (Toggle) | 2          |        |
| Turbo X3 (Toggle) | 3          |        |
| Turbo X4 (Toggle) | 4          |        |
| Reset             | r          |        |
| Change Palette    | p          |        |
| Reset Palette     | Shift + p  |        |
| Fullscreen        | F11 / f    |        |
| Frameskip (Toggle)| o          |        |
| Interlace (Toggle)| i          |        |
| Dump BMP (Toggle) | b          |        |

Frameskip and Interlaced modes are both off by default. The Frameskip toggles
between 60 FPS and 30 FPS.

Pressing 'b' will dump each frame as a 24-bit bitmap file in the current
folder. See /screencaps/README.md for more information.


## License

This project is licensed under the MIT License.
