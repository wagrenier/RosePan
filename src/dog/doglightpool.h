/* ==========================================================================
 *  dog/doglightpool.h
 *
 *  DogLightPool (dog/doglightpool.cpp, 0x1103dc..0x112b08): the scene's
 *  lights at MagScene +0x2050 -- per kind (DOG_LIGHT_AMBIENT, DIRECT,
 *  POINT, SPOT ...) a light array and its count, and eight light sets,
 *  each a bit mask per kind of the lights it uses. MagScene::draw turns
 *  set i into the i3d light of render stage i (createLight). EE size 0x10c
 *  (m_data at +0x108; the scene's next member is at +0x215c).
 * ======================================================================== */
#ifndef _DOG_DOGLIGHTPOOL_H
#define _DOG_DOGLIGHTPOOL_H

#include "common/ror_assert.h"
#include "i3d/i3denv.h"
#include "lib/unassigned.h"

/* The light kinds (the assert texts of dog/doglightpool.cpp and
 * dog/doglight.cpp name all six; kinds 4 and 5 are 0x30 bytes, the size of
 * the i3d dir2 / vec3 lights, and the pool's setup never gives them any). */
enum
{
    DOG_LIGHT_AMBIENT  = 0,
    DOG_LIGHT_DIRECT   = 1,
    DOG_LIGHT_POINT    = 2,
    DOG_LIGHT_SPOT     = 3,
    DOG_LIGHT_HEMI     = 4,
    DOG_LIGHT_DIST     = 5,
    DOG_MAX_LIGHT_TYPE = 6,
};

/* The parameters setLightParam / getLightParam (and DogLight's setParam /
 * getParam) address (names inferred from the fields they pick; the pool's
 * 3 and 4 only assert). */
enum
{
    DOG_LIGHT_PARAM_POS    = 0, /* point, spot (DogLight: dist too) */
    DOG_LIGHT_PARAM_DIR    = 1, /* direct, spot (DogLight: hemi too) */
    DOG_LIGHT_PARAM_COLOR  = 2, /* every kind */
    DOG_LIGHT_PARAM_HEMI2  = 3, /* DogLight: a hemi light's third vector (+0x20) */
    DOG_LIGHT_PARAM_DIST2  = 4, /* DogLight: a dist light's third vector (+0x20) */
    DOG_LIGHT_PARAM_CONE   = 5, /* spot: two angles, degrees outside, radians inside */
    DOG_LIGHT_PARAM_ENABLE = 6, /* DogLight: a kind's bit in m_enable (theType is the kind, theNo unused) */
};

/* The number of light sets (the asserts' "theSetNo >= 0 && theSetNo < 8"). */
#define DOG_LIGHT_SET_MAX 8

/* At most 0x20 lights per kind (newLight; a set's mask is one word). */
#define DOG_LIGHT_ID_MAX 0x20

/* An ambient light (stride 0x10): its colour. */
struct DogAmbientLight
{
    sceVu0FVECTOR m_color;      // ROR 0x00
};
static_assert(sizeof(DogAmbientLight) == 0x10, "DogAmbientLight is 0x10 bytes on the EE");

/* A directional light (stride 0x20), copied into an I3dParallelLight. */
struct DogDirectLight
{
    sceVu0FVECTOR m_dir;        // ROR 0x00
    sceVu0FVECTOR m_color;      // ROR 0x10
};
static_assert(offsetof(DogDirectLight, m_color) == 0x10, "m_color");
static_assert(sizeof(DogDirectLight) == 0x20, "DogDirectLight is 0x20 bytes on the EE");

/* A point light (stride 0x20), copied into an I3dPointLight. */
struct DogPointLight
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    sceVu0FVECTOR m_color;      // ROR 0x10
};
static_assert(offsetof(DogPointLight, m_color) == 0x10, "m_color");
static_assert(sizeof(DogPointLight) == 0x20, "DogPointLight is 0x20 bytes on the EE");

/* A spot light (stride 0x40), I3dSpotLight's layout. */
struct DogSpotLight
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    sceVu0FVECTOR m_dir;        // ROR 0x10
    sceVu0FVECTOR m_color;      // ROR 0x20
    float         m_cone[4];    // ROR 0x30: [0], [1] the cone angles in radians; [2], [3] unverified
};
static_assert(offsetof(DogSpotLight, m_dir) == 0x10, "m_dir");
static_assert(offsetof(DogSpotLight, m_color) == 0x20, "m_color");
static_assert(offsetof(DogSpotLight, m_cone) == 0x30, "m_cone");
static_assert(sizeof(DogSpotLight) == 0x40, "DogSpotLight is 0x40 bytes on the EE");

/* r = g = b = the colour's luminance (0.299 r + 0.587 g + 0.114 b). */
void dogLightToGray(float *color);                                     // ROR 0x00110bb8

/* A light set copied out of a pool, with its own i3d light description
 * (DogLightPool::copyToBuffer). The arrays have the layout of the pool's
 * m_data block (8 ambient, 8 direct, 16 point, 8 spot = 0x580 bytes).
 * Nothing in the ROM calls copyToBuffer, so no instance is known and
 * nothing past the counts is.
 * layout: host -- I3dLightDesc holds pointers; EE offsets in the comments. */
struct DogLightBuffer
{
    /* Copies the lights of kind theType whose bit is set in mask from
     * theLight into this buffer, at most the kind's capacity. */
    void copyLights(int theType, const void *theLight, u_int mask);  // ROR 0x001103dc
    /* m_desc over the copied lights. */
    void setupDesc();                                                  // ROR 0x00110578
    /* An empty description, no lights. */
    void clear();                                                      // ROR 0x00110614

    DogAmbientLight  m_ambient[8];  // ROR 0x000
    I3dParallelLight m_direct[8];   // ROR 0x080
    I3dPointLight    m_point[16];   // ROR 0x180
    I3dSpotLight     m_spot[8];     // ROR 0x380
    I3dLightDesc     m_desc;        // ROR 0x580
    int              m_num[DOG_MAX_LIGHT_TYPE]; // ROR 0x5c0
};

/* layout: host -- pointers; EE offsets in the comments. */
struct DogLightPool
{
    /* PORT: the ROM's MagScene constructor calls init for this member. */
    DogLightPool() { init(); }

    /* Everything zero: no lights, no sets, no m_data. */
    void init();                                                       // ROR 0x00110a94
    /* Frees the six light arrays (asserts first: a path not meant to
     * run), then init. */
    void freeLights();                                                 // ROR 0x00110af8
    /* Frees m_data, then init (MagScene::loadLight, ~MagScene). PORT: no
     * host destructor calls it: g_magSystem is a static whose heap is gone
     * by the time static destructors run. */
    void term();                                                       // ROR 0x00110b78
    /* Grows kind theType by one zeroed light; its id (theType * 32 +
     * index), -1 when the kind is full or memory ran out. */
    int  newLight(int theType);                                        // ROR 0x00110c0c
    /* The size of the block create allocates (0x580). */
    int  getDataSize();                                                // ROR 0x00110d60
    /* The stage's lights in one block: 8 ambient, 8 direct, 16 point and
     * 8 spot lights (not cleared), no sets. Always 1. */
    int  create();                                                     // ROR 0x00110d68
    /* Separate arrays of num[type] zeroed lights per kind. */
    void alloc(const int *num);                                        // ROR 0x00110e64
    /* Empties light set theSet and frees the lights no set uses any
     * more; theSet < 0 starts over (init + create). */
    void resetSet(int theSet);                                         // ROR 0x00110f78
    /* resetSet(theSet), then gives the set num[type] free lights of each
     * kind. */
    void buildSet(int theSet, const int *num);                         // ROR 0x00111064
    /* A free light of kind theType, marked used; -1 when none. */
    int  allocId(int theType);                                         // ROR 0x00111120
    /* Light theNo of kind theType leaves every set. */
    void removeLight(int theType, int theNo);                          // ROR 0x001111e4
    /* Light theID of kind theType joins light set theSet. */
    void addToSet(int theSet, int theType, int theID);                 // ROR 0x001112a8
    /* Light theID of kind theType joins every set. */
    void addToAllSets(int theType, int theID);                         // ROR 0x0011139c
    /* The lights of the masks (one per kind) into buf, then its desc. */
    void copyToBuffer(DogLightBuffer *buf, const u_int *mask);         // ROR 0x00111460
    /* Light set theDst uses the lights of set theSrc. */
    void copySet(int theDst, int theSrc);                              // ROR 0x00111528

    /* Parameter param (DOG_LIGHT_PARAM_*) of light n of kind type: four
     * floats, or two angles in degrees for DOG_LIGHT_PARAM_CONE. */
    void setLightParam(int type, int n, int param, const float *v);    // ROR 0x00111584
    void getLightParam(int type, int n, int param, float *v) const;    // ROR 0x00111b5c
    /* The same for the n-th light of kind type in light set set. */
    void setSetLightParam(int type, int n, int param, const float *v, int set);  // ROR 0x00112134
    void getSetLightParam(int type, int n, int param, float *v, int set) const;  // ROR 0x001123b0

    /* The theNo-th light of a kind in light set theSetNo, NULL when the set
     * has fewer. */
    DogAmbientLight *getAmbientLight(int theSetNo, int theNo) const;   // ROR 0x00110660
    DogDirectLight  *getDirectLight(int theSetNo, int theNo) const;    // ROR 0x0011076c
    DogPointLight   *getPointLight(int theSetNo, int theNo) const;     // ROR 0x00110878
    DogSpotLight    *getSpotLight(int theSetNo, int theNo) const;      // ROR 0x00110988

    /* The i3d light of light set `set`: its first ambient light (scaled
     * by scale, alpha 1), up to 8 direct and 16 point lights; with gray
     * set (the scene's +0x22cc word) every colour turns to its luminance.
     * NULL on failure. */
    I3dLight *createLight(i3dContext *ctx, int set, int gray, float scale) const; // ROR 0x0011262c
    /* The point light of light set `set` nearest to pos, -1 when the set
     * has none. */
    int findNearestPointLight(int set, const float *pos);               // ROR 0x001129cc

    /* Light n of a kind (inline; doglightpool.h lines 129..132). */
    DogAmbientLight *getAmbientLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_AMBIENT]); /* 129 */
        ROR_ASSERT(m_light[DOG_LIGHT_AMBIENT]); /* 129 */
        return (DogAmbientLight *)m_light[DOG_LIGHT_AMBIENT] + n;
    }
    DogDirectLight *getDirectLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_DIRECT]); /* 130 */
        ROR_ASSERT(m_light[DOG_LIGHT_DIRECT]); /* 130 */
        return (DogDirectLight *)m_light[DOG_LIGHT_DIRECT] + n;
    }
    DogPointLight *getPointLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_POINT]); /* 131 */
        ROR_ASSERT(m_light[DOG_LIGHT_POINT]); /* 131 */
        return (DogPointLight *)m_light[DOG_LIGHT_POINT] + n;
    }
    DogSpotLight *getSpotLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_SPOT]); /* 132 */
        ROR_ASSERT(m_light[DOG_LIGHT_SPOT]); /* 132 */
        return (DogSpotLight *)m_light[DOG_LIGHT_SPOT] + n;
    }

    /* Their const twins (lines 140..143: createLight, getLightParam). */
    const DogAmbientLight *getAmbientLight(int n) const
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_AMBIENT]); /* 140 */
        ROR_ASSERT(m_light[DOG_LIGHT_AMBIENT]); /* 140 */
        return (const DogAmbientLight *)m_light[DOG_LIGHT_AMBIENT] + n;
    }
    const DogDirectLight *getDirectLight(int n) const
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_DIRECT]); /* 141 */
        ROR_ASSERT(m_light[DOG_LIGHT_DIRECT]); /* 141 */
        return (const DogDirectLight *)m_light[DOG_LIGHT_DIRECT] + n;
    }
    const DogPointLight *getPointLight(int n) const
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_POINT]); /* 142 */
        ROR_ASSERT(m_light[DOG_LIGHT_POINT]); /* 142 */
        return (const DogPointLight *)m_light[DOG_LIGHT_POINT] + n;
    }
    const DogSpotLight *getSpotLight(int n) const
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_SPOT]); /* 143 */
        ROR_ASSERT(m_light[DOG_LIGHT_SPOT]); /* 143 */
        return (const DogSpotLight *)m_light[DOG_LIGHT_SPOT] + n;
    }

    void   *m_light[DOG_MAX_LIGHT_TYPE]; // ROR 0x00: per kind, its lights (AMBIENT 0x10 apart, DIRECT / POINT 0x20, SPOT 0x40, 4 / 5 0x30)
    int     m_num[DOG_MAX_LIGHT_TYPE];   // ROR 0x18: per kind, the count
    u_int   m_used[DOG_MAX_LIGHT_TYPE];  // ROR 0x30: per kind, bit i = light i is in some set (allocId's free list)
    u_int   m_set_mask[DOG_LIGHT_SET_MAX][DOG_MAX_LIGHT_TYPE]; // ROR 0x48: per light set and kind, bit i = light i in the set
    void   *m_data;                      // ROR 0x108: create's block, m_light[0..3] point into it
};

#endif /* _DOG_DOGLIGHTPOOL_H */
