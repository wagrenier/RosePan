/* ==========================================================================
 *  utl/utlmath.cpp
 *
 *  The map-area sound id swap, the i3r draw / node display wrappers and
 *  the vector helpers. See utlmath.h.
 *
 *  The node display word: on the EE utlSetI3rNodeDisp stores disp's
 *  address into the node slot's data word, which i3rDrawModel reads back
 *  as a pointer. Relocated i3r fields are self-relative on the host
 *  (i3rroot.h), so the word is written with i3rSetPtr and read with
 *  getData(); disp must lie within 2 GB of the model file (DogModel and
 *  DogActor records and the files all come from the game heap).
 * ======================================================================== */
#include "utl/utlmath.h"

#include <math.h>

#include "game/gamman.h"
#include "i3d/i3rtype.h"
#include "lib/unassigned.h"
#include "rx/rxthread.h"

/* ---- the ROM's angle constants --------------------------------------- */

static const float UTL_PI = 3.1415929794311523f;     /* 0x40490fdc */
static const float UTL_2PI = 6.2831859588623047f;    /* 0x40c90fdc */
static const float UTL_PI_2 = 1.5707962512969971f;   /* 0x3fc90fda */

/* ---- data ------------------------------------------------------------- */

/* One sound in its three map-area versions (column = GamMan::getMapArea);
 * 0xffffffff where an area has none. Ends with a row of 0xfffffffe. */
struct UtlSndIdRow
{
    u_int m_id[3];
};
static_assert(sizeof(UtlSndIdRow) == 0xc, "UtlSndIdRow is 0xc bytes on the EE");

static const UtlSndIdRow s_sndIdTable[54] = {           /* data 0x295f08 */
    { 0xa00008e, 0xa010192, 0xa020293 },  // 0x00295f08
    { 0xa00008f, 0xa010193, 0xa020294 },  // 0x00295f14
    { 0xa00009a, 0xa0101a0, 0xa020295 },  // 0x00295f20
    { 0xa00009b, 0xa0101a1, 0xa020296 },  // 0x00295f2c
    { 0xa000090, 0xa01019a, 0xa020299 },  // 0x00295f38
    { 0xa000091, 0xa01019b, 0xa02029a },  // 0x00295f44
    { 0xffffffff, 0xa010198, 0xa020297 },  // 0x00295f50
    { 0xffffffff, 0xa010199, 0xa020298 },  // 0x00295f5c
    { 0xa000096, 0xa010196, 0xffffffff },  // 0x00295f68
    { 0xa000097, 0xa010197, 0xffffffff },  // 0x00295f74
    { 0xa00009e, 0xa0101a2, 0xffffffff },  // 0x00295f80
    { 0xa00009f, 0xa0101a3, 0xffffffff },  // 0x00295f8c
    { 0xa0000a2, 0xa0101a4, 0xffffffff },  // 0x00295f98
    { 0xa0000a3, 0xa0101a5, 0xffffffff },  // 0x00295fa4
    { 0xa0000b2, 0xa0101b2, 0xffffffff },  // 0x00295fb0
    { 0xa0000b3, 0xa0101b3, 0xffffffff },  // 0x00295fbc
    { 0xa0000aa, 0xa0101a8, 0xffffffff },  // 0x00295fc8
    { 0xa0000ab, 0xa0101a9, 0xffffffff },  // 0x00295fd4
    { 0xa0000b0, 0xa0101b4, 0xffffffff },  // 0x00295fe0
    { 0xa0000b1, 0xa0101b5, 0xffffffff },  // 0x00295fec
    { 0xa000089, 0xa010183, 0xa020288 },  // 0x00295ff8
    { 0xa00008a, 0xa010184, 0xa02028b },  // 0x00296004
    { 0xa000084, 0xffffffff, 0xa020291 },  // 0x00296010
    { 0xa000085, 0xffffffff, 0xa020292 },  // 0x0029601c
    { 0xa0000b4, 0xffffffff, 0xa02029b },  // 0x00296028
    { 0xa0000b5, 0xffffffff, 0xa02029c },  // 0x00296034
    { 3, 0xa0101b6, 0xa02029d },  // 0x00296040
    { 3, 0xa0101b7, 0xa02029e },  // 0x0029604c
    { 0xa0000b8, 0xa0101b8, 0xa02029f },  // 0x00296058
    { 0xa0000b9, 0xa0101b9, 0xa0202a0 },  // 0x00296064
    { 0xa0000ba, 0xa0101ba, 0xffffffff },  // 0x00296070
    { 0xa0000bb, 0xa0101bb, 0xffffffff },  // 0x0029607c
    { 0xa0000bc, 0xa0101bc, 0xa0202a6 },  // 0x00296088
    { 0xa0000bd, 0xa0101bd, 0xa0202a1 },  // 0x00296094
    { 0xa0000be, 0xa0101be, 0xa0202a2 },  // 0x002960a0
    { 0xa0000bf, 0xa0101bf, 0xa0202a3 },  // 0x002960ac
    { 0xa0000c0, 0xa0101c0, 0xa0202a4 },  // 0x002960b8
    { 0xa0000c6, 0xffffffff, 0xa0202a5 },  // 0x002960c4
    { 0xa0000c7, 0xa0101c7, 0xffffffff },  // 0x002960d0
    { 0xa0000c9, 0xa0101cb, 0xffffffff },  // 0x002960dc
    { 0xa0000ca, 0xa0101cc, 0xffffffff },  // 0x002960e8
    { 0xa0000cb, 0xa0101cd, 0xffffffff },  // 0x002960f4
    { 0xa0000cc, 0xa0101ce, 0xffffffff },  // 0x00296100
    { 0xa0000ce, 0xa0101d3, 0xffffffff },  // 0x0029610c
    { 0xa0000cf, 0xa0101d4, 0xffffffff },  // 0x00296118
    { 0xa0000d0, 0xa0101d5, 0xffffffff },  // 0x00296124
    { 0xa0000d1, 0xa0101d6, 0xffffffff },  // 0x00296130
    { 0xa0000d2, 0xa0101d7, 0xffffffff },  // 0x0029613c
    { 0xa0000d3, 0xa0101d8, 0xffffffff },  // 0x00296148
    { 0xa0000d4, 0xa0101d9, 0xffffffff },  // 0x00296154
    { 0xa0000d5, 0xa0101da, 0xffffffff },  // 0x00296160
    { 0xa0000d6, 0xa0101db, 0xffffffff },  // 0x0029616c
    { 0xa0000d7, 0xa0101dc, 0xffffffff },  // 0x00296178
    { 0xfffffffe, 0xfffffffe, 0xfffffffe },  // 0x00296184
};

/* ---- the sound id swap -------------------------------------------------- */

// ROR 0x00106658
u_int utlConvSndId(u_int id)
{
    int col;

    switch (id & 0xffff0000)
    {
    case 0x0a000000: col = 0; break;
    case 0x0a010000: col = 1; break;
    case 0x0a020000: col = 2; break;
    default: return id;
    }

    int area = g_gamMan.getMapArea();
    if (col == area || area == -1)
        return id;

    for (const UtlSndIdRow *row = s_sndIdTable; row->m_id[col] != 0xfffffffe; row++)
    {
        if (row->m_id[col] == id)
            return row->m_id[area];
    }
    return id;
}

/* ---- i3r ---------------------------------------------------------------- */

// ROR 0x00106760
int utlDrawI3r(I3dLayer *layer, I3rRoot *root, sceVu0FMATRIX *nodes, sceVu0FVECTOR *world,
               const void *opt)
{
    rxThDisableDispatch();
    int ret = i3rDrawModel(layer, root, nodes, world, opt);
    rxThEnableDispatch();
    return ret;
}

// ROR 0x001067d8
int utlSetI3rDispControl(I3rRoot *root, void *disp)
{
    const I3rItem *item = &root->m_item;
    if (item == NULL)
        return 1;

    int num = i3rGetNumChild52_3(item);
    for (int i = 0; i < num; i++)
    {
        I3rItem *node = i3rGetChild52_3(item, i);
        if (node == NULL)
            continue;
        I3rItem *slot = i3rGetSlot2A(node);
        if (slot != NULL)
            i3rSetPtr(&slot->m_data, disp);
    }
    return 0;
}

// ROR 0x0010686c
int utlSetI3rNodeDisp(I3rRoot *root, void *disp, int n)
{
    const I3rItem *item = &root->m_item;
    if (item == NULL)
        return 1;

    I3rItem *node = i3rGetChild52_3(item, n);
    if (node == NULL)
        return 1;
    I3rItem *slot = i3rGetSlot2A(node);
    if (slot == NULL)
        return 1;
    i3rSetPtr(&slot->m_data, disp);
    return 0;
}

// ROR 0x001068d4
void *utlGetI3rNodeDisp(I3rRoot *root, int n)
{
    const I3rItem *item = &root->m_item;
    if (item == NULL)
        return NULL;

    I3rItem *node = i3rGetChild52_3(item, n);
    if (node == NULL)
        return NULL;
    I3rItem *slot = i3rGetSlot2A(node);
    if (slot == NULL)
        return NULL;
    return slot->getData();
}

/* ---- rotations ---------------------------------------------------------- */

// ROR 0x00106920
void utlVecRotX(float angle, float *out, const float *in)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float x = in[0], y = in[1], z = in[2], w = in[3];

    out[0] = x;
    out[1] = y * c - z * s;
    out[2] = y * s + z * c;
    out[3] = w;
}

// ROR 0x001069b0
void utlVecRotY(float angle, float *out, const float *in)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float x = in[0], y = in[1], z = in[2], w = in[3];

    out[0] = x * c - z * s;
    out[1] = y;
    out[2] = x * s + z * c;
    out[3] = w;
}

// ROR 0x00106a40
void utlVecRotZ(float angle, float *out, const float *in)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float x = in[0], y = in[1], z = in[2], w = in[3];

    out[0] = x * c - y * s;
    out[1] = x * s + y * c;
    out[2] = z;
    out[3] = w;
}

// ROR 0x00106ad0
float utlVecLength3(const float *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/* The one-turn wrap the matrix forms apply before building the matrix. */
static float utlWrapAngle(float angle)
{
    if (angle >= UTL_PI)
        angle -= UTL_2PI;
    else if (angle < -UTL_PI)
        angle += UTL_2PI;
    return angle;
}

// ROR 0x00106b24
void utlVecRotMatX(float angle, float *v)
{
    sceVu0FMATRIX m;

    sceVu0UnitMatrix(m);
    sceVu0RotMatrixX(m, m, utlWrapAngle(angle));
    sceVu0ApplyMatrix(v, m, v);
}

// ROR 0x00106bc8
void utlVecRotMatY(float angle, float *v)
{
    sceVu0FMATRIX m;

    sceVu0UnitMatrix(m);
    sceVu0RotMatrixY(m, m, utlWrapAngle(angle));
    sceVu0ApplyMatrix(v, m, v);
}

// ROR 0x00106c6c
void utlVecRotMatZ(float angle, float *v)
{
    sceVu0FMATRIX m;

    sceVu0UnitMatrix(m);
    sceVu0RotMatrixZ(m, m, utlWrapAngle(angle));
    sceVu0ApplyMatrix(v, m, v);
}

/* ---- products, lengths, distances --------------------------------------- */

// ROR 0x00106d10
float utlVecDot(const float *a, const float *b)
{
    return sceVu0InnerProduct((float *)a, (float *)b);
}

// ROR 0x00106d24
void utlVecCross(float *out, const float *a, const float *b)
{
    sceVu0OuterProduct(out, (float *)a, (float *)b);
}

// ROR 0x00106d40
void utlLerp4(float t, float *out, const float *a, const float *b)
{
    float u = 1.0f - t;

    out[0] = b[0] * t + a[0] * u;
    out[1] = b[1] * t + a[1] * u;
    out[2] = b[2] * t + a[2] * u;
    out[3] = b[3] * t + a[3] * u;
}

// ROR 0x00106db0
void utlNormalize3(float *v)
{
    v[3] = 0.0f;
    sceVu0Normalize(v, v);
}

// ROR 0x00106dd0
float utlVecLength(const float *v)
{
    return sqrtf(sceVu0InnerProduct((float *)v, (float *)v));
}

// ROR 0x00106e14
float utlVecDistSq(const float *a, const float *b)
{
    sceVu0FVECTOR d;

    d[0] = a[0] - b[0];
    d[1] = a[1] - b[1];
    d[2] = a[2] - b[2];
    d[3] = 0.0f;
    return sceVu0InnerProduct(d, d);
}

// ROR 0x00106e70
float utlVecDist(const float *a, const float *b)
{
    return sqrtf(utlVecDistSq(a, b));
}

// ROR 0x00106eb4
float utlVecDistInline(const float *a, const float *b)
{
    sceVu0FVECTOR d;

    d[0] = a[0] - b[0];
    d[1] = a[1] - b[1];
    d[2] = a[2] - b[2];
    d[3] = 0.0f;
    return sqrtf(sceVu0InnerProduct(d, d));
}

// ROR 0x00106f38
float utlVecDistXZ(const float *a, const float *b)
{
    sceVu0FVECTOR d;

    d[0] = a[0] - b[0];
    d[1] = 0.0f;
    d[2] = a[2] - b[2];
    d[3] = 0.0f;
    return sqrtf(sceVu0InnerProduct(d, d));
}

/* ---- directions to angles ----------------------------------------------- */

/* The y rotation of (x, z), the rules utlVecToRotY, utlVecDistRotY and
 * utlVecToRot share (each has it inline in the ROM). */
static float utlRotYOf(float x, float z)
{
    if (z == 0.0f)
    {
        if (x == 0.0f)
            return 0.0f;
        return x < 0.0f ? -UTL_PI_2 : UTL_PI_2;
    }
    float r = atanf(x / z);
    if (z < 0.0f)
    {
        r += UTL_PI;
        if (r >= UTL_PI)
            r -= UTL_2PI;
    }
    return r;
}

// callcheck: atanf (in utlRotYOf, the y rotation rule the ROM inlines)
// ROR 0x00106fb4
float utlVecDistRotY(const float *a, float *rot, const float *b)
{
    sceVu0FVECTOR d;

    d[0] = b[0] - a[0];
    d[1] = b[1] - a[1];
    d[2] = b[2] - a[2];
    d[3] = 0.0f;
    float dist = sqrtf(sceVu0InnerProduct(d, d));
    *rot = utlRotYOf(d[0], d[2]);
    return dist;
}

// callcheck: atanf (in utlRotYOf, the y rotation rule the ROM inlines)
// ROR 0x001070f8
float utlVecToRotY(const float *dir)
{
    return utlRotYOf(dir[0], dir[2]);
}

// ROR 0x001071c8
float utlVecToRotX(const float *dir)
{
    if (dir[2] == 0.0f)
    {
        if (dir[0] == 0.0f)
            return UTL_PI_2;
        return asinf(-dir[1]);
    }
    if (dir[1] != 0.0f)
        return asinf(-dir[1]);
    return 0.0f;
}

// callcheck: atanf (in utlRotYOf, the y rotation rule the ROM inlines)
// ROR 0x00107248
void utlVecToRot(float *dir, float *rot)
{
    utlNormalize3(dir);
    rot[2] = 0.0f;

    if (dir[2] == 0.0f)
    {
        if (dir[0] == 0.0f)
        {
            rot[1] = 0.0f;
            rot[0] = UTL_PI_2;
            return;
        }
        rot[1] = dir[0] >= 0.0f ? UTL_PI_2 : -UTL_PI_2;
        rot[0] = asinf(-dir[1]);
        return;
    }

    rot[0] = dir[1] == 0.0f ? 0.0f : asinf(-dir[1]);
    rot[1] = utlRotYOf(dir[0], dir[2]);
}
