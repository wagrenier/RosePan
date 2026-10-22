---
name: rosepan-next-tu
description: Port the next Rule of Rose translation unit into RosePan (one TU per run), commit and push.
---

RosePan scheduled run: port exactly ONE translation unit (TU) of Rule of Rose into RosePan, commit, push, report.

Repository: E:\Programming\Git\Github\RosePan (Rule of Rose PS2 -> PC port; work branch ror-port; remote origin = github.com/wagrenier/RosePan). Work only inside it; ../ReiPan and ../MioPan are read-only references. Use absolute paths or cd into the repository for every command: this run may have started in another folder. Nobody is watching this run: never wait for answers or ask questions -- decide, write doubts into E:\Programming\Git\Github\RosePan\.claude\skills\rosepan-port\references\open-items.md, and carry on.

1. Preflight. Run these from E:\Programming\Git\Github\RosePan and STOP at the first one that fails, changing nothing, with a one-line reply saying which check failed:
   - tools/reverse/work/STOP does not exist (the user's stop switch).
   - tools/reverse/work/autopilot.lock does not exist (the terminal autopilot.py is running in this checkout).
   - `git status --porcelain` prints nothing. A dirty tree means another run is mid-TU or someone is working; a run that crashed also leaves it dirty -- say so in the reply so the user can look.
   - `git rev-parse --abbrev-ref HEAD` is a work branch (ror-port), not main and not a detached HEAD.
   - Ghidra is up with the RoR program open: from tools/reverse, `python -c "import rosepan; print(rosepan.ghidra_up())"` prints True.

2. Port one TU. Invoke the Skill tool with skill "rosepan-port" and args "next". If the Skill tool does not offer rosepan-port (the run started outside the repository), Read E:\Programming\Git\Github\RosePan\.claude\skills\rosepan-port\SKILL.md and follow it the same way. Follow its "One TU per session" protocol end to end: `python tus.py next --claim` (from tools/reverse), brief, boundaries, names, header, code, `python tus.py check <TU> --run 15`, record knowledge in the skill references, and `python tus.py finish <TU> --status done|verify|partial|blocked --summary ... --notes ... --coauthor "<the Co-Authored-By line from your session instructions>"`. `tus.py next` honours the pinned queue in tools/reverse/data/tus.json (host tasks first, such as the platform bootstrap), then the user's priorities in open-items.md "Next up". Work on that one TU only; do not start a second one.

3. Look at the screen -- the visual test decides, not the logs. Every smoke run saves screenshots to tools/reverse/work/shots/ (shot_<frame>.png every 60 frames, last.png); open them with the Read tool. Reference pictures: `python tm2png.py <file.tm2>` or `python tm2png.py --scan <archive>` (from tools/reverse) writes the game's own textures as PNGs to work/ref/. Log lines, stub hits and callcheck only show that code ran. A TU on the drawing path is done only when the screenshot shows what the reference shows; a wrong picture, or code still to write, means --status partial with the difference in the notes. Complete code that nothing on screen can show yet finishes with --status verify. `tus.py finish` commits the last screenshot to progress/.

4. Push. After `tus.py finish` has committed, run `git push origin HEAD` from the repository. The user has approved pushing every run's commit (2026-10-22; open-items.md "Decisions"). Never force-push, rewrite history, delete branches or tags. If the push fails, say so in the report; do not retry with other flags.

5. Report in 3-5 lines: the TU, its status, the commit hash and whether it was pushed, what the screenshot shows compared with the reference (and its progress/ file), and the next blocker.

Rules: never start other sessions, agents or scheduled tasks; do not edit tools/reverse/autopilot.py or this routine; leave the tree clean (finish commits everything; check `git status` before finishing for files that are not yours). Do not put backticks in --notes or --summary (the shell runs them). If the slice cannot be finished, keep what builds, revert what does not, and finish with --status partial or blocked and notes the next run can act on.
