/* ==========================================================================
 *  mag/magscenecmd.cpp
 *
 *  The scene's command glue (0x143d3c..0x145020, between RoR's
 *  "mag/magsceneif.cpp" and the enemy classes; no source string names the
 *  file): MagScene's fog fade -- setFog keeps a from / to pair and
 *  updateFog blends the drawn fog between them once a frame -- the debug
 *  window's constructor, and Brown's glue: free functions over the scene's
 *  MagLifeBwnCtrl for GamMan, the enemies, the effects and the scripts,
 *  whose native 039 lands in magBwnSeqCommand.
 * ======================================================================== */
#include "mag/magscenecmd.h"

#include "dog/dogactor.h"
#include "dog/dogtexinfo.h"
#include "i3d/i3denv.h"
#include "lib/unassigned.h"
#include "mag/maglifebwnctrl.h"
#include "mag/maglifewindow.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sequence/seqcall.h"
#include "utl/utlgame.h"

/* The scene's Brown controller (bss 0x727aa0). */
#define BWN_CTRL (g_magSystem.m_scene.m_bwn_ctrl)

/* Builds four-char script codes as the ROM compares them ('ifsc'). */
#define CMD4(a, b, c, d) \
    (((u_int)(a) << 24) | ((u_int)(b) << 16) | ((u_int)(c) << 8) | (u_int)(d))

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143d3c

void MagScene::setFog(const float *color, float p0, float p1, float p2, float p3, int frames)
{
    I3dFog fog;
    fog.m_color[0] = color[0];
    fog.m_color[1] = color[1];
    fog.m_color[2] = color[2];
    fog.m_color[3] = color[3];
    fog.m_param[0] = p0;
    fog.m_param[1] = p1;
    fog.m_param[2] = p2;
    fog.m_param[3] = p3;
    setFog(&fog, frames);
}

// ──────────────────────────────────────────────────────────────────────
// frames > 0 fades from the drawn fog to fog over that many frames
// (updateFog); otherwise fog is drawn at once.
// ROR 0x00143d7c

void MagScene::setFog(const I3dFog *fog, int frames)
{
    if (frames > 0)
    {
        m_fog[2] = m_fog[0];
        m_fog[3] = *fog;
        m_fog_frames = frames;
        m_fog_frame = 0;
        return;
    }
    m_fog[3] = *fog;
    m_fog[0] = m_fog[3];
    m_fog_frame = 0;
    m_fog_frames = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143eec

void MagScene::addFog(const I3dFog *delta, int frames)
{
    I3dFog fog = m_fog[3];
    for (int i = 0; i < 4; i++)
        fog.m_color[i] += delta->m_color[i];
    for (int i = 0; i < 4; i++)
        fog.m_param[i] += delta->m_param[i];
    setFog(&fog, frames);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143fd0

void MagScene::updateFog()
{
    int frames = m_fog_frames;
    int frame  = m_fog_frame + 1;
    if (m_fog_frame >= frames)
        return;
    m_fog_frame = frame;
    if (frame >= frames)
    {
        m_fog[0] = m_fog[3];
        return;
    }
    float t = (float)frame / (float)frames;
    const I3dFog &from = m_fog[2];
    const I3dFog &to   = m_fog[3];
    for (int i = 0; i < 4; i++)
        m_fog[0].m_color[i] = from.m_color[i] + (to.m_color[i] - from.m_color[i]) * t;
    for (int i = 0; i < 4; i++)
        m_fog[0].m_param[i] = from.m_param[i] + (to.m_param[i] - from.m_param[i]) * t;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144214

MagLifeWindowDebug::MagLifeWindowDebug()
    : MagLifeWindow()  // callcheck: MagLifeWindow (the base constructor)
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144244

int magBwnIsBarkPhase3(void)
{
    return BWN_CTRL.isBarkPhase3();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014426c

void magBwnHurt(int react, int damage)
{
    (void)damage;
    BWN_CTRL.requestHurt(react);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001442a4

int magBwnIsHurt(void)
{
    return BWN_CTRL.isHurt();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001442cc

void magBwnNop(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001442d4

int magBwnReturn0(void)
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001442dc

int magBwnBarkQuery(int v)
{
    return BWN_CTRL.barkQuery(v);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144308

void magBwnBarkSet(int v)
{
    BWN_CTRL.barkSet(v);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144334

void gamSetBwnFollow(int on)
{
    if (on != 0)
    {
        BWN_CTRL.setLevel(0);
        return;
    }
    BWN_CTRL.setLevel(2);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144388

int magBwnIsLevelAuto(void)
{
    return BWN_CTRL.m_ctrl_level == 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144398

void gamSetBwnPos(const float *pos)
{
    dbgTraceVector4(pos);
    BWN_CTRL.setPos(pos);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001443d8

void gamSetBwnRot(const float *rot)
{
    dbgTraceVector4(rot);
    BWN_CTRL.setRot(rot);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144418

void gamGetBwnPos(float *out)
{
    if (out == NULL)
        return;
    float v[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    BWN_CTRL.getPos(v);
    out[0] = v[0];
    out[2] = v[2];
    out[1] = v[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014448c

void gamGetBwnRot(float *out)
{
    if (out == NULL)
        return;
    float v[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    BWN_CTRL.getRot(v);
    out[0] = v[0];
    out[2] = v[2];
    out[1] = v[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144500

void gamSetBwnItem(int item)
{
    BWN_CTRL.setFindItem(item, 0, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144538

void magBwnReset(void)
{
    BWN_CTRL.reset();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014457c

void gamSetBwnWait(int on)
{
    if (on != 0)
    {
        BWN_CTRL.setLevel(1);
        return;
    }
    BWN_CTRL.setLevel(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001445c8

int magBwnReturn0b(void)
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001445d0

int magBwnReturn0c(void)
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Brown's state as the script sees it (op 3).
// ROR 0x001445d8

static intptr_t magBwnGetStateCode(void)
{
    switch (g_magSystem.getBwnState())
    {
    case 0:  return CMD4('N', 'U', 'L', 'L');
    case 1:  return CMD4('W', 'A', 'I', 'T');
    case 2:  return CMD4('F', 'O', 'L', 'W');
    case 3:  return CMD4('S', 'T', 'A', 'Y');
    case 4:  return CMD4('F', 'I', 'N', 'D');
    case 5:  return CMD4('F', 'D', 'E', 'D');
    case 6:  return CMD4('M', 'O', 'V', 'E');
    case 7:  return CMD4('M', 'V', 'E', 'D');
    case 8:  return CMD4('R', 'O', 'T', 'A');
    case 9:  return CMD4('R', 'T', 'E', 'D');
    case 10: return CMD4('D', 'A', 'M', 'G');
    case 11: return CMD4('B', 'A', 'R', 'K');
    case 12: return CMD4(0, 'H', 'U', 'G');
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op spmn: a special menu by name (data 0x2ef298: key, menu id), once the
// game pauses for it; unknown names open 0x1000b.
// ROR 0x001446b8

static const int s_special_menus[13][2] = {
    {(int)CMD4('K', 'L', 'C', 'K'), 0x1000b},
    {(int)CMD4('B', 'V', 'I', 'L'), 0x1000c},
    {(int)CMD4('B', 'L', 'N', 'D'), 0x1000d},
    {(int)CMD4('B', 'F', 'M', 'L'), 0x1000e},
    {(int)CMD4('B', 'C', 'P', 'L'), 0x1000f},
    {(int)CMD4('S', 'P', 'M', '0'), 0x1000b},
    {(int)CMD4('s', 'p', 'm', '0'), 0x1000b},
    {0,                             0x1000b},
    {0x1000b,                       0x1000b},
    {(int)CMD4('S', 'P', 'M', '1'), 0x1000c},
    {(int)CMD4('s', 'p', 'm', '1'), 0x1000c},
    {1,                             0x1000c},
    {0x1000c,                       0x1000c},
};

static intptr_t magBwnCmdSpecialMenu(const intptr_t *args)
{
    if (!utlPauseForMenu())
        return 0;
    int menu = 0x1000b;
    for (int i = 0; i < 13; i++)
    {
        if (s_special_menus[i][0] == (int)args[1])
        {
            menu = s_special_menus[i][1];
            break;
        }
    }
    g_magSystem.m_scene.requestMenu(menu, (int)args[2]);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op eqds: disp / DISP / 1 shows the PC's equip item; anything else hides
// it and clears the PC's suffix motion.
// ROR 0x00144820

static intptr_t magBwnCmdEquipDisp(const intptr_t *args)
{
    MagScene &scene = g_magSystem.m_scene;
    int arg = (int)args[1];
    if (arg == (int)CMD4('d', 'i', 's', 'p') || arg == (int)CMD4('D', 'I', 'S', 'P') || arg == 1)
    {
        int no = utlGetPcEquipItemChrID();
        if (no >= 0)
            scene.m_actors[no].setHide(0);
    }
    else
    {
        int no = utlGetPcEquipItemChrID();
        if (no >= 0)
            scene.m_actors[no].setHide(1);
        no = utlGetPcChrNo();
        if (no >= 0)
            scene.m_actors[no].setMotionSubBySuffix(0, NULL, NULL);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op whti: a white fade in over args[1] frames.
// ROR 0x00144924

static intptr_t magBwnCmdWhiteIn(const intptr_t *args)
{
    g_magSystem.m_scene.fadeIn((int)args[1], 7, 1);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op whto: the white fade out.
// ROR 0x00144960

static intptr_t magBwnCmdWhiteOut(const intptr_t *args)
{
    g_magSystem.m_scene.fadeOut((int)args[1], 7, 1);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op ilts: light set args[1] gets the three lights args[2..4].
// ROR 0x0014499c

static intptr_t magBwnCmdLightFlags(const intptr_t *args)
{
    int num[6] = {(int)args[2], (int)args[3], (int)args[4], 0, 0, 0};
    g_magSystem.m_scene.setLightFlags(num, (int)args[1]);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op adlt.
// ROR 0x001449fc

static intptr_t magBwnCmdAddLight(const intptr_t *args)
{
    g_magSystem.addLight((int)args[1], (unsigned int)args[2], (const float *)args[3],
                         (const float *)args[4]);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op gfrs.
// ROR 0x00144a34

static intptr_t magBwnGetFindResult(void)
{
    return BWN_CTRL.m_find_result;
}

// ──────────────────────────────────────────────────────────────────────
// Op ifsc; GamArea::update too.
// ROR 0x00144a40

int magBwnGetFoundOid(void)
{
    int oid = 0;
    if (BWN_CTRL.m_find_result != 0)
        oid = BWN_CTRL.m_found_oid;
    return oid;
}

// ──────────────────────────────────────────────────────────────────────
// Op gfit.
// ROR 0x00144a6c

static intptr_t magBwnGetFindItem(void)
{
    return BWN_CTRL.m_find_item;
}

// ──────────────────────────────────────────────────────────────────────
// Op clfd.
// ROR 0x00144a78

static intptr_t magBwnCmdCancelFind(void)
{
    BWN_CTRL.cancelFind();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Op rcst: the PC's command 0x14.
// ROR 0x00144aa8

static intptr_t magBwnCmdResetPC(void)
{
    g_magSystem.m_scene.commandPC(0x14, NULL);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00144ae0

intptr_t magBwnSeqCommand(const intptr_t *args)
{
    MagLifeBwnCtrl &ctrl = BWN_CTRL;
    u_int op = (u_int)args[0];
    switch (op)
    {
    case 3:
        return magBwnGetStateCode();
    case 0x14:
    {
        dbgTraceVector4((const float *)args[1]);
        const float *src = (const float *)args[1];
        float v[4] = {src[0], src[1], src[2], src[3]};
        ctrl.setRot(v);
        return 0;
    }
    case 0x15:
    {
        dbgTraceVector4((const float *)args[1]);
        const float *src = (const float *)args[1];
        float v[4] = {src[0], src[1], src[2], src[3]};
        ctrl.setPos(v);
        return 0;
    }
    case 0x1f:
        dbgTraceVector4((const float *)args[1]);
        ctrl.moveTo((const float *)args[1]);
        return 0;
    case 0x20:
    case 0x23:
        return ctrl.isStateDone() != 0 ? 0 : SEQ_RET_WAIT;
    case 0x21:
        ctrl.setAngle(*(const float *)args[1]);
        return 0;
    case 0x22:
        ctrl.rotateTo(*(const float *)args[1]);
        return 0;
    case 0x24:
        return 1;
    case CMD4('i', 'f', 's', 'c'):
        return magBwnGetFoundOid();
    case CMD4('c', 'l', 'f', 'd'):
        return magBwnCmdCancelFind();
    case CMD4('a', 'd', 'l', 't'):
        return magBwnCmdAddLight(args);
    case CMD4('a', 'm', 's', 'b'):
    {
        int no = utlGetActorNo((u_int)args[1]);
        if (no < 0)
            no = 0;
        g_magSystem.m_scene.m_actors[no].setMotionSubBySuffix((int)args[2], (const char *)args[3],
                                                              (const char **)args[4]);
        return 0;
    }
    case CMD4('g', 'f', 'i', 't'):
        return magBwnGetFindItem();
    case CMD4('g', 'f', 'r', 's'):
        return magBwnGetFindResult();
    case CMD4('g', 'm', 'n', 'r'):
        return g_magSystem.m_scene.m_menu_result;
    case CMD4('e', 'q', 'd', 's'):
        return magBwnCmdEquipDisp(args);
    case CMD4('s', 'e', 'p', 'o'):
    {
        int arg = (int)args[1];
        g_magSystem.m_scene.m_sepia =
            (arg == (int)CMD4(0, 0, 'O', 'N') || arg == (int)CMD4(0, 0, 'o', 'n') || arg == 1);
        return 0;
    }
    case CMD4('r', 'c', 's', 't'):
        return magBwnCmdResetPC();
    case CMD4('s', 'e', 'p', 'c'):
    {
        int r = (int)args[1], g = (int)args[2], b = (int)args[3];
        if (r > 0xff)
            r = 0xff;
        if (g > 0xff)
            g = 0xff;
        if (b > 0xff)
            b = 0xff;
        g_magSystem.m_scene.m_tex_info.setSepiaColor(r, g, b);
        return 0;
    }
    case CMD4('i', 'l', 't', 's'):
        return magBwnCmdLightFlags(args);
    case CMD4('s', 'p', 'm', 'n'):
        return magBwnCmdSpecialMenu(args);
    case CMD4('w', 'h', 't', 'i'):
        return magBwnCmdWhiteIn(args);
    case CMD4('w', 'h', 't', 'o'):
        return magBwnCmdWhiteOut(args);
    case CMD4('s', 't', 'f', 'd'):
    {
        /* Find the item args[1]; args[2] agin / AGIN / 1 searches again. */
        int arg   = (int)args[2];
        int again = (arg == (int)CMD4('A', 'G', 'I', 'N') || arg == (int)CMD4('a', 'g', 'i', 'n') ||
                     arg == 1);
        ctrl.setFindItem((int)args[1], 1, again);
        return 0;
    }
    }
    return 0;
}
