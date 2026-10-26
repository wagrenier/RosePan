/* ==========================================================================
 *  utl/utlbtl.h
 *
 *  The hit checks (utl/utlbtl.cpp, the source string at 0x2e75f8; the
 *  asserts name the record theHitCheck and its m_origin_pos /
 *  m_target_pos). Declared for the task/tsk_chr.cpp range tests; not
 *  written yet.
 * ======================================================================== */
#ifndef _UTL_UTLBTL_H
#define _UTL_UTLBTL_H

/* One hit check (EE 0x14 bytes). The positions carry their radius in w.
 * m_dir NULL tests the spheres; else a sweep of m_origin_pos along m_dir
 * (normalised, scaled by m_len). m_10 non-zero picks the other test of
 * each kind. layout: host -- holds pointers. */
struct UtlHitCheck
{
    const float *m_origin_pos;  // ROR 0x00
    const float *m_dir;         // ROR 0x04
    float        m_len;         // ROR 0x08
    const float *m_target_pos;  // ROR 0x0c
    float        m_10;          // ROR 0x10
};

/* Non-zero when the check hits. */
int utlHitCheck(UtlHitCheck *theHitCheck);                               // ROR 0x0010456c

#endif /* _UTL_UTLBTL_H */
