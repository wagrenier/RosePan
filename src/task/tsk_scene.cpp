/* ==========================================================================
 *  task/tsk_scene.cpp
 *
 *  The event scene player and the script tasks next to it (pinned
 *  0x1f6c64..0x1fc4f0); see tsk_scene.h. Written so far: TskSnd (sound
 *  fades, delayed SEs, the stream switch), TskCamera (the script's stick /
 *  face-to-face cameras), gamSystemDataLoaded / gamStage2Loaded.
 * ======================================================================== */
#include "task/tsk_scene.h"

#include <math.h>
#include <string.h>

#include "bed/bedmemory.h"
#include "bed/bedpad.h"
#include "game/gamman.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sdk/libvu0.h"
#include "snd/sndman.h"
#include "utl/utlmisc.h"

// ══════════════════════════════════════════════════════════════════════
//  TskSnd
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// Copies the parameter block and picks the kind's step; kind 6 ends at
// once (stepEnd) when the SE does not play (funcSE 0x16), kind 7 keeps
// the stream's name. The exec step is always stepFade.
// ROR 0x001f8fc0

TskSnd::TskSnd(int kind, const TskSndParam *param)
{
    m_param = *param;
    m_frames = m_param.m_frames;
    m_frame = 0;
    m_exec = &TskSnd::stepFade;
    m_kind = kind;
    memset(m_name, 0, sizeof(m_name));   /* PC: the ROM leaves the name unterminated past 0x1b chars */
    switch ((u_int)kind)
    {
    case 0:
        m_step = &TskSnd::stepSEVol;
        break;
    case 1:
        m_step = &TskSnd::stepStrVol;
        break;
    case 2:
        m_step = &TskSnd::stepMstVol;
        break;
    case 3:
        m_step = &TskSnd::stepMstVolSe;
        break;
    case 4:
        m_step = &TskSnd::stepMstVolStr;
        break;
    case 5:
        m_step = &TskSnd::stepPlaySE;
        break;
    case 6:
        if (g_gamMan.m_sndMan.funcSE((u_int)m_param.m_snd, 0x16, NULL) == 0)
            m_step = &TskSnd::stepEnd;
        else
            m_step = &TskSnd::stepSEFadeOut;
        break;
    case 7:
        strncpy(m_name, param->m_name, 0x1b);
        m_step = &TskSnd::stepPlayStream;
        break;
    default:
        m_step = &TskSnd::stepEnd;       /* PC: the ROM leaves the step unset for kinds above 7 */
        break;
    }
    m_state = 100;
}

// ──────────────────────────────────────────────────────────────────────
// The exec step (stepFade).
// ROR 0x001f91dc

int TskSnd::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    return (this->*m_exec)();
}

// ──────────────────────────────────────────────────────────────────────
// A master volume fade ends on its target (both words the left volume,
// as the ROM passes them).
// ROR 0x001f9244

void TskSnd::term()
{
    if (m_kind == 2)
        g_gamMan.m_sndMan.funcSnd(0xd, m_param.m_volL, m_param.m_volL, 0, -1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f9294

float TskSnd::getRate()
{
    float rate = 0.0f;
    if (m_frames > 0)
        rate = (float)m_frame / (float)m_frames;
    return rate;
}

// ──────────────────────────────────────────────────────────────────────
// In state 100: the kind's step with the volume m_rate of the way from
// the start to the target (the rate taken before the frame count goes
// up). State 101 ends the task, others wait.
// ROR 0x001f92c8

int TskSnd::stepFade()
{
    if (m_state == 100)
    {
        float rate = getRate();
        m_frame++;
        return (this->*m_step)(
            (int)((float)m_param.m_volL * rate + (float)m_param.m_curL * (1.0f - rate)),
            (int)((float)m_param.m_volR * rate + (float)m_param.m_curR * (1.0f - rate)));
    }
    return m_state == 101;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0: the SE track's volume; at 0 / 0 the SE is stopped (funcSE 1)
// first. Ends when the SE is gone or the frames have run.
// ROR 0x001f93d8

int TskSnd::stepSEVol(int volL, int volR)
{
    GamSndMan &snd = g_gamMan.m_sndMan;
    if (volL == 0 && volR == 0)
        snd.funcSE((u_int)m_param.m_snd, 1, NULL);
    if (snd.setVolSEtrack((u_int)m_param.m_snd, m_param.m_id, volL, volR) == 0)
        return 1;
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1: the stream track's volume; at 0 / 0 the stream is stopped
// (funcStr 1, or 0x17 with m_arg) and the task ends.
// ROR 0x001f9484

int TskSnd::stepStrVol(int volL, int volR)
{
    GamSndMan &snd = g_gamMan.m_sndMan;
    int str = m_param.m_snd;
    if (volL == 0 && volR == 0)
    {
        if (m_param.m_arg == 0)
            snd.funcStr(str, 1, NULL);
        else
            snd.funcStr(str, 0x17, NULL);
        return 1;
    }
    if (snd.setVolSTRtrack((u_int)str, m_param.m_id, volL, volR) == 0)
        return 1;
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 2: the master volume.
// ROR 0x001f9550

int TskSnd::stepMstVol(int volL, int volR)
{
    g_gamMan.m_sndMan.funcSnd(0xd, volL, volR, 0, -1);
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 3: the SE master volume.
// ROR 0x001f95a8

int TskSnd::stepMstVolSe(int volL, int volR)
{
    g_gamMan.m_sndMan.funcSnd(0xe, volL, volR, 0, -1);
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 4: the stream master volume.
// ROR 0x001f9600

int TskSnd::stepMstVolStr(int volL, int volR)
{
    g_gamMan.m_sndMan.funcSnd(0xf, volL, volR, 0, -1);
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f9658

int TskSnd::getVolume(int *volL, int *volR)
{
    *volL = m_param.m_volL;
    *volR = m_param.m_volR;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 5 (GamSndMan::playSE's delay): plays the SE now, at the block's
// volume, pitch and attribute; ends.
// ROR 0x001f9670

int TskSnd::stepPlaySE(int volL, int volR)
{
    (void)volL;
    (void)volR;
    g_gamMan.m_sndMan.playSE((u_int)m_param.m_snd, m_param.m_volL, m_param.m_volR,
                             m_param.m_pitch, m_param.m_attr, 0, 0);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 7: stops the old stream (SndMan id m_id) each frame until it has
// stopped, then plays m_name into id m_id2 (looping with m_arg); ends.
// ROR 0x001f96c0

int TskSnd::stepPlayStream(int volL, int volR)
{
    (void)volL;
    (void)volR;
    if (g_sndMan.getStreamState(m_param.m_id) == -1)
    {
        g_sndMan.playStream(m_name, m_param.m_mode, m_param.m_id2, m_param.m_arg);
        if (m_param.m_arg != 0)
            g_sndMan.setStreamLoop(m_param.m_id2, 1);
        return 1;
    }
    g_sndMan.stopStream(m_param.m_id);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 6 while the SE plays: after m_frames (counted twice a frame, here
// and in stepFade) the SE fades to 0 over 10 frames (funcSE 0x13); ends.
// ROR 0x001f974c

int TskSnd::stepSEFadeOut(int volL, int volR)
{
    (void)volL;
    (void)volR;
    int frame = m_frame++;
    int end = m_frames <= frame;
    if (end)
    {
        TskSndParam fade = {};   /* PC: zeroed (the ROM sets only these) */
        fade.m_volL = 0;
        fade.m_volR = 0;
        fade.m_frames = 10;
        g_gamMan.m_sndMan.funcSE((u_int)m_param.m_snd, 0x13, &fade);
    }
    return end;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 6 when the SE did not play.
// ROR 0x001f97c0

int TskSnd::stepEnd(int volL, int volR)
{
    (void)volL;
    (void)volR;
    return 1;
}

// ══════════════════════════════════════════════════════════════════════
//  TskCamera
// ══════════════════════════════════════════════════════════════════════

/* setupFace's offset of the eye from object 0, turned by its heading
 * (data 0x2fefc0). */
static const sceVu0FVECTOR s_faceEyeOfs = { 30.0f, 0.0f, 0.0f, 1.0f };

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fbc0c

TskCamera::TskCamera(intptr_t kind, intptr_t param)
{
    m_step = &TskCamera::stepNone;   /* PC: the ROM leaves it unset for kinds above 1 */
    m_kind = (int)kind;
    if (kind == 0)
        setupPad((const int *)param);
    else if (kind == 1)
        setupFace((const int *)param);
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0: the centre, limits and speed from the script's words; the
// angles and the drift start at 0.
// ROR 0x001fbc6c

void TskCamera::setupPad(const int *param)
{
    const float *f = (const float *)param;
    m_0c = 0;
    m_step = &TskCamera::stepNone;
    m_center[0] = f[1];
    m_center[1] = f[2];
    m_center[2] = f[3];
    m_center[3] = 0.0f;
    m_min[0] = f[4];
    m_min[1] = f[5];
    m_min[2] = f[6];
    m_min[3] = 0.0f;
    m_max[0] = f[7];
    m_max[1] = f[8];
    m_max[2] = f[9];
    m_max[3] = 0.0f;
    m_speed = f[10];
    for (int i = 0; i < 4; i++)
    {
        m_angle[i] = 0.0f;
        m_angleSmooth[i] = 0.0f;
        m_drift[i] = 0.0f;
        m_driftSmooth[i] = 0.0f;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1: object a turns to face b (and b to face a with param[2]); the
// camera (-0.2 pitch, mode 1) goes 30 to object a's side and 70 behind
// on the line from b's head (160 up) -- 15 nearer b when the scene is
// hit on the way -- and looks at b's head.
// ROR 0x001fbd2c

void TskCamera::setupFace(const int *param)
{
    GamMan &gam = g_gamMan;
    u_int a = (u_int)param[0];
    int both = param[2];
    u_int b = (u_int)param[1];
    sceVu0FVECTOR dir, tmp, look, eye, posB, posA, rotA, rot, eye2;

    gam.getObjPos(a, eye);
    gam.getObjPos(b, posB);
    sceVu0SubVector(dir, posB, eye);
    gam.getObjRot(a, rot);
    rot[1] = atan2f(dir[0], dir[2]);
    gam.setObjRot(a, rot);
    if (both != 0)
    {
        gam.getObjRot(b, rot);
        rot[1] = atan2f(dir[0], dir[2]) + 0x1.921faep+1f; /* 0x40490fd7 */
        gam.setObjRot(b, rot);
    }

    memset(rot, 0, sizeof(rot));
    rot[0] = -0x1.999998p-3f; /* 0xbe4ccccc (-0.2) */
    gam.setCameraMode(1, 0);  // the ROM passes a third word 0

    gam.getObjPos(a, posA);
    gam.getObjRot(a, rotA);
    float ry = rotA[1];
    sceVu0FVECTOR ofs;
    for (int i = 0; i < 4; i++)
        ofs[i] = s_faceEyeOfs[i];
    eye[0] = cosf(ry) * ofs[0] + sinf(ry) * ofs[2] + posA[0];
    eye[1] = ofs[1] + posA[1];
    eye[2] = -sinf(ry) * ofs[0] + cosf(ry) * ofs[2] + posA[2];

    gam.getObjPos(b, posB);
    posB[1] -= 160.0f;
    eye[1] -= 160.0f;
    sceVu0SubVector(dir, eye, posB);
    sceVu0ScaleVector(tmp, dir, 0.0f);
    sceVu0SubVector(posB, posB, tmp);
    sceVu0Normalize(tmp, dir);
    sceVu0ScaleVector(tmp, tmp, -70.0f);
    sceVu0SubVector(eye2, eye, tmp);
    sceVu0SubVector(look, posB, eye2);

    MagSceneRayHit hit;
    for (int i = 0; i < 5; i++)
    {
        hit.m_vec[i][0] = 0.0f;
        hit.m_vec[i][1] = 0.0f;
        hit.m_vec[i][2] = 0.0f;
        hit.m_vec[i][3] = 1.0f;
    }
    hit.m_50 = -1;
    sceVu0FVECTOR from, along;
    for (int i = 0; i < 4; i++)
    {
        from[i] = eye2[i];
        along[i] = tmp[i];
    }
    float t = g_magSystem.m_scene.castRay(&hit, from, along, 0);
    if (0.0f <= t && t <= 1.0f)
    {
        sceVu0Normalize(tmp, look);
        sceVu0ScaleVector(tmp, tmp, 15.0f);
        sceVu0AddVector(eye2, eye2, tmp);
    }

    DogCamera &cam = g_magSystem.m_scene.m_camera_entity;
    cam.m_trans[0] = eye2[0];
    cam.m_trans[2] = eye2[2];
    cam.m_dirty = 1;
    cam.m_trans[1] = eye2[1];
    rot[1] = atan2f(look[0], look[2]);
    cam.m_rotate[1] = rot[1];
    cam.m_rotate[0] = rot[0];
    cam.m_rotate[2] = rot[2];
    cam.normalizeRotate();
    cam.m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc164

void TskCamera::init()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc16c

void TskCamera::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc174

int TskCamera::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    if (m_kind == 0)
        return execPad();
    if (m_kind == 1)
        return execFace();
    return (this->*m_step)();
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0, every frame: the left stick moves the angle (up / down on x,
// left / right on y) within the limits, a random drift of +-0.005 a
// frame decays by 0.9; both are smoothed and added to the centre as the
// scene camera's rotation. Never ends.
// ROR 0x001fc210

int TskCamera::execPad()
{
    BedPad &pad = g_magSystem.m_pad[0];
    float ly = pad.getLeftY();
    float lx = pad.getLeftX();

    float x = m_angle[0] - ly * m_speed;
    float cx = m_min[0];
    if (cx <= x)
    {
        cx = m_max[0];
        if (x <= m_max[0])
            cx = x;
    }
    float y = m_angle[1] + lx * m_speed;
    m_angle[0] = cx;
    float cy = m_min[1];
    if (cy <= y)
    {
        cy = m_max[1];
        if (y <= m_max[1])
            cy = y;
    }
    m_angle[1] = cy;

    m_drift[0] += ((float)RosePan_Rand() / ROSEPAN_RAND_MAXF - 0.5f) * 0x1.47ae14p-7f; /* 0x3c23d70a */  // callcheck: rand (RosePan_Rand keeps the ROM 31-bit range)
    m_drift[1] += ((float)RosePan_Rand() / ROSEPAN_RAND_MAXF - 0.5f) * 0x1.47ae14p-7f;

    sceVu0FVECTOR a, b;
    sceVu0ScaleVectorXYZ(m_drift, m_drift, 0.9f);                 /* 0x3f666666 */
    sceVu0ScaleVectorXYZ(a, m_driftSmooth, 0x1.b33332p-1f);       /* 0x3f599999 */
    sceVu0ScaleVectorXYZ(b, m_drift, 0x1.999998p-4f);             /* 0x3dcccccc */
    sceVu0AddVector(m_driftSmooth, a, b);
    sceVu0ScaleVectorXYZ(a, m_angleSmooth, 0.9f);
    sceVu0ScaleVectorXYZ(b, m_angle, 0x1.999998p-4f);
    sceVu0AddVector(m_angleSmooth, a, b);
    sceVu0AddVector(a, m_center, m_angleSmooth);
    sceVu0AddVector(a, a, m_driftSmooth);

    DogCamera &cam = g_magSystem.m_scene.m_camera_entity;
    cam.m_rotate[0] = a[0];
    cam.m_rotate[1] = a[1];
    cam.m_rotate[2] = a[2];
    cam.normalizeRotate();
    cam.m_dirty = 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1: the camera stays where setupFace put it. Never ends.
// ROR 0x001fc46c

int TskCamera::execFace()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc474

int TskCamera::stepNone()
{
    return 0;
}

// ══════════════════════════════════════════════════════════════════════
//  GamMan's load hooks
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// Empty in the ROM.
// ROR 0x001f8fb8

void gamSystemDataLoaded(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// The heap's free memory after a stage load, for the debug print
// ("free mem   max/total/size   = %d/%d"; rorDebugPrintf is empty).
// ROR 0x001fbbd0 // callcheck: rorDebugPrintf (the empty debug print, left out)

void gamStage2Loaded(void)
{
    BEDMEMINFO info;
    g_utlMemory.getInfo(&info);   // the ROM passes a third word 0
    (void)info;
}
