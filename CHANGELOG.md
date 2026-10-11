# Changelog

All notable changes, newest first. Each version is tagged and has a GitHub release.

## v4.39
### Performance (see README, Performance)
- Effects: sort-based duplicate removal only where duplicates can arise; a 60,000-edge budget every multiplying effect respects (chains reached 404,000 edges); stages that produce non-finite vertices are undone instead of poisoning the frame.
- Scenes build on worker threads (`SceneSystem::meshAsync`, `prefetch`); the render loop never waits for the 70-300 ms implicit-surface rebuilds, not even when a scene change outruns its prefetch (the previous mesh stays on screen until the new one is ready). Stress test at 600 BPM, 83 scene changes: worst frame 8.9 ms CPU (was 101 ms).
- Upload: O(edges) `geo::renderable` check, direct index upload, buffer orphaning; `geo::validate` is sort-based.
- GPU: the procedural background and art layer moved to a half-resolution pass (`shaders/art.frag`), skipped while a logo card covers the screen: the simplest scene went from 34 to 167 fps uncapped, the heaviest from 3 to over 100 fps.
- `--quality` now also scales the art pass (it had no effect on the main GPU cost before).
### Fixes
- A rejected mesh no longer ends the show: it is logged once and the previous mesh stays.
- Full-screen is vsynced (it ran uncapped, tearing and heating the GPU); `--no-vsync` for benchmarks.
- The downbeat solarize flash no longer lifts the black background to grey.
### New
- `--stats`: fps and CPU time per frame every 5 seconds.
- Tests: edge budget and renderability for every scene through the auto recipe, full validation of every effect, async scene fetch convergence, tempo continuity.
- Soundtrack playlist: **"Digital Rendezvous" by Drozerix** (Public Domain, The Mod Archive 180821) now follows Stars, and the list loops. The credit in the ticker and the beat tempo switch with the track; the beat count carries on without a jump (`Timeline::retempo`). `--music` can be repeated for your own playlist.
- Detected tempos are folded into 70-180 BPM (fast modules use 8 rows per beat; Digital Rendezvous reads as 276 and is 138).
- The default music is found from the executable's directory and its ancestors too, so the demo is no longer silent when started from `build/`.

## v4.38
- New default soundtrack: **"Stars (4ch. version)" by cs127** (CC BY 4.0, The Mod Archive 201917), credited in the README, NOTICE, music README and in the ticker while it plays.
- The visuals lock to the module's own tempo (tempo x 6 / speed) unless `--bpm` is given. Silicon Dancer turns out to be 125 BPM, not the 132 the docs assumed.
- `make run-silicon` plays Silicon Dancer; the fetch script verifies both modules against The Mod Archive's MD5s.
- WAV playback: `--music file.wav` now works (SDL decodes and loops it; band analysis and the music clock work as for modules). The bundled `wireframe_pulse.wav` and `make run-pulse` were silently falling back to the BPM clock because libopenmpt only reads tracker modules.
- Golden scenes: light rays tinted gold.
- The music clock is now total played time, not the position in the track, so the show keeps going when a track loops instead of restarting its scene sequence and logo show (a 30 s loop never got past scene 8).

## v4.37
- New scenes 57-58, ported from uber256-lab: **Golden Galaxy** (de Jong attractor, GALAXY.COM) and **Golden Cliff** (Clifford attractor, CLIFF.COM) as glowing 3D orbit clouds with the original constants, start points and parameter morph.
- New golden glimmer wire style: twinkling gold dashes, a sweeping shimmer band, four-point glints and gold cross streaks in the post pass; warps and colour grading stay off so the shape and colour survive.

## v4.36
- 16 new "unknown" 3D wireframe effects (64-79): Hopf rotation, Klein fold, hyperbolic drift, Thomas flow, galactic disk, harmonic bloom, FCC crystal snap, tendril growth, shell cage, barbed wire, torus attractor, Moebius map, soliton wave, quaternion square, Lissajous satellites, spectral harmonics.
- 6 new recipes: Hopf Fibration Storm, Hyperbolic Drift, Keplerian Galaxy, Barbed Crystal, Soliton Garden, Quaternion Bloom (80 effects and 30 recipes in total).

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
