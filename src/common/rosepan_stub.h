/* ==========================================================================
 *  common/rosepan_stub.h
 *
 *  Marks a Rule of Rose function that is called by reconstructed code but not
 *  written yet. The body logs once (name + RoR address) and returns, so the
 *  build links and a run shows exactly which missing pieces were reached.
 *
 *  tools/reverse/stubgen.py writes these; tools/reverse/status.py counts them.
 *  Replace the whole definition when the function is reconstructed.
 * ======================================================================== */
#ifndef _COMMON_ROSEPAN_STUB_H
#define _COMMON_ROSEPAN_STUB_H

#include <stdio.h>

#define ROSEPAN_STUB(name, ror_addr)                                             \
    do {                                                                         \
        static int rosepan_stub_seen_;                                           \
        if (!rosepan_stub_seen_) {                                               \
            rosepan_stub_seen_ = 1;                                              \
            printf("[stub] %s (ROR 0x%08x) reached\n", name, (unsigned)(ror_addr)); \
        }                                                                        \
    } while (0)

/* A temporary stand-in that works but is not RoR's code (platform-side
 * behaviour so later TUs can run). The tools count it exactly like a stub --
 * still to reconstruct, listed for deletion in its TU's brief, ranked by
 * smoke-run hits -- and the log line says so. */
#define ROSEPAN_BRIDGE(name, ror_addr)                                           \
    do {                                                                         \
        static int rosepan_stub_seen_;                                           \
        if (!rosepan_stub_seen_) {                                               \
            rosepan_stub_seen_ = 1;                                              \
            printf("[stub] %s (ROR 0x%08x) reached (bridge)\n", name,            \
                   (unsigned)(ror_addr));                                        \
        }                                                                        \
    } while (0)

#endif /* _COMMON_ROSEPAN_STUB_H */
