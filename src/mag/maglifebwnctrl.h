/* ==========================================================================
 *  mag/maglifebwnctrl.h
 *
 *  MagLifeBwnCtrl (mag/maglifebwnctrl.cpp; vtable _vt$14MagLifeBwnCtrl
 *  0x2e13e0, base MagLife, vptr at +0x14, constructor 0x12ba80): Brown's
 *  command layer over MagLifeBwn. The scene holds one at +0x2bd20 (EE
 *  0x130: the scene's menu words follow at +0x2be50); its state word is what
 *  MagSystem::getBwnState reads. Minimal: what the scene touches. EE 0x130:
 *  the words at +0x130 / +0x138 an earlier session gave it are the scene's
 *  menu id and menu result (+0x2be50 / +0x2be58, MagScene::closeMenu).
 * ======================================================================== */
#ifndef _MAG_MAGLIFEBWNCTRL_H
#define _MAG_MAGLIFEBWNCTRL_H

#include <stddef.h>

#include "mag/maglife.h"
#include "utl/utlfind.h"

struct BedPad;
struct MagLifeBwn;
struct MagLifePC;

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifeBwnCtrl : MagLife
{
    /* EE slot 2 (+0x10) */
    void live() override;                                                // ROR 0x0012a61c
    /* EE slot 9 (+0x48): empty. */
    void processPad(BedPad *pad) {}                                      // ROR 0x0012a6a0
    /* Empty (MagScene::resetEntities). */
    void reset() {}                                                      // ROR 0x0012ab18

    /* Brown's states (m_state; magBwnGetStateCode's four-char codes):
     * 0 NULL, 1 WAIT, 2 FOLW, 3 STAY, 4 FIND, 5 FDED, 6 MOVE, 7 MVED,
     * 8 ROTA, 9 RTED, 10 DAMG, 11 BARK, 12 HUG. */
    void requestState(int state, int arg, int force);                    // ROR 0x0012a56c
    /* requestState(10, react). */
    void requestHurt(int react);                                         // ROR 0x0012a6a8
    int  isHurt();                                                       // ROR 0x0012a6cc
    /* State 11: the bark sub object's answer for v (1 / -1); -1 otherwise. */
    int  barkQuery(int v);                                               // ROR 0x0012a6dc
    void barkSet(int v);                                                 // ROR 0x0012a71c
    /* m_find_item = item, the finder set for item >= 0; request: state 4
     * (again: the finder's flag), else m_78 = 0. */
    void setFindItem(int item, int request, int again);                  // ROR 0x0012a758
    /* State 4 / 5 back to 1. */
    void cancelFind();                                                   // ROR 0x0012a7cc
    /* Brown's actor position / rotation (vec4), each then requestState(0). */
    void setPos(const float *v);                                         // ROR 0x0012a808
    void setRot(const float *v);                                         // ROR 0x0012a89c
    void getPos(float *out);                                             // ROR 0x0012a974
    void getRot(float *out);                                             // ROR 0x0012a9ec
    /* Walks to xyz (state 6). */
    void moveTo(const float *xyz);                                       // ROR 0x0012aa70
    /* The angle word +0x40, then state 0 / state 8 (turn to it). */
    void setAngle(float a);                                              // ROR 0x0012aac0
    void rotateTo(float a);                                              // ROR 0x0012aae8
    int  isStateDone();                                                  // ROR 0x0012ab10
    /* theLevel MAG_BWNCTRL_LEVEL_AUTO (0) .. MAG_BWNCTRL_LEVEL_MANUAL (2). */
    void setLevel(int theLevel);                                         // ROR 0x0012b7cc
    int  isBarkPhase3();                                                 // ROR 0x0012ba54
    /* The PC's dog orders (MagLifePC::orderDog passes 0). */
    void orderCome(int a1);                                              // ROR 0x0012b4a0
    void orderStay(int a1);                                              // ROR 0x0012b5f8
    void orderGo(int a1);                                                // ROR 0x0012b65c
    /* requestState(12 HUG, arg, 1); word +0x124 = arg when arg >= 0. */
    void requestHug(int arg);                                            // ROR 0x0012b788
    /* > 0 when the hug can go on: phase 0 (start) or 1 / 2 (Brown's
     * distance and heading to the PC); -3 without Brown, -2 in state 3. */
    int  checkHug(int phase);                                            // ROR 0x0012b870
    /* Brown's position into out (untouched while Brown is unbound). */
    void getBwnPosition(float *out);                                     // ROR 0x0012ba2c

    u_char      _28[0x44 - 0x28];
    int         m_find_item;    // ROR 0x44: setFindItem's item (op gfit reads it)
    UtlFind     m_find;         // ROR 0x48: cleared by MagScene::clearPC and termWorld
    u_char      _60[0x68 - 0x60];
    int         m_found_oid;    // ROR 0x68: read while m_find_result is set (op ifsc)
    int         m_ctrl_level;   // ROR 0x6c: setLevel's MAG_BWNCTRL_LEVEL
    MagLifeBwn *m_bwn;          // ROR 0x70: Brown's controller while Brown is bound (MagScene::setBwn)
    MagLifePC  *m_pc;           // ROR 0x74: the PC's controller while Brown is bound
    int         m_78;           // ROR 0x78: MagScene::clearPC zeroes it; role unknown
    int         m_find_result;  // ROR 0x7c: op gfrs
    int         m_state_done;   // ROR 0x80: isStateDone
    u_char      _84[0x130 - 0x84];
};
static_assert(offsetof(MagLifeBwnCtrl, m_find_item) >= 0x44, "m_find_item");
static_assert(offsetof(MagLifeBwnCtrl, m_find) >= 0x48, "m_find");
static_assert(offsetof(MagLifeBwnCtrl, m_found_oid) >= 0x68, "m_found_oid");
static_assert(offsetof(MagLifeBwnCtrl, m_bwn) >= 0x70, "m_bwn");
static_assert(offsetof(MagLifeBwnCtrl, m_find_result) >= 0x7c, "m_find_result");
static_assert(sizeof(MagLifeBwnCtrl) >= 0x130, "MagLifeBwnCtrl is 0x130 bytes on the EE");

#endif /* _MAG_MAGLIFEBWNCTRL_H */
