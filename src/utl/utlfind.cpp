/* ==========================================================================
 *  utl/utlfind.cpp
 *
 *  The dog's item search (ROR 0x1060e8..0x106658). See utlfind.h.
 *
 *  utlFindBest and utlFindFirstVFchr walk GamMan::m_vfchrList's used list
 *  (sentinel 0x79ef90), as utlFindVFchrByUid does.
 * ======================================================================== */
#include "utl/utlfind.h"

#include <math.h>

#include "common/ror_assert.h"
#include "debug/mimu_debug.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "utl/utlgame.h"
#include "wrd/wrdsrv.h"

// ROR 0x001060e8
int utlFindObjWrdNo(FCHRstat *theHandle)
{
    ROR_ASSERT(theHandle != 0);  /* 22 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    int param[4];
    theHandle->getParam(8, param);
    return param[0];
}

// ROR 0x00106138
int utlFindObjUid(FCHRstat *theHandle)
{
    ROR_ASSERT(theHandle != 0);  /* 36 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    return (u_short)theHandle->m_kind;
}

// ROR 0x00106178
unsigned int utlFindObjId(FCHRstat *theHandle)
{
    ROR_ASSERT(theHandle != 0);  /* 48 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    return theHandle->m_oid;
}

// ROR 0x001061b8
void utlFindObjPos(FCHRstat *theHandle, float *out)
{
    ROR_ASSERT(theHandle != 0);  /* 60 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    theHandle->getPos(out);
}

// ROR 0x0010620c
void utlFindClear(UtlFind *p)
{
    for (int i = 0; i < 4; i++)
        p->m_id[i] = -1;
    p->m_again = 0;
    p->m_num = 0;
}

// ROR 0x00106240
int utlFindHasAny(UtlFind *p)
{
    for (int i = 0; i < p->m_num; i++)
    {
        if (p->m_id[i] > 0)
            return 1;
    }
    return 0;
}

// ROR 0x00106280
void utlFindSetItem(UtlFind *p, int item, int arg)
{
    p->m_again = arg;
    p->m_num = utlGetItemUids(p->m_id, 4, (u_int)item);
}

// ROR 0x001062b4
// The ROM tail-calls utlFindItemByUids(obj, p->m_id, p->m_num): obj is the
// search's position out, so a found object's position lands on obj's first
// 16 bytes (its vtable and header). Kept as the ROM has it; nothing calls
// MagLifeBwnCtrlModeFind::isFindTarget, its only caller.
int utlFindMatchObj(UtlFind *p, FCHRstat *obj)
{
    // PORT: the ROM returns the 64-bit uid << 32 | oid in v0; the int
    // answer keeps whether one was found.
    return utlFindItemByUids((float *)obj, p->m_id, p->m_num) != 0;
}

// ROR 0x001062d8
FCHRstat *utlFindFirstVFchr(UtlFind *p)
{
    for (int i = 0; i < p->m_num; i++)
    {
        FCHRstat *obj = utlFindVFchrByUid((u_int)p->m_id[i]);
        if (obj != NULL)
            return obj;
    }
    return NULL;
}

// ROR 0x00106344
FCHRstat *utlFindBest(UtlFind *p, const float *pos, int wrd)
{
    if (pos == NULL)
        return utlFindFirstVFchr(p);

    int inRoom = 0;
    dbgTraceVector4(pos);
    FCHRstat *best = NULL;
    float bestScore = 0.0f;
    for (int i = 0; i < p->m_num; i++)
    {
        for (SeqNode<FCHRstat *> *node = g_gamMan.m_vfchrList.m_used.begin();
             node != g_gamMan.m_vfchrList.m_used.end(); node = node->m_next)
        {
            FCHRstat *obj = node->m_data;
            u_int uid = (u_short)obj->m_kind;
            if ((u_int)p->m_id[i] != uid)
                continue;

            float score = obj->m_smellRange;
            if (score == 0.0f)
                score = 1e6f;
            int param[4];       // PORT: getParam may write more than the word the ROM reads
            param[0] = -100;
            obj->getParam(8, param);
            int area = param[0];
            ROR_ASSERT(area >= 0);  /* 172 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

            /* The first object in room wrd drops the others found so far;
             * after it only room wrd's objects count. */
            if (!inRoom)
            {
                if (area == wrd)
                {
                    bestScore = 0.0f;
                    best = NULL;
                    inRoom = 1;
                }
            }
            else if (area != wrd)
                continue;

            float local[4];
            obj->getPos(local);
            float world[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            g_wrdSrv.cnvPosL2W(world, area, local);
            float dx = pos[0] - world[0];
            float dy = pos[1] - world[1];
            float dz = pos[2] - world[2];
            float dist = sqrtf(dx * dx + dy * dy * 100.0f + dz * dz);
            score = score / (dist / 100.0f);
            dbgTraceVector4(world);
            if (1.0f <= score)
            {
                if (p->m_again == 0 && utlIsItemFlag2((short)uid))
                    score = score / 10.0f;
                if (bestScore < score)
                {
                    bestScore = score;
                    best = obj;
                }
            }
        }
    }
    return best;
}
