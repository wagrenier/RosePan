/* ==========================================================================
 *  utl/utlbtl.h
 *
 *  The battle checks (utl/utlbtl.cpp, the source string at 0x2e75f8; the
 *  asserts name the record theHitCheck and its m_origin_pos /
 *  m_target_pos, and the sight record's m_pos_self / m_pos_aim /
 *  m_pos_target). utlHitCheck serves the shot ray (mag/maglifepcbtl.cpp)
 *  and the range tests (task/tsk_chr.cpp); the sight checks have no
 *  callers in the ROM.
 * ======================================================================== */
#ifndef _UTL_UTLBTL_H
#define _UTL_UTLBTL_H

/* One hit check (EE 0x14 bytes). The positions carry their radius in w.
 * m_dir NULL tests the spheres; else a sweep of m_origin_pos along m_dir
 * (normalised, scaled by m_len). m_10 non-zero stretches the target into a
 * cylinder of that height on y. layout: host -- holds pointers. */
struct UtlHitCheck
{
    const float *m_origin_pos;  // ROR 0x00
    const float *m_dir;         // ROR 0x04
    float        m_len;         // ROR 0x08
    const float *m_target_pos;  // ROR 0x0c
    float        m_10;          // ROR 0x10
};

/* One sight check (EE 0x28 bytes, cleared by utlInitSightCheck): does the
 * one at m_pos_self, facing m_pos_aim, see m_pos_target? m_type picks the
 * range and the test: a facing dot, or a cone whose half-width at the
 * target's distance (distance * tan(angle)) must stay within m_width.
 * Words 0x00 and 0x1c..0x27 are never read. layout: host -- holds
 * pointers. */
struct UtlSightCheck
{
    int          m_00;          // ROR 0x00
    int          m_type;        // ROR 0x04
    int          m_range;       // ROR 0x08
    const float *m_pos_self;    // ROR 0x0c
    const float *m_pos_target;  // ROR 0x10
    const float *m_pos_aim;     // ROR 0x14
    float        m_width;       // ROR 0x18
    int          m_1c[3];       // ROR 0x1c
};

/* Clears the record (memset of its 0x28 bytes). */
void utlInitSightCheck(UtlSightCheck *theHitCheck);                      // ROR 0x0010452c
/* Clears the record (memset of its 0x14 bytes). */
void utlInitHitCheck(UtlHitCheck *theHitCheck);                          // ROR 0x0010454c
/* Non-zero when the check hits. */
int utlHitCheck(UtlHitCheck *theHitCheck);                               // ROR 0x0010456c
/* 1 when the target is in sight, by m_type 0..3. */
int utlSightCheck(UtlSightCheck *theHitCheck);                           // ROR 0x001046b4
/* The same test for types 0 and 0x100, without the asserts. */
int utlSightCheck2(UtlSightCheck *theHitCheck);                          // ROR 0x00104970
/* The same test for types 0 and 6. */
int utlSightCheck3(UtlSightCheck *theHitCheck);                          // ROR 0x00104ad0
/* The same test for types 0, 2, 4 and 5. */
int utlSightCheck4(UtlSightCheck *theHitCheck);                          // ROR 0x00104c44

#endif /* _UTL_UTLBTL_H */
