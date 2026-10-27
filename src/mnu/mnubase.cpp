/* ==========================================================================
 *  mnu/mnubase.cpp
 *
 *  mnuBase and the mnu pool (0x192ac0..0x1947a4; RoR names no file here:
 *  mnu/ and the name from the class, whose methods every Mnu* book calls).
 *  See mnu/mnubase.h. The constructor and destructor are inline in the
 *  header.
 *
 *  PORT: the draw helpers write the ROM's VU1 sprite packets
 *  (i3dPktBeginVu1Sprite ..) and draw models through utlDrawI3r; the
 *  native i3d draw module has no VU1 path yet, so nothing they write
 *  shows on the host until it grows one.
 * ======================================================================== */
#include "mnu/mnubase.h"

#include <new>
#include <string.h>

#include "bed/bedpfh.h"
#include "dog/dogtexinfo.h"
#include "game/gamman.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3mroot.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "utl/utlmisc.h"

MnuPoolBlock *g_mnuPool;        // sdata 0x0029bfb4

// ──────────────────────────────────────────────────────────────────────
// The work memory of a model slot: the pose records, the local and model
// matrices and the motion node map in one pool block; then the textures
// and the local transforms. Inline in the ROM (updateLoad, copyModel).

static void mnuSetupModel(mnuBase *menu, MnuModel *m)
{
    int n = i3rGetNumNode(m->m_root);
    u_char *work = (u_char *)mnuAlloc(n * 0xb2 + 2);
    m->m_pose = work;
    m->m_local = (I3dMatrixF4x4 *)(work + n * 0x30);
    m->m_model = (I3dMatrixF4x4 *)(work + n * 0x30 + n * 0x40);
    m->m_map = (u_short *)(work + n * 0x30 + n * 0x80);
    i3rAssignTex(m->m_root, menu->m_tex->m_tex_info, menu->m_tex->m_num_tex);
    i3rCopyLocalBasicTransform(m->m_root, m->m_local);
}

// ──────────────────────────────────────────────────────────────────────
// mnuBase

// ROR 0x00192ba4

void mnuBase::setupWork(DogTexInfo *tex, int nFiles, int nModels, int nMotions,
                        int nTexs)
{
    int filesSize = nFiles * sizeof(MnuFile);
    int modelsSize = nModels * sizeof(MnuModel);
    m_tex = tex;
    mnuMergeFree();
    u_char *work = (u_char *)mnuAlloc(filesSize + modelsSize + nMotions * sizeof(MnuMotion) +
                                      nTexs * sizeof(MnuTex));
    m_maxFiles = nFiles;
    m_files = (MnuFile *)work;
    m_models = (MnuModel *)(work + filesSize);
    m_motions = (MnuMotion *)(work + filesSize + modelsSize);
    m_texs = (MnuTex *)((u_char *)m_motions + nMotions * sizeof(MnuMotion));
    for (int i = 0; i < m_maxFiles; i++) {
        m_files[i].m_kind = 0;
        m_files[i].m_name[0] = '\0';
        m_files[i].m_buf = NULL;
        m_files[i].m_size = 0;
        m_files[i].m_arg = -1;
    }
    m_numModels = nModels;
    memset(m_models, 0, modelsSize);
    m_numMotions = nMotions;
    memset(m_motions, 0, nMotions * sizeof(MnuMotion));
    m_numTexs = nTexs;
    for (int i = 0; i < m_numTexs; i++) {
        m_texs[i].m_tm2 = NULL;
        m_texs[i].m_size = 0;
        m_texs[i].m_tex = -1;
        m_texs[i].m_set = -1;
    }
    if (m_numModels == 0)
        m_models = NULL;
    if (m_numMotions == 0)
        m_motions = NULL;
    if (m_numTexs == 0)
        m_texs = NULL;
}

// ROR 0x00192d60

void mnuBase::term()
{
    if (m_models) {
        for (int i = 0; i < m_numModels; i++)
            freeModel(i);
        m_models = NULL;
        m_numModels = 0;
    }
    if (m_motions) {
        for (int i = 0; i < m_numMotions; i++)
            freeMotion(i);
        m_motions = NULL;
        m_numMotions = 0;
    }
    if (m_texs) {
        for (int i = 0; i < m_numTexs; i++)
            freeTex(i);
        m_texs = NULL;
        m_numTexs = 0;
    }
    if (m_files) {
        mnuFree(m_files);
        m_files = NULL;
    }
    m_maxFiles = 0;
    m_numFiles = 0;
    closePack();
    m_tex = NULL;
    m_flags = 0;
    m_10 = -1;
    m_step = -1;
}

// ROR 0x00192e94

void mnuBase::pushCamera(float viewFar, float angle)
{
    g_gamMan.pushCamera();
    DogCamera *cam = &m_scene->m_camera_entity;
    cam->m_dirty = 1;
    cam->m_trans[0] = 0.0f;
    cam->m_trans[1] = 0.0f;
    cam->m_trans[2] = 0.0f;
    cam->setRotate(0.0f, 0.0f, 0.0f);
    cam->m_angle = angle;
    m_saveFar = m_scene->m_view[0].m_far;
    m_scene->setViewFar(viewFar);
}

// ROR 0x00192f3c

void mnuBase::popCamera()
{
    m_scene->setViewFar(m_saveFar);
    g_gamMan.popCamera();
}

// ROR 0x00192f68

void mnuBase::beginDraw(I3dLayer *layer)
{
    DogCamera *cam = &m_scene->m_camera_entity;
    cam->transform();
    memcpy(m_view, cam->m_matrix, sizeof(m_view));
    const MagViewParam *view = &m_scene->m_view[0];
    float depth = view->m_far / view->m_near;
    m_viewW = (view->m_right - view->m_left) * depth;
    m_viewH = (view->m_top - view->m_bottom) * depth;

    sceVu0FMATRIX m;
    i3dMatUnit(m);
    m[3][0] = m_view[3][0];
    m[3][1] = m_view[3][1];
    m[3][2] = m_view[3][2] + view->m_far;
    m[3][3] = m_view[3][3];
    i3dMatMul(m_view, m, m_view);

    setDrawSet(layer);
    m_pkt = i3dLayerBeginSet(layer, 0.0f);
    m_pkt = i3dPktCallView(m_pkt, layer);
    m_pkt = i3dPktSetReg(m_pkt, 0x47 /* TEST_1 */, 0x5000d);
    m_pkt = i3dPktBeginVu1Sprite(m_pkt, layer, 1);
}

// ROR 0x001930f8

void mnuBase::endDraw(I3dLayer *layer)
{
    i3dLayerEndSet(layer, i3dPktEndVu1Sprite(m_pkt));
    m_pkt = NULL;
    m_drawSet = NULL;
}

// ROR 0x00193140

void mnuBase::setDrawSet(I3dLayer *layer)
{
    int texNo[64];
    int n = 0;
    for (int i = 0; i < m_numTexs; i++) {
        if (isTexLoaded(i)) {
            texNo[n] = m_texs[i].m_tex;
            m_texs[i].m_set = n;
            n++;
        }
    }
    I3dTexSetDesc desc;
    desc.m_depth = 0.0f;
    desc.m_tex_no = texNo;
    desc.m_tex_count = n;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_sort_param = 0;
    m_drawSet = i3dLayerFindDrawSet(layer, &desc);
    i3dLayerSetDrawSet(layer, m_drawSet);
}

// ROR 0x00193210

void mnuBase::useTex(int n)
{
    m_pkt = i3dPktRefTexRegs(m_pkt, i3dDrawSetUseTex(m_drawSet, m_texs[n].m_set));
}

// ROR 0x00193258

void mnuBase::drawSprite(float x, float y, const MnuSprite *spr)
{
    sceVu0FMATRIX m;
    sceVu0FVECTOR v;
    sceVu0FVECTOR scale = { 1.0f, 1.0f, 1.0f, 0.0f };   // rodata 0x2f7d10
    memset(v, 0, sizeof(v));
    i3dMatUnit(m);
    if (spr->m_angle == 0.0f) {
        m[3][0] = m_viewW * (((x - spr->m_x) + spr->m_w * 0.5f) - 256.0f) * (1.0f / 512.0f);
        m[3][1] = (m_viewH * (((y - spr->m_y) + spr->m_h * 0.5f) - 224.0f)) / 448.0f;
        i3dMatMul(m, m, m_view);
        v[2] = 1.0f;
        v[0] = m_viewW * spr->m_w * (1.0f / 512.0f);
        v[1] = (m_viewH * spr->m_h) / 448.0f;
        i3dMatScaleXYZ(m, m, v);
    } else {
        float axisW[2], axisH[2], pos[2];
        // the picture's two sides, turned, on the view plane
        v[0] = spr->m_w;
        v[1] = 0.0f;
        utlVecRotZ(spr->m_angle, v, v);
        axisW[0] = m_viewW * v[0] * (1.0f / 512.0f);
        axisW[1] = (m_viewH * v[1]) / 448.0f;
        v[0] = 0.0f;
        v[1] = spr->m_h;
        utlVecRotZ(spr->m_angle, v, v);
        axisH[0] = m_viewW * v[0] * (1.0f / 512.0f);
        axisH[1] = (m_viewH * v[1]) / 448.0f;
        // its centre, turned about the pivot
        v[0] = spr->m_w * 0.5f - spr->m_x;
        v[1] = spr->m_h * 0.5f - spr->m_y;
        utlVecRotZ(spr->m_angle, v, v);
        pos[0] = x + v[0];
        pos[1] = y + v[1];
        toView(pos, pos);
        i3dMatRotZ(spr->m_angle, m, m);
        m[3][0] = pos[0];
        m[3][1] = pos[1];
        i3dMatMul(m, m, m_view);
        v[0] = axisW[0];
        v[1] = axisW[1];
        scale[0] = utlVecLength3(v);
        v[0] = axisH[0];
        v[1] = axisH[1];
        scale[1] = utlVecLength3(v);
        i3dMatScaleXYZ(m, m, scale);
    }
    /* The ROM passes +0x10 (the rectangle) first and +0x20 (the colour)
     * last: see MnuSprite. */
    m_pkt = i3dPktVu1Color(m_pkt, spr->m_uv);
    m_pkt = i3dPktVu1Matrix(m_pkt, m);
    m_pkt = i3dPktVu1Sprite(m_pkt, m[3], spr->m_color);
}

// ROR 0x001935c4

void mnuBase::drawModelRot(I3dLayer *layer, const float *pos, const float *rot,
                           const float *scale, int n, int motion, int frame)
{
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotZXY(m, m, rot);
    i3dMatScale(m, m, scale);
    memcpy(m[3], pos, sizeof(m[3]));
    drawModel(layer, m, n, motion, frame);
}

// ROR 0x0019368c

void mnuBase::drawModel(I3dLayer *layer, sceVu0FVECTOR *world, int n, int motion, int frame)
{
    MnuModel *mdl = &m_models[n];
    if (motion == -1) {
        utlDrawI3r(layer, mdl->m_root, NULL, world, 0);
        return;
    }
    MnuMotion *mo = &m_motions[motion];
    float length = i3mGetLength(mo->m_root);
    i3mGetPose(mo->m_root, mdl->m_pose, 0x111, (length * (float)frame) / (float)mo->m_frames);
    i3mPoseToMatrices(mdl->m_pose, (sceVu0FMATRIX *)mdl->m_local, mdl->m_map);
    i3rConvertLocalToModel(mdl->m_root, mdl->m_model, mdl->m_local);
    utlDrawI3r(layer, mdl->m_root, (sceVu0FMATRIX *)mdl->m_model, world, 0);
}

// ROR 0x001937c0

void mnuBase::toView(float *out, const float *in)
{
    out[0] = m_viewW * (in[0] - 256.0f) * (1.0f / 512.0f);
    out[1] = (m_viewH * (in[1] - 224.0f)) / 448.0f;
}

// ROR 0x0019381c

void mnuBase::lowerVolume()
{
    if (g_gamMan.m_saved_volume == -1) {
        m_flags |= MNU_VOLUME_LOW;
        g_gamMan.switchVolume(0);
    }
}

// ROR 0x00193868

void mnuBase::restoreVolume()
{
    if (m_flags & MNU_VOLUME_LOW) {
        m_flags &= ~MNU_VOLUME_LOW;
        g_gamMan.switchVolume(1);
    }
}

// ROR 0x001938a8

void mnuBase::openPack()
{
    m_packBuf = mnuAlloc(0x1000);
    new (&m_req) FileReadReq();
    m_req.m_size = 0x1000;
    m_req.m_buf = m_packBuf;
    m_req.m_name = m_packName;
    m_req.m_whence = 0;
    m_req.m_offset = 0;
    m_req.m_close = 0;
    m_reader.read(&m_req, 0);
    m_req.m_buf = NULL;
}

// ROR 0x00193920

void mnuBase::closePack()
{
    if (m_packBuf) {
        m_req.m_whence = 0;
        m_req.m_offset = 0;
        m_req.m_size = 0;
        m_req.m_buf = NULL;
        m_req.m_close = 1;
        m_reader.read(&m_req, 0);
        mnuFree(m_packBuf);
        m_packBuf = NULL;
    }
}

// ROR 0x0019397c

void mnuBase::addFile(const char *name, u_int kind, int arg)
{
    if (m_numFiles == m_maxFiles)
        return;
    MnuFile *f = m_files;
    for (int i = 0; i < m_numFiles; i++, f++) {
        if ((f->m_kind & MNU_FILE_KIND_MASK) == (kind & MNU_FILE_KIND_MASK) && f->m_arg == arg)
            return;
    }
    strcpy(f->m_name, name);
    f->m_kind = kind;
    f->m_arg = arg;
    m_numFiles++;
}

// ──────────────────────────────────────────────────────────────────────
// The size and offset of the pack file name in the open pack's directory
// (-1 without the table). Inline in the ROM (updateLoad, readFile).
// PORT: a name the pack lacks indexes the ROM's tables at -1 (the word
// before them); the host answers -1 as for a missing table.

static void mnuFindPackFile(const void *packBuf, const char *name, int *size, int *offset)
{
    BedPfh pfh(packBuf);
    int n = pfh.getFileIndex(name);
    *size = (n < 0) ? -1 : pfh.getFileSize(n);
    *offset = (n < 0) ? -1 : pfh.getFileOffset(n);
}

// ROR 0x00193a1c  // callcheck: clear, getFileIndex, i3rAssignTex, i3rCopyLocalBasicTransform, i3rGetNumNode, BedPfh (in mnuFindPackFile / mnuSetupModel, the ROM's inline copies; the BedPfh constructor runs clear)

void mnuBase::updateLoad()
{
    for (;;) {
        MnuFile *f = m_files;
        if (m_flags & MNU_READING) {
            if (m_reader.update(1) != 0)
                return;
            if (m_flags & MNU_DROP_HEAD) {
                clearFile(0);
                m_flags &= ~MNU_DROP_HEAD;
            } else {
                switch (f->m_kind & MNU_FILE_KIND_MASK) {
                case MNU_FILE_MOTION: {
                    MnuMotion *mo = &m_motions[f->m_arg];
                    mo->m_root = (I3mRoot *)f->m_buf;
                    i3mRelocate(mo->m_root);
                    mo->m_frames = (int)(i3mGetLength(mo->m_root) * 30.0f);
                    break;
                }
                case MNU_FILE_MODEL: {
                    MnuModel *mdl = &m_models[f->m_arg];
                    mdl->m_root = (I3rRoot *)f->m_buf;
                    i3rRelocate(mdl->m_root);
                    mdl->m_size = f->m_size;
                    mnuSetupModel(this, mdl);
                    break;
                }
                case MNU_FILE_TEX: {
                    MnuTex *t = &m_texs[f->m_arg];
                    t->m_tex = m_tex->addData(f->m_name, f->m_buf, 0, -1);
                    t->m_tm2 = f->m_buf;
                    t->m_size = f->m_size;
                    break;
                }
                default:
                    // the buffer stays allocated (the ROM drops the pointer)
                    break;
                }
            }
            f->m_buf = NULL;
            clearFile(0);
            memmove(m_files, m_files + 1, (m_numFiles - 1) * sizeof(MnuFile));
            m_numFiles--;
            m_flags &= ~MNU_READING;
        }
        if (m_numFiles == 0)
            return;

        f = m_files;
        int offset;
        mnuFindPackFile(m_packBuf, f->m_name, &f->m_size, &offset);
        f->m_buf = mnuAlloc(f->m_size);
        m_req.m_whence = 0;
        m_req.m_offset = offset;
        m_req.m_size = f->m_size;
        m_req.m_buf = f->m_buf;
        if ((f->m_kind & MNU_FILE_WAIT) == 0) {
            m_reader.read(&m_req, 1);
            m_flags |= MNU_READING;
            return;
        }
        m_reader.read(&m_req, 0);
        m_flags |= MNU_READING;
    }
}

// ROR 0x00193d1c

void mnuBase::cancelFile(u_int kind, int arg)
{
    if (m_numFiles <= 0)
        return;
    if ((m_files[0].m_kind & MNU_FILE_KIND_MASK) == (kind & MNU_FILE_KIND_MASK) &&
        m_files[0].m_arg == arg) {
        m_flags |= MNU_DROP_HEAD;
        return;
    }
    for (int i = 1; i < m_numFiles; i++) {
        MnuFile *f = &m_files[i];
        if ((f->m_kind & MNU_FILE_KIND_MASK) == (kind & MNU_FILE_KIND_MASK) && f->m_arg == arg) {
            clearFile(i);
            memmove(f, f + 1, (m_numFiles - i - 1) * sizeof(MnuFile));
            m_numFiles--;
            return;
        }
    }
}

// ROR 0x00193dfc

void mnuBase::cancelLoad()
{
    if (m_numFiles > 0) {
        m_numFiles = 1;
        m_flags |= MNU_DROP_HEAD;
    }
}

// ROR 0x00193e20

void mnuBase::clearFile(int n)
{
    MnuFile *f = &m_files[n];
    if (f->m_buf) {
        mnuFree(f->m_buf);
        f->m_buf = NULL;
    }
    f->m_size = 0;
    f->m_name[0] = '\0';
    f->m_kind = 0;
    f->m_arg = -1;
}

// ROR 0x00193e7c  // callcheck: clear, getFileIndex, BedPfh (in mnuFindPackFile, the ROM's inline copy; the BedPfh constructor runs clear)

void *mnuBase::readFile(const char *name, int *size)
{
    if (m_packBuf == NULL || (m_flags & MNU_READING))
        return NULL;
    int fileSize, offset;
    mnuFindPackFile(m_packBuf, name, &fileSize, &offset);
    void *buf = mnuAlloc(fileSize);
    m_req.m_whence = 0;
    m_req.m_offset = offset;
    m_req.m_size = fileSize;
    m_req.m_buf = buf;
    m_reader.read(&m_req, 0);
    if (size)
        *size = fileSize;
    return buf;
}

// ROR 0x00193f74

void mnuBase::freeData(void *p)
{
    if (p)
        mnuFree(p);
}

// ROR 0x00193f98

void mnuBase::freeModel(int n)
{
    MnuModel *mdl = &m_models[n];
    if (mdl->m_root) {
        freeData(mdl->m_root);
        mnuFree(mdl->m_pose);
        memset(mdl, 0, sizeof(*mdl));
    }
}

// ROR 0x00193fec

void mnuBase::bindMotion(int n, int motion)
{
    MnuMotion *mo = &m_motions[motion];
    MnuModel *mdl = &m_models[n];
    int count = i3mGetNumNode(mo->m_root);
    mdl->m_map[0] = (u_short)count;
    for (int i = 0; i < count; i++) {
        int node = i3rGetNodeIndexByName(mdl->m_root, i3mGetNodeName(mo->m_root, i & 0xffff));
        mdl->m_map[1 + i] = (node < 0) ? 0xffff : (u_short)node;
    }
}

// ROR 0x001940a4

int mnuBase::findFreeModel(int lo, int hi)
{
    int end = hi + 1;
    if (m_numModels < end)
        end = m_numModels;
    for (; lo < end; lo++) {
        if (m_models[lo].m_root)
            continue;
        int i;
        for (i = 0; i < m_numFiles; i++) {
            if ((m_files[i].m_kind & MNU_FILE_KIND_MASK) == MNU_FILE_MODEL && m_files[i].m_arg == lo)
                break;
        }
        if (i == m_numFiles)
            return lo;
    }
    return -1;
}

// ROR 0x00194148  // callcheck: i3rAssignTex, i3rCopyLocalBasicTransform, i3rGetNumNode (in mnuSetupModel, the ROM's inline copy)

int mnuBase::copyModel(int dst, int src)
{
    if (m_models[src].m_root == NULL)
        return 0;
    if (findFreeModel(dst, dst) != dst)
        return 0;
    MnuModel *s = &m_models[src];
    MnuModel *d = &m_models[dst];
    i3rUnrelocate(s->m_root);
    d->m_root = (I3rRoot *)mnuAlloc(s->m_size);
    memcpy(d->m_root, s->m_root, s->m_size);
    i3rRelocate(d->m_root);
    d->m_size = s->m_size;
    mnuSetupModel(this, d);
    i3rRelocate(s->m_root);
    return 1;
}

// ROR 0x00194280

void mnuBase::freeMotion(int n)
{
    MnuMotion *mo = &m_motions[n];
    if (mo->m_root) {
        freeData(mo->m_root);
        memset(mo, 0, sizeof(*mo));
    }
}

// ROR 0x00194370

void mnuBase::freeTex(int n)
{
    if (isTexLoaded(n)) {
        MnuTex *t = &m_texs[n];
        m_tex->release(t->m_tex);
        freeData(t->m_tm2);
        t->m_tm2 = NULL;
        t->m_size = 0;
        t->m_tex = -1;
    }
}

// ROR 0x001943d8

int mnuBase::isTexLoaded(int n)
{
    MnuTex *t = &m_texs[n];
    return t->m_tm2 != NULL && t->m_tex != -1;
}

// ROR 0x00194404

int mnuBase::findFreeTex(int lo, int hi)
{
    int end = hi + 1;
    if (m_numTexs < end)
        end = m_numTexs;
    for (; lo < end; lo++) {
        if (isTexLoaded(lo))
            continue;
        int i;
        for (i = 0; i < m_numFiles; i++) {
            if ((m_files[i].m_kind & MNU_FILE_KIND_MASK) == MNU_FILE_TEX && m_files[i].m_arg == lo)
                break;
        }
        if (i == m_numFiles)
            return lo;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// The mnu pool

// ROR 0x001944d0

void mnuInitPool(void)
{
    int size = MNU_POOL_BLOCKS * sizeof(MnuPoolBlock) + MNU_POOL_SIZE;
    g_mnuPool = (MnuPoolBlock *)g_utlMemory.alloc(size, -1);
    memset(g_mnuPool, 0, size);
    g_mnuPool[0].m_addr = (u_char *)(g_mnuPool + MNU_POOL_BLOCKS);
    g_mnuPool[0].m_size = MNU_POOL_SIZE;
    g_mnuPool[0].m_flags = MNU_BLOCK_VALID;
}

// ROR 0x00194548

void mnuTermPool(void)
{
    if (g_mnuPool) {
        g_utlMemory.free(g_mnuPool);
        g_mnuPool = NULL;
    }
}

// ROR 0x00194588

void *mnuAlloc(u_int size)
{
    // PORT: the ROM reads a NULL pool's table at address 0; the host refuses.
    if (g_mnuPool == NULL)
        return NULL;
    u_int units = (size + 0xf) >> 4;
    u_int bytes = units * 16;
    MnuPoolBlock *best = NULL;
    u_int left = 0xffffffff;
    for (int i = 0; i < MNU_POOL_BLOCKS; i++) {
        MnuPoolBlock *b = &g_mnuPool[i];
        if ((b->m_flags & (MNU_BLOCK_VALID | MNU_BLOCK_USED)) == MNU_BLOCK_VALID &&
            bytes <= b->m_size && (best == NULL || b->m_size - bytes < left)) {
            left = b->m_size - bytes;
            best = b;
        }
    }
    if (best == NULL)
        return NULL;
    if (left == 0) {
        best->m_flags |= MNU_BLOCK_USED;
    } else {
        int i;
        for (i = 0; i < MNU_POOL_BLOCKS; i++) {
            if ((g_mnuPool[i].m_flags & MNU_BLOCK_VALID) == 0)
                break;
        }
        if (i == MNU_POOL_BLOCKS)
            return NULL;
        MnuPoolBlock *e = &g_mnuPool[i];
        e->m_flags = MNU_BLOCK_VALID | MNU_BLOCK_USED;
        e->m_addr = best->m_addr;
        e->m_size = bytes;
        best->m_addr += bytes;
        best->m_size -= bytes;
        best = e;
    }
    return best->m_addr;
}

// ROR 0x00194688

void mnuFree(void *p)
{
    // PORT: see mnuAlloc (the menus' destructors run after mnuTermPool at exit).
    if (g_mnuPool == NULL)
        return;
    for (int i = 0; i < MNU_POOL_BLOCKS; i++) {
        MnuPoolBlock *b = &g_mnuPool[i];
        if ((b->m_flags & (MNU_BLOCK_VALID | MNU_BLOCK_USED)) == (MNU_BLOCK_VALID | MNU_BLOCK_USED) &&
            b->m_addr == p) {
            b->m_flags &= ~MNU_BLOCK_USED;
            return;
        }
    }
}

// ROR 0x001946e4

void mnuMergeFree(void)
{
    for (int i = 0; i < MNU_POOL_BLOCKS - 1; i++) {
        MnuPoolBlock *a = &g_mnuPool[i];
        if ((a->m_flags & (MNU_BLOCK_VALID | MNU_BLOCK_USED)) != MNU_BLOCK_VALID)
            continue;
        for (int j = i + 1; j < MNU_POOL_BLOCKS; j++) {
            MnuPoolBlock *b = &g_mnuPool[j];
            if ((b->m_flags & (MNU_BLOCK_VALID | MNU_BLOCK_USED)) != MNU_BLOCK_VALID)
                continue;
            if (a->m_addr + a->m_size == b->m_addr) {
                a->m_size += b->m_size;
            } else if (b->m_addr + b->m_size == a->m_addr) {
                a->m_addr = b->m_addr;
                a->m_size += b->m_size;
            } else {
                continue;
            }
            b->m_flags = 0;
            j = i;      // start over after a
        }
    }
}
