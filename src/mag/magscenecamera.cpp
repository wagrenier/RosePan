// mag/magscenecamera.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/magscene.h"

// ──────────────────────────────────────────────────────────────────────
// Empty in the ROM (mainInitGame calls it).
// ROR 0x0013c928

void MagScene::initCamera()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013d5ac

int MagScene::getCameraMode(int *out)
{
    if (out)
    {
        out[0] = m_camera_mode;
        out[1] = m_camera_2ad58;
    }
    return m_camera_mode;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0013caf0
void MagScene::setCameraMode(int mode)
{
    ROSEPAN_STUB("MagScene::setCameraMode", 0x0013caf0);
}
