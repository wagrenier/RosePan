/* ==========================================================================
 *  dog/dogactor.h
 *
 *  DogActor (dog/dogactor.cpp; vtable _vt$8DogActor 0x2e0a80, base
 *  DogModel, vptr at +0x94, EE 0x4d0: the scene's actor stride, entry 18
 *  of the i3d size table): a model with motions -- up to four motion banks
 *  (m_motion_data), a base rotation, the Y offsets the motion root moves
 *  by. The scene holds 128 of them (MagScene::m_actors). Minimal: what
 *  MagSystem's actor methods touch.
 *
 *  EE virtual slots added or overridden (DogModel's are in dogmodel.h):
 *    3 ~DogActor 0x10f16c, 7 getNodeMatrixPtr 0x2812b4, 12 0x10e5c0,
 *    14 0x10f244, 15 0x10f1f0, 17 getNodeWorldMatrix 0x10eab8,
 *    18 0x10eb28, 19 0x10e3ac, 20 0x10e534, 21 0x10cffc
 *  PORT: not polymorphic on the host yet (dogentity.h).
 * ======================================================================== */
#ifndef _DOG_DOGACTOR_H
#define _DOG_DOGACTOR_H

#include <stddef.h>

#include "dog/dogmodel.h"
#include "dog/dogmotionblend.h"
#include "i3d/i3mroot.h"

struct I3rItem;

/* setNodeScale's list length. */
#define DOG_SCALE_NODE_MAX 5
/* The motion banks (m_pack_motion): 0 the model pack's own, 1..3 loaded
 * packs. */
#define DOG_MOTION_LEVEL_MAX 4

/* The ROM's angle bounds (0x40490fdc, a hair above pi, and twice it). */
#define DOG_PI   0x1.921fb8p+1f
#define DOG_PI2  0x1.921fb8p+2f

/* m_flag_motion_data: who owns the motion files. FLAG_MOTION_EACH is the
 * ROM's name (an assert); the others are named from their use. */
enum
{
    FLAG_MOTION_NONE = -1,      // no records
    FLAG_MOTION_PACK = 0,       // the records point into m_pack_motion's packs
    FLAG_MOTION_EACH = 1,       // each record's i3m is its own block (freed one by one)
};

/* A motion's track: the root move (x, y, z, the turn w) per frame, or tag
 * data. From the file: plain EE layout. The two header words are ints, or
 * floats in some files (getTrackNumFrame / getTrackType convert those). */
struct DogMotionTrack
{
    u_int  m_numframe;          // ROR 0x00: frames (bit 31 set on a tag track: loadMotion's assert)
    u_int  m_type;              // ROR 0x04: the layout of a frame (getTrackStride)
    u_char _08[0x10 - 0x08];
    float  m_data[1];           // ROR 0x10: m_numframe frames of getTrackStride floats
};
static_assert(offsetof(DogMotionTrack, m_data) == 0x10, "m_data");

/* One motion record (m_motion_data). EE 0x20. layout: host -- pointers
 * into the motion packs; EE offsets in the comments. */
struct DogMotionData
{
    I3mRoot        *m_i3m_root; // ROR 0x00: the motion, NULL = a free record
    int            *m_param;    // ROR 0x04: a nested pack's file 1 past its first word; *m_param is setMotion's default m_1d8
    DogMotionTrack *m_root;     // ROR 0x08: the root move track (getRootMove), NULL none
    DogMotionTrack *m_tag_data; // ROR 0x0c: the tag track after it, NULL none
    int             m_10;       // ROR 0x10: a nested pack's file 1 first word (setMotion tests its low byte)
    int             m_no;       // ROR 0x14: the motion id (setMotion's theID; the pack's index, or its user data & 0xfff)
    int             m_level;    // ROR 0x18: the bank it came from, -1 free
    const char     *m_name;     // ROR 0x1c: the pack's file name, NULL none
};

/* dog/dogmotionblend.h's lines 122 / 123. */
inline DogMotionData &DogMotionBlend::getData(int n)
{
    ROR_ASSERT(m_motion); /* 122 */
    ROR_ASSERT(n>=0 && n<m_max_motion); /* 122 */
    return m_motion[n];
}

/* EE 0x4d0. layout: host -- a DogModel (pointers); EE offsets in the
 * comments. */
struct DogActor : DogModel
{
    /* Releases the model (the master's reference when shared) with the
     * scene's textures. */
    void releaseModel(DogTexInfo *tex);                                  // ROR 0x00109ed0
    /* The shadow light becomes the previous one (none now, fade count -1),
     * flag 8 in m_mode, and motion 0 at 1/60 from frame 0;
     * MagScene::resetEntities after a load. */
    void resetShadow();                                                  // ROR 0x0010f298
    /* Stores the value at +0x1d8 (MagScene::loadActor sets 5); role unverified. */
    void setParam1d8(int v);                                             // ROR 0x0010bf00
    /* The records for a motion pack's files into bank theIndex (< 0: the
     * first free one from 1, or 0 while bank 0 is empty): the bank, 0 for
     * an empty pack, -1 no bank / a bad i3m, -3 no posture block, -100 on a
     * shared model. */
    int  loadMotion(const BedPfh &pfh, int theIndex);                    // ROR 0x0010c568
    /* Loads the model pack (file 0 the model pack, file 1 its motions)
     * with the scene's textures (sepia: loadPack's); mode 2 copies the
     * pack first, mode > 0 makes the actor own it. 0. */
    int  load(const BedPfh &pfh, DogTexInfo *tex, int mode, int sepia);  // ROR 0x0010cd5c
    /* EE slots 14 / 15 (+0x70 / +0x78): DogModel's, then the eye node. */
    int  loadPack(DogTexInfo *tex, const BedPfh &pfh, int sepia, int sepia_disp); // ROR 0x0010f244
    int  loadData(DogTexInfo *tex, void *data, void *i3c);               // ROR 0x0010f1f0
    /* The model node named "eye" (and its material), the eye words reset. */
    void setupEye();                                                     // ROR 0x0010f3a4
    /* The constructor: the entity / model words, both motion states, the
     * vectors at 0x3b0 / 0x3c0 / 0x3e0 (0, 0, 0, 1), then clear. */
    DogActor();                                                          // ROR 0x0010eebc
    /* DogModel's clear, then every actor word to its unloaded value. */
    void clear();                                                        // ROR 0x00109714
    /* Grows or shrinks the records to theN (new ones free); points both
     * motion states at them. */
    void setMaxMotion(int theN);                                         // ROR 0x00109860
    /* The records past the last used one go. */
    void trimMotion();                                                   // ROR 0x00108850
    void freeMotionData();                                               // ROR 0x00109b48
    /* The posture block for motion n's nodes (once): two 0x30-byte pose
     * records per node and the node map; motion n becomes current. 0, -1. */
    int  allocPosture(int n);                                            // ROR 0x00108674
    void freePosture();                                                  // ROR 0x00108814
    /* m_assign_tbl: the count, then the model node of each of motion n's
     * nodes (0xffff: none); returns how many have none. */
    int  setupAssignTbl(int n);                                          // ROR 0x0010bf08
    /* Every bank, the records, the posture, the model and the pack copy. */
    void releaseAll();                                                   // ROR 0x00109e28
    /* A shared model's original losing a reference (releaseAll and clear
     * on the last). */
    void releaseShared();                                                // ROR 0x00109fb4
    /* Releases bank m_sub_motion (when > 0). No callers seen yet. */
    void releaseSubMotion();                                             // ROR 0x00109dec
    /* The root track's frame count / frame layout / floats per frame. */
    int  getTrackNumFrame(const DogMotionTrack *trk);                    // ROR 0x001088fc
    int  getTrackType(const DogMotionTrack *trk);                        // ROR 0x0010893c
    int  getTrackStride(const DogMotionTrack *trk);                      // ROR 0x0010897c
    /* Frame n of trk as (x, y, z, turn); (0, 0, 0, 1) without a track. */
    void getTrackKey(const DogMotionTrack *trk, float *out, int n);      // ROR 0x00108a14
    /* The current motion's root move at frame t (interpolated; past
     * either end the track's whole move is added once): 0 no track, 1,
     * 2 when it wrapped. */
    int  getRootMove(float t, float *out);                               // ROR 0x00108c90
    /* The root track's turn at the current frame (-m_80 y while blending in). */
    float getRootRotY();                                                 // ROR 0x0010a584
    /* Bit 1 of m_mode: set by a motion start with m_1d8 > 0 (role unverified). */
    int  isBlendIn();                                                    // ROR 0x0010db38
    /* The index of the record with id theID, -1 none. */
    int  findMotionById(int theID);                                      // ROR 0x0010a874
    /* m_mode for a started motion (bit 4; bit 2 with m_1d8 > 0; bit 1
     * without, or with flag 0x40), m_f4 = 0. */
    void startMotionMode();                                              // ROR 0x0010ae58
    /* The pose records into the previous ones; the motion state too. */
    void copyPosture();                                                  // ROR 0x0010edbc
    void savePosture();                                                  // ROR 0x0010edec
    /* m_trans_ofs_y_save / m_rot_ofs_y from the root (changeMotion). */
    void saveRootOffset();                                               // ROR 0x0010e930
    void saveRotOfsY();                                                  // ROR 0x0010e7b4
    void clearNodeScale();                                               // ROR 0x0010ec50
    /* EE slot 21 (+0xa8): uses src's master model (reference counted); 0 on success. */
    int  shareModel(DogActor *src);                                      // ROR 0x0010cffc
    /* True when the actor's model is named theName. */
    int  checkName(const char *theName);                                 // ROR 0x0010f2dc
    int  getMotionMove(float t0, float t1, float *move, float *rot, float *pos); // ROR 0x00109348
    /* The event bits of motion (-1: the current one) between frames t0
     * and t1 (bit 0: a footstep). */
    u_int getMotionEvent(float t0, float t1, int motion);                // ROR 0x0010bcc8
    /* Runs the motion command list (asserts m_motion_cmd_idx < DOG_MAX_MOTION_CMD). */
    void runMotionCmd();                                                 // ROR 0x0010d388
    /* The motion time in 60 Hz frames. */
    float getMotionFrame();                                              // ROR 0x0010da4c
    /* m_motion.m_frame * 60 * 2 (TskMtnMove compares it with the motion's
     * length * 120). */
    float getMotionFrame120();                                           // ROR 0x0010da80
    /* The motion speed in frames per frame. */
    float getMotionStep();                                               // ROR 0x0010dad8
    void releaseMotion(int theId);                                       // ROR 0x00109b88
    /* Reads file theIndex of the pack file theFileName (its first 2048
     * bytes are the header) and loads its motions into bank theLevel (< 0:
     * the first free one; a used one is released first). The bank, -1 on
     * failure; on a shared model the original loads and the records are
     * copied. MagScene::setActorAnim passes its two ints swapped. */
    u_int loadPackMotion(const char *theFileName, int theIndex, int theLevel); // ROR 0x0010c17c
    void addTrans(const float *d, float trans_ofs_y, float rot_y_aar);   // ROR 0x0010a038
    /* EE slot 17 (+0x88): out = m_matrix x the model matrix of node n. */
    void getNodeWorldMatrix(sceVu0FMATRIX out, int n);                   // ROR 0x0010eab8
    /* EE slot 12 (+0x60): steps the motion, culls, transforms, drawModel. */
    void draw(I3dLayer *layer, const float *cam_rotate, const float *cam_trans); // ROR 0x0010e5c0
    /* EE slot 18 (+0x90): the world position of node n. */
    void getNodeWorldPos(float *out, int n);                             // ROR 0x0010eb28
    /* EE slot 19 (+0x98): the model into layer, with the node matrices when
     * nodes is set, from entity src's matrix (NULL: this). */
    void drawModel(I3dLayer *layer, int nodes, DogEntity *src);          // ROR 0x0010e3ac
    /* EE slot 20 (+0xa0): the shadow model (or the model) into a shadow layer. */
    void drawShadow(I3dLayer *layer);                                    // ROR 0x0010e534
    /* The index of the motion record named theName, -1 when none. */
    /* A 5-entry node / value list (a node scale, by chrRestoreLimb's 1.0;
     * unverified): node's entry takes scale, 1.0 frees it; otherwise the
     * first free entry, or one at 1.0, takes node and scale. */
    void setNodeScale(float scale, int node);                            // ROR 0x0010eba0
    int  findMotionByName(const char *theName);                          // ROR 0x0010aa9c
    /* The motion record whose name after its first '_' is theName (any
     * case), -1 when none; getMotionNoBySuffix gives its word +0x14. */
    int  findMotionBySuffix(const char *theName);                        // ROR 0x0010a8b8
    int  getMotionNoBySuffix(const char *theName);                       // ROR 0x0010aa5c
    /* m_mode 2: ~m_f4 as a float, else the motion frame. */
    float getMotionTime();                                               // ROR 0x0010d9ec
    /* The Y heading (-pi..pi), the motion's root turn included. */
    float getRotY();                                                     // ROR 0x0010a650
    /* Starts motion theID at speed (frames per frame); flags / flags2 go to
     * +0x3f0 / +0x3f4, p1d8 < 0 takes the motion's own +0x1d8 value.
     * Returns the motion index, -1 when it has none. */
    int  setMotion(float speed, int theID, u_int flags, int flags2, int p1d8); // ROR 0x0010af18
    /* Blends to motion theNo (flag 4 with the same motion: only the speed). */
    int  changeMotion(float speed, int theNo, u_int flags, int flags2, int arg); // ROR 0x0010b1b0
    /* Motion theID's i3m length (its root's float +0x18, in frames). */
    float getMotionLength(int theID);                                    // ROR 0x0010d0e8

    // ROR 0x00281244
    void setBaseRotate(const float *v)
    {
        m_base_rotate[0] = v[0];
        m_base_rotate[1] = v[1];
        m_base_rotate[2] = v[2];
        m_base_rotate[3] = v[3];
    }
    // ROR 0x0028126c
    void setBaseRotate(float x, float y, float z)
    {
        m_base_rotate[0] = x;
        m_base_rotate[3] = 1.0f;
        m_base_rotate[1] = y;
        m_base_rotate[2] = z;
    }
    void setId(int id) { m_id = id; }                                    // ROR 0x00280fd4
    int  getId() const { return m_id; }                                  // ROR 0x00280fdc
    int  getMode() const { return m_mode; }                              // ROR 0x00280fe4
    /* m_mode != 0 (the actor is in a mode; GamMan::pause tests the dog's). */
    bool isModeSet() const;                                              // ROR 0x0010a834
    /* The eye texture (the model node named "eye" and its material):
     * state picks the texture offset {0, 0.32, 0.65}. */
    void setEyeTex(int state);                                           // ROR 0x0010f5e4
    /* Eye flag ops (native 281 op 21; nothing without an eye node). Bit 0
     * of m_eye_flags runs the count m_eye_count up to m_eye_period, bit 1
     * goes with it; game meaning (blinking?) unverified. */
    void eyeHold();                                                      // ROR 0x0010f70c
    void eyeStart();                                                     // ROR 0x0010f73c
    void eyeStep();                                                      // ROR 0x0010f7a0
    void eyeReset();                                                     // ROR 0x0010f808
    // ROR 0x00280fec
    void ready()
    {
        m_f4 = -1;
        m_mode = 0;
    }
    /* m_base_rotate y = r wrapped once into -pi..pi (asserted). */
    void setBaseRotateY(float r);                                        // ROR 0x0010ed2c
    void clearRotYAAR() { m_rot_y_aar = 0.0f; }                          // ROR 0x0028128c
    void saveTransOfsY() { m_trans_ofs_y_save = m_trans_ofs[1]; }        // ROR 0x00280fb8
    float getTransOfsYSave() const { return m_trans_ofs_y_save; }        // ROR 0x00280fc4

    /* The record inlines (this header's lines 345, 368..370). */
    DogMotionTrack *getRootTrack(int n)
    {
        ROR_ASSERT(m_motion_data); /* 345 */
        /* PC: the ROM hangs in the assert; no records, no track. */
        if (m_motion_data == NULL)
            return NULL;
        return m_motion_data[n].m_root;
    }
    int relocateMotion(int n)
    {
        ROR_ASSERT(n>=0 & n<m_max_motion); /* 368 */
        ROR_ASSERT(m_motion_data); /* 368 */
        ROR_ASSERT(m_motion_data[n].m_i3m_root); /* 368 */
        return i3mRelocate(m_motion_data[n].m_i3m_root);
    }
    int getMotionNumNode(int n)
    {
        ROR_ASSERT(n>=0 & n<m_max_motion); /* 369 */
        ROR_ASSERT(m_motion_data); /* 369 */
        ROR_ASSERT(m_motion_data[n].m_i3m_root); /* 369 */
        return i3mGetNumNode(m_motion_data[n].m_i3m_root);
    }
    const char *getMotionNodeName(int n, int i)
    {
        ROR_ASSERT(n>=0 & n<m_max_motion); /* 370 */
        ROR_ASSERT(m_motion_data); /* 370 */
        ROR_ASSERT(m_motion_data[n].m_i3m_root); /* 370 */
        ROR_ASSERT(0<=i); /* 370 */
        return i3mGetNodeName(m_motion_data[n].m_i3m_root, (u_short)i);
    }
    void clearRotOfsY() { m_rot_ofs_y = 0.0f; }                          // ROR 0x00281294

    /* ROR 0x148..0x1a0: DogModel's (the shadow light words at 0x160..) */
    int           m_id;                 // ROR 0x1a0: setId / getId (the character id it was loaded as)
    void         *m_model_copy;         // ROR 0x1a4: the model pack when the actor owns it (load mode > 0; mode 2 copies it)
    int           m_flag_motion_data;   // ROR 0x1a8: FLAG_MOTION_*
    BedPfhHead   *m_pack_motion[DOG_MOTION_LEVEL_MAX]; // ROR 0x1ac: the motion pack of each bank (1..3 heap blocks)
    int           m_sub_motion;         // ROR 0x1bc: a bank releaseSubMotion frees (no writer seen yet)
    int           m_max_motion;         // ROR 0x1c0: records in m_motion_data
    DogMotionData *m_motion_data;       // ROR 0x1c4: the motion records
    int           m_num_posture_node;   // ROR 0x1c8: nodes in the posture block (the first motion's)
    u_char       *m_posture;            // ROR 0x1cc: one block: a 0x30-byte pose record per node,
    u_char       *m_posture_prev;       // ROR 0x1d0: the previous pose records (savePosture),
    u_short      *m_assign_tbl;         // ROR 0x1d4: then the node map (setupAssignTbl)
    int           m_1d8;                // ROR 0x1d8: setParam1d8; read as a frame count by TskMoveAdj (a blend-in length?)
    float         m_1dc;                // ROR 0x1dc: < 0 starts a motion from its end (0x10ae10)
    u_char        _1e0[0x1e4 - 0x1e0];
    DogMotionBlend m_motion;            // ROR 0x1e4: the current motion (its number, speed, frame)
    DogMotionBlend m_motion_prev;       // ROR 0x2c4: the one before (savePosture)
    u_char        _3a4[0x3b0 - 0x3a4];
    sceVu0FVECTOR m_3b0;                // ROR 0x3b0: the constructor's (0, 0, 0, 1)
    sceVu0FVECTOR m_base_rotate;        // ROR 0x3c0: getBaseRotateY reads y
    float         m_rot_y_aar;          // ROR 0x3d0: clearRotYAAR; addTrans's second float
    int           m_control_node;       // ROR 0x3d4: getControlNode
    int           m_control_node_flg;   // ROR 0x3d8: setControlNodeFlg / getControlNodeFlg
    u_char        _3dc[0x3e0 - 0x3dc];
    sceVu0FVECTOR m_3e0;                // ROR 0x3e0: the constructor's (0, 0, 0, 1)
    int           m_motion_flags;       // ROR 0x3f0: setMotion's flags (0x80: start from the motion's length; 0x40 added by the motion record)
    int           m_motion_flags2;      // ROR 0x3f4: setMotion's second word; role unknown
    int           m_scale_node[DOG_SCALE_NODE_MAX];  // ROR 0x3f8: setNodeScale's nodes, < 0 free
    float         m_scale[DOG_SCALE_NODE_MAX];       // ROR 0x40c: their values (1.0 frees an entry)
    float         m_trans_ofs_y_save;   // ROR 0x420: saveTransOfsY copies m_trans_ofs[1]
    float         m_rot_ofs_y;          // ROR 0x424: clearRotOfsY / getRotOfsYSave
    u_char        _428[0x42c - 0x428];
    int           m_42c[8];             // ROR 0x42c: clear -1
    int           m_44c;                // ROR 0x44c: clear -1
    u_char        _450[0x454 - 0x450];
    int           m_454;                // ROR 0x454: clear 0
    u_char        _458[0x45c - 0x458];
    int           m_45c;                // ROR 0x45c: getMotionStep: a count (> 0: the step only at its last entry)
    int           m_460;                // ROR 0x460: and the current entry
    u_char        _464[0x470 - 0x464];
    float         m_eye_uv[4];          // ROR 0x470: setupEye (0, 0, 1, 1); the loads clear 0x470..0x49f
    I3rItem      *m_eye_mat;            // ROR 0x480: the eye node's material (setEyeTex)
    I3rItem      *m_eye_node;           // ROR 0x484: the model node named "eye", NULL = none
    u_int         m_eye_flags;          // ROR 0x488: bits 0 and 1 (eyeHold .. eyeReset)
    int           m_eye_state;          // ROR 0x48c: setEyeTex's state
    int           m_eye_count;          // ROR 0x490
    int           m_eye_period;         // ROR 0x494
    u_char        _498[0x4a0 - 0x498];
    float         m_4a0[4];             // ROR 0x4a0: clear 0
    float         m_4b0[4];             // ROR 0x4b0: clear 1.0
    float         m_4c0;                // ROR 0x4c0: clear 0.1
    u_char        m_4c4;                // ROR 0x4c4: clear 1
    u_char        m_4c5;                // ROR 0x4c5: clear 1
    u_char        _4c6[0x4d0 - 0x4c6];
};
static_assert(offsetof(DogActor, m_shadow_light) >= 0x160, "m_shadow_light");
static_assert(offsetof(DogActor, m_shadow_fade) >= 0x168, "m_shadow_fade");
static_assert(offsetof(DogActor, m_id) >= 0x1a0, "m_id");
static_assert(offsetof(DogActor, m_model_copy) >= 0x1a4, "m_model_copy");
static_assert(offsetof(DogActor, m_flag_motion_data) >= 0x1a8, "m_flag_motion_data");
static_assert(offsetof(DogActor, m_pack_motion) >= 0x1ac, "m_pack_motion");
static_assert(offsetof(DogActor, m_sub_motion) >= 0x1bc, "m_sub_motion");
static_assert(offsetof(DogActor, m_max_motion) >= 0x1c0, "m_max_motion");
static_assert(offsetof(DogActor, m_motion_data) >= 0x1c4, "m_motion_data");
static_assert(offsetof(DogActor, m_num_posture_node) >= 0x1c8, "m_num_posture_node");
static_assert(offsetof(DogActor, m_posture) >= 0x1cc, "m_posture");
static_assert(offsetof(DogActor, m_posture_prev) >= 0x1d0, "m_posture_prev");
static_assert(offsetof(DogActor, m_assign_tbl) >= 0x1d4, "m_assign_tbl");
static_assert(offsetof(DogActor, m_1d8) >= 0x1d8, "m_1d8");
static_assert(offsetof(DogActor, m_1dc) >= 0x1dc, "m_1dc");
static_assert(offsetof(DogActor, m_motion) >= 0x1e4, "m_motion");
static_assert(offsetof(DogActor, m_motion_prev) >= 0x2c4, "m_motion_prev");
static_assert(offsetof(DogActor, m_3b0) >= 0x3b0, "m_3b0");
static_assert(offsetof(DogActor, m_3e0) >= 0x3e0, "m_3e0");
static_assert(offsetof(DogActor, m_motion_flags) >= 0x3f0, "m_motion_flags");
static_assert(offsetof(DogActor, m_trans_ofs_y_save) >= 0x420, "m_trans_ofs_y_save");
static_assert(offsetof(DogActor, m_42c) >= 0x42c, "m_42c");
static_assert(offsetof(DogActor, m_44c) >= 0x44c, "m_44c");
static_assert(offsetof(DogActor, m_454) >= 0x454, "m_454");
static_assert(offsetof(DogActor, m_45c) >= 0x45c, "m_45c");
static_assert(offsetof(DogActor, m_460) >= 0x460, "m_460");
static_assert(offsetof(DogActor, m_eye_uv) >= 0x470, "m_eye_uv");
static_assert(offsetof(DogActor, m_eye_mat) >= 0x480, "m_eye_mat");
static_assert(offsetof(DogActor, m_eye_node) >= 0x484, "m_eye_node");
static_assert(offsetof(DogActor, m_eye_flags) >= 0x488, "m_eye_flags");
static_assert(offsetof(DogActor, m_eye_period) >= 0x494, "m_eye_period");
static_assert(offsetof(DogActor, m_4a0) >= 0x4a0, "m_4a0");
static_assert(offsetof(DogActor, m_4b0) >= 0x4b0, "m_4b0");
static_assert(offsetof(DogActor, m_4c0) >= 0x4c0, "m_4c0");
static_assert(offsetof(DogActor, m_4c4) >= 0x4c4, "m_4c4");
static_assert(offsetof(DogActor, m_4c5) >= 0x4c5, "m_4c5");
static_assert(offsetof(DogActor, m_base_rotate) >= 0x3c0, "m_base_rotate");
static_assert(offsetof(DogActor, m_motion_flags2) >= 0x3f4, "m_motion_flags2");
static_assert(offsetof(DogActor, m_rot_y_aar) >= 0x3d0, "m_rot_y_aar");
static_assert(offsetof(DogActor, m_scale_node) >= 0x3f8, "m_scale_node");
static_assert(offsetof(DogActor, m_scale) >= 0x40c, "m_scale");
static_assert(offsetof(DogActor, m_rot_ofs_y) >= 0x424, "m_rot_ofs_y");
static_assert(sizeof(DogActor) >= 0x4d0, "DogActor is 0x4d0 bytes on the EE");

#endif /* _DOG_DOGACTOR_H */
