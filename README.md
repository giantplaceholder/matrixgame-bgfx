# What is this?

This is a version of MatrixGame, a 3D engine used in Space Rangers HD: A War Apart planetary battles mode, modified to run on both Windows and Linux.

<p align=center><img width="2560" height="1440" alt="Shot-2026-10-24-13-56-08-009" src="https://github.com/user-attachments/assets/03363876-591e-4250-87f7-b70395918abc" /></p>

<p align=center>(this screenshot has been captured in 2560x1440, with game running on Arch Linux)<br />
<strong><a href=https://www.youtube.com/watch?v=taUra2KYckM target=_blank>Live gameplay demo on YouTube</a></strong> (will open in a new tab)</p>

In order to avoid a major engine re-write, this version uses [bgfx](https://github.com/bkaradzic/bgfx), translating native Direct3D calls to Vulkan. Input and system integration are handled by [SDL](https://github.com/libsdl-org/SDL). Additionally, the engine has been updated to work natively on 64-bit platforms. 32-bit platform support has been dropped.

This project would not have been possible without the outstanding work of [murgesku](https://github.com/murgesku) and [vladislavrv](https://github.com/vladislavrv/MatrixGame), whom developed their custom versions of the original MatrixGame, which became a base for this one.

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

- Multi-language support (don't have the time for it right now)
- Everything else I did not mention

Known issues/limitations:

- There are still numbers of undebugged crashes on bigger maps
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

Unlike the aforementioned custom version of Space Rangers HD 2D engine, this project does not support Windows 7. What this means specifically: binary will work, but there are a lot of Win7-specific bugs I don't have the time for right now.

Only 64-bit platforms are supported. I don't have any desire to support 32-bit platforms in late 2026, sorry.

**PLEASE NOTE**: in this context, "supported client OS" means "the ones that I've tried to launch this on". It is not an exhaustive list.

# How to build?

This project can be built both on Linux and Windows. Please refer to [BUILD.md](BUILD.md) for specific build instructions.

# AI involvement?

For the most part this code is written by a human, i.e myself. 

A locally hosted instance of Qwen3.8 Flash with throughoutput of barely 20 t/s on my Radeon 7900 was used for code clean-up and to speed up debugging process on a few occasions. 

If you think that this is vibe-coded, think again. Any commercial LLM would've probably written it better.

# License?

This project is licensed under [GPLv2](LICENSE) or any later, per your choice.

# Acknowledgements?

This code is based on [vladislavrv](https://github.com/vladislavrv/MatrixGame)'s version of MatrixGame, which is in turn based on [murgesku](https://github.com/murgesku/MatrixGame)'s version of MatrixGame - which is in turn based on the [original GPLv2 release of MatrixGame](https://snk-games.net/matrixgame.tar.gz), as released by CHK-Games back in December of 2019 ([see original post](https://web.archive.org/web/20220102100125/https://snk-games.net/forums/viewtopic.php?f=41&t=2154) in Russian). All of those are also licensed as GPLv2.
