// sequence/sequence.cpp -- the event sequence system (0x20e3d8..0x20ff34).
//
// Written so far: the data folder name and the global sequence's path, at
// the head of the file (0x20e3d8..0x20e454). The Sequence constructor /
// destructor / term and the image loader / per-frame exec are still stubs.

#include <stdio.h>

#include "common/rosepan_stub.h"
#include "sequence/sequence.h"

// The sequence data folder (data 0x2d9910, 0x40 bytes).
static char s_dataDir[0x40];

// ──────────────────────────────────────────────────────────────────────
// The US build has one folder whatever the language: both arguments are
// unused.
// ROR 0x0020e3d8

void seqSetLanguage(int lang, int arg)
{
    (void)lang;
    (void)arg;
    snprintf(s_dataDir, sizeof(s_dataDir), "SEQ");
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020e400

char *seqGetDataDir(void)
{
    return s_dataDir;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020e40c

char *seqMakeGlobalPath(char *buf, int size, int kind)
{
    if (kind == 0)
        snprintf(buf, size, "data/seq/seq_global/g000.seq");
    else
        snprintf(buf, size, "data/SEQ/G000.GRB");
    return buf;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0020f98c
void Sequence::exec()
{
    ROSEPAN_STUB("Sequence::exec", 0x0020f98c);
}
