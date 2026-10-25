/* rosepan/rosepan_seqtest.cpp -- see rosepan_seqtest.h. */

#include "rosepan/rosepan_seqtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "rosepan/io/rosepan_log.h"
#include "rosepan/io/rosepan_paths.h"
#include "sequence/sequence.h"

/* The script packs are RTPK packs (bed/bedpfh.h): file count at +0xe, the
 * offsets table at +0x20 (flag 2). The image is the file starting
 * "ELFR". */
static const u_char *FindImage(const std::vector<u_char> &pack, size_t *size)
{
    if (pack.size() < 0x20 || memcmp(pack.data(), "RTPK", 4) != 0)
        return NULL;
    unsigned short count;
    memcpy(&count, &pack[0xe], sizeof(count));
    for (int i = 0; i < count; i++) {
        u_int offset, next = (u_int)pack.size();
        memcpy(&offset, &pack[0x20 + i * 4], sizeof(offset));
        if (i + 1 < count)
            memcpy(&next, &pack[0x20 + (i + 1) * 4], sizeof(next));
        if (offset + 4 <= pack.size() && memcmp(&pack[offset], "ELFR", 4) == 0) {
            *size = next - offset;
            return &pack[offset];
        }
    }
    return NULL;
}

/* The ELFR file of pack name into a zeroed buffer of buffer bytes. */
static bool LoadImage(const char *name, size_t buffer, std::vector<u_char> *out)
{
    char path[1024];
    FILE *f = NULL;
    if (RosePan_PathResolveData(name, path, sizeof(path)))
        f = fopen(path, "rb");
    if (f == NULL) {
        RosePan_LogPrintf("seq test: %s not found\n", name);
        return false;
    }
    fseek(f, 0, SEEK_END);
    const long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<u_char> pack((size_t)fsize);
    const size_t got = fread(pack.data(), 1, (size_t)fsize, f);
    fclose(f);
    size_t size = 0;
    const u_char *image = got == (size_t)fsize ? FindImage(pack, &size) : NULL;
    if (image == NULL || size > buffer) {
        RosePan_LogPrintf("seq test: no ELFR image in %s\n", name);
        return false;
    }
    out->assign(buffer, 0);
    memcpy(out->data(), image, size);
    RosePan_LogPrintf("seq test: %s image (0x%x bytes) loaded\n", name, (u_int)size);
    return true;
}

void RosePan_SeqForcedTest(void)
{
    static int s_enabled = -1;
    static int s_frame;
    static std::vector<u_char> s_system;
    static std::vector<u_char> s_stage;

    if (s_enabled < 0) {
        const char *env = getenv("ROSEPAN_SEQ_TEST");
        s_enabled = env != NULL && env[0] == '1';
        if (!s_enabled)
            return;
        /* loadSystemImage / loadStageImage copy 0x20000 / 0x18000 bytes. */
        if (!LoadImage("DATA/SEQ/G000.GRB", 0x20000, &s_system)) {
            s_enabled = 0;
            return;
        }
        g_sequence.loadSystemImage(s_system.data());
        if (LoadImage("DATA/SEQ/W099/W000.WRD", 0x18000, &s_stage))
            g_sequence.loadStageImage(s_stage.data());
    }
    if (!s_enabled)
        return;

    s_frame++;
    if (s_frame > 600 || (s_frame > 8 && s_frame % 60 != 0))
        return;
    RosePan_LogPrintf("seq test: frame %d, started %d, ended %d\n", s_frame,
                      g_sequence.m_start.m_used.m_count, g_sequence.m_ended.m_used.m_count);
    for (int image = 0; image < 3; image++) {
        SeqPool<SeqThread> *pool = g_sequence.m_threads[image];
        if (pool == NULL)
            continue;
        for (SeqNode<SeqThread> *it = pool->m_used.begin(); it != pool->m_used.end(); it = it->m_next)
            RosePan_LogPrintf("seq test:   image %d id 0x%08x pc +0x%x flags 0x%x\n", image,
                              it->m_data.m_id,
                              (u_int)(it->m_data.m_pc - (intptr_t)g_sequence.m_image[image]),
                              it->m_data.m_flags);
    }
}
