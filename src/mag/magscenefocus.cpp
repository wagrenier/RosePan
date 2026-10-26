// mag/magscenefocus.cpp -- the scene's focus effect (the efffocus texture,
// assert m_focus.m_texindex != -1): its mode (+0x853bc), three values
// (+0x853c8) and their lerps (+0x853fc), drawn by drawFocus.
//
// Written so far: startLerp and setFocusLerp (native 281 op 13); the rest
// are stubs.

#include "common/rosepan_stub.h"
#include "mag/magscene.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00140d94

void MagScene::setFocusLerp(int which, float from, float to, int frames)
{
    if (which == 1)
    {
        if (from < 0.0f)
            from = m_focus_val[1];
        m_focus_val[1] = startLerp(from, to, &m_focus_lerp[1], frames);
    }
    else if (which == 2)
    {
        if (from < 0.0f)
            from = m_focus_val[2];
        m_focus_val[2] = startLerp(from, to, &m_focus_lerp[2], frames);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00140e60

float MagScene::startLerp(float from, float to, MagLerp *rec, int frames)
{
    if (frames == 0)
    {
        rec->m_frames = 0;
        return to;
    }
    rec->m_frames = frames;
    rec->m_to = to;
    rec->m_from = from;
    rec->m_frame = 0;
    return from;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00140b20
void MagScene::setFocus(int mode)
{
    ROSEPAN_STUB("MagScene::setFocus", 0x00140b20);
}
