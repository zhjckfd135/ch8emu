# CH8EMU
A Windows and Linux CHIP-8 emulator written in C.

## Features

- Cross-platform (Windows / Linux)
- Pure C
- Terminal interface
- Simple debug mode
- Sound support
- Unicode and ASCII rendering

## Screenshots

<img width="400" height="300" alt="image" src="https://github.com/user-attachments/assets/88e63ab3-a4d8-4c2c-bf7e-b8449928c5e0" />
<img width="400" height="300" alt="image" src="https://github.com/user-attachments/assets/ef119f29-6760-4bd0-a26c-7aa285e7fa0f" />

## Requirements for building

- C compiler (GCC / Clang / MSVC — not tested)
- CMake ≥ 3.10

### Platform notes

- Linux: You need to install ALSA development libraries:
```sudo apt install libasound2-dev```
- Windows: no additional dependencies required

The emulator runs in terminal mode only.

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

## What was this created for?


This is the first project I’d like to publish on GitHub. Although I’m sure it won’t gain any traction, I have to start somewhere. I might come back to this project, but there’s no guarantee.

## Thank You for reading
      
