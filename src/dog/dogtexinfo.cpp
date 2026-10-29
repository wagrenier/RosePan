// dog/dogtexinfo.cpp -- the named texture table (0x1161e4..0x11764c).
//
// A DogTexInfo keeps m_max_tex textures by name: m_tex_info is the i3d
// context's texture table (name, texture, CLUT), m_tex_data the records
// that own the i3d texture / CLUT objects, the TIM2 they read and a
// reference count. Textures come from a TIM2 (addData; addPfh walks a
// pack), or from an image already in VRAM (add). A released record keeps
// its slot with an empty name; findFree reuses it, and releaseModel /
// releasePfh trim m_num_tex past the last named record. Every change sets
// m_dirty, and the owner hands the table to the context again
// (setTexTable).

#include "dog/dogtexinfo.h"

#include <stdint.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "dog/dogmodel.h"
#include "lib/unassigned.h"
#include "utl/utlfile.h"
#include "utl/utlmisc.h"
#include "utl/utltm2.h"

static const char s_file[] = "dog/dogtexinfo.cpp";

/* rodata 0x2e8e80: the name a released record keeps. */
static const char s_empty[] = "";

/* The record a name record's picture points at (the texture starts it). */
static inline DogTexData *dogTexData(I3dVramHandle *picture)
{
    return (DogTexData *)picture;
}

/* m_num_tex down to the last record with a name (0 when none). The ROM
 * repeats this loop at the end of releaseModel and releasePfh. */
static inline void dogTexTrim(DogTexInfo *t)
{
    int i;
    for (i = t->m_num_tex - 1; i >= 0; i--)
    {
        const char *name = t->m_tex_info[i].pszTexName;
        if (name != NULL && *name != '\0')
            break;
    }
    t->m_num_tex = i + 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x001161e4

I3dTexEntry *DogTexInfo::getTexInfo(int theN)
{
    ROR_ASSERT(m_tex_info); /* 47 */
    ROR_ASSERT((theN >= 0) && (theN < m_num_tex)); /* 48 */
    return &m_tex_info[theN];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011626c

void DogTexInfo::setSepiaColor(int r, int g, int b)
{
    m_sepia_b = b;
    m_sepia_r = r;
    m_sepia_g = g;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011627c

void DogTexInfo::clear()
{
    m_sepia_r = 0xff;
    m_sepia_g = 0xdc;
    m_sepia_b = 0x98;
    m_dirty = 0;
    m_max_tex = 0;
    m_num_tex = 0;
    m_tex_info = NULL;
    m_tex_data = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Empties the table: theMax zeroed name records and texture records from
// the game heap (term frees the old ones first).
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x001162ac

void DogTexInfo::init(int theMax)
{
    ROR_ASSERT(theMax > 0); /* 99 */
    term();
    m_max_tex = theMax;
    m_tex_info = (I3dTexEntry *)g_utlMemory.alloc(theMax * (int)sizeof(I3dTexEntry), -1);
    ROR_ASSERT(m_tex_info); /* 107 */
    memset(m_tex_info, 0, theMax * sizeof(I3dTexEntry));
    m_tex_data = (DogTexData *)g_utlMemory.alloc(theMax * (int)sizeof(DogTexData), -1);
    ROR_ASSERT(m_tex_data); /* 113 */
    /* PC: (int) of a host pointer would truncate; the ROM's text is
     * ( ((int)m_tex_data) % 8 ) == 0. */
    ROR_ASSERT(( ((uintptr_t)m_tex_data) % 8 ) == 0); /* 114 */
    memset(m_tex_data, 0, theMax * sizeof(DogTexData));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001163dc

int DogTexInfo::find(const char *theName) const
{
    for (int i = 0; i < m_num_tex; i++)
    {
        const char *name = m_tex_info[i].pszTexName;
        if (name != NULL && *name != '\0' && strcmp(name, theName) == 0)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011646c

int DogTexInfo::findFree() const
{
    for (int i = 0; i < m_num_tex; i++)
    {
        const char *name = m_tex_info[i].pszTexName;
        if (name == NULL || *name == '\0')
            return i;
    }
    if (m_num_tex >= m_max_tex)
        return -1;
    return m_num_tex;
}

// ──────────────────────────────────────────────────────────────────────
// Frees the TIM2 copies and the texture records, terms every named
// record's texture / CLUT and frees the name records, then clear. The ROM
// terms the textures after freeing the records that hold them (they stay
// in the game heap, so the reads still find them).
// ROR 0x001164d0

void DogTexInfo::term()
{
    if (m_tex_data != NULL)
    {
        for (int i = 0; i < m_num_tex; i++)
        {
            DogTexData *d = &m_tex_data[i];
            if (d->m_own != 0 && d->m_tm2 != NULL)
            {
                g_utlMemory.free(d->m_tm2);
                m_tex_data[i].m_tm2 = NULL;
            }
        }
        g_utlMemory.free(m_tex_data);
        m_tex_data = NULL;
    }
    if (m_tex_info != NULL)
    {
        for (int i = 0; i < m_num_tex; i++)
        {
            I3dTexEntry *e = &m_tex_info[i];
            if (e->pszTexName == NULL)
                continue;
            if (e->pPicture != NULL)
                i3dTexTerm((I3dTexture *)e->pPicture);
            if (m_tex_info[i].m_clut != NULL)
                i3dClutTerm((I3dClut *)m_tex_info[i].m_clut);
        }
        g_utlMemory.free(m_tex_info);
        m_tex_info = NULL;
    }
    clear();
}

// ──────────────────────────────────────────────────────────────────────
// The last reference frees the texture: terms its texture / CLUT, leaves
// the name record empty ("") and clears the record (freeing its TIM2 copy).
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x00116618

void DogTexInfo::release(int theN)
{
    if (theN < 0 || theN >= m_max_tex || theN >= m_num_tex)
        return;
    ROR_ASSERT(m_tex_info); /* 245 */
    ROR_ASSERT(m_tex_data); /* 246 */
    ROR_ASSERT(m_tex_info[theN].pszTexName != NULL); /* 247 */
    if (m_tex_info[theN].pszTexName != NULL)
    {
        DogTexData *p = dogTexData(m_tex_info[theN].pPicture);
        ROR_ASSERT(p); /* 253 */
        /* PC: the ROM goes on with p NULL (it reads and writes low EE
         * memory); the host stops here. */
        if (p != NULL && --p->m_ref <= 0)
        {
            if (m_tex_info[theN].pPicture != NULL)
                i3dTexTerm((I3dTexture *)m_tex_info[theN].pPicture);
            if (m_tex_info[theN].m_clut != NULL)
                i3dClutTerm((I3dClut *)m_tex_info[theN].m_clut);
            m_tex_info[theN].pszTexName = s_empty;
            m_tex_info[theN].pPicture = NULL;
            m_tex_info[theN].m_clut = NULL;
            ROR_ASSERT(p); /* 269 */
            if (p->m_own != 0)
                g_utlMemory.free(p->m_tm2);
            memset(p, 0, sizeof(DogTexData));
        }
    }
    m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// Releases one reference to each texture the model's root names, then
// trims m_num_tex.
// callcheck: dbgAssertFailed, i3rGetNumTex, i3rGetTexName (the assert handler inside ROR_ASSERT; inside the inline DogModel::get_nTex / getTexName)
// ROR 0x001167f4

void DogTexInfo::releaseModel(DogModel *model)
{
    ROR_ASSERT(m_tex_info); /* 289 */
    ROR_ASSERT(m_tex_data); /* 290 */
    for (int i = 0; i < model->get_nTex(); i++)
    {
        const char *name = model->getTexName(i);
        for (int j = 0; j < m_num_tex; j++)
        {
            const char *n = m_tex_info[j].pszTexName;
            if (n != NULL && strcmp(n, name) == 0)
            {
                release(j);
                break;
            }
        }
    }
    dogTexTrim(this);
}

// ──────────────────────────────────────────────────────────────────────
// Releases one reference to each texture named in a pack's name table,
// which follows its header and offset table, then trims m_num_tex.
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x001169d0

void DogTexInfo::releasePfh(const BedPfhHead *head)
{
    int n = head->m_num_file;
    const char *name = (const char *)head + sizeof(BedPfhHead) + n * 4;
    for (; n > 0; n--)
    {
        ROR_ASSERT(name); /* 338 */
        ROR_ASSERT((*name)!=0); /* 339 */
        for (int i = 0; i < m_num_tex; i++)
        {
            const char *t = m_tex_info[i].pszTexName;
            if (t != NULL && strcmp(t, name) == 0)
            {
                release(i);
                break;
            }
        }
        while (*name++ != '\0')
            ;
    }
    dogTexTrim(this);
}

// ──────────────────────────────────────────────────────────────────────
// A texture already in VRAM: an existing name returns its record as it is
// (no reference counted); a new one takes the last empty record, else the
// next new one.
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x00116b5c

int DogTexInfo::add(const I3dTexDesc *desc, const char *name)
{
    int place = -1;
    int i;
    for (i = 0; i < m_num_tex; i++)
    {
        const char *n = m_tex_info[i].pszTexName;
        if (n == NULL || *n == '\0')
            place = i;
        else if (strcmp(n, name) == 0)
            return i;
    }
    if (place < 0)
    {
        place = m_num_tex;
        if (place >= m_max_tex)
            return -1;
        m_num_tex = place + 1;
        ROR_ASSERT(m_num_tex <= m_max_tex); /* 422 */
    }
    DogTexData *d = &m_tex_data[place];
    int result = i3dTexInit(&d->m_tex);
    ROR_ASSERT(result == I3D_OK); /* 431 */
    i3dTexSetDesc(&d->m_tex, desc);
    m_tex_info[place].pPicture = &d->m_tex.m_handle;
    m_tex_info[place].pszTexName = name;
    m_tex_info[place].m_clut = NULL;
    m_tex_info[place].m_0c = 0;
    m_tex_data[place].m_tm2 = NULL;
    m_tex_data[place].m_own = 0;
    m_tex_data[place].m_ref = 1;
    m_dirty = 1;
    return place;
}

// ──────────────────────────────────────────────────────────────────────
// A texture and CLUT from a TIM2 (a heap copy when flags is not 0; the
// ROM sizes it twice and tags the block 0 or 2 by the second size). A
// TIM2 without a CLUT leaves the record's CLUT NULL.
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x00116d14

int DogTexInfo::addData(const char *theTexName, void *theTM2, int flags, int tcc)
{
    ROR_ASSERT(theTM2); /* 464 */
    ROR_ASSERT(theTexName); /* 465 */
    ROR_ASSERT(strlen(theTexName) < 40); /* 466 */

    int place;
    if ((flags & DOG_TEX_NEW) == 0 && (place = find(theTexName)) >= 0)
    {
        m_tex_data[place].m_ref++;
        return place;
    }
    place = findFree();
    if (place < 0)
        return -1;

    short own;
    if (flags == 0)
    {
        own = 0;
    }
    else
    {
        int size = utlTm2GetSize(theTM2);
        void *copy;
        if (utlTm2GetSize(theTM2) == 0)
            copy = g_utlMemory.allocDebug(size, s_file, 501, place, 0);
        else
            copy = g_utlMemory.allocDebug(size, s_file, 505, place, 2);
        if (copy == NULL)
            return -2;
        memcpy(copy, theTM2, size);
        own = 1;
        theTM2 = copy;
    }

    m_tex_data[place].m_tm2 = theTM2;
    m_tex_data[place].m_own = own;
    m_tex_data[place].m_ref = 1;
    utlStrCopy(m_tex_data[place].m_name, DOG_TEX_NAME_MAX, theTexName);

    I3dTexture *tex = &m_tex_data[place].m_tex;
    int result = i3dTexInit(tex);
    ROR_ASSERT(result == I3D_OK); /* 544 */
    if (i3dTexSetTim2(tex, theTM2, 0, 0) != 0)
        return 3;
    i3dTexSetTcc(tex, tcc < 0 ? (u_long)utlIsTccName(theTexName) : (u_long)(tcc & 1));
    i3dTexSetTfx(tex, 0);

    I3dClut *clut = &m_tex_data[place].m_clut;
    result = i3dClutInit(clut);
    ROR_ASSERT(result == I3D_OK); /* 570 */
    I3dClut *use_clut = clut;
    if (i3dClutSetTim2(clut, theTM2, 0, 0) != 0)
    {
        use_clut = NULL;
        i3dClutTerm(clut);
    }

    m_tex_info[place].pszTexName = m_tex_data[place].m_name;
    m_tex_info[place].pPicture = &tex->m_handle;
    m_tex_info[place].m_clut = use_clut != NULL ? &use_clut->m_handle : NULL;
    m_tex_info[place].m_0c = 0;
    if (place == m_num_tex)
        m_num_tex = place + 1;
    ROR_ASSERT(place < m_num_tex); /* 607 */
    ROR_ASSERT(m_num_tex <= m_max_tex); /* 608 */
    m_dirty = 1;
    return place;
}

// ──────────────────────────────────────────────────────────────────────
// Adds a pack's TIM2s under the pack's names, or -- with an i3r model --
// the pack's TIM2s the model names, under the model's names. sepia not 0
// turns each picture sepia first (utlTm2ToSepia with the table's colour).
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT and BedPfh's inline getNumFile / getFile)
// ROR 0x001170b8

int DogTexInfo::addPfh(const BedPfh *pfh, I3rRoot *theI3r, int flags, int sepia)
{
    const char *name[256];
    int numtex = pfh->getNumFile();
    ROR_ASSERT(numtex <= 256); /* 649 */
    const char *p = pfh->m_name;
    for (int i = 0; i < numtex; i++)
    {
        name[i] = p;
        p += strlen(p) + 1;
    }

    if (theI3r == NULL)
    {
        for (int i = 0; i < numtex; i++)
        {
            const char *texname = name[i];
            void *tm2 = pfh->getFile(i);
            if (sepia != 0)
                utlTm2ToSepia(tm2, m_sepia_r, m_sepia_g, m_sepia_b);
            if (addData(texname, tm2, flags, -1) < 0)
                return -1;
        }
        return 0;
    }

    ROR_ASSERT(I3D_OK == I3rRoot__CheckIdString( theI3r )); /* 660 */
    int ntex = i3rGetNumTex(theI3r);
    ROR_ASSERT(ntex >= 0); /* 662 */
    for (int j = 0; j < ntex; j++)
    {
        const char *texname = i3rGetTexName(theI3r, j);
        int i;
        for (i = 0; ; i++)
        {
            if (i >= numtex)
                return -3;
            if (strcmp(name[i], texname) == 0)
                break;
        }
        void *tm2 = pfh->getFile(i);
        if (sepia != 0)
            utlTm2ToSepia(tm2, m_sepia_r, m_sepia_g, m_sepia_b);
        if (addData(texname, tm2, flags, -1) < 0)
            return -2;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Texture fname takes texture ename's picture, CLUT and index (the
// picture's VRAM copy is dropped, i3dTexRelease).
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x001173d4

void DogTexInfo::copy(const char *fname, const char *ename)
{
    int fn = find(fname);
    ROR_ASSERT(fn >= 0); /* 735 */
    int en = find(ename);
    ROR_ASSERT(en >= 0); /* 738 */
    /* PC: the ROM writes record -1 when a name is missing. */
    if (fn < 0 || en < 0)
        return;
    m_tex_info[fn].pPicture = m_tex_info[en].pPicture;
    m_tex_info[fn].m_clut = m_tex_info[en].m_clut;
    m_tex_info[fn].m_0c = m_tex_info[en].m_0c;
    i3dTexRelease((I3dTexture *)m_tex_info[fn].pPicture);
    m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x001174c4

void DogTexInfo::setTexFlag(int theNo, int tcc)
{
    ROR_ASSERT(theNo >= 0 && theNo < m_max_tex); /* 768 */
    /* PC: the ROM indexes and writes on after a failed assert. */
    if (theNo < 0 || theNo >= m_max_tex)
        return;
    ROR_ASSERT(m_tex_info[theNo].pPicture); /* 769 */
    if (m_tex_info[theNo].pPicture == NULL)
        return;
    i3dTexSetTcc((I3dTexture *)m_tex_info[theNo].pPicture, (u_long)tcc);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed, i3dCtxSetTexTable (the assert handler inside ROR_ASSERT; a tail jump)
// ROR 0x0011756c

void DogTexInfo::setTexTable(i3dContext *p)
{
    ROR_ASSERT(p != NULL); /* 818 */
    ROR_ASSERT(m_tex_info != NULL); /* 819 */
    ROR_ASSERT(m_num_tex > 0); /* 820 */
    int i;
    for (i = m_num_tex - 1; i >= 0; i--)
    {
        if (m_tex_info[i].pPicture != NULL)
            break;
    }
    m_dirty = 0;
    m_num_tex = i + 1;
    i3dCtxSetTexTable(p, m_tex_info, m_num_tex);
}
