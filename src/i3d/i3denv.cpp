/* ==========================================================================
 *  i3d/i3denv.cpp
 *
 *  The i3d frame environment (ROR 0x21a120..0x21a3e8, the fog, and
 *  0x21ac80..0x21b220, the lights): fog records into FOGCOL, a fog ramp
 *  and a VU1 packet; light descriptions into lights in the context tail.
 *  See i3denv.h. One file for both is a guess (the i3r helpers
 *  0x21a3e8..0x21ac80 sit between them in the ROM).
 *
 *  PORT: the fog packet carries the ROM's VIF / GIF codes; the host's DMA
 *  does not follow chains and nothing on the host consumes them yet (no GS
 *  emulation), so it is kept for i3dPktCallFog's address and parity.
 * ======================================================================== */
#include "i3d/i3denv.h"

#include <math.h>
#include <string.h>

#include "lib/unassigned.h"

/* The spot cone's half-angle limits (data 0x302548 / 0x302554) and the
 * share of the outer angle the inner one may reach (0x30254c). */
#define I3D_CONE_MAX   1.5697963f       /* pi / 2 - 0.001 */
#define I3D_CONE_MIN   0.001f
#define I3D_CONE_INNER 0.95f

/* VIF codes of the fog packet. */
#define I3D_VIF_STCYCL_1_1   0x01000101u        /* STCYCL cl 1 wl 1 */
#define I3D_VIF_UNPACK_V4_32 0x6c008000u        /* UNPACK V4-32 to TOPS + 0, NUM in bits 16.. */
#define I3D_VIF_MSCAL_1      0x14000001u        /* MSCAL 1 (micro address 8) */
#define I3D_DMA_RET          0x60000000u        /* tag id 6 */

/* The VU1 buffer commands of the fog packet (roles from their use here,
 * the microprogram is not read yet): 0x11 kicks the next (word 1) + 1
 * qwords to the GS, 0x190 copies the next ones to the VU address in its
 * second dword, 0x0a ends the buffer. */
#define I3D_VU1_KICK   0x11u
#define I3D_VU1_COPY   0x190u
#define I3D_VU1_END    0x0au

#define I3D_GS_FOGCOL  0x3dull
#define I3D_GS_AD      0x0eull

// ──────────────────────────────────────────────────────────────────────
// fog
// ──────────────────────────────────────────────────────────────────────

// ROR 0x0021a120
void i3dFogInit(I3dFog *fog)
{
    i3dVecZero(fog->m_color);
    fog->m_param[1] = 0.0f;
    fog->m_param[2] = 0.0f;
    fog->m_param[3] = 0.0f;
    fog->m_param[0] = 0.0f;
}

// ROR 0x0021a158
void *i3dCtxSetFog(i3dContext *ctx, const I3dFog *fog)
{
    I3dFogState *state = (I3dFogState *)i3dCtxAllocTail(ctx, 2);

    // vmaxx / vminibcw clamp to 0..1, * 255, vftoi0, ppach / ppacb pack
    // the low byte of each word; alpha is dropped.
    u_int col = 0;
    for (int i = 0; i < 4; i++)
    {
        float c = fog->m_color[i];
        if (c < 0.0f)
            c = 0.0f;
        if (c > 1.0f)
            c = 1.0f;
        col |= ((u_int)(int)(c * 255.0f) & 0xff) << (8 * i);
    }
    state->m_fogcol = (u_long)(col & 0xffffff);

    // F(z) = base + scale / z through F(near) and F(far).
    float f_near = (1.0f - fog->m_param[1]) * 255.0f;
    float f_far = (1.0f - fog->m_param[3]) * 255.0f;
    state->m_00 = 0;
    state->m_04 = 0;
    float k = (fog->m_param[2] * (f_near - f_far)) / (fog->m_param[2] - fog->m_param[0]);
    state->m_ramp_base = f_near - k;
    state->m_ramp_scale = fog->m_param[0] * k;

    u_char *head = i3dCtxGetHead(ctx);
    state->m_packet = i3dDmaAddr(head);
    i3dCtxSetHead(ctx, i3dPktMakeFog(head, state));
    return state;
}

// ROR 0x0021a270
u_char *i3dPktMakeFog(u_char *pkt, const I3dFogState *fog)
{
    u_int *unpack = (u_int *)(pkt + 0xc);
    u_char *kick = pkt + 0x10;
    u_char *gif = pkt + 0x20;
    *(u_int *)(pkt + 0x8) = I3D_VIF_STCYCL_1_1;

    u_char *p = pkt + 0x30;
    ((u_long *)p)[0] = fog->m_fogcol;
    ((u_long *)p)[1] = I3D_GS_FOGCOL;
    p += 0x10;

    ((u_long *)gif)[1] = I3D_GS_AD;
    ((u_long *)gif)[0] = (u_long)((p - gif - 0x10) >> 4) | (1ull << 60) | 0x8000;    // NLOOP, NREG 1, EOP
    ((u_long *)kick)[0] = ((u_long)((u_int)(p - kick) >> 4) - 1) << 32 | I3D_VU1_KICK;
    ((u_long *)kick)[1] = 0;

    if (fog != NULL)
    {
        u_char *copy = p;
        memcpy(p + 0x10, fog, 0x10);
        p += 0x20;
        ((u_long *)copy)[1] = 0x3f8;
        ((u_long *)copy)[0] = ((u_long)((u_int)(p - copy) >> 4) + 0x3f7) << 32 | I3D_VU1_COPY;
    }

    // Only the first word of these two qwords is written.
    *(u_int *)p = I3D_VU1_END;
    p += 0x10;
    *(u_int *)p = 0;
    p += 0x10;
    *unpack = ((u_int)(p - (u_char *)unpack - 4) >> 4) << 16 | I3D_VIF_UNPACK_V4_32;

    ((u_int *)p)[0] = I3D_VIF_MSCAL_1;
    ((u_int *)p)[1] = 0;
    ((u_int *)p)[2] = 0;
    ((u_int *)p)[3] = 0;
    p += 0x10;

    *(u_long *)pkt = (u_long)((u_int)(p - pkt - 0x10) >> 4) | I3D_DMA_RET;
    return p;
}

// ──────────────────────────────────────────────────────────────────────
// lights
// ──────────────────────────────────────────────────────────────────────

// ROR 0x0021ac80
void i3dLightDescInit(I3dLightDesc *desc)
{
    i3dVecZero(desc->m_ambient);
    desc->m_vec3 = NULL;
    desc->m_parallel_count = 0;
    desc->m_parallel = NULL;
    desc->m_point_count = 0;
    desc->m_point = NULL;
    desc->m_spot_count = 0;
    desc->m_spot = NULL;
    desc->m_dir2_count = 0;
    desc->m_dir2 = NULL;
    desc->m_vec3_count = 0;
}

// ROR 0x0021acd0
int i3dLightSetParallel(I3dLight *light, int count, const I3dParallelLight *list)
{
    light->m_parallel_count = count;
    if (count == 0)
    {
        light->m_parallel = NULL;
        return 0;
    }

    I3dParallelLight *out = (I3dParallelLight *)i3dCtxAllocTail(light->m_ctx, count * 2);
    for (u_int i = 0; i < (u_int)count; i++)
    {
        i3dVecNormalize2(out[i].m_dir, list[i].m_dir);
        i3dVecCopyW(out[i].m_color, list[i].m_color);
    }
    light->m_parallel = out;
    return 0;
}

// ROR 0x0021ad88
int i3dLightSetDir2(I3dLight *light, int count, const I3dDir2Light *list)
{
    light->m_dir2_count = count;
    if (count == 0)
    {
        light->m_dir2 = NULL;
        return 0;
    }

    I3dDir2Light *out = (I3dDir2Light *)i3dCtxAllocTail(light->m_ctx, count * 3);
    for (u_int i = 0; i < (u_int)count; i++)
    {
        i3dVecNormalize2(out[i].m_dir, list[i].m_dir);
        i3dVecCopyW(out[i].m_10, list[i].m_10);
        i3dVecCopyW(out[i].m_20, list[i].m_20);
    }
    light->m_dir2 = out;
    return 0;
}

// ROR 0x0021ae58
int i3dLightSetVec3(I3dLight *light, int count, const I3dVec3Light *list)
{
    light->m_vec3_count = count;
    if (count == 0)
    {
        light->m_vec3 = NULL;
        return 0;
    }

    I3dVec3Light *out = (I3dVec3Light *)i3dCtxAllocTail(light->m_ctx, count * 3);
    memcpy(out, list, count * sizeof(I3dVec3Light));
    light->m_vec3 = out;
    return 0;
}

// ROR 0x0021aed8
int i3dLightSetPoint(I3dLight *light, int count, const I3dPointLight *list)
{
    light->m_point_count = count;
    if (count == 0)
    {
        light->m_point = NULL;
        return 0;
    }

    I3dPointLight *out = (I3dPointLight *)i3dCtxAllocTail(light->m_ctx, count * 2);
    memcpy(out, list, count * sizeof(I3dPointLight));
    light->m_point = out;
    return 0;
}

// ROR 0x0021af50
int i3dLightSetSpot(I3dLight *light, int count, const I3dSpotLight *list)
{
    light->m_spot_count = count;
    if (count == 0)
    {
        light->m_spot = NULL;
        return 0;
    }

    I3dSpotLight *out = (I3dSpotLight *)i3dCtxAllocTail(light->m_ctx, count * 4);
    for (u_int i = 0; i < (u_int)count; i++)
    {
        const I3dSpotLight *src = &list[i];
        I3dSpotLight *dst = &out[i];
        i3dVecCopyW(dst->m_pos, src->m_pos);
        i3dVecNormalize2(dst->m_dir, src->m_dir);
        i3dVecCopyW(dst->m_color, src->m_color);

        float outer = src->m_cone[1];
        float inner = src->m_cone[0];
        if (I3D_CONE_MAX < outer)
            outer = I3D_CONE_MAX;
        if (outer < I3D_CONE_MIN)
            outer = I3D_CONE_MIN;
        if (I3D_CONE_MAX < inner)
            inner = I3D_CONE_MAX;
        if (inner < I3D_CONE_MIN)
            inner = I3D_CONE_MIN;
        if (outer * I3D_CONE_INNER < inner)
            inner = outer * I3D_CONE_INNER;

        float cos_inner = cosf(inner);
        float cos_outer = cosf(outer);
        dst->m_cone[0] = cos_outer;
        dst->m_cone[2] = cos_outer * cos_outer;
        dst->m_cone[1] = 1.0f / (cos_inner - cos_outer);
        dst->m_cone[3] = 1.0f / sinf(outer);
    }
    light->m_spot = out;
    return 0;
}

// ROR 0x0021b100
int i3dLightSetAmbient(I3dLight *light, const float *ambient)
{
    i3dVecCopyW(light->m_ambient, ambient);
    return 0;
}

// ROR 0x0021b110
void i3dLightInit(I3dLight *light)
{
    i3dVecZero(light->m_ambient);
    light->m_ctx = NULL;
    light->m_parallel_count = 0;
    light->m_parallel = NULL;
    light->m_point_count = 0;
    light->m_point = NULL;
    light->m_spot_count = 0;
    light->m_spot = NULL;
    light->m_dir2_count = 0;
    light->m_dir2 = NULL;
    light->m_vec3_count = 0;
    light->m_vec3 = NULL;
}

// The ROM takes 4 qwords for the light; PC: the host-sized record.
// ROR 0x0021b148
I3dLight *i3dCtxCreateLight(i3dContext *ctx, const I3dLightDesc *desc)
{
    I3dLight *light = (I3dLight *)i3dCtxAllocTail(ctx, I3D_QWC_OF(I3dLight));
    i3dLightInit(light);
    light->m_ctx = ctx;
    i3dLightSetAmbient(light, desc->m_ambient);
    if (desc->m_parallel != NULL)
        i3dLightSetParallel(light, desc->m_parallel_count, desc->m_parallel);
    if (desc->m_point != NULL)
        i3dLightSetPoint(light, desc->m_point_count, desc->m_point);
    if (desc->m_spot != NULL)
        i3dLightSetSpot(light, desc->m_spot_count, desc->m_spot);
    if (desc->m_dir2 != NULL)
        i3dLightSetDir2(light, desc->m_dir2_count, desc->m_dir2);
    if (desc->m_vec3 != NULL)
        i3dLightSetVec3(light, desc->m_vec3_count, desc->m_vec3);
    return light;
}
