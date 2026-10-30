// dog/doglight.cpp -- one block of lights (0x112b08..0x114034).
//
// A DogLight keeps the lights of the six kinds (ambient, direct, point,
// spot, hemi, dist) in one block create allocates, an i3d light
// description in front, a per-kind enable mask and a dirty word that every
// change sets. It is DogLightPool's simpler cousin: no light sets, no ids,
// and two kinds more (the 0x30-byte hemi and dist lights the pool sizes but
// never fills).
//
// Nothing in the ROM reaches this file: no call, jump, data word or
// lui / addiu pair points at any entry (ghidra.py refs, 2026-10-30), and
// nothing reads m_desc or m_dirty. It is written for completeness.

#include "dog/doglight.h"

#include <string.h>

#include "common/ror_assert.h"
#include "i3d/i3denv.h"
#include "utl/utlmisc.h"

/* The cone angles' degree <-> radian factor: the ROM's pi is 0x40490fdc,
 * one ulp above the float nearest pi (the pool's too). */
static const float s_pi = 0x1.921fb8p+1f;

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00112b08

void DogLight::init()
{
    i3dLightDescInit(&m_desc);
    m_data    = NULL;
    m_ambient = NULL;
    m_direct  = NULL;
    m_point   = NULL;
    m_spot    = NULL;
    m_hemi    = NULL;
    m_dist    = NULL;
    m_enable  = 0xffffffff;
    m_dirty   = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00112b58

void DogLight::clear()
{
    term();
    init();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00112b84

void DogLight::term()
{
    if (m_data != NULL)
    {
        g_utlMemory.free(m_data);
        m_data = NULL;
    }
    init();
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00112bc8

int DogLight::getDataSize(const int *theNum) const
{
    ROR_ASSERT(theNum); /* 92 */
    ROR_ASSERT(theNum[DOG_LIGHT_AMBIENT] >= 0 && theNum[DOG_LIGHT_AMBIENT] <= 1); /* 94 */
    ROR_ASSERT(theNum[DOG_LIGHT_DIRECT] >= 0 && theNum[DOG_LIGHT_DIRECT] <= 16); /* 95 */
    ROR_ASSERT(theNum[DOG_LIGHT_POINT] >= 0 && theNum[DOG_LIGHT_POINT] <= 16); /* 96 */
    ROR_ASSERT(theNum[DOG_LIGHT_SPOT] >= 0 && theNum[DOG_LIGHT_SPOT] <= 16); /* 97 */
    ROR_ASSERT(theNum[DOG_LIGHT_HEMI] >= 0 && theNum[DOG_LIGHT_HEMI] <= 16); /* 98 */
    ROR_ASSERT(theNum[DOG_LIGHT_DIST] >= 0 && theNum[DOG_LIGHT_DIST] <= 16); /* 99 */

    int sz = theNum[DOG_LIGHT_AMBIENT] * (int)sizeof(DogAmbientLight)
           + theNum[DOG_LIGHT_DIRECT]  * (int)sizeof(DogDirectLight)
           + theNum[DOG_LIGHT_POINT]   * (int)sizeof(DogPointLight)
           + theNum[DOG_LIGHT_SPOT]    * (int)sizeof(DogSpotLight)
           + theNum[DOG_LIGHT_HEMI]    * (int)sizeof(DogHemiLight)
           + theNum[DOG_LIGHT_DIST]    * (int)sizeof(DogDistLight);
    ROR_ASSERT(sz > 0); /* 111 */
    return sz;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00112d60

void DogLight::create(const int *theNum)
{
    ROR_ASSERT(theNum); /* 129 */
    ROR_ASSERT(theNum[DOG_LIGHT_AMBIENT] >= 0 && theNum[DOG_LIGHT_AMBIENT] <= 1); /* 133 */
    ROR_ASSERT(theNum[DOG_LIGHT_DIRECT] >= 0 && theNum[DOG_LIGHT_DIRECT] <= 16); /* 134 */
    ROR_ASSERT(theNum[DOG_LIGHT_POINT] >= 0 && theNum[DOG_LIGHT_POINT] <= 16); /* 135 */
    ROR_ASSERT(theNum[DOG_LIGHT_SPOT] >= 0 && theNum[DOG_LIGHT_SPOT] <= 16); /* 136 */
    ROR_ASSERT(theNum[DOG_LIGHT_HEMI] >= 0 && theNum[DOG_LIGHT_HEMI] <= 16); /* 137 */
    ROR_ASSERT(theNum[DOG_LIGHT_DIST] >= 0 && theNum[DOG_LIGHT_DIST] <= 16); /* 138 */

    term();
    int sz = getDataSize(theNum);
    if (sz > 0)
    {
        u_char *p = (u_char *)g_utlMemory.alloc(sz, -1);
        m_data = p;
        /* PC: the ROM clears and carves whatever alloc returned. */
        if (p == NULL)
        {
            m_dirty = 1;
            return;
        }
        memset(p, 0, sz);

        m_num[DOG_LIGHT_AMBIENT] = theNum[DOG_LIGHT_AMBIENT];
        if (theNum[DOG_LIGHT_AMBIENT] > 0)
        {
            m_ambient = (DogAmbientLight *)p;
            p += theNum[DOG_LIGHT_AMBIENT] * sizeof(DogAmbientLight);
        }
        m_num[DOG_LIGHT_DIRECT] = theNum[DOG_LIGHT_DIRECT];
        if (theNum[DOG_LIGHT_DIRECT] > 0)
        {
            m_direct = (DogDirectLight *)p;
            p += theNum[DOG_LIGHT_DIRECT] * sizeof(DogDirectLight);
        }
        m_num[DOG_LIGHT_POINT] = theNum[DOG_LIGHT_POINT];
        if (theNum[DOG_LIGHT_POINT] > 0)
        {
            m_point = (DogPointLight *)p;
            p += theNum[DOG_LIGHT_POINT] * sizeof(DogPointLight);
        }
        m_num[DOG_LIGHT_SPOT] = theNum[DOG_LIGHT_SPOT];
        if (theNum[DOG_LIGHT_SPOT] > 0)
        {
            m_spot = (DogSpotLight *)p;
            p += theNum[DOG_LIGHT_SPOT] * sizeof(DogSpotLight);
        }
        m_num[DOG_LIGHT_HEMI] = theNum[DOG_LIGHT_HEMI];
        if (theNum[DOG_LIGHT_HEMI] > 0)
        {
            m_hemi = (DogHemiLight *)p;
            p += theNum[DOG_LIGHT_HEMI] * sizeof(DogHemiLight);
        }
        m_num[DOG_LIGHT_DIST] = theNum[DOG_LIGHT_DIST];
        if (theNum[DOG_LIGHT_DIST] > 0)
            m_dist = (DogDistLight *)p;
    }
    m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// A kind with no array leaves setParam silently (before the index
// assert); a colour other than the ambient one gets w = 0.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00112fc8

void DogLight::setParam(int theType, int theNo, int param, const float *v)
{
    float *dst = NULL;
    bool   color = false;

    if (param == DOG_LIGHT_PARAM_CONE)
    {
        if (theType != DOG_LIGHT_SPOT)
        {
            ROR_ASSERT(0); /* 280 */
            return;
        }
        if (m_spot == NULL)
            return;
        ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 274 */
        m_spot[theNo].m_cone[0] = v[0] * s_pi / 180.0f;
        m_spot[theNo].m_cone[1] = v[1] * s_pi / 180.0f;
        return;
    }

    switch (param)
    {
    case DOG_LIGHT_PARAM_POS:
        switch (theType)
        {
        case DOG_LIGHT_POINT:
            if (m_point == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_POINT]); /* 291 */
            dst = m_point[theNo].m_pos;
            break;
        case DOG_LIGHT_SPOT:
            if (m_spot == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 299 */
            dst = m_spot[theNo].m_pos;
            break;
        case DOG_LIGHT_DIST:
            if (m_dist == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIST]); /* 307 */
            dst = m_dist[theNo].m_pos;
            break;
        default:
            ROR_ASSERT(0); /* 312 */
            break;
        }
        break;

    case DOG_LIGHT_PARAM_DIR:
        switch (theType)
        {
        case DOG_LIGHT_DIRECT:
            if (m_direct == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIRECT]); /* 322 */
            dst = m_direct[theNo].m_dir;
            break;
        case DOG_LIGHT_SPOT:
            if (m_spot == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 330 */
            dst = m_spot[theNo].m_dir;
            break;
        case DOG_LIGHT_HEMI:
            if (m_hemi == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_HEMI]); /* 338 */
            dst = m_hemi[theNo].m_dir;
            break;
        default:
            ROR_ASSERT(0); /* 343 */
            break;
        }
        break;

    case DOG_LIGHT_PARAM_COLOR:
        switch (theType)
        {
        case DOG_LIGHT_AMBIENT:
            if (m_ambient == NULL)
                return;
            ROR_ASSERT(theNo == 0); /* 353 */
            /* The ROM writes the first ambient light whatever theNo is. */
            dst = m_ambient[0].m_color;
            break;
        case DOG_LIGHT_POINT:
            if (m_point == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_POINT]); /* 361 */
            dst = m_point[theNo].m_color;
            color = true;
            break;
        case DOG_LIGHT_DIRECT:
            if (m_direct == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIRECT]); /* 370 */
            dst = m_direct[theNo].m_color;
            color = true;
            break;
        case DOG_LIGHT_SPOT:
            if (m_spot == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 379 */
            dst = m_spot[theNo].m_color;
            color = true;
            break;
        case DOG_LIGHT_HEMI:
            if (m_hemi == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_HEMI]); /* 388 */
            dst = m_hemi[theNo].m_color;
            color = true;
            break;
        case DOG_LIGHT_DIST:
            if (m_dist == NULL)
                return;
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIST]); /* 397 */
            dst = m_dist[theNo].m_color;
            color = true;
            break;
        default:
            ROR_ASSERT(0); /* 403 */
            break;
        }
        break;

    case DOG_LIGHT_PARAM_HEMI2:
        if (theType != DOG_LIGHT_HEMI)
        {
            ROR_ASSERT(0); /* 418 */
            break;
        }
        if (m_hemi == NULL)
            return;
        ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_HEMI]); /* 413 */
        dst = m_hemi[theNo].m_20;
        break;

    case DOG_LIGHT_PARAM_DIST2:
        if (theType != DOG_LIGHT_DIST)
        {
            ROR_ASSERT(0); /* 433 */
            break;
        }
        /* The ROM tests m_hemi here (lw +0x54 at 0x1135a8), not m_dist,
         * then writes m_dist: ported as is, with a PC guard for the NULL
         * m_dist it would write through. */
        if (m_hemi == NULL)
            return;
        ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIST]); /* 428 */
        if (m_dist != NULL)
            dst = m_dist[theNo].m_20;
        break;

    case DOG_LIGHT_PARAM_ENABLE:
    {
        u_int bit = 1u << (theType & 0x1f);
        if (v == NULL || v[0] == 0.0f)
            m_enable &= ~bit;
        else
            m_enable |= bit;
        break;
    }

    default:
        ROR_ASSERT(0); /* 461 */
        break;
    }

    if (dst != NULL)
    {
        for (int i = 0; i < 4; i++)
            dst[i] = v[i];
        if (color)
            dst[3] = 0.0f;
    }
    m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// The read twin of setParam: a kind with no array asserts its name (and
// the ROM reads through NULL plus the offset; the PC reads nothing), the
// ambient colour asserts theNo == 0, colours keep their w.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001136d0

void DogLight::getParam(int theType, int theNo, int param, float *v) const
{
    const float *src = NULL;

    if (param == DOG_LIGHT_PARAM_CONE)
    {
        if (theType != DOG_LIGHT_SPOT)
        {
            ROR_ASSERT(0); /* 497 */
            return;
        }
        ROR_ASSERT(m_spot); /* 490 */
        ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 491 */
        if (m_spot == NULL)
            return;
        v[0] = m_spot[theNo].m_cone[0] * 180.0f / s_pi;
        v[1] = m_spot[theNo].m_cone[1] * 180.0f / s_pi;
        return;
    }

    switch (param)
    {
    case DOG_LIGHT_PARAM_POS:
        switch (theType)
        {
        case DOG_LIGHT_POINT:
            ROR_ASSERT(m_point); /* 505 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_POINT]); /* 506 */
            if (m_point != NULL)
                src = m_point[theNo].m_pos;
            break;
        case DOG_LIGHT_SPOT:
            ROR_ASSERT(m_spot); /* 511 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 512 */
            if (m_spot != NULL)
                src = m_spot[theNo].m_pos;
            break;
        case DOG_LIGHT_DIST:
            ROR_ASSERT(m_dist); /* 517 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIST]); /* 518 */
            if (m_dist != NULL)
                src = m_dist[theNo].m_pos;
            break;
        default:
            ROR_ASSERT(0); /* 523 */
            break;
        }
        break;

    case DOG_LIGHT_PARAM_DIR:
        switch (theType)
        {
        case DOG_LIGHT_DIRECT:
            ROR_ASSERT(m_direct); /* 530 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIRECT]); /* 531 */
            if (m_direct != NULL)
                src = m_direct[theNo].m_dir;
            break;
        case DOG_LIGHT_SPOT:
            ROR_ASSERT(m_spot); /* 536 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 537 */
            if (m_spot != NULL)
                src = m_spot[theNo].m_dir;
            break;
        case DOG_LIGHT_HEMI:
            ROR_ASSERT(m_hemi); /* 542 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_HEMI]); /* 543 */
            if (m_hemi != NULL)
                src = m_hemi[theNo].m_dir;
            break;
        default:
            ROR_ASSERT(0); /* 548 */
            break;
        }
        break;

    case DOG_LIGHT_PARAM_COLOR:
        switch (theType)
        {
        case DOG_LIGHT_AMBIENT:
            ROR_ASSERT(m_ambient); /* 555 */
            ROR_ASSERT(theNo == 0); /* 556 */
            if (m_ambient != NULL)
                src = m_ambient[0].m_color;
            break;
        case DOG_LIGHT_POINT:
            ROR_ASSERT(m_point); /* 561 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_POINT]); /* 562 */
            if (m_point != NULL)
                src = m_point[theNo].m_color;
            break;
        case DOG_LIGHT_DIRECT:
            ROR_ASSERT(m_direct); /* 567 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIRECT]); /* 568 */
            if (m_direct != NULL)
                src = m_direct[theNo].m_color;
            break;
        case DOG_LIGHT_SPOT:
            ROR_ASSERT(m_spot); /* 573 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_SPOT]); /* 574 */
            if (m_spot != NULL)
                src = m_spot[theNo].m_color;
            break;
        case DOG_LIGHT_HEMI:
            ROR_ASSERT(m_hemi); /* 579 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_HEMI]); /* 580 */
            if (m_hemi != NULL)
                src = m_hemi[theNo].m_color;
            break;
        case DOG_LIGHT_DIST:
            ROR_ASSERT(m_dist); /* 585 */
            ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIST]); /* 586 */
            if (m_dist != NULL)
                src = m_dist[theNo].m_color;
            break;
        default:
            ROR_ASSERT(0); /* 591 */
            break;
        }
        break;

    case DOG_LIGHT_PARAM_HEMI2:
        if (theType != DOG_LIGHT_HEMI)
        {
            ROR_ASSERT(0); /* 604 */
            break;
        }
        ROR_ASSERT(m_hemi); /* 598 */
        ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_HEMI]); /* 599 */
        if (m_hemi != NULL)
            src = m_hemi[theNo].m_20;
        break;

    case DOG_LIGHT_PARAM_DIST2:
        if (theType != DOG_LIGHT_DIST)
        {
            ROR_ASSERT(0); /* 617 */
            break;
        }
        ROR_ASSERT(m_dist); /* 611 */
        ROR_ASSERT(theNo >= 0 && theNo < m_num[DOG_LIGHT_DIST]); /* 612 */
        if (m_dist != NULL)
            src = m_dist[theNo].m_20;
        break;

    case DOG_LIGHT_PARAM_ENABLE:
        v[0] = (m_enable & (1u << (theType & 0x1f))) ? 1.0f : 0.0f;
        break;

    default:
        ROR_ASSERT(0); /* 637 */
        break;
    }

    if (src != NULL)
    {
        for (int i = 0; i < 4; i++)
            v[i] = src[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00113ea8

void DogLight::copyTo(DogLight *dst) const
{
    dst->create(m_num);
    memcpy(dst->m_data, m_data, getDataSize(m_num));
    dst->m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// Two GCC 2 type_info functions the export places here (they link next to
// nothing of this file): the host compiler's RTTI stands in.
// ROR 0x00282e08 MagLifePCModeAtkMove2D::__tf
// ROR 0x002832f4 MagLifePCModeMove::__tf
