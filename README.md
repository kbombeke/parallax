# Head-Tracked 3D First-Person Runner

A Wolfenstein 3D-style first-person runner game that uses webcam head tracking to create stereoscopic depth illusion through motion parallax.

## Requirements

- GCC or Clang compiler
- SDL2 development libraries
- OpenGL
- OpenCV 4.x (or 3.x)

### macOS Installation

```bash
brew install sdl2 opencv
```

### Linux Installation (Ubuntu/Debian)

```bash
sudo apt-get install libsdl2-dev libopencv-dev libgl1-mesa-dev
```

### Linux Installation (Fedora)

```bash
sudo dnf install SDL2-devel opencv-devel mesa-libGL-devel
```

## Building

```bash
make
```

## Running

```bash
make run
# or
./stereoscopicgame
```

The game will request camera access. Grant permission to enable head tracking.

## Controls

- **W/A/S/D**: Move forward/left/backward/right
- **ESC**: Exit game

## How It Works

The game uses a raycasting engine (like Wolfenstein 3D) combined with head tracking. Your webcam tracks your head position, and the virtual camera view updates in real-time based on your head movement relative to the screen. This creates motion parallax that gives the illusion of depth without requiring stereoscopic displays.

## Technical Details

- **Raycasting**: DDA algorithm for efficient wall rendering
- **Head Tracking**: OpenCV face detection and landmark tracking
- **Window Mode**: Screen acts as a fixed window; your head position defines the virtual camera origin
- **Temporal Smoothing**: Reduces jitter in head tracking
