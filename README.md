# macOS Impossible Wireframe

C++20 / OpenGL 4.1 Core wireframe demoscene for macOS, focused on mathematically unusual geometry, HDR-style shader compositing, and tracker-music-reactive motion. The project packages the audited impossible-wireframe design as a standalone public repo with tests, provenance, screenshots, and bundled demo music.

Copyright (c) 2026 Ulf Bertilsson. Code is MIT licensed.


## Screenshots

![Impossible Wireframe scene](scene.png)
![Impossible Wireframe alternate scene](scene2.png)
![Impossible Wireframe shader scene](scene3.png)

## Implemented scenes
The show now contains 51 validated scenes. Exact/derived scenes include the
600-cell projection, exact 600-cell face-plane slice and dual-derived 120-cell
projection. Parametric/numerical scenes include TPMS surfaces, Hopf fibres, Boy
surface, superformula, Clifford torus, quaternion-Julia boundary slice,
Lissajous and chaotic attractors, 25 v4 exotic geometry families and 10 ported
unknown-lab procedural wire objects.

`data/object_catalog.csv` records provenance. The hyperbolic and quaternion scenes are visualizations, not claimed canonical honeycomb/fractal meshes.

## Build
macOS (Homebrew):
```sh
brew install cmake glfw
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/impossible_wireframe --bpm 132
```
Linux: install a C++20 compiler, CMake, OpenGL development headers and GLFW3 development package, then use the same CMake commands.

If OpenGL/GLFW are absent, CMake still builds `iw_geometry` and `geometry_tests`, allowing headless CI validation.

## Controls
- Left / Right: previous / next scene and enter manual scene mode
- Space: return to automatic beat/bar scene sequencing
- Escape: quit
- `--bpm N`: synchronization tempo (default 132)

## Music

The repo includes Drozerix — **Silicon Dancer** (`MOD`), listed by the Quinlight Audio project as Public Domain, for immediate local playback. The fetcher can refresh the same file if needed:

```sh
python3 assets/music/fetch_other_music.py
./build/impossible_wireframe --music assets/music/drozerix_-_silicon_dancer.mod --bpm 132
```

When SDL2 and libopenmpt are available, the module plays locally and its decoded energy drives line glow, bloom intensity, timeline seconds, and background motion. Without those dependencies, the demo remains deterministic from its BPM clock.

## Correctness gates
Project targets compile with `-Wall -Wextra -Wpedantic -Werror` (or `/W4 /WX`). Tests assert canonical V/E/F counts for tesseract, 16-cell, 24-cell, 600-cell and V/E for the dual 120-cell; exercise every procedural family; reject non-finite vertices, invalid indices, self-edges and duplicate edges; and verify timeline beat/bar math.

## Architecture
- `Geometry.*`: canonical polychora, projections, slicing, base parametric surfaces, validation.
- `AdvancedGeometry.*`: TPMS extraction, dual 120-cell, quaternion boundary lattice, hyperbolic visualization, attractors, v4 exotic families, unknown-lab adapters and deterministic discovery.
- `Scene.*`: scene catalogue, provenance and update-rate cache. Expensive implicit/fractal geometry is not rebuilt at video refresh rate.
- `Timeline.*`: deterministic BPM/beat/bar synchronization.
- `Renderer.*`: OpenGL 4.1 indexed line renderer with checked external GLSL compilation/linking, reusable buffers, RGBA16F HDR render target, shader composite, breathing size cycle, zoom/flyover camera choreography and guarded framebuffer setup.
- `shaders/post.*`: RGBA16F HDR-style composite pass with procedural background,
  glimmer, bloom-like highlight shaping, tone mapping and music-reactive light.
- `shaders/wire.*`: object-space wire color cycling, lighting-matrix bands and
  music-reactive electric edge highlights.

See `design.md` for mathematical provenance and design constraints.
See `docs/EFFECTS.md` for the implemented effect catalogue.
