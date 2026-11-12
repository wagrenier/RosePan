/* ==========================================================================
 *  dog/doglight.h
 *
 *  DogLight (dog/doglight.cpp, 0x112b08..0x114034): one block of lights of
 *  the six kinds (ambient, direct, point, spot, hemi, dist) with an i3d
 *  light description in front, a per-kind enable mask and a dirty word.
 *  The class name is inferred from the file (no symbol, no vtable). Nothing
 *  in the ROM calls any of it: the whole file is unreferenced code
 *  (ghidra.py refs on every entry, 2026-10-30), so no instance and no EE
 *  size are known past +0x7c.
 * ======================================================================== */
#ifndef _DOG_DOGLIGHT_H
#define _DOG_DOGLIGHT_H

#include "common/ror_assert.h"
#include "dog/doglightpool.h"
#include "i3d/i3denv.h"

/* A hemi light (stride 0x30, I3dDir2Light's size): a direction and two
 * vectors, the first one the colour setParam writes for
 * DOG_LIGHT_PARAM_COLOR. Roles past m_dir unverified. */
struct DogHemiLight
{
    sceVu0FVECTOR m_dir;        // ROR 0x00
    sceVu0FVECTOR m_color;      // ROR 0x10
    sceVu0FVECTOR m_20;         // ROR 0x20: DOG_LIGHT_PARAM_HEMI2
};
static_assert(offsetof(DogHemiLight, m_color) == 0x10, "m_color");
static_assert(offsetof(DogHemiLight, m_20) == 0x20, "m_20");
static_assert(sizeof(DogHemiLight) == 0x30, "DogHemiLight is 0x30 bytes on the EE");

/* A dist light (stride 0x30, I3dVec3Light's size): a position and two
 * vectors. Roles past m_pos unverified. */
struct DogDistLight
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    sceVu0FVECTOR m_color;      // ROR 0x10
    sceVu0FVECTOR m_20;         // ROR 0x20: DOG_LIGHT_PARAM_DIST2
};
static_assert(offsetof(DogDistLight, m_color) == 0x10, "m_color");
static_assert(offsetof(DogDistLight, m_20) == 0x20, "m_20");
static_assert(sizeof(DogDistLight) == 0x30, "DogDistLight is 0x30 bytes on the EE");

/* layout: host -- I3dLightDesc and the arrays are pointers; EE offsets in
 * the comments. */
struct DogLight
{
    /* No lights, no block, every kind enabled, dirty. */
    void init();                                                       // ROR 0x00112b08
    /* term, then init. */
    void clear();                                                      // ROR 0x00112b58
    /* Frees the block, then init. */
    void term();                                                       // ROR 0x00112b84
    /* The bytes of a block for theNum[type] lights of each kind (does not
     * read this). */
    int  getDataSize(const int *theNum) const;                         // ROR 0x00112bc8
    /* term, then one zeroed block of theNum[type] lights per kind. */
    void create(const int *theNum);                                    // ROR 0x00112d60
    /* Parameter param (DOG_LIGHT_PARAM_*) of light theNo of kind theType:
     * four floats, two angles in degrees for DOG_LIGHT_PARAM_CONE, the
     * kind's enable bit (v[0] != 0) for DOG_LIGHT_PARAM_ENABLE. */
    void setParam(int theType, int theNo, int param, const float *v); // ROR 0x00112fc8
    void getParam(int theType, int theNo, int param, float *v) const; // ROR 0x001136d0
    /* dst gets this light's counts and a copy of its block. */
    void copyTo(DogLight *dst) const;                                  // ROR 0x00113ea8

    I3dLightDesc     m_desc;        // ROR 0x00 (EE 0x40)
    void            *m_data;        // ROR 0x40: create's block, the arrays point into it
    DogAmbientLight *m_ambient;     // ROR 0x44
    DogDirectLight  *m_direct;      // ROR 0x48
    DogPointLight   *m_point;       // ROR 0x4c
    DogSpotLight    *m_spot;        // ROR 0x50
    DogHemiLight    *m_hemi;        // ROR 0x54
    DogDistLight    *m_dist;        // ROR 0x58
    int              m_num[DOG_MAX_LIGHT_TYPE]; // ROR 0x5c: per kind, the count
    u_int            m_enable;      // ROR 0x74: bit type = kind type on (init: all)
    int              m_dirty;       // ROR 0x78: 1 after any change (nothing clears it)
};

#endif /* _DOG_DOGLIGHT_H */
