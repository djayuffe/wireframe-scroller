# Architecture

Impossible Wireframe is a single-threaded render loop plus one audio callback thread. Geometry and effects run on the CPU and produce an indexed line mesh; the GPU draws it additively into an HDR target and a full-screen post shader turns that into the final picture.

```
            +-------------------+     +------------------+
 time/BPM ->|     Timeline      |     |   AudioPlayer    |  (SDL2 callback thread)
            | beat/bar/pulse    |     | libopenmpt + 3   |
            +---------+---------+     | band IIR split   |
                      |               +--------+---------+
                      v                        | atomic bands/level
  +-------------+   +-------------------------v--------------------------+
  | SceneSystem |-->|                      main loop                      |
  | 57 meshes,  |   |  pick scene -> EffectRecipe -> applyRecipe(mesh)    |
  | cached      |   |  upload if changed -> draw passes -> screenshot     |
  +-------------+   +-------------------------+--------------------------+
        ^                                      |
 Geometry / AdvancedGeometry /                 v
 ShowpieceGeometry                  Renderer (OpenGL 4.1 Core)
                                    1 background  -> HDR FBO (logo card, opacity from the logo show)
                                    2 wireframe   -> HDR FBO (additive lines, gain by edge count)
                                    3 travellers  -> HDR FBO (depth sorted octahedra)
                                    4 post.frag   -> screen  (warps, art, FX, tone map)
                                    5 scroller    -> screen  (alpha-blended text band)
```

## Frame flow (`src/main.cpp`)
1. Parse options, validate `--window`, `--recipe`, `--scene`; install SIGINT/SIGTERM flag; open the GLFW window (OpenGL 4.1 core), `Renderer::init`, `Renderer::loadLogos`, optional `AudioPlayer::open`.
2. Each frame: sample `Timeline` at the show time for beat, bar and pulse; read `MusicState` if audio is active; handle keys (scene, recipe, scroller, seed).
3. Choose the scene: automatic from the bar index, or `manual` (arrow keys, `--scene`). `SceneSystem::mesh()` returns the cached mesh for the current update tick.
4. Build an `EffectContext` (time, amount, bass/mid/treble/beat, seed) and an `EffectRecipe`: the curated recipe for the scene plus a two-stage mutation, a fixed recipe in recipe mode, or, for showpiece scenes 51-56, no mutation and a periodic "Pure form" window where no recipe is applied.
5. `applyRecipe` chains the stages on a copy of the mesh and restores it if any stage yields an empty or invalid mesh. A secondary mesh (next scene) is computed only if a stage uses it.
6. `Renderer::upload` only when the scene, mesh pointer or (vertex count, edge count, radius) signature changed; a rejected mesh ends the show with a message.
7. Draw passes, scroller, then optional `screenshot` and the `--frames` exit check.

## Modules
| Module | Responsibility |
|---|---|
| `Geometry` | `V3`/`V4`, `Mesh3` (vertices + edge list), `Polytope4`; canonical polychora (tesseract, 16-, 24-, 600-cell), 4D rotation and projection, hyperplane slicing, `validate`/`stats` |
| `AdvancedGeometry` | Triply periodic implicit surfaces, 120-cell dual, quaternion-Julia slice, hyperbolic ball, Clifford torus, knots, attractors, v4 exotic families (parametric surfaces, algebraic isosurfaces, Sierpinski tetrahedron), 10 "unknown lab" objects, deterministic discovered objects |
| `ShowpieceGeometry` | Scenes 51-56: geodesic dome, tube trefoil, DNA helix, atom orbits, wave terrain, Platonic compound; a `Builder` that de-duplicates edges |
| `Scene` | The 57-entry catalogue (`SceneInfo`: id, name, dynamic flag, update Hz, provenance) and a per-scene cache keyed by the quantised time tick |
| `Effects` | 80 vertex warpers, 30 recipes, procedural mutation, `sanitize()` (drops self, duplicate and out-of-range edges), `applyRecipe` with restore-on-failure |
| `Timeline` | Pure function from seconds and BPM to beat, bar, phase and pulse |
| `Audio` | libopenmpt module rendering through SDL2; one-pole IIR band split into bass, mid, treble; publishes atomics read by the render thread (compiled out without `IW_HAS_AUDIO`) |
| `Renderer` | All GL state: programs, VAOs, the dynamic line buffers, RGBA16F HDR FBO, logo textures and the logo-show state machine, traveller mesh, scroller font and code-strip textures, screenshot |
| `TextScroller` | 5x7 font table (`textfont::pack`), text measurement, constant-speed scroll offset |
| `Image` | stb_image decode and logo discovery |
| `WindowSpec` | Shared `--window` parser (used by the app and the tests) |

## Geometry data model
A `Mesh3` is just `std::vector<V3> v` and `std::vector<Edge> e` (index pairs). Every generator must produce finite vertices, in-range indices and no self or duplicate edges; `geo::validate` enforces this in tests and `Effects::sanitize` repairs it after warping. Scenes are flagged dynamic with an update rate so animated meshes are regenerated at a fixed rate and cached in between.

## Effect system (`src/Effects.cpp`)
An effect is `applyEffect(mesh, id, ctx, secondary)`. A recipe is up to six `EffectStage`s (effect id, amount, time scale and offset, seed offset, uses-secondary). The mutation generator derives stages from a seed, so a scene always looks the same for a given seed and `P` changes it. Every stage is followed by sanitisation and an edge-count cap of 50 000.

## Rendering pipeline (`src/Renderer.cpp`, `shaders/`)
- **HDR target**: RGBA16F colour plus depth, sized by `--quality`. Lines are drawn with additive `GL_ONE, GL_ONE` blending, so overlapping wires glow instead of occluding.
- **`bg.frag`**: draws the logo card (two slots cross-fade on scene change), V-flipped and zoomed to cover the screen, multiplied by the logo-show opacity.
- **Logo show** (`Renderer::drawBackground`): a time-driven 14.5 s cycle (fade in 1 s, hold 3.5 s, fade out 1 s, black 9 s) sets `logoVis_`. `logoVis_` scales the card opacity, trims wire gain and is passed to the post shader as `uLogoVis`.
- **`wire.frag`/`wire.vert`**: object-space colour cycling and music-reactive highlights; `uGain` is `clamp(0.80*sqrt(500/lines), 0.05, 1)` so dense meshes do not burn out.
- **`traveler.*`**: one small octahedron mesh whose per-vertex lane index (`aPath`) lets the vertex shader ping-pong each traveller along its own Lissajous-style path.
- **`post.frag`** (in order): UV warps (lensing, shockwave, barrel, wild event, chromatic aberration) gated by `1 - hasLogo`; bloom and streak taps; procedural background plus `artLayer` when the logo is away; additive compositing with logo-aware gain; SDF triangle, holographic stripes, glitch slices; scanlines, vignette, shadow, grain; hue drift, solarize, posterize; ACES tone map and gamma. `hasLogo = uHasLogo * uLogoVis`, so every logo-dependent term fades smoothly.
- **`scroller.*`**: a full-screen triangle; the fragment shader finds the glyph cell for each pixel, reads the character code from the `uCodes` strip texture (fixed 512 texels), extracts the glyph bit with integer ops and writes premultiplied alpha.

### Platform notes
macOS stops at OpenGL 4.1, so `glTexStorage2D` (4.2) is avoided, 1D textures are avoided (Metal-backed GL), `glLineWidth > 1` is not used in the core profile, and stale GL errors are flushed before `glReadPixels`. Shaders are read from `shaders/` at runtime (copied beside the binary at build time), so a shader edit needs a `make`.

## Build system
`CMakeLists.txt` defines `iw_geometry` (all non-GL code, always built, so headless CI works), `geometry_tests`, and, when GLFW and OpenGL are found, `impossible_wireframe`. Audio is optional (`IW_HAS_AUDIO` when SDL2 and libopenmpt are found). `Makefile` wraps CMake with a stamp file so a changed toolchain triggers reconfiguration.

## Testing strategy
`tests/geometry_tests.cpp` is one executable of `req(cond, message)` checks that exit non-zero on the first failure: canonical polytope counts, validity of every scene and recipe, timeline math, window parsing, scroller buffer limits and shader convention checks. The visual layers are verified by `--screenshot --frames N` captures.

## Extending
- **New scene**: add a generator (keep it valid and duplicate-free), add a `SceneInfo` row and a `case` in `SceneSystem::mesh`, bump the count in `tests/geometry_tests.cpp`, add a row to `data/object_catalog.csv`.
- **New effect**: add a `case` in `applyEffect`, bump `effectCount()`, optionally include it in a recipe.
- **New post effect**: edit `shaders/post.frag`; gate it with `hasLogo` if it must not touch the logo card; run `make` to copy shaders.
