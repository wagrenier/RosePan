/* ==========================================================================
 *  emy/emy_clr.h
 *
 *  emy/emy_clr.cpp (0x14dc7c..0x150b24): ClrMain, enemy type 8, and its
 *  per-state outs ClrOut*. No source string names the file; it is the run
 *  from ClrMain::initOut to NorMain::initOut (every enemy file opens with
 *  its initOut), named like emy_bds.cpp. The ELF's linkonce symbols name
 *  the classes, their constructors and GetType.
 *
 *  Unlike the walking enemies, ClrMain is a fixed stage enemy that drops
 *  in, attacks and climbs away along y (y grows downward: 0 / -52.653 /
 *  -97.346 are its three floor heights, -347.346 out of sight above).
 *  States (m_out index, initOut): 1 Move (pick a spot, drop down to it),
 *  2 Atk (three TskAttack attacks or a turning arm swing), 3 and 5 Up
 *  (climb out of sight), 4 DeathUp (killed: climb out). ClrOutWin has a
 *  vtable and code but no initOut builds it. The outs share ClrMain's
 *  m_phase. Its spit leaves hit points (m_hit) that hurt the PC or the dog
 *  standing in them for a while.
 *  Decide inputs: 0 / 1 the PC came into sight, 2 lost from sight, 3..8 the
 *  PC still in sight and the base tests (class 1 twice, ahead twice, class 7
 *  off axis twice), 9..13 never (their tests return 0). The outs hand back
 *  0xd (Up, Move and Atk done).
 * ======================================================================== */
#ifndef _EMY_EMY_CLR_H
#define _EMY_EMY_CLR_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* A damaging spot left by the spit (EE 0x20, ClrMain +0x274 x 64): after
 * m_wait frames (its SE when it turns live) it hurts the PC or the dog
 * within m_radius of m_pos for m_life frames, once. */
struct ClrHitPoint
{
    int   m_used;               // ROR 0x00: 0 a free record
    int   m_wait;               // ROR 0x04: frames until live
    int   m_life;               // ROR 0x08: frames live
    int   m_loud;               // ROR 0x0c: its SE whatever the slot (else only every 20th slot)
    float m_radius;             // ROR 0x10
    float m_pos[3];             // ROR 0x14
};
static_assert(sizeof(ClrHitPoint) == 0x20, "ClrHitPoint");
static_assert(offsetof(ClrHitPoint, m_radius) == 0x10, "ClrHitPoint::m_radius");
static_assert(offsetof(ClrHitPoint, m_pos) == 0x14, "ClrHitPoint::m_pos");

/* Type 8 (vtable 0x2e2640, vptr at +0x200, EE 0xa80). layout: host --
 * polymorphic; EE offsets in the comments. */
class ClrMain : public EmyMain
{
public:
    ClrMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00287244
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~ClrMain();                                                          // EE slot 1 (+0x08), ROR 0x0014de54
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014e224
    int  GetType() { return 8; }                                         // EE slot 3 (+0x18), ROR 0x0028727c
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0014deb4
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014e0b4
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014e16c
    /* The outs, the motion list, the start height and the hit points. */
    virtual void initOut();                                              // EE slot 7 (+0x38), ROR 0x0014dc7c
    /* decide's tests: 3 / 4 the PC's state class 1, 5 / 6 the PC ahead,
     * 7 / 8 the PC in class 7 off axis (EmyMain's), 9..13 never. */
    int  testInput3();                                                   // ROR 0x0014e2ec
    int  testInput4();                                                   // ROR 0x0014e300
    int  testInput5();                                                   // ROR 0x0014e314
    int  testInput6();                                                   // ROR 0x0014e328
    int  testInput7();                                                   // ROR 0x0014e33c
    int  testInput8();                                                   // ROR 0x0014e350
    int  testInput13();                                                  // ROR 0x0014e364
    int  testInput12();                                                  // ROR 0x0014e36c
    int  testInput11();                                                  // ROR 0x0014e374
    int  testInput10();                                                  // ROR 0x0014e37c
    int  testInput9();                                                   // ROR 0x0014e384
    /* emyChkInSight(0, 3 pi / 4) of the PC. */
    int  isPCInSight();                                                  // ROR 0x0014e38c
    /* A free m_hit record from m_hit_next on (then from 0): its index, -1
     * when all 64 are used. */
    int  addHitPoint(float radius, const float *pos, int wait, int life, int loud); // ROR 0x0014e3d0
    /* Counts the records down and hits the PC (bit 1) or the dog (bit 2)
     * standing in a live one (chr word 0x32); returns the bits. */
    u_int updateHitPoints();                                             // ROR 0x0014e4d4

    u_char m_pc_seen;           // ROR 0x204: decide: the PC is in sight; exec clears it on a change
    /* selectOut: the current state's row has the input (decide reads
     * 0..13); EE 0x208..0x240, as many words as the table has inputs.
     * PC: EE 0x238 (EmyMain::m_238 on the host) is m_input[12] here. */
    int    m_input[14];         // ROR 0x208
    u_char m_244;               // ROR 0x244: initOut clears it
    int    m_hold;              // ROR 0x248: 1 once a Move started (EmyManager::getEmyHoldWord248)
    int    m_hit_count;         // ROR 0x24c: hits taken in this state (exec); Atk's chance to give up
    float  m_base_pos[4];       // ROR 0x250: the start position at height 1
    int    m_phase;             // ROR 0x260: the outs' shared step (5..7 Up, 0..5 / 8 Move)
    int    m_height;            // ROR 0x264: the floor a Move drops to: 0 at 0, 1 -52.653, 2 -97.346
    int    m_268;               // ROR 0x268: 4 when a drop starts
    int    m_wait;              // ROR 0x26c: utlRandRange(90, 210), then (30, 150) after a climb
    int    m_hit_next;          // ROR 0x270: addHitPoint's search start
    ClrHitPoint m_hit[64];      // ROR 0x274
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* States 3 and 5 (vtable 0x2e2688, EE 0xe0): climbs 4 a frame out of sight
 * (to y -347.346, then parked at x / z -800), hand back 0xd. layout:
 * host -- polymorphic. */
class ClrOutUp : public EmyOut
{
public:
    ClrOutUp(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287468
    ~ClrOutUp() {}                                                       // EE slot 1, ROR 0x002873b8
    void Init();                                                         // EE slot 2, ROR 0x0014e73c
    int  Exec();                                                         // EE slot 3, ROR 0x0014e750
    void Term();                                                         // EE slot 4, ROR 0x0014e834
    /* The climb; start restarts it. True once out of sight. */
    bool execUp(int start);                                              // ROR 0x0014e850

    float m_pos[4];             // ROR 0x40: the achr's position while it climbs
    int   m_90;                 // ROR 0x90: cleared at the start, never read
    float m_count;              // ROR 0x94: frames (a float in the ROM)
    int   m_up_step;            // ROR 0x9c: 0 start, 1 climb, 2 wait, 3, 9999 done
    int   m_hit_seen;           // ROR 0xa8: hit while climbing: decide stays off
    u_int m_prop;               // ROR 0xac: g_sequence.m_global[0x1e2], the object it drags (motion 2)
};

/* State 4 (vtable 0x2e26b8, EE 0xb0): killed: climbs 2 a frame out of
 * sight. layout: host -- polymorphic. */
class ClrOutDeathUp : public EmyOut
{
public:
    ClrOutDeathUp(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287548
    ~ClrOutDeathUp() {}                                                  // EE slot 1, ROR 0x00287498
    void Init();                                                         // EE slot 2, ROR 0x0014eb54
    int  Exec();                                                         // EE slot 3, ROR 0x0014ebcc
    void Term();                                                         // EE slot 4, ROR 0x0014ecf4
    bool execUp(int start);                                              // ROR 0x0014ed10

    float m_pos[4];             // ROR 0x40
    int   m_90;                 // ROR 0x90: cleared at the start, never read
    int   m_up_step;            // ROR 0x9c: 0 start, 1 climb, 2, 9999 done
};

/* State 1 (vtable 0x2e26e8, EE 0x100): picks a spot (random, or near the
 * PC), waits with its voice, spits (effects 0xa / 0xc and a hit point),
 * drops down to m_floor_y hitting what it touches, hands back 0xd.
 * layout: host -- polymorphic. */
class ClrOutMove : public EmyOut
{
public:
    ClrOutMove(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287628
    ~ClrOutMove() {}                                                     // EE slot 1, ROR 0x00287578
    void Init();                                                         // EE slot 2, ROR 0x0014ee40
    int  Exec();                                                         // EE slot 3, ROR 0x0014ee9c
    void Term();                                                         // EE slot 4, ROR 0x0014f220
    /* The achr's rot y becomes the PC's direction less its rot y (the
     * caller zeroed it first). Returns 0. */
    int  turnToPC(int arg);                                              // ROR 0x0014f23c
    /* The drop; start restarts it. True once landed and motion 9 over. */
    bool execDown(int start);                                            // ROR 0x0014f360
    /* Effects 0xa (from m_eff_pos up) and 0xc at the spot, a hit point. */
    void createDropEff();                                                // ROR 0x0014f72c

    float m_pos[4];             // ROR 0x40
    float m_start[4];           // ROR 0x50: the position the drop starts from
    float m_rot[4];             // ROR 0x70: only y is set (0)
    int   m_90;                 // ROR 0x90: cleared at the start, never read
    float m_count;              // ROR 0x94: frames
    float m_floor_y;            // ROR 0x98: the drop's end (ClrMain::m_height)
    int   m_down_step;          // ROR 0x9c: 0 start, 1 drop, 2 land, 9999 done
    int   m_hit_pc;             // ROR 0xa0: the PC was hit this drop
    int   m_hit_dog;            // ROR 0xa4: the dog was hit
    int   m_wait;               // ROR 0xa8: utlRandRange(60, 150) before the spit
    int   m_voice;              // ROR 0xac: playSE's id (stopped at the end)
    u_int m_prop;               // ROR 0xb0: g_sequence.m_global[0x1e2]
    float m_eff_pos[4];         // ROR 0xd8: effect 0xa's position (the spot)
    float m_eff_dir[4];         // ROR 0xe8: effect 0xa's direction (0, 1, 0)
};

/* State 2 (vtable 0x2e2718, EE 0x19c): one of three TskAttack attacks
 * (kinds 0x2e..0x30, motions 0x12..0x14; 0x12 / 0x13 spit along p_eff /
 * p_gero3) or, when hurt, an arm swing turning toward the PC's side
 * (motions 3 / 4); hits make it give up (0xd). layout: host --
 * polymorphic. */
class ClrOutAtk : public EmyOut
{
public:
    ClrOutAtk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287708
    ~ClrOutAtk() {}                                                      // EE slot 1, ROR 0x00287658
    void Init();                                                         // EE slot 2, ROR 0x0014f88c
    int  Exec();                                                         // EE slot 3, ROR 0x0014f8e8
    void Term();                                                         // EE slot 4, ROR 0x00150a24
    /* The next attack (or give up: 0xd); first: Init's call. */
    int  selectAttack(int first);                                        // ROR 0x0014febc
    /* The spit's effects along nodes p_eff / p_gero3 and its hit points. */
    void emitSpit();                                                     // ROR 0x00150290
    /* 0 the PC within pi / 3 ahead, 1 to the left (angle <= 0), 2 right. */
    int  getPCSide();                                                    // ROR 0x001506a0
    /* The swinging arm's node within target's radius + 32 of its spine. */
    bool isArmTouching(u_int target);                                    // ROR 0x00150804
    /* Returns 0; Exec calls it last with 1. */
    int  execExtra(int arg);                                             // ROR 0x00150a1c

    float m_eff_pos[10][4];     // ROR 0x38: p_eff's position per spit frame (effect 0xa reads it)
    float m_eff_dir[10][4];     // ROR 0xd8: p_eff's z axis per spit frame
    int   m_count;              // ROR 0x178: step 2's count down, step 3's spit frames
    int   m_time;               // ROR 0x17c: step 3's frames (120 / 300)
    int   m_attack;             // ROR 0x180: the attack picked (3, 4, 0x12..0x14)
    int   m_last;               // ROR 0x184: the previous one, -1 none
    float m_turn;               // ROR 0x188: the swing's turn a frame (+-0.0952)
    int   m_frame;              // ROR 0x18c: the swing's frames
    int   m_hit_pc;             // ROR 0x190
    int   m_hit_dog;            // ROR 0x194: never set (the dog's hit sets m_hit_pc)
    int   m_right;              // ROR 0x198: the swing uses the right arm
};

/* No state (vtable 0x2e2748, EE 0x3c): initOut never builds it. Chases the
 * target, plays motion 0x15 once within a random range. layout: host --
 * polymorphic. */
class ClrOutWin : public EmyOut
{
public:
    ClrOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002877e8
    ~ClrOutWin() {}                                                      // EE slot 1, ROR 0x00287738
    void Init();                                                         // EE slot 2, ROR 0x00150a40
    int  Exec();                                                         // EE slot 3, ROR 0x00150a90
    void Term();                                                         // EE slot 4, ROR 0x00150b08

    float m_range;              // ROR 0x38: utlRandRangeF(100, 200) at Init
};

#endif /* _EMY_EMY_CLR_H */
