/* ==========================================================================
 *  main.cpp
 *
 *  PC: the process entry, a placeholder until Rule of Rose's own main
 *  (ROR 0x002045a8: prints the build date, loads ioprp300.img, then runs the
 *  game) is reconstructed. It brings the platform up the way the game will --
 *  the log first, then the game thread under RosePan_HostMain -- and presents
 *  empty frames at the V-blank rate, so the window, the frame pacing and the
 *  smoke run's screenshots (ROSEPAN_SHOT_DIR) work before any game code does.
 *  The game's main replaces GameMain below and moves to its own file.
 * ======================================================================== */

#include "rosepan/io/rosepan_log.h"
#include "rosepan/os/rosepan_host.h"
#include "rosepan/rendering/rosepan_renderer.h"
#include "sdk/libgraph.h"

static int GameMain(void)
{
    // NTSC, interlaced, frame mode: what the retail game asks for.
    sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME);
    for (;;)
    {
        RosePan_RendererBeginFrame();
        sceGsSyncV(0);
        RosePan_RendererEndFrame();
    }
}

int main(void)
{
    RosePan_LogInit();
    return RosePan_HostMain(GameMain);
}
