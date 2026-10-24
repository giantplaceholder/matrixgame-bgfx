# Building basics

This document will tell you how to build MatrixGame binaries from this repo.

It assumes that you are somewhat of a power user, so we won't be covering the basics here.

This document also does not cover building a compatible Rangers.exe. Refer [here](https://github.com/giantplaceholder/spacerangershd-fpc-fixes) for additional details.

# Supported host operating systems

This project can be built both on Linux and Windows. 

Linux distros tested:

- Ubuntu 24.04.1
- Linux Mint Xia 22.1 (basically, an Ubuntu 24.04)
- Debian 13 Trixie
- Arch Linux/CachyOS

Windows versions tested:

- Windows 10 (22H2)
- Windows 11 (25H2)

Network is required to download bgfx dependencies, as well as cross-build dependencies.

# Linux dependencies

Ubuntu 24.04/Debian Trixie/Linux Mint:

```
sudo apt-get update && sudo apt-get install -y build-essential git cmake curl ca-certificates pkg-config g++-mingw-w64-x86-64-posix libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxrender-dev libxinerama-dev libxxf86vm-dev libxss-dev libgl1-mesa-dev libegl-dev libwayland-dev libasound2-dev libvulkan1 mesa-vulkan-drivers libgl1 libegl1 libx11-6 libxext6 libxrandr2 libxi6 libxcursor1 libxfixes3 libxss1 libasound2t64 libwayland-client0 libwayland-egl1
```

Arch Linux/CachyOS:

```
sudo pacman -Syyu && sudo pacman -S --needed base-devel git cmake curl ca-certificates pkgconf mingw-w64-gcc libx11 libxext libxrandr libxcursor libxi libxfixes libxrender libxinerama libxxf86vm libxss mesa wayland alsa-lib vulkan-icd-loader vulkan-radeon vulkan-intel libglvnd mingw-w64-binutils
```

# Windows dependencies

1) Install Git for Windows: https://git-scm.com/install/windows. Install the most recent version if possible.

Other dependencies will be downloaded by the build script automatically.

# Build on Linux

1) Clone this repo:

```
git clone https://github.com/giantplaceholder/matrixgame-bgfx.git
```

2) Launch build script: either run  ```./build_linux.sh``` to build Linux .so, or ```./build_windows.sh``` to build Windows DLL. Binaries will be placed to ```/out``` directory next to the build scripts.
3) Now just copy the binaries to your game folder, run Rangers.exe and try to play a planetary battle.

# Build on Windows

1) Open the terminal, clone this repo:

```
git clone https://github.com/giantplaceholder/matrixgame-bgfx.git
```

2) Launch build script: ```./build_windows_10_11.bat```. Binaries will be placed to ```/out``` directory next to the build scripts.

3) Now just copy the binaries to your game folder, run Rangers.exe and try to play a planetary battle.

