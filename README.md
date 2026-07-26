# CH8EMU
A Windows and Linux CHIP-8 emulator written in C.

## Features

- Cross-platform (Windows / Linux)
- Pure C
- Terminal interface
- Simple debug mode
- Sound support

## Supported systems

- CHIP-8 ✅
- Super CHIP (planned)

## Screenshots

<img width="40%" alt="Code_VrUEOGc7PQ" src="https://github.com/user-attachments/assets/f6a6c82e-2c71-4093-85a6-52645e384095" />
<img width="40%" alt="Code_i7UxdqHbJN" src="https://github.com/user-attachments/assets/2fff78a6-d51b-4a23-9c73-5f80e41b8038" />
<img width="40%" alt="image" src="https://github.com/user-attachments/assets/7a400549-7f65-4acb-ad70-82e827f7396a" />
<img width="40%" alt="image" src="https://github.com/user-attachments/assets/ef3f9e5f-7981-4749-88a7-05f5945dabf3" />

## Requirements for building

- C compiler (GCC / Clang / MSVC — not tested)
- CMake ≥ 3.10

### Platform notes

- Linux: Install the ALSA development libraries.

  Ubuntu/Debian:
  ```bash
  sudo apt install libasound2-dev
  ```

  Arch Linux:
  ```bash
  sudo pacman -S alsa-lib
  ```
- Windows: Make sure `winmm.lib` is available in your build environment.

The emulator runs in terminal mode only.

## Build

```bash
git clone https://github.com/zhjckfd135/ch8emu.git
cd ch8emu

cmake -B build
cmake --build build
```

## Input

CHIP-8 uses a 16-key hexadecimal keypad.

Each CHIP-8 key is mapped to a physical keyboard key:

```
CHIP-8          Keyboard
-------------------------
1  2  3  C      1  2  3  4
4  5  6  D      Q  W  E  R
7  8  9  E      A  S  D  F
A  0  B  F      Z  X  C  V
```

## License

This project is licensed under the MIT License.

## What was this created for?

This is the first project I’d like to publish on GitHub. Although I’m sure it won’t gain any traction, I have to start somewhere. I might come back to this project, but there’s no guarantee.

I ended up coming back to this project after all) I plan to add Super CHIP-8 later.

## Special thanks

- [ArkoSammy12](https://github.com/ArkoSammy12)  - For pointing out mistakes I hadn't even thought about.

## Thanks for reading!
      
