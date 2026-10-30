/* ==========================================================================
 *  eff/effgeneric.h
 *
 *  EffGeneric1 (kind 0x33), the effect built from a pack of its own: a
 *  model, an optional motion and an "eff" record file that animates the
 *  model's material uv and textures. EffBg1 (eff/effbg.h) derives from it
 *  and runs the same records over the map's model. Its code
 *  (0x173df8..0x174d14, between EffFoot2Prim and EffLight1) is not written
 *  yet: this header holds what EffBg1 needs -- the members, the record file
 *  and the record helpers -- and the overrides EffMan::createEff news.
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
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028c648 (linkonce)

    /* Type 4: the record's uv on its material. */
    void applyUvScroll(const EffGenScroll *rec);                            // ROR 0x00174bb0
    /* Type 5: the uv of the last key at or before m_frame on its material. */
    void applyUvKeys(const EffGenUvKeys *rec);                              // ROR 0x00174bd8
    /* The material of m_root named name gets (u, v) in its m_uv entry, on
     * every layer, and its change count bumped. */
    void setMatUv(float u, float v, const char *name);                      // ROR 0x00174c4c

    I3rRoot       *m_root;      // ROR 0x24: the model (EffGeneric1 owns a copy; EffBg1 borrows the map's)
    I3mRoot       *m_motion;    // ROR 0x28: the pack's first motion, NULL none
    void          *m_nodes;     // ROR 0x2c: the node transform block (one allocation)
    I3dMatrixF4x4 *m_local;     // ROR 0x30: in it, the local basic transforms
    I3dMatrixF4x4 *m_world;     // ROR 0x34: in it, one matrix per node after them
    u_short       *m_node_map;  // ROR 0x38: effMakeNodeMap of the motion (with one)
    float        (*m_uv)[4];    // ROR 0x3c: per root material: u, v offsets, 1, 1 (the material entity)
    EffGenFile    *m_file;      // ROR 0x40: the record file, a copy of the heap's
    u_short        m_frame;     // ROR 0x44: the loop's frame, 0..m_frames
    u_short        m_frames;    // ROR 0x46
    u_char         m_48[8];     // ROR 0x48: not decoded
    float          m_pos[4];    // ROR 0x50: param +0x1c, w 1
    float          m_rot[4];    // ROR 0x60: param +0x28, w 0
    float          m_scale[4];  // ROR 0x70: param +0x34, w 1
    float          m_color[4];  // ROR 0x80: param +0x40
};

#endif /* _EFF_EFFGENERIC_H */
