/* ==========================================================================
 *  eff/effbg.cpp
 *
 *  EffBg1 (0x16afcc..0x16b62c): EffGeneric1's record animation over the
 *  map's model. See eff/effbg.h.
 * ======================================================================== */
#include "eff/effbg.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "dog/dogmodel.h"
#include "eff/effman.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "i3d/i3rtype.h"
#include "mag/magscene.h"
#include "utl/utlmisc.h"

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
// ROR 0x0016afcc

EffBg1::EffBg1()
{
    // callcheck: EffGeneric1 (the base constructor, compiler-made)
}

// ──────────────────────────────────────────────────────────────────────
// The model is the map's: cleared so EffGeneric1's destructor leaves it.
// ROR 0x0016affc

EffBg1::~EffBg1()
{
    // callcheck: ~EffGeneric1, operator_delete (the base destructor and the deleting variant, compiler-made)
    m_root = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Takes the map's model (the scene's first sky slot), copies the pack's
// "eff" file (the one after its models, motions and textures; without one
// an empty file of flags 7), points each uv / texture record at the
// param's material, and gives every root material a uv entity (0, 0, 1,
// 1). 0 without a scene or a map model (createEff deletes the object).
// callcheck: BedPfh, dbgAssertFailed (the local's constructor; BedPfh::getFile's inline asserts, lines 85 / 86); strcasecmp (the host's effStrCaseCmp)
// ROR 0x0016b060

int EffBg1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    DogModel *model = &g_effScene->m_tenkyu[0].m_model[0];
    if (model == NULL)
        return 0;
    m_root = model->getI3rRootPtr();
    if (m_root == NULL)
        return 0;

    BedPfh pfh;
    pfh.clear();
    const EffPack *pack = param->m_pack;
    pfh = BedPfh(pack->m_data);
    int n = pack->m_num_i3r + pack->m_num_i3m + pack->m_num_tex;
    if (effStrCaseCmp(pfh.getFileName(n), "eff") == 0)
    {
        int size = pfh.getFileSize(n);
        m_file = (EffGenFile *)g_utlMemory.alloc(size, -1);
        memcpy(m_file, pfh.getFile(n), size);
    }
    else
    {
        m_file = (EffGenFile *)g_utlMemory.alloc(sizeof(EffGenFile), -1);
        memset(m_file, 0, sizeof(EffGenFile));
        m_file->m_flags = 7;
    }

    EffGenRec *rec = m_file->first();
    for (int i = 0; i < m_file->m_num; i++)
    {
        switch (rec->m_type)
        {
        case EFF_GEN_UV_SCROLL:
        case EFF_GEN_UV_KEYS:
        case EFF_GEN_TEX_KEYS:
            memcpy(rec->m_name, param->m_bg.m_mat, sizeof(rec->m_name));
            break;
        }
        rec = effGenNext(rec);
    }
    m_frame = 0;
    m_frames = m_file->m_frames;

    int num = i3rGetNumChild52(&m_root->m_item);
    if (num < 1)
    {
        m_uv = NULL;
        return 1;
    }
    m_uv = (float (*)[4])g_utlMemory.alloc(num * 0x10, -1);
    for (int i = 0; i < num; i++)
    {
        m_uv[i][0] = 0.0f;
        m_uv[i][1] = 0.0f;
        m_uv[i][2] = 1.0f;
        m_uv[i][3] = 1.0f;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The loop's next frame (0..m_frames), and each scroll record's uv moved
// by its speed / 60, kept to its fraction. Never over.
// ROR 0x0016b348

int EffBg1::liveMain()
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
// Each record onto the map's model before it is drawn.
// ROR 0x0016b444

void EffBg1::renderPreMain(RENDERPARAM *)
{
    EffGenRec *rec = m_file->first();
    for (int i = 0; i < m_file->m_num; i++)
    {
        switch (rec->m_type)
        {
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
}

// ──────────────────────────────────────────────────────────────────────
// The keys from the last back to the first key reached by m_frame; its
// texture copied (in the scene's table) over each layer texture of the
// material that differs from it.
// callcheck: strcasecmp (the host's effStrCaseCmp)
// ROR 0x0016b50c

void EffBg1::applyTexKeys(const EffGenTexKeys *rec)
{
    I3rItem *mat = effFindChild52(&m_root->m_item, rec->m_head.m_name, NULL);
    if (mat == NULL)
        return;
    for (int i = rec->m_num - 1; i >= 0; i--)
    {
        const EffGenTexKey *key = &rec->m_key[i];
        if (key->m_frame > m_frame)
            continue;
        int num = i3rGetNumChild25(mat);
        for (int j = 0; j < num; j++)
        {
            I3rLayerData *layer = (I3rLayerData *)i3rGetChild25(mat, j)->getData();
            const char *tex = (const char *)i3rPtr(&layer->m_tex_name);
            if (tex != NULL && effStrCaseCmp(tex, key->m_tex) != 0)
                g_effScene->copyTexture(tex, key->m_tex);
        }
        return;
    }
}
