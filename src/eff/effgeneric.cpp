/* ==========================================================================
 *  eff/effgeneric.cpp
 *
 *  EffGeneric1 (0x173df8..0x174dd8): a model of its own pack placed,
 *  keyed and drawn by the pack's "eff" record file. See eff/effgeneric.h.
 * ======================================================================== */
#include "eff/effgeneric.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "dog/dogcamera.h"
#include "dog/dogmodel.h"
#include "dog/dogtexinfo.h"
#include "eff/effcommon.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3mpose.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "i3d/i3rtype.h"
#include "mag/magscene.h"
#include "utl/utlmath.h"
#include "utl/utlmisc.h"

/* The file's flag: the nodes face the camera (and keep the model's node
 * transforms without a motion). */
#define EFF_GEN_FACE_CAMERA 4

/* PC: the ROM's libc strcasecmp (0x279e98); MSVC has no such name. */
static int effStrCaseCmp(const char *a, const char *b)
{
    for (;; a++, b++)
    {
        int ca = tolower((u_char)*a);
        int cb = tolower((u_char)*b);
        if (ca != cb || ca == 0)
            return ca - cb;
    }
}

/* The record after rec. */
static EffGenRec *effGenNext(EffGenRec *rec)
{
    return (EffGenRec *)((u_char *)rec + rec->m_size);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00173df8

EffGeneric1::EffGeneric1()
{
    // callcheck: Eff0 (the base constructor, compiler-made)
    m_root = NULL;
    m_motion = NULL;
    m_nodes = NULL;
    m_local = NULL;
    m_world = NULL;
    m_node_map = NULL;
    m_uv = NULL;                // PC: the ROM leaves it (the destructor frees it after a failed init)
    m_file = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Frees the model copy, the node block, the node map, the uv entities and
// the record file (EffBg1 clears m_root first: the map's model is not its).
// ROR 0x00173e48

EffGeneric1::~EffGeneric1()
{
    // callcheck: ~Eff0, operator_delete (the base destructor and the deleting variant, compiler-made)
    if (m_root != NULL)
    {
        g_utlMemory.free(m_root);
        m_root = NULL;
    }
    if (m_nodes != NULL)
    {
        g_utlMemory.free(m_nodes);
        m_nodes = NULL;
        m_local = NULL;
        m_world = NULL;
    }
    if (m_node_map != NULL)
    {
        g_utlMemory.free(m_node_map);
        m_node_map = NULL;
    }
    if (m_uv != NULL)
    {
        g_utlMemory.free(m_uv);
        m_uv = NULL;
    }
    if (m_file != NULL)
    {
        g_utlMemory.free(m_file);
        m_file = NULL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Copies the pack's model 0 (unrelocated, copied, the copy relocated) and
// assigns the scene's textures; takes the pack's first motion; copies the
// "eff" file after the models, motions and textures (without one an empty
// file of flags 7). The loop runs over the motion's length in frames, or
// the file's m_frames. The node block: with a motion pose, local and
// model matrices; without one and with flag 4 the model's local and model
// matrices. Every root material gets a uv entity (0, 0, 1, 1); then the
// param's placement and colour.
// callcheck: BedPfh, dbgAssertFailed (the local's constructor; BedPfh::getFile's inline asserts, lines 85 / 86)
// callcheck: strcasecmp (the host's effStrCaseCmp)
// ROR 0x00173f3c

int EffGeneric1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;

    BedPfh pfh;
    pfh.clear();
    MagScene *scene = g_effScene;
    const EffPack *pack = param->m_pack;
    pfh = BedPfh(pack->m_data);

    int size = pfh.getFileSize(0);
    m_root = (I3rRoot *)g_utlMemory.alloc(size, -1);
    i3rUnrelocate((I3rRoot *)pfh.getFile(0));
    memcpy(m_root, pfh.getFile(0), size);
    i3rRelocate(m_root);
    if (pack->m_num_i3m > 0)
        m_motion = (I3mRoot *)pack->m_file[pack->m_num_i3r].m_ptr;
    i3rAssignTex(m_root, scene->m_tex_info.m_tex_info, scene->m_tex_info.m_num_tex);

    int n = pack->m_num_i3r + pack->m_num_i3m + pack->m_num_tex;
    if (effStrCaseCmp(pfh.getFileName(n), "eff") == 0)
    {
        size = pfh.getFileSize(n);
        m_file = (EffGenFile *)g_utlMemory.alloc(size, -1);
        memcpy(m_file, pfh.getFile(n), size);
    }
    else
    {
        m_file = (EffGenFile *)g_utlMemory.alloc(sizeof(EffGenFile), -1);
        memset(m_file, 0, sizeof(EffGenFile));
        m_file->m_flags = 7;
    }

    if (m_motion != NULL)
    {
        m_node_map = effMakeNodeMap(m_root, m_motion);
        m_frames = (u_short)(u_int)(i3mGetLength(m_motion) * 60.0f);
    }
    else
    {
        m_frames = m_file->m_frames;
    }
    m_frame = 0;

    if (m_motion != NULL)
    {
        int num = i3rGetNumNode(m_root);
        u_char *block = (u_char *)g_utlMemory.alloc(num * 0xb0, -1);
        m_nodes = block;
        m_local = (I3dMatrixF4x4 *)(block + num * 0x30);
        m_world = m_local + num;
        i3rCopyLocalBasicTransform(m_root, m_local);
    }
    else if (m_file->m_flags & EFF_GEN_FACE_CAMERA)
    {
        int num = i3rGetNumNode(m_root);
        u_char *block = (u_char *)g_utlMemory.alloc(num << 7, -1);
        m_nodes = block;
        m_local = (I3dMatrixF4x4 *)block;
        m_world = m_local + num;
        i3rCopyLocalBasicTransform(m_root, m_local);
    }

    int num = i3rGetNumChild52(&m_root->m_item);
    if (num < 1)
    {
        m_uv = NULL;
    }
    else
    {
        m_uv = (float (*)[4])g_utlMemory.alloc(num << 4, -1);
        for (int i = 0; i < num; i++)
        {
            m_uv[i][0] = 0.0f;
            m_uv[i][1] = 0.0f;
            m_uv[i][2] = 1.0f;
            m_uv[i][3] = 1.0f;
        }
    }

    m_pos[0] = param->m_generic.m_pos[0];
    m_pos[1] = param->m_generic.m_pos[1];
    m_pos[2] = param->m_generic.m_pos[2];
    m_pos[3] = 1.0f;
    m_rot[0] = param->m_generic.m_rot[0];
    m_rot[1] = param->m_generic.m_rot[1];
    m_rot[2] = param->m_generic.m_rot[2];
    m_rot[3] = 0.0f;
    m_scale[0] = param->m_generic.m_scale[0];
    m_scale[1] = param->m_generic.m_scale[1];
    m_scale[2] = param->m_generic.m_scale[2];
    m_scale[3] = 1.0f;
    for (int i = 0; i < 4; i++)
        m_color[i] = param->m_generic.m_color[i];
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The loop's next frame (0..m_frames), and each scroll record's uv moved
// by its speed / 60, kept to its fraction. Never over.
// ROR 0x00174438

int EffGeneric1::liveMain()
{
    u_short frame = m_frame + 1;
    if (frame > m_frames)
        frame = frame - m_frames - 1;
    m_frame = frame;

    EffGenRec *rec = m_file->first();
    for (int i = 0; i < m_file->m_num; i++)
    {
        if (rec->m_type == EFF_GEN_UV_SCROLL)
        {
            EffGenScroll *scroll = (EffGenScroll *)rec;
            float whole;
            scroll->m_uv[0] += scroll->m_speed[0] / 60.0f;
            scroll->m_uv[1] += scroll->m_speed[1] / 60.0f;
            scroll->m_uv[0] = modff(scroll->m_uv[0], &whole);
            scroll->m_uv[1] = modff(scroll->m_uv[1], &whole);
        }
        rec = effGenNext(rec);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The records fold into a translation (0), rotation (0), scale (1) and
// colour (1), the uv records go onto their materials; the motion poses
// the nodes at m_frame / 60 s (or, with flag 4, the model's own local
// transforms make the model matrices). The draw matrix: rotation m_rot +
// rot (z, x, y), scale m_scale * scale, at m_pos + trans. With flag 4
// each node's matrix becomes the camera's scaled, at the node's position
// placed by that matrix, which then becomes the unit matrix. Every node
// but the root shows a display control of colour m_color * colour (0x20
// the float 1000) while the model is drawn, its own afterwards.
// ROR 0x00174534

void EffGeneric1::renderPreMain(RENDERPARAM *param)
{
    float trans[4];
    float rot[4];
    float scale[4];
    float color[4];
    memset(trans, 0, sizeof(trans));
    trans[3] = 1.0f;
    memset(rot, 0, sizeof(rot));
    for (int i = 0; i < 4; i++)
    {
        scale[i] = 1.0f;
        color[i] = 1.0f;
    }

    EffGenRec *rec = m_file->first();
    for (int i = 0; i < m_file->m_num; i++)
    {
        switch (rec->m_type)
        {
        case EFF_GEN_TRANS:
            applyTransKeys((const EffGenValKeys *)rec, trans);
            break;
        case EFF_GEN_ROT:
            applyRotKeys((const EffGenValKeys *)rec, rot);
            break;
        case EFF_GEN_SCALE:
            applyScaleKeys((const EffGenValKeys *)rec, scale);
            break;
        case EFF_GEN_COLOR:
            applyColorKeys((const EffGenValKeys *)rec, color);
            break;
        case EFF_GEN_UV_SCROLL:
            applyUvScroll((const EffGenScroll *)rec);
            break;
        case EFF_GEN_UV_KEYS:
            applyUvKeys((const EffGenUvKeys *)rec);
            break;
        case EFF_GEN_TEX_KEYS:
            applyTexKeys((const EffGenTexKeys *)rec);
            break;
        }
        rec = effGenNext(rec);
    }

    if (m_motion != NULL)
    {
        i3mGetPose(m_motion, m_nodes, 0x111, (float)m_frame / 60.0f);
        i3mPoseToMatrices(m_nodes, (sceVu0FMATRIX *)m_local, m_node_map);
        i3rConvertLocalToModel(m_root, m_world, m_local);
    }
    else if (m_file->m_flags & EFF_GEN_FACE_CAMERA)
    {
        i3rConvertLocalToModel(m_root, m_world, m_local);
    }

    I3rItem *item = &m_root->m_item;
    int num = i3rGetNumChild52_3(item);
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dVecAdd(rot, m_rot, rot);
    i3dMatRotZXY(m, m, rot);
    i3dVecMul(scale, m_scale, scale);
    i3dMatScale(m, m, scale);
    i3dVecAdd(m[3], m_pos, trans);
    m[3][3] = 1.0f;

    if (m_file->m_flags & EFF_GEN_FACE_CAMERA)
    {
        DogCamera *camera = &g_effScene->m_camera_entity;
        camera->callTransform();
        for (int i = 0; i < num; i++)
        {
            sceVu0FVECTOR *world = (sceVu0FVECTOR *)m_world[i].m;
            i3dMatApply(trans, m, world[3]);
            memcpy(world, camera->m_matrix, sizeof(sceVu0FMATRIX));
            i3dMatScale(world, world, scale);
            world[3][0] = trans[0];
            world[3][1] = trans[1];
            world[3][2] = trans[2];
            world[3][3] = trans[3];
        }
        i3dMatUnit(m);
    }

    /* PC: zeroed whole (the ROM leaves 0x26..0x2f as the stack had them). */
    DogDispControl disp;
    memset(&disp, 0, sizeof(disp));
    for (int i = 0; i < 4; i++)
        disp.m_10[i] = m_color[i] * color[i];
    disp.m_20 = 0x447a0000;     /* the float 1000.0's bits */
    disp.m_25 = 1;
    disp.m_24 = 1;

    /* PORT: the ROM keeps the old pointers in a 16-word stack array (a
     * model of more nodes overruns it); the host keeps each field's raw
     * word and, for a far pointer, its target. layout: host. */
    struct Saved { u_int m_raw; void *m_far; };
    Saved saved_small[16];
    Saved *saved = num <= 16 ? saved_small : (Saved *)g_utlMemory.alloc(num * (int)sizeof(Saved), -1);
    for (int i = 1; i < num; i++)
    {
        I3rItem *slot = i3rGetSlot2A(i3rGetChild52_3(item, i));
        saved[i].m_raw = slot->m_data;
        saved[i].m_far = slot->m_data == I3R_FAR_PTR ? i3rFarPtrGet(&slot->m_data) : NULL;
        /* PORT: the ROM stores the pointer; the host field holds a far
         * pointer (the record is on the stack, not in the file). */
        i3rFarPtrSet(&slot->m_data, &disp);
    }
    utlDrawI3r(param->m_cur, m_root, (sceVu0FMATRIX *)m_world, m, NULL);
    for (int i = 1; i < num; i++)
    {
        I3rItem *slot = i3rGetSlot2A(i3rGetChild52_3(item, i));
        if (saved[i].m_raw == I3R_FAR_PTR)
            i3rFarPtrSet(&slot->m_data, saved[i].m_far);
        else
            slot->m_data = saved[i].m_raw;
    }
    if (saved != saved_small)
        g_utlMemory.free(saved);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001749fc

void EffGeneric1::applyTransKeys(const EffGenValKeys *rec, float *out)
{
    float v = getKeyValue(rec);
    if (rec->m_mask & 1)
        out[0] = v;
    if (rec->m_mask & 2)
        out[1] = v;
    if (rec->m_mask & 4)
        out[2] = v;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174a60

void EffGeneric1::applyRotKeys(const EffGenValKeys *rec, float *out)
{
    if (m_file->m_flags & EFF_GEN_FACE_CAMERA)
        return;
    float v = getKeyValue(rec);
    if (rec->m_mask & 1)
        out[0] = v;
    if (rec->m_mask & 2)
        out[1] = v;
    if (rec->m_mask & 4)
        out[2] = v;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174ad8

void EffGeneric1::applyScaleKeys(const EffGenValKeys *rec, float *out)
{
    float v = getKeyValue(rec);
    if (rec->m_mask & 1)
        out[0] = v;
    if (rec->m_mask & 2)
        out[1] = v;
    if (rec->m_mask & 4)
        out[2] = v;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174b3c

void EffGeneric1::applyColorKeys(const EffGenValKeys *rec, float *out)
{
    float v = getKeyValue(rec);
    if (rec->m_mask & 1)
        out[0] = v;
    if (rec->m_mask & 2)
        out[1] = v;
    if (rec->m_mask & 4)
        out[2] = v;
    if (rec->m_mask & 8)
        out[3] = v;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174bb0

void EffGeneric1::applyUvScroll(const EffGenScroll *rec)
{
    setMatUv(rec->m_uv[0], rec->m_uv[1], rec->m_head.m_name);
}

// ──────────────────────────────────────────────────────────────────────
// The keys from the last back to the first key reached by m_frame.
// ROR 0x00174bd8

void EffGeneric1::applyUvKeys(const EffGenUvKeys *rec)
{
    for (int i = rec->m_num - 1; i >= 0; i--)
    {
        const EffGenUvKey *key = &rec->m_key[i];
        if (key->m_frame <= m_frame)
        {
            setMatUv(key->m_uv[0], key->m_uv[1], rec->m_head.m_name);
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174c4c

void EffGeneric1::setMatUv(float u, float v, const char *name)
{
    int index;
    I3rItem *mat = effFindChild52(&m_root->m_item, name, &index);
    if (mat == NULL)
        return;
    float *uv = m_uv[index];
    uv[0] = u;
    uv[1] = v;
    int num = i3rGetNumChild25(mat);
    for (int i = 0; i < num; i++)
        i3rSetMatEntity0(i3rGetChild25(mat, i), uv);
    u_short *changes = (u_short *)((u_char *)mat->getData() + 8);
    (*changes)++;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174d14

float EffGeneric1::getKeyValue(const EffGenValKeys *rec)
{
    int i = 0;
    while (i < rec->m_num && rec->m_key[i].m_frame <= m_frame)
        i++;
    if (i < 1)
        return 0.0f;
    const EffGenValKey *prev = &rec->m_key[i - 1];
    if (i != rec->m_num && (rec->m_key[i].m_flags & 1))
    {
        const EffGenValKey *next = &rec->m_key[i];
        return prev->m_value + ((next->m_value - prev->m_value) * (float)(int)(m_frame - prev->m_frame))
                             / (float)(int)(next->m_frame - prev->m_frame);
    }
    return prev->m_value;
}
