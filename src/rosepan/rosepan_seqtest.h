/* ==========================================================================
 *  rosepan/rosepan_seqtest.h
 *
 *  PC test (no ROM counterpart): with ROSEPAN_SEQ_TEST=1, loads the system
 *  script image (the "ELFR" file of DATA/SEQ/G000.GRB) and the boot stage's
 *  (DATA/SEQ/W099/W000.WRD) into the sequence VM once, as GamMan's
 *  loadSystemData / loadStage(99) will, and logs the threads (pc as an
 *  image offset) so a smoke run shows how far the scripts get before
 *  GamMan's loads are written. Called from Sequence::exec.
 * ======================================================================== */
#ifndef ROSEPAN_SEQTEST_H
#define ROSEPAN_SEQTEST_H

void RosePan_SeqForcedTest(void);

#endif /* ROSEPAN_SEQTEST_H */
