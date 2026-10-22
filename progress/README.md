# Screen progress

One screenshot per finished translation unit: the last frame of the session's smoke run
(`tools/reverse/build.py --run`), copied here by `tus.py finish` as `<date>-<time>_<tu>.png` and
committed with the TU. Browse them in order to see what the port draws after each TU; the commit that
adds a picture names it on its `Screen:` line.

What a screen should look like: `python tools/reverse/tm2png.py <file.tm2>` (or `--scan <archive>`)
converts the game's own textures to PNGs in `tools/reverse/work/ref/`.
