# chernobot

A one-size-fits-all program for Singapore American School's MATE program, written in C++17. Undergoing heavy restructuring!

## Getting Started

### macOS

#### Automatic Script

The following script will automatically check if dependencies are installed, install dependencies from Homebrew, then compile and install Chernobot.

Paste this into a Terminal prompt
```
curl -sSL --output chernobotinstaller.sh https://raw.githubusercontent.com/hyperum/chernobot/master/install.sh && grep '#!/bin/bash' chernobotinstaller.sh > /dev/null && bash ./chernobotinstaller.sh dl
```


#### Manual

You'll need the latest version of Clang for C++17, SDL2, Ninja (optional), CMake, and various other tools.

If you haven't already, install the XCode Developer Tools with:

```
xcode-select install
```

Install [Homebrew at brew.sh](brew.sh), then:

```
brew install sdl2 ninja cmake opencv
```

You can install and run this program from the root directory by: (You can substitute `Ninja` with `"Unix Makefiles"`)

```
mkdir tmp
cd tmp
cmake .. -GNinja
ninja install
../chernobot
```

### MATE 2019

Functionality that is needed to be implemented:
- Measuring water temperature
- FFMPEG video capture
- Shape recognition with OpenCV + OVERLAY

Add: Documentation, containing an algo. desc. and a data flow diagram.

Functionality that would be nice to implement:
- Something with parallax, accelerometry, and length measurement.
- 
- 
