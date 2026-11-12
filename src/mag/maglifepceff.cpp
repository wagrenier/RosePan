/* ==========================================================================
 *  mag/maglifepceff.cpp
 *
 *  MagLifePC's gun muzzle flash and messages 0xe..0x11. 0x12ccac..0x12ce3c
 *  (pinned 2026-10-30; the asserts name this file in 0x12cdb0, 0x12cddc and
 *  0x12ce08, createMuzzleEff opens it, the empty message17 closes the
 *  series before mag/maglifepcmsg.cpp).
 *
 *  Messages 0xe..0x10 are assert(0): the source still routes them to the
 *  PC, but the shipped game must never send them (the assert hangs on the
 *  PS2; the host prints it and returns).
 * ======================================================================== */

#include "mag/maglifepc.h"

#include <string.h>

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "eff/effman.h"
#include "mag/magscene.h"

// ROR 0x0012ccac

void MagLifePC::createMuzzleEff()
{
    u_int chr = (u_int)getEquipItemChrNo();
    if (chr >= 0x80)
        return;
    DogActor *gun = &m_scene->m_actors[chr];
    int n = gun->getNodeIndexByName("gun_mazzle");   // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)
    if (n < 0)
        return;
    sceVu0FMATRIX m;
    gun->getNodeWorldMatrix(m, n);
    EffParam param;
    memset(&param, 0, sizeof(param));   /* PC: the ROM leaves the rest stack garbage */
    param.m_kind = 0x26;
    param.m_08 = 0;
    param.m_muzzle.m_pos[0] = m[3][0];
    param.m_muzzle.m_pos[1] = m[3][1];
    param.m_muzzle.m_pos[2] = m[3][2];
    m_scene->m_eff_man.createEff(&param);
}

// ROR 0x0012cdb0

void MagLifePC::message14()
{
    ROR_ASSERT(0); /* 118 */  // callcheck: dbgAssertFailed (the assert handler)
}

// ROR 0x0012cddc

void MagLifePC::message15()
{
    ROR_ASSERT(0); /* 140 */  // callcheck: dbgAssertFailed (the assert handler)
}

// ROR 0x0012ce08

void MagLifePC::message16()
{
    ROR_ASSERT(0); /* 175 */  // callcheck: dbgAssertFailed (the assert handler)
}

// ROR 0x0012ce34

void MagLifePC::message17()
{
}
