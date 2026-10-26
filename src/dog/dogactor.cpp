// dog/dogactor.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "dog/dogactor.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0010a038
void DogActor::addTrans(const float *d, float trans_ofs_y, float rot_y_aar)
{
    ROSEPAN_STUB("DogActor::addTrans", 0x0010a038);
}

// stub of ROR 0x00109348
int DogActor::getMotionMove(float t0, float t1, float *move, float *rot, float *pos)
{
    ROSEPAN_STUB("DogActor::getMotionMove", 0x00109348);
    return {};
}

// stub of ROR 0x0010eab8
void DogActor::getNodeWorldMatrix(sceVu0FMATRIX out, int n)
{
    ROSEPAN_STUB("DogActor::getNodeWorldMatrix", 0x0010eab8);
}

// stub of ROR 0x00109b88
void DogActor::releaseMotion(int theId)
{
    ROSEPAN_STUB("DogActor::releaseMotion", 0x00109b88);
}

// stub of ROR 0x0010c17c
u_int DogActor::loadPackMotion(const char *theFileName, int a, int b)
{
    ROSEPAN_STUB("DogActor::loadPackMotion", 0x0010c17c);
    return {};
}

// stub of ROR 0x0010f2dc
int DogActor::checkName(const char *theName)
{
    ROSEPAN_STUB("DogActor::checkName", 0x0010f2dc);
    return {};
}

// stub of ROR 0x0010cd5c
int DogActor::load(const BedPfh &pfh, DogTexInfo *tex, int mode, int light)
{
    ROSEPAN_STUB("DogActor::load", 0x0010cd5c);
    return {};
}

// stub of ROR 0x0010c568
u_int DogActor::loadMotion(const BedPfh &pfh, int arg)
{
    ROSEPAN_STUB("DogActor::loadMotion", 0x0010c568);
    return {};
}

// stub of ROR 0x00109ed0
void DogActor::releaseModel(DogTexInfo *tex)
{
    ROSEPAN_STUB("DogActor::releaseModel", 0x00109ed0);
}

// stub of ROR 0x0010bf00
void DogActor::setParam1d8(int v)
{
    ROSEPAN_STUB("DogActor::setParam1d8", 0x0010bf00);
}

// stub of ROR 0x0010cffc
int DogActor::shareModel(DogActor *src)
{
    ROSEPAN_STUB("DogActor::shareModel", 0x0010cffc);
    return {};
}

// stub of ROR 0x0010e5c0
void DogActor::draw(I3dLayer *layer, const float *cam_rotate, const float *cam_trans)
{
    ROSEPAN_STUB("DogActor::draw", 0x0010e5c0);
}

// stub of ROR 0x0010e3ac
void DogActor::drawModel(I3dLayer *layer, int nodes, DogEntity *src)
{
    ROSEPAN_STUB("DogActor::drawModel", 0x0010e3ac);
}

// stub of ROR 0x0010eb28
void DogActor::getNodeWorldPos(float *out, int n)
{
    ROSEPAN_STUB("DogActor::getNodeWorldPos", 0x0010eb28);
}

// stub of ROR 0x0010e534
void DogActor::drawShadow(I3dLayer *layer)
{
    ROSEPAN_STUB("DogActor::drawShadow", 0x0010e534);
}

// stub of ROR 0x0010bcc8
u_int DogActor::getMotionEvent(float t0, float t1, int motion)
{
    ROSEPAN_STUB("DogActor::getMotionEvent", 0x0010bcc8);
    return {};
}

// stub of ROR 0x0010da4c
float DogActor::getMotionFrame()
{
    ROSEPAN_STUB("DogActor::getMotionFrame", 0x0010da4c);
    return {};
}

// stub of ROR 0x0010dad8
float DogActor::getMotionStep()
{
    ROSEPAN_STUB("DogActor::getMotionStep", 0x0010dad8);
    return {};
}

// stub of ROR 0x0010d388
void DogActor::runMotionCmd()
{
    ROSEPAN_STUB("DogActor::runMotionCmd", 0x0010d388);
}

// stub of ROR 0x0010f298
void DogActor::resetShadow()
{
    ROSEPAN_STUB("DogActor::resetShadow", 0x0010f298);
}

// stub of ROR 0x0010b1b0
int DogActor::changeMotion(float speed, int theNo, u_int flags, int flags2, int arg)
{
    ROSEPAN_STUB("DogActor::changeMotion", 0x0010b1b0);
    return {};
}

// stub of ROR 0x0010aa9c
int DogActor::findMotionByName(const char *theName)
{
    ROSEPAN_STUB("DogActor::findMotionByName", 0x0010aa9c);
    return {};
}

// stub of ROR 0x0010d0e8
float DogActor::getMotionLength(int theID)
{
    ROSEPAN_STUB("DogActor::getMotionLength", 0x0010d0e8);
    return {};
}

// stub of ROR 0x0010af18
int DogActor::setMotion(float speed, int theID, u_int flags, int flags2, int p1d8)
{
    ROSEPAN_STUB("DogActor::setMotion", 0x0010af18);
    return {};
}

// ──────────────────────────────────────────────────────────────────────
// The actor is in a mode (native 004 case 17 waits while it is).
// ROR 0x0010a834

bool DogActor::isModeSet() const
{
    return m_mode != 0;
}

// ──────────────────────────────────────────────────────────────────────
// The eye flag ops of native 281 op 21: each does nothing unless the
// model has an eye node with a material (setEyeTex found them).
// ROR 0x0010f70c

void DogActor::eyeHold()
{
    if (m_eye_node != NULL && m_eye_mat != NULL)
        m_eye_flags = (m_eye_flags & ~2u) | 1;
}

// ROR 0x0010f73c

void DogActor::eyeStart()
{
    if (m_eye_node == NULL || m_eye_mat == NULL)
        return;
    if ((m_eye_flags & 1) == 0)
    {
        m_eye_flags |= 3;
        if (m_eye_state != 0)
            return;
    }
    else if (m_eye_count >= m_eye_period)
    {
        return;
    }
    m_eye_count = 0;
    m_eye_period = 1;
}

// ROR 0x0010f7a0

void DogActor::eyeStep()
{
    if (m_eye_node == NULL || m_eye_mat == NULL)
        return;
    u_int flags = m_eye_flags;
    if ((flags & 1) != 0)
    {
        if (m_eye_count < m_eye_period)
            m_eye_flags = flags & ~3u;
        else
            m_eye_flags = flags | 2;
        return;
    }
    if (m_eye_count >= m_eye_period)
        m_eye_flags = flags | 3;
}

// ROR 0x0010f808

void DogActor::eyeReset()
{
    if (m_eye_node == NULL || m_eye_mat == NULL)
        return;
    setEyeTex(2);
    m_eye_period = 0;
    m_eye_count = 0;
    m_eye_flags &= ~3u;
}

// stub of ROR 0x0010f5e4
void DogActor::setEyeTex(int state)
{
    ROSEPAN_STUB("DogActor::setEyeTex", 0x0010f5e4);
}

// stub of ROR 0x0010aa5c
int DogActor::getMotionNoBySuffix(const char *theName)
{
    ROSEPAN_STUB("DogActor::getMotionNoBySuffix", 0x0010aa5c);
    return {};
}

// stub of ROR 0x0010d9ec
float DogActor::getMotionTime()
{
    ROSEPAN_STUB("DogActor::getMotionTime", 0x0010d9ec);
    return {};
}

// stub of ROR 0x0010a650
float DogActor::getRotY()
{
    ROSEPAN_STUB("DogActor::getRotY", 0x0010a650);
    return {};
}

// ──────────────────────────────────────────────────────────────────────
// The motion frame in 120ths (written ahead with task/tsk_chr.cpp, whose
// TskMtnMove compares it with the motion's length * 120).
// ROR 0x0010da80

float DogActor::getMotionFrame120()
{
    float f = m_motion.m_frame * 60.0f;
    return f + f;
}

// stub of ROR 0x0010eba0
void DogActor::setNodeScale(float scale, int node)
{
    ROSEPAN_STUB("DogActor::setNodeScale", 0x0010eba0);
}
