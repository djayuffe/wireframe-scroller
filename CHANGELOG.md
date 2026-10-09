# Changelog

All notable changes, newest first. Each version is tagged and has a GitHub release.

## v4.35
- Windows support: an in-tree OpenGL 4.1 function loader (`GLWin32`), MSVC build definitions and warning policy, and a Windows CI job that builds and tests (it has not been run on a physical Windows machine yet).
- CI fixes: GCC misleading-indentation, Apple clang `sprintf` deprecation in the vendored stb header, correct vcpkg port name.

## v4.34
- Six new showpiece 3D objects (scenes 51-56): geodesic dome, tube trefoil knot, DNA double helix, atom with orbiting electrons, rippling wave terrain, Platonic compound (icosahedron + dodecahedron + stella octangula). They get a clean 'Pure form' window each ~12 s before the effect warp swells in.
- New `--scene N` option to start on a given scene.

## v4.33
- Wild shader events: every 6 s one warp takes over (swirl, radial ripples, kaleidoscope fold, mosaic crunch, datamosh block shift, liquid wobble, mirror horizon), plus hue drift, downbeat solarize and neon posterize bursts. Three new art styles: orbit-trap Julia fractal, op-art moire, Turing spots.

## v4.32
- Living background: a raymarched 3D metaball organism that melts, breathes with the beat and music, and is lit with fresnel and specular; the whole art field now swells and shears like tissue, with drifting light pools for an uneven look.

## v4.31
- First release where the scroller fix and the updated shader test are both in and the suite passes. v4.29 and v4.30 were tagged with that one test failing.

## v4.30
- Tag only; superseded by v4.31.

## v4.29
- Fixed the text scroller: it was upside-down at the top of the screen with corrupted glyphs (inconsistent font table, wrong code-strip texel mapping, inexact `pow(2,bit)`). It is now a crisp ticker at the bottom with a clean 5x7 font.

## v4.28
- Arty procedural background layer and anamorphic streak / light-ray eye candy in the post pass; new `uLogoVis` uniform fades them against the logo show.

## v4.27
- Fixed hangs: a false GLFW context-lost check caused per-frame reinitialisation, and a rejected mesh with duplicate edges ended the show. `sanitize()` now removes duplicate, self and out-of-range edges, with a regression test.
- Logo now renders the right way round and fills the screen.
- New timed logo show with fades and long black breaks.
- Wire gain scaled by edge count, and a reduced bloom, exposure and gamma lift over the logo, so the logo and wireframe are no longer washed out.
- `--screenshot` now flushes stale GL errors, and `--frames N` captures on frame N.
- macOS fixes: no `glTexStorage2D` (GL 4.2), GLFW static link.
