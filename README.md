# PeanutButterGB

PeanutButterGB is a Gameboy emulator based off of [PeanutGB](https://github.com/deltabeard/Peanut-GB)
that is specifically designed to run on Wi-Fi Routers. I decided to make this project because why not?!
PeanutButterGB uses the router's USB interface to output sound via ALSA and recieve joystick inputs
via the Linux USB joystick library. This project has been tested working on OpenWRT, however lots of
setup is required that will be documented Soon(tm).

This emulator supports Game Boy (DMG) and Game Boy Color (CGB) games.

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

## License

This project is licensed under the MIT License.
