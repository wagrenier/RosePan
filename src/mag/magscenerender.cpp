// mag/magscenerender.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/magscene.h"

// ──────────────────────────────────────────────────────────────────────
// After the frame's draw: the PC's and Brown's motion sequences.
// callcheck: updateMotionSeq (inlined MagLifeControlChara::renderPostProcess calls it)
// ROR 0x0013c29c

void MagScene::postDraw()
{
    m_pc.renderPostProcess();
    m_bwn.renderPostProcess();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013c2ec

void MagScene::setCameraTarget(const float *pos, const float *at)
{
    m_camera_pos[0] = pos[0];
    m_camera_pos[1] = pos[1];
    m_camera_pos[2] = pos[2];
    m_camera_pos[3] = pos[3];
    m_camera_at[0] = at[0];
    m_camera_at[1] = at[1];
    m_camera_at[2] = at[2];
    m_camera_at[3] = at[3];
    m_camera_target_set = 1;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0013bcc0
void MagScene::draw(i3dContext *ctx, sceGsDBuff *dbuff, int frame)
{
    ROSEPAN_STUB("MagScene::draw", 0x0013bcc0);
}
