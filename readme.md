# chernobot

A one-size-fits-all program for Singapore American School's MATE program, written in C++17. Undergoing heavy restructuring!

## Getting Started

### macOS

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
../bin/chernobot
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
