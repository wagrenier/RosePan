// mag/maglife.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/maglife.h"

#include <string.h>

#include "dog/dogactor.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"

/* PC: written by the mag/maglifepc.cpp session (2026-10-27) because
 * MagLifePC cannot hold a state without them: the constructor and the
 * four small state helpers below. The rest of the file is still stubs. */

// ──────────────────────────────────────────────────────────────────────
// The bound-actor controller's constructor (MagLifePC and MagLifeBwn run
// it first). MagLife0's part is inlined: level 1, the state quad cleared,
// m_04 -1.
// ROR 0x0011a888
MagLifeControlChara::MagLifeControlChara()
{
    m_level = 1;
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    for (int i = 0; i < 4; i++)
        m_prev_state[i] = 0;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    m_seq_no = 0;
    m_seq_count = 0;
    m_48 = 0;
    m_4c = 0;
    m_no = -1;
    m_secta[0] = NULL;
    m_mode = 0;
    m_actor = NULL;
    m_74 = 0;
    m_78 = 0;
    clearMotionSeq();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 9: enters state with arg. A running state (m_state != 0) is
// saved first, the whole quad m_state..m_0c (MagLifePC::restoreState
// brings it back).
// ROR 0x00119fe0
void MagLifeControlChara::setState(int state, int arg)
{
    if (m_state != 0)
    {
        m_prev_state[0] = m_state;
        m_prev_state[1] = m_04;
        m_prev_state[2] = m_08;
        m_prev_state[3] = m_0c;
    }
    m_08 = arg;
    m_state = state;
    m_04 = -1;
}

// ROR 0x00119b34
void MagLifeControlChara::clearGoal()
{
    m_70 = -1;
    memset(m_secta, 0, sizeof(m_secta));
}

// ROR 0x0011a130
void MagLifeControlChara::clearMotionSeq()
{
    m_seq_no = -1;
    m_seq_count = -1;
}

// ROR 0x0011a144
void MagLifeControlChara::setMotionSeqTable(const MagLifeMotionSeq *table)
{
    m_motion_seq = table;
}

/* PC: the motion sequence's small accessors, written by the
 * mag/maglifepcmode.cpp session (2026-10-27) with the modes that ask them;
 * changeMotionSeq itself is still a stub. */

// ROR 0x00119f88
void MagLifeControlChara::getPosition(float *out)
{
    DogActor *actor = m_actor;
    if (actor)
    {
        out[0] = actor->m_trans[0];
        out[1] = actor->m_trans[1];
        out[2] = actor->m_trans[2];
        out[3] = actor->m_trans[3];
    }
}

// ROR 0x00119fb0
float MagLifeControlChara::getRotY()
{
    if (m_actor)
        return m_actor->getRotY();
    return 0.0f;
}

// ROR 0x0011a010
void MagLifeControlChara::setMotionSeqNo(int no)
{
    m_seq_no = no;
    m_seq_count = 0;
    m_48 = 0;
    m_4c = 0;
}

// ROR 0x0011a028
int MagLifeControlChara::getMotionSeq()
{
    return m_seq_no;
}

// ROR 0x0011a054
int MagLifeControlChara::isMotionSeqEnd()
{
    return 1 < m_48;
}

// PC: the two secta helpers below were written by the mag/maglifebwn.cpp
// session (2026-10-27): Brown's path search and the PC's moveBy need them.

// ──────────────────────────────────────────────────────────────────────
// The secta cells under the actor (radius 40), searched from the cells
// held when the first one is set.
// ROR 0x0011a6b0
void MagLifeControlChara::updateSectaCells()
{
    float pos[4];
    for (int i = 0; i < 4; i++)
        pos[i] = m_actor->m_trans[i];
    FUN_0017c340(&m_scene->m_8542c, pos, m_secta, m_secta[0] != NULL, 40.0f, 0.0f);
}

// ──────────────────────────────────────────────────────────────────────
// m_secta, queried first when it holds no cell.
// ROR 0x0011a64c
GamSecta **MagLifeControlChara::getSectaCells()
{
    if (m_secta[0] == NULL)
    {
        float pos[4];
        for (int i = 0; i < 4; i++)
            pos[i] = m_actor->m_trans[i];
        FUN_0017c340(&m_scene->m_8542c, pos, m_secta, 0, 40.0f, 0.0f);
    }
    return m_secta;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0011a14c
void MagLifeControlChara::updateMotionSeq()
{
    ROSEPAN_STUB("MagLifeControlChara::updateMotionSeq", 0x0011a14c);
}

// stub of ROR 0x0011a738
void MagLifeControlChara::playStepSe(const int *table)
{
    ROSEPAN_STUB("MagLifeControlChara::playStepSe", 0x0011a738);
}

// stub of ROR 0x0011a320
void MagLifeControlChara::changeMotionSeq(int theState, int blend)
{
    ROSEPAN_STUB("MagLifeControlChara::changeMotionSeq", 0x0011a320);
}

// stub of ROR 0x0011a27c
void MagLifeControlChara::changeMotionSeqAlt(int theState, int blend)
{
    ROSEPAN_STUB("MagLifeControlChara::changeMotionSeqAlt", 0x0011a27c);
}

// stub of ROR 0x00119e1c
float MagLifeControlChara::followGround()
{
    ROSEPAN_STUB("MagLifeControlChara::followGround", 0x00119e1c);
    return {};
}
