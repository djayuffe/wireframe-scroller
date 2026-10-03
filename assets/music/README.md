# Optional music

The demo can run from its internal BPM clock and does not require bundled audio.
For local music playback, the project fetcher uses Drozerix — **Silicon Dancer**
(`drozerix_-_silicon_dancer.mod`), listed by the Quinlight Audio project as
Public Domain.

Suggested local layout:

```text
assets/music/drozerix_-_silicon_dancer.mod
```

Music files are ignored by Git. Run:

```sh
python3 assets/music/fetch_other_music.py
./build/impossible_wireframe --music assets/music/drozerix_-_silicon_dancer.mod --bpm 132
```

When audio support is compiled in, libopenmpt/SDL2 play the module and its RMS
energy drives wire brightness. Without audio dependencies, the renderer falls
back to deterministic BPM timing.
