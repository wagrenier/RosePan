/* ==========================================================================
 *  eff/effgeneric.h
 *
 *  EffGeneric1 (kind 0x33), the effect built from a pack of its own: a
 *  model, an optional motion and an "eff" record file that animates the
 *  model's placement, colour and material uv. EffBg1 (eff/effbg.h) derives
 *  from it and runs the uv records over the map's model. Code
 *  eff/effgeneric.cpp, 0x173df8..0x174dd8 (between EffFoot2Prim and
 *  EffLight1; no source string names the file, the class does).
 *
 *  Each frame liveMain steps the loop frame (0..m_frames: the motion's
 *  length, or the file's m_frames without one) and the uv scrolls;
 *  renderPreMain folds the keyed records into a translation, rotation,
 *  scale and colour, poses the model from its motion, and draws it at
 *  m_pos turned by m_rot (z, x, y) and scaled by m_scale, every node but
 *  the root showing a display control of m_color times the keyed colour.
 *  With the file's flag 4 the nodes face the camera (each node's matrix
 *  is the camera's, scaled, at the node's placed position) and the
 *  rotation records are ignored.
 * ======================================================================== */
#ifndef _EFF_EFFGENERIC_H
#define _EFF_EFFGENERIC_H

#include <stddef.h>

#include "eff/eff0.h"

struct I3mRoot;
struct I3rRoot;
struct I3dMatrixF4x4;

/* The record types of an "eff" file; the others are skipped. */
enum
{
    EFF_GEN_TRANS     = 0,      // EffGenValKeys: the translation added to m_pos (EffGeneric1 only)
    EFF_GEN_ROT       = 1,      // EffGenValKeys: the rotation added to m_rot, radians (not with flag 4)
    EFF_GEN_SCALE     = 2,      // EffGenValKeys: the scale m_scale is multiplied by
    EFF_GEN_COLOR     = 3,      // EffGenValKeys: the colour m_color is multiplied by (mask bit 8 = alpha)
    EFF_GEN_UV_SCROLL = 4,      // EffGenScroll: uv moving at a constant speed
    EFF_GEN_UV_KEYS   = 5,      // EffGenUvKeys: uv set by frame
    EFF_GEN_TEX_KEYS  = 6,      // EffGenTexKeys: the texture swapped by frame (EffBg1 only)
};

/* A record's head. Records follow each other m_size bytes apart. EE 0x24;
 * from the file: plain EE layout (as are the records and keys below). */
struct EffGenRec
{
    u_short m_type;             // ROR 0x00: EFF_GEN_*
    u_short m_size;             // ROR 0x02: bytes to the next record
    char    m_name[0x20];       // ROR 0x04: the material it drives (a substring of its name: effFindChild52)
};

/* Type 4. EE 0x34. */
struct EffGenScroll
{
    EffGenRec m_head;
    float     m_speed[2];       // ROR 0x24: u / v per second (liveMain adds speed / 60 a frame)
    float     m_uv[2];          // ROR 0x2c: the offsets now, kept in -1..1 (their fractions)
};

/* A key of a type-0..3 record. EE 8. */
struct EffGenValKey
{
    u_short m_frame;            // ROR 0x00: the key's frame of the loop
    u_short m_flags;            // ROR 0x02: bit 0 = reached linearly from the key before
    float   m_value;            // ROR 0x04
};

/* Types 0..3: one value keyed over the loop, written into the components
 * m_mask selects (bit 0 x / r, 1 y / g, 2 z / b, 3 alpha). No name: the
 * value goes into the whole model. EE 8 + m_num keys, ascending frames. */
struct EffGenValKeys
{
    u_short      m_type;        // ROR 0x00: EFF_GEN_TRANS .. EFF_GEN_COLOR
    u_short      m_size;        // ROR 0x02: bytes to the next record
    u_short      m_num;         // ROR 0x04
    u_short      m_mask;        // ROR 0x06
    EffGenValKey m_key[1];      // ROR 0x08
};

/* A key of a type-5 record. EE 0x10. */
struct EffGenUvKey
{
    u_short m_frame;            // ROR 0x00: applies from this frame of the loop
    u_short _02;
    float   m_uv[2];            // ROR 0x04
    float   _0c;
};

/* Type 5. EE 0x28 + m_num keys, ascending frames. */
struct EffGenUvKeys
{
    EffGenRec   m_head;
    u_short     m_num;          // ROR 0x24
    u_short     _26;
    EffGenUvKey m_key[1];       // ROR 0x28
};

/* A key of a type-6 record. EE 0x28. */
struct EffGenTexKey
{
    u_short m_frame;            // ROR 0x00: applies from this frame of the loop
    u_short _02;
    char    m_tex[0x24];        // ROR 0x04: the scene texture copied onto the material's layers
};

/* Type 6. EE 0x28 + m_num keys, ascending frames. */
struct EffGenTexKeys
{
    EffGenRec    m_head;
    u_short      m_num;         // ROR 0x24
    u_short      _26;
    EffGenTexKey m_key[1];      // ROR 0x28
};

/* The pack's file named "eff" (the one after its models, motions and
 * textures); without one init makes an empty 0x10-byte file of flags 7.
 * EE 0x10 + the records. */
struct EffGenFile
{
    u_short m_flags;            // ROR 0x00: bit 2 = EffGeneric1 keeps node transforms without a motion
    u_short m_num;              // ROR 0x02: records
    u_short m_frames;           // ROR 0x04: the loop's last frame (EffBg1; EffGeneric1 with no motion)
    u_short _06[5];

    EffGenRec *first() { return (EffGenRec *)(this + 1); }
};

static_assert(sizeof(EffGenRec) == 0x24, "EffGenRec");
static_assert(offsetof(EffGenScroll, m_speed) == 0x24, "m_speed");
static_assert(offsetof(EffGenScroll, m_uv) == 0x2c, "m_uv");
static_assert(sizeof(EffGenValKey) == 8, "EffGenValKey");
static_assert(offsetof(EffGenValKey, m_value) == 0x04, "EffGenValKey::m_value");
static_assert(offsetof(EffGenValKeys, m_num) == 0x04, "EffGenValKeys::m_num");
static_assert(offsetof(EffGenValKeys, m_mask) == 0x06, "EffGenValKeys::m_mask");
static_assert(offsetof(EffGenValKeys, m_key) == 0x08, "EffGenValKeys::m_key");
static_assert(sizeof(EffGenUvKey) == 0x10, "EffGenUvKey");
static_assert(offsetof(EffGenUvKey, m_uv) == 0x04, "EffGenUvKey::m_uv");
static_assert(offsetof(EffGenUvKeys, m_num) == 0x24, "EffGenUvKeys::m_num");
static_assert(offsetof(EffGenUvKeys, m_key) == 0x28, "EffGenUvKeys::m_key");
static_assert(sizeof(EffGenTexKey) == 0x28, "EffGenTexKey");
static_assert(offsetof(EffGenTexKey, m_tex) == 0x04, "EffGenTexKey::m_tex");
static_assert(offsetof(EffGenTexKeys, m_num) == 0x24, "EffGenTexKeys::m_num");
static_assert(offsetof(EffGenTexKeys, m_key) == 0x28, "EffGenTexKeys::m_key");
static_assert(offsetof(EffGenFile, m_num) == 0x02, "EffGenFile::m_num");
static_assert(offsetof(EffGenFile, m_frames) == 0x04, "EffGenFile::m_frames");
static_assert(sizeof(EffGenFile) == 0x10, "EffGenFile");

/* kind 0x33; EE 0x90, vtable 0x2e3f00. layout: host. */
class EffGeneric1 : public Eff0
{
public:
    EffGeneric1();                                                          // ROR 0x00173df8
    virtual ~EffGeneric1();                                                 // ROR 0x00173e48
    virtual int  init(const EffParam *param);                               // ROR 0x00173f3c
    virtual int  liveMain();                                                // ROR 0x00174438
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x00174534
    /* Drawn in renderPreMain through utlDrawI3r. */
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028c648 (linkonce)

    /* Types 0..3: the record's value at m_frame into the m_mask components
     * of out (translation, rotation, scale, colour). */
    void applyTransKeys(const EffGenValKeys *rec, float *out);              // ROR 0x001749fc
    /* Not with the file's flag 4 (the nodes face the camera). */
    void applyRotKeys(const EffGenValKeys *rec, float *out);                // ROR 0x00174a60
    void applyScaleKeys(const EffGenValKeys *rec, float *out);              // ROR 0x00174ad8
    void applyColorKeys(const EffGenValKeys *rec, float *out);              // ROR 0x00174b3c
    /* Type 6 does nothing on EffGeneric1's own model (EffBg1 swaps). */
    void applyTexKeys(const EffGenTexKeys *rec) {}                          // ROR 0x00174c44
    /* The value at m_frame: 0 before the first key, the last key at or
     * before it, moving linearly toward the next key when that key's flag
     * bit 0 is set. */
    float getKeyValue(const EffGenValKeys *rec);                            // ROR 0x00174d14

    /* Type 4: the record's uv on its material. */
    void applyUvScroll(const EffGenScroll *rec);                            // ROR 0x00174bb0
    /* Type 5: the uv of the last key at or before m_frame on its material. */
    void applyUvKeys(const EffGenUvKeys *rec);                              // ROR 0x00174bd8
    /* The material of m_root named name gets (u, v) in its m_uv entry, on
     * every layer, and its change count bumped. */
    void setMatUv(float u, float v, const char *name);                      // ROR 0x00174c4c

    I3rRoot       *m_root;      // ROR 0x24: the model (EffGeneric1 owns a copy; EffBg1 borrows the map's)
    I3mRoot       *m_motion;    // ROR 0x28: the pack's first motion, NULL none
    void          *m_nodes;     // ROR 0x2c: the node block (one allocation); with a motion its pose (0x30 a node)
    I3dMatrixF4x4 *m_local;     // ROR 0x30: in it, the local transforms (after the pose; at its start without)
    I3dMatrixF4x4 *m_world;     // ROR 0x34: in it, one model matrix per node after them
    u_short       *m_node_map;  // ROR 0x38: effMakeNodeMap of the motion (with one)
    float        (*m_uv)[4];    // ROR 0x3c: per root material: u, v offsets, 1, 1 (the material entity)
    EffGenFile    *m_file;      // ROR 0x40: the record file, a copy of the heap's
    u_short        m_frame;     // ROR 0x44: the loop's frame, 0..m_frames
    u_short        m_frames;    // ROR 0x46
    u_char         _48[8];      // ROR 0x48: not read
    float          m_pos[4];    // ROR 0x50: EffParam m_generic.m_pos, w 1
    float          m_rot[4];    // ROR 0x60: m_generic.m_rot, w 0
    float          m_scale[4];  // ROR 0x70: m_generic.m_scale, w 1
    float          m_color[4];  // ROR 0x80: m_generic.m_color
};

#endif /* _EFF_EFFGENERIC_H */
