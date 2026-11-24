# RosePan

A PC port of Rule of Rose (PS2, US `SLUS_214.48`), reconstructed function by function from the retail
executable into C++ on a PS2-SDK platform layer -- after MioPan (Fatal Frame 2) and ReiPan (Fatal Frame 3).

- The game's executable and data are not distributed: dump them from your own disc (the ELF goes at the
  repository root).
- How the work is done: `.claude/skills/rosepan-port/SKILL.md` (one translation unit per session, the
  tools in `tools/reverse/`, the references next to the skill).
- Progress: `.claude/skills/rosepan-port/references/status.md` (generated) and `progress/` (one
  screenshot per finished unit).

## Building on Linux

Needs a C/C++ compiler, CMake, Make, Git and SDL3's system development packages; the dependencies are
fetched during the build. FFmpeg is built from source with only what the movies use (the MPEG-2 decoder,
its parser and colour conversion) and linked statically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DROSEPAN_COMPILE_SHADERS=OFF
cmake --build build -j
```

The executable is `build/RosePan/RosePan`.

## Releases

Every push to `main` that changes what ships (`src/`, `resources/`, `extern/`, `cmake/`, `CMakeLists.txt`)
becomes the next patch release: `.github/workflows/Release.yml` takes the highest `vX.Y.Z` tag, adds one
(`v0.0.1` first), builds both platforms and publishes the zips under the new tag. Docs and tooling pushes
release nothing, and a failed build leaves its number unused. To raise the minor or major number, run the
workflow by hand (Actions > Release > Run workflow) and pick the part.

| zip | built with |
|---|---|
| `RosePan_windows_vX.Y.Z.zip` | MSVC on `windows-latest`, static CRT, MioPan's prebuilt FFmpeg |
| `RosePan_linux_vX.Y.Z.zip` | gcc-14 in Valve's Steam Runtime 3 SDK (Steam Deck; glibc 2.31 or newer), minimal FFmpeg from source |

The version is in the log banner (`dev` for any other build).
