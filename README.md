# RosePan

A PC port of Rule of Rose (PS2, US `SLUS_214.48`), reconstructed function by function from the retail
executable into C++ on a PS2-SDK platform layer -- after MioPan (Fatal Frame 2) and ReiPan (Fatal Frame 3).

- The game's executable and data are not distributed: dump them from your own disc (the ELF goes at the
  repository root).
- How the work is done: `.claude/skills/rosepan-port/SKILL.md` (one translation unit per session, the
  tools in `tools/reverse/`, the references next to the skill).
- Progress: `.claude/skills/rosepan-port/references/status.md` (generated) and `progress/` (one
  screenshot per finished unit).
