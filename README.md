# CHROMATIC ECHOES — INFO-H-502

## Setup (one time)

### 1. Install GLFW
```bash
brew install glfw
```

### 2. Copy your third_party libraries
```bash
cp -r /path/to/your/third_party ./third_party/
```
Required:
- `third_party/glad/include/glad/glad.h`
- `third_party/glad/include/KHR/khrplatform.h`
- `third_party/glad/src/glad.c`
- `third_party/stb/stb_image.h`
- `third_party/glm/glm/glm.hpp`
- `third_party/assimp/lib/libassimp.dylib`

### 3. Add your puzzle image
```bash
cp your_image.jpg assets/textures/puzzle_image.jpg
```

## Build & Run
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/bin/chromatic_echoes
```

## Controls
| Key | Action |
|-----|--------|
| P or Left Click | Place next puzzle piece |
| WASD | Fly through the universe |
| Mouse | Look around |
| Space | Move up |
| Left Shift | Move down |
| ESC | Quit |

## Puzzle
- 30 pieces (6×5 grid)
- Each piece is a 3D cube showing a slice of puzzle_image.jpg on every face
- Pieces float and bob in front of the universe background
- Press P to snap each piece into place with an arc animation
- When all 30 are placed the full image is reconstructed

## Universe Background
- Infinite black void with 2000 twinkling stars
- 21 colored light orbs (purple, pink, cyan, amber, green, red...)
- Light rays connecting the orbs
- Freely explorable with WASD + mouse
- Puzzle stays fixed at center regardless of camera position
