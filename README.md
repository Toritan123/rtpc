# Rhythm Tengoku — PC port

A native PC build (macOS and Windows) of the Rhythm Tengoku decompilation.
Jump to [PC port](#pc-port) for how to build and run it.

The rest of this README is the upstream decompilation's, which this repository
is a fork of and which the port is built on top of.

---

This is a decompilation of Rhythm Tengoku for the Gameboy Advance. It can build either a fully byte-for-byte matching ROM from scratch, or rebuild a different functionally equivalent ROM with a different memory layout to be easier to mod with. This can be controlled with the `NONMATCHING` parameter when building.

This repository requires an unmodified ROM of Rhythm Tengoku (rev. 0) to build. This ROM will not be provided for you.

When built in matching mode, it builds the following ROMs:

* **rhythmtengoku.gba** `sha1: 67f8adacff79c15d028fffd90de3a77d9ad0602d`
* **rhythmtengoku_rev1.gba** `sha1: e0aaca45045e408e7e1072bde5b39278111e1952`


## Installation

To install the decomp, you need access to a Linux terminal. If you are on Windows 10 or 11, you can access a Linux terminal easily by installing **WSL (Windows Subsystem for Linux)**. If you are already on Linux, you can skip to the **Installing Dependencies** section. Otherwise, follow this guide to install WSL:

#### Installing WSL

To install WSL, first open up either a Command Prompt or Windows PowerShell window as an administrator. Run the command `wsl --install` to automatically install WSL defaulting to the Ubuntu distribution. Once the process finishes, restart your computer to finish the installation.

*Note: you will need to have virtualization enabled in your BIOS settings to use WSL, so if you encounter issues, check how to boot into BIOS and enable virtualization for your computer. For any other issues with installation, you can refer to the [official installation guide](https://docs.microsoft.com/en-us/windows/wsl/install).*
<br>
Once WSL is installed, upon opening it you will be prompted for a username and password. Note that when typing in the password, the characters will not show up, not even as asterisks, so type carefully as you will need to remember your password.

Once you've created a user, you should run two more commands to finish setting up your terminal. First, run `sudo apt update`, and then after this command has finished, run `sudo apt upgrade`. These commands will require you to enter your password. When prompted with `Do you want to continue? [Y/n]`, simply enter `y`. After doing this, WSL should be fully set up and ready to use.
<br>
It is also recommended to mount WSL to a drive letter to access your WSL filesystem from Windows more easily. To do this, follow [this guide](https://github.com/HackerN64/HackerSM64/wiki/Mounting-WSL-to-Drive).

If you are not familiar with the Linux terminal, a helpful command to know is `cd` which will change the current working directory. `cd ~/` will take you to your home directory, where it is recommended to store the repository. Additionally, copy and pasting in the WSL terminal is done through right-clicking instead of Ctrl+C / Ctrl+V, so to paste commands from this guide into your terminal, simply copy them with Ctrl+C, and then right click in your terminal to paste.


#### Installing Dependencies

To install the required dependencies for the decomp, first run this command:

`sudo apt install build-essential binutils-arm-none-eabi git libpng-dev ffmpeg`

After this, you should also run this command:

`sudo ln -s /proc/self/mounts /etc/mtab`

This command can help with issues with WSL when installing devkitPro. It is not always necessary to run, and may throw an error. If it does error, don't worry and just move on to the next step.

After this, you will need to install devkitPro. This process is very involved, so just run all these listed commands in this order, entering your password when prompted:

`wget https://apt.devkitpro.org/install-devkitpro-pacman`
`chmod +x ./install-devkitpro-pacman`
`sudo ./install-devkitpro-pacman`
`export DEVKITPRO=/opt/devkitpro`
`export DEVKITARM=/opt/devkitpro/devkitARM`
`export DEVKITPPC=/opt/devkitpro/devkitPPC`
`sudo dkp-pacman -Sy`
`sudo dkp-pacman -S gba-dev`

After running the final command, press enter and then type `y` to finish installing.


#### Cloning the Repository

Now you are ready to clone the repository. First, navigate to your home directory with `cd ~/` and then run this command:

`git clone https://github.com/arthurtilly/rhythmtengoku rt`

This will clone the repository into a folder named `rt` in your home directory. You can change `rt` to some other name if you wish to name the repository folder something else.

Before building the ROM, you will also need to install agbcc, the compiler used for Rhythm Tengoku. Make sure you are still in your home directory, and run

`git clone https://github.com/pret/agbcc`

After this, navigate to the agbcc directory with `cd ~/agbcc` and build the compiler by running `./build.sh`. Finally, install the compiler into the Rhythm Tengoku decomp repository by running `./install.sh ~/rt`.

Lastly, you will need to acquire an unmodified rev0 Rhythm Tengoku ROM in order to build the decomp. This ROM is not provided, and you must source it yourself. Once you obtain this ROM, rename it to `baserom.gba` and place it in the `rt` directory.


#### Building the ROM

You are finally ready to build the repository! Navigate to the repository folder with `cd ~/rt`, and build the ROM by running `make -j`. (The `-j` parameter makes the build process able to run on multiple cores of your CPU, heavily speeding up the process.) Once the ROM has finished building, it will output a file at `build/rhythmtengoku.gba`! This is your compiled ROM.

## Using the decomp to mod Rhythm Tengoku

If you wish to create mods for Rhythm Tengoku, you should install a code editor in order to be able to easily edit code files. The recommended editor is **VSCode**. Once you install VSCode, you should also install the WSL and C/C++ extensions from the sidebar. Open a WSL window by clicking on the very bottom-left of the VSCode window, and open your `rt` repository.

When modding, it is recommended to compile the ROM in nonmatching mode in order to make adding new files easier. To do this, open the `Makefile` file in the root of the repository. Find the line near the top that reads `NONMATCHING ?= 0` and change this to `NONMATCHING ?= 1`. This will set up your repository to focus on being easy to use for modding rather than building a matching ROM byte-for-byte.

Now you are free to make any changes! After you make a change, run `make -j` to rebuild the ROM and see your changes in action. By default the `make` command will only rebuild changed files in order to save time when building. This can occasionally lead to dependency issues, so you can run `make clean` to force the repository to build from scratch to occasionally help with strange issues when building.

If you have any other questions or concerns, join the [RHModding discord server](https://discord.com/invite/ps4rq53)!
---

## PC port

This is a fork of [arthurtilly/rhythmtengoku](https://github.com/arthurtilly/rhythmtengoku);
all of the decompilation work is theirs. This fork adds a native PC build of
the game on top of it. It
runs the decompiled C directly — there is no emulator and no ROM involved at
run time; the GBA hardware layer (PPU, DMA, sound FIFO, key registers, BIOS
calls) is reimplemented in `platform/` against SDL2.

**This repository contains no game data.** The graphics, samples and music
sequences are written at build time from your own copy of the ROM (rev. 0,
`sha1: 67f8adacff79c15d028fffd90de3a77d9ad0602d`) by `tools/extract_assets.py`,
which checks every file against `tools/asset_manifest.json` (ROM offsets and
hashes only). Put the ROM at `baserom.gba` in the repository root, or pass
`BASEROM=<path>` to make. Nothing from the ROM is ever committed.

Building the GBA ROM works as described above once the data is extracted
(`python3 tools/extract_assets.py`), and the PC port lives behind
`PLATFORM_PC` so it never enters that build.

### Building

Requires SDL2, Python 3, a C compiler and your own ROM as `baserom.gba` (see
above). devkitPro is **not** needed for the PC build. The first build extracts
the game data (a couple of seconds); `make -f Makefile.pc assets` does only that.

**macOS**

```
brew install sdl2
make -f Makefile.pc app
```

This produces `dist/Rhythm Tengoku.app`. SDL2 (and, if Homebrew's `sdl2` is
really `sdl2-compat`, the SDL3 it loads) is copied inside the bundle, so the
app runs on a Mac without Homebrew installed.

**Windows**

Natively under MSYS2 (MINGW64 shell):

```
pacman -S make python mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2
make -f Makefile.pc PLATFORM=win CC=gcc LD=gcc \
     SDL2_CFLAGS="$(sdl2-config --cflags)" SDL2_LIBS="$(sdl2-config --libs)"
```

Or cross-compiled from macOS/Linux with mingw-w64, against the SDL2 mingw
development package unpacked into `third_party/SDL2-mingw`:

```
make -f Makefile.pc win-dist
```

`dist/windows/` then holds `rhythmtengoku.exe` and `SDL2.dll`.

### Where the build goes

Objects go to `build/<os>/` (`build/macos`, `build/windows`) and the finished
thing to `dist/`: `dist/Rhythm Tengoku.app` and `dist/windows/`. Both are
gitignored — they are build output and are not committed. They stay on disk
until you delete them or run `make -f Makefile.pc clean`.

`PLATFORM_PC`, the C define the port is guarded by, means "not the GBA" and
applies to every desktop target; it is not the macOS build. The per-OS pieces
are named for their OS.

### Controls

| Key | GBA |
| --- | --- |
| K / J | A / B |
| W A S D or arrows | D-pad |
| Q / E | L / R |
| Enter | START |
| Backspace | SELECT |
| Esc | quit |

A+B+START+SELECT together is the GBA soft-reset combo, as on hardware.

### Diagnostics

All are environment variables, off by default and free when unset.

| Variable | Effect |
| --- | --- |
| `RTPC_TRACE=1` | log scene transitions |
| `RTPC_BS=1` | log every beatscript command, with the callee's symbol name |
| `RTPC_PROF=1\|2` | per-frame timing breakdown (`2` logs every frame) |
| `RTPC_AUTO=1\|2\|3` | autopilot: tap A / also skip tutorials / mash everything. For testing only — it synthesises key presses, so the game appears to play itself. It announces itself on stderr when on. |
| `RTPC_HEADLESS=1` | no window, renderer or audio; runs the engine only (used by CI) |
| `RTPC_SHOTS=N` | write a BMP screenshot every N frames |
| `RTPC_AUDIO_DUMP=f.wav` | record the audio output for offline analysis |
| `RTPC_AUDIO_STATS=1` | log audio device format, queue depth, underruns |
| `RTPC_AUDIO_BUF=N` | SDL device buffer size in frames (default 512) |
| `RTPC_AUDIO_TARGET=ms` | target output queue depth (default 30 ms) |
| `RTPC_AUDIO_8BIT=1` | use the GBA's 8-bit DAC output instead of the high-resolution tap |
| `RTPC_MIX_MULT=2\|3\|4` | run the MIDI mixer above the GBA's 13379 Hz |
| `RTPC_VOLUME=<pct>` | master volume, default 100 (the game's own level, -16.2 dBFS RMS). Higher values are soft-limited rather than clipped; 160 measures -12.8 dBFS. |
| `RTPC_NO_PSG=1` | mute the PSG (square/wave/noise) channels |
| `RTPC_PSG_VOLUME=<pct>` | PSG level relative to the sampled music, default 100 |
| `RTPC_PSG_TEST=<n>` | drive PSG channel 2 with a sustained square at frequency register n, for checking the synthesiser |

### Status

Playable through Karate Man to the results screen. Most other scenes reach
their engine but are not verified end to end. `tools/bs2c.py` translates the
`.bs` scene scripts into C so scenes use their real scripts rather than
hand-written stand-ins; 135 of them parse, and the ones not yet compiled into
the build are the remaining work.
