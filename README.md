# What is this?

This repo contains a version of MatrixGame, 3D engine used in Space Rangers HD: A War Apart, modified to run both on Windows and Linux.

In order to avoid a major engine re-write, this version uses [bgfx](https://github.com/bkaradzic/bgfx), translating native Direct3D calls to Vulkan. Input and system integration are handled by [SDL](https://github.com/libsdl-org/SDL).

Additionally, the engine has been updated to work natively on 64-bit platforms. 32-bit platform support has been dropped.

This project would not have been possible without the outstanding work of [murgesku](https://github.com/murgesku) and [vladislavrv](https://github.com/vladislavrv/MatrixGame), whom developed their versions of MatrixGame which became a base for this one.

# Work in progress?

Very much so. This has been hastily thrown together in under two weeks during my vacation. The last time I wrote anything in C or C++ was around 7-8 years ago, and complexity of the engine and some of the architectural decisions made by the original developers made it a way more harder enterprise than I originally thought.

Let me be blunt: as is, this is a demo or maybe an alpha. Bugs may and will be encountered.

# What works/what doesn't?

Tested and works:
- Launching the planetary battle from the main menu of Space Rangers HD
- Launching the battle as government quest: host game does not crash, player gets the reward
- Finishing the battle in-game and start another after exiting it
- Main gameplay loop works OK (training map is finishable)
- Big maps load and run (they do not crash the game per se, but AI seems to crush me :)
- End game stats are tracked properly
- Music and sound play normally
- Stencil shadows, volume shadows, anisotropic filtering and anti-aliasing should work now

What is not tested:
- Everything else I did not mention

Known issues/limitations:
- No text or button captions in standalone (.exe) mode. At the moment this is by design: this version mirrors the behavior of the original MatrixGame from SR2\HD, and does not render any text bitmaps by itself, instead relying on host game to do that. The exe does run, though.
- In exe mode, MatrixGame requires additional libraries to be present or linked, like SDL 2.0 or WinThreads implementation from MinGW - and looks for them next to itself. DLL carries all of them built-in.
- The build process contains an enormous amount of warnings I didn't have time to clean up yet.
  
# How to play this?

In order to play planetary battles, you have to run [this version](https://github.com/giantplaceholder/spacerangershd-fpc-fixes) of a [reverse-engineered engine of Space Rangers HD](https://github.com/pakompom/SpaceRangersHD_FPC) which has a patch for integration with this version of MatrixGame. 

Visit [the repo](https://github.com/giantplaceholder/spacerangershd-fpc-fixes) to figure out how to build it and how to run it. You need to own the game on Steam or GOG and bring your own game data.

Then build your own Rangers.exe as instructed, build your own MatrixGame as described below, put it into the game dir and play.

# Supported client OS?

- Windows 10 or 11
- Linux (any modern distro; tested on Ubuntu 24.04.4, Debian Trixie, Linux Mint Xia 22.1, Arch Linux/CachyOS, and SteamOS 3.8.26 on Steam Deck)

Only 64-bit platforms are supported.

**PLEASE NOTE**: in this context, "supported client OS" means "the ones that I've tested this on". It is not an exhaustive list.

# Ready-to-use binaries when?

Github Actions are enabled for this repo. Binaries are automatically built for every new tag.

Please note that Linux binaries are NOT portable: they are built in Ubuntu 24.04 runner environment, and they will probably run without any issues only on similar Ubuntu or Linux Mint distros.

# How to build?

Right now you have to run Linux to build this. I recommend using Ubuntu 24.04 or Debian Trixie. 

Network is required to download bgfx dependencies, as well as cross-build dependencies.

On Ubuntu or Mint, make sure that you have enabled universe repo, as some of the packages build requires are there.

1. Install dependencies:

```
sudo apt-get update && sudo apt-get install -y build-essential git cmake curl ca-certificates pkg-config g++-mingw-w64-x86-64-posix libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxrender-dev libxinerama-dev libxxf86vm-dev libxss-dev libgl1-mesa-dev libegl-dev libwayland-dev libasound2-dev libvulkan1 mesa-vulkan-drivers libgl1 libegl1 libx11-6 libxext6 libxrandr2 libxi6 libxcursor1 libxfixes3 libxss1 libasound2t64 libwayland-client0 libwayland-egl1
```

2. Clone this repo:

```
git clone https://github.com/giantplaceholder/matrixgame-bgfx.git
```

3. Launch build script: either run  ```./build_linux.sh``` to build Linux .so, or ```./build_windows.sh``` to build Windows DLL. Binaries will be placed to ```/out``` directory next to the build scripts.

4. Now just copy the binaries to your game folder, run Rangers.exe and try to play a planetary battle.

# AI involvement?

For the most part this code is written by a human, i.e myself. 

A locally hosted instance of Qwen3.8 Flash with throughoutput of barely 20 t/s on my Radeon 7900 was used for code clean-up and to speed up debugging process on a few occasions. 

If you think that this is vibe-coded, think again. Any commercial LLM would've probably written it better.

# License?

This project is licensed under [GPLv2](LICENSE) or any later, per your choice.

# Acknowledgements?

This code is based on [vladislavrv](https://github.com/vladislavrv/MatrixGame)'s version of MatrixGame, which is in turn based on [murgesku](https://github.com/murgesku/MatrixGame)'s version of MatrixGame - which is in turn based on the [original GPLv2 release of MatrixGame](https://snk-games.net/matrixgame.tar.gz), as released by CHK-Games back in December of 2019 ([see original post](https://web.archive.org/web/20220102100125/https://snk-games.net/forums/viewtopic.php?f=41&t=2154) in Russian). All of those are also licensed as GPLv2.
