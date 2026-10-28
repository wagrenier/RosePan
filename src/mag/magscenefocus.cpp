// mag/magscenefocus.cpp -- the scene's focus effect (the efffocus texture,
// assert m_focus.m_texindex != -1): a depth-of-field pass. Each frame the
// frame buffer is shrunk into the 256 x 256 work buffer at FRAME 0x138
// (efffocus is a VRAM-only texture over the same block, bilinear), and the
// work buffer is laid back over the frame as camera-facing sprites filling
// the view at the far1 / far2 distances, z-tested so only what lies beyond
// is blurred. A focus track (an event pack's text file, setFocusData) or
// native 281 op 13 turns it on and moves its alpha and distances.
//
// PC: the native i3d module models neither FRAME_1 targets nor textures
// that live only in VRAM, so neither pass draws on the host yet: the
// shrink's hand-written GIF packet is not recorded, and the plane's draw
// set has no image (the module skips it). The game logic is complete.

#include "mag/magscene.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "common/ror_assert.h"
#include "i3d/i3dctx.h"
#include "i3d/i3denv.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dtex.h"
#include "i3d/i3dview.h"
#include "i3d/i3dvu1pkt.h"
#include "sdk/libgraph.h"

/* PC: the ROM's libc strlwr (0x27a490, lowercases in place through the
 * ctype table); MSVC and glibc do not agree on the name. */
static char *focusStrLwr(char *s)
{
    for (char *p = s; *p != '\0'; p++)
        *p = (char)tolower((u_char)*p);
    return s;
}

/* rodata, setFocus: the efffocus texture DogTexInfo::add builds -- an
 * I3dTexDesc (256 x 256, one level at block 0x2700 = the work buffer's
 * FRAME 0x138, PSMCT32, tcc 1, bilinear; 0x20 bytes with the padding). */
alignas(4) static const u_char s_efffocus_image[0x20] = {
    0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ──────────────────────────────────────────────────────────────────────
// A textured FST sprite (alpha blended when abe) as one REGLIST GIF tag
// behind a DIRECT tag: UV (u0, v0) -> (u1, v1), XYZ2 (x0, y0) -> (x1, y1),
// both in 1/16 units.
// PC: written as the ROM writes it and not recorded for the native i3d
// module: its texture is the frame buffer and its target the work buffer.
// ROR 0x00140990

static u_long *magPktUVSprite(u_long *pkt, int abe, int u0, int v0, int u1, int v1,
                              int x0, int y0, int x1, int y1)
{
    u_long *p = i3dPktDirectTag(pkt, 4, 0x10, NULL);
    p[2] = ((u_long)abe << 6) | 0x116;                  /* PRIM: sprite, TME, FST */
    p[4] = (u_long)u0 | ((u_long)v0 << 16);             /* UV */
    p[6] = (u_long)u1 | ((u_long)v1 << 16);             /* UV */
    p[5] = (u_long)x0 | ((u_long)y0 << 16);             /* XYZ2 */
    p[7] = (u_long)x1 | ((u_long)y1 << 16);             /* XYZ2 */
    p[0] = 0x6400000000008001ULL;                       /* NLOOP 1, EOP, REGLIST, 6 regs */
    p[1] = 0x5353f0;                                    /* PRIM NOP UV XYZ2 UV XYZ2 */
    return p + 8;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00140a84

void MagScene::initFocus()
{
    memset(&m_focus, 0, sizeof(m_focus));
    m_focus.m_texindex = -1;
    m_focus.m_val[0] = 1.0f;
    m_focus.m_val[1] = 300.0f;
    m_focus.m_val[2] = 600.0f;
    m_focus.m_track_pos = 0;
    m_focus.m_track = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Mode 0 releases efffocus and drops the track; any other adds efffocus
// (once) and becomes the mode.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x00140b20

void MagScene::setFocus(int mode)
{
    int tex = m_focus.m_texindex;
    if (mode == 0)
    {
        if (tex != -1)
        {
            m_tex_info.release(tex);
            m_focus.m_texindex = -1;
        }
        m_focus.m_track_pos = 0;
        m_focus.m_mode = 0;
        m_focus.m_track = NULL;
        return;
    }
    if (tex == -1)
    {
        m_focus.m_texindex = m_tex_info.add((const I3dTexDesc *)s_efffocus_image, "efffocus");
        ROR_ASSERT(m_focus.m_texindex != -1); /* 137 */
    }
    m_focus.m_mode = mode;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00140c68

void MagScene::updateFocus()
{
    if (m_focus.m_lerp[0].m_frames > 0)
        m_focus.m_val[0] = stepLerp(&m_focus.m_lerp[0]);
    if (m_focus.m_lerp[1].m_frames > 0)
        m_focus.m_val[1] = stepLerp(&m_focus.m_lerp[1]);
    if (m_focus.m_lerp[2].m_frames > 0)
        m_focus.m_val[2] = stepLerp(&m_focus.m_lerp[2]);
    if (m_focus.m_track != NULL)
        runFocusTrack();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00140d40

float MagScene::stepLerp(MagLerp *rec)
{
    int frames = rec->m_frames;
    int frame = rec->m_frame + 1;
    rec->m_frame = frame;
    if (frame >= frames)
        rec->m_frames = 0;
    return rec->m_from + ((rec->m_to - rec->m_from) * (float)frame) / (float)frames;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00140d94

void MagScene::setFocusLerp(int which, float from, float to, int frames)
{
    if (which == 1)
    {
        if (from < 0.0f)
            from = m_focus.m_val[1];
        m_focus.m_val[1] = startLerp(from, to, &m_focus.m_lerp[1], frames);
    }
    else if (which == 2)
    {
        if (from < 0.0f)
            from = m_focus.m_val[2];
        m_focus.m_val[2] = startLerp(from, to, &m_focus.m_lerp[2], frames);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00140e60

float MagScene::startLerp(float from, float to, MagLerp *rec, int frames)
{
    if (frames == 0)
    {
        rec->m_frames = 0;
        return to;
    }
    rec->m_frames = frames;
    rec->m_to = to;
    rec->m_from = from;
    rec->m_frame = 0;
    return from;
}

// ──────────────────────────────────────────────────────────────────────
// The focus planes, after everything else of the scene: far2 twice (mode
// bit 1), then far1 (bit 0), each over a fresh shrink of the frame. The
// frame buffer is the one being drawn (fbp 0x70 on even frames); the
// alpha is kept above 0 while they draw.
// ROR 0x00140e88

void MagScene::drawFocus(i3dContext *ctx, int frame)
{
    if (m_focus.m_texindex == -1)
        return;

    u_int fbp = (frame & 1) ? 0 : 0x70;
    float alpha = m_focus.m_val[0];
    m_focus.m_bias = 0.0f;
    m_focus.m_val[0] = alpha <= 0.0f ? 0.01f : alpha;
    if (m_focus.m_mode & 2)
    {
        shrinkFocusFrame(ctx, fbp);
        drawFocusPlane(m_focus.m_val[2], ctx, fbp);
        shrinkFocusFrame(ctx, fbp);
        drawFocusPlane(m_focus.m_val[2], ctx, fbp);
    }
    if (m_focus.m_mode & 1)
    {
        shrinkFocusFrame(ctx, fbp);
        drawFocusPlane(m_focus.m_val[1], ctx, fbp);
    }
    m_focus.m_val[0] = alpha;
}

// ──────────────────────────────────────────────────────────────────────
// FRAME 0x138 (256 wide), no offset, TEX0 the 512-wide CT32 frame at fbp,
// clamped, bilinear (TEX1 0x60), FBA 1 while the 512 x 448 frame is drawn
// into 256 x 256.
// ROR 0x00140fdc

void MagScene::shrinkFocusFrame(i3dContext *ctx, u_int fbp)
{
    I3dLayer *layer = i3dCtxCreateLayer(ctx, m_focus.m_bias + 860.0f);
    m_focus.m_bias += 0.01f;
    u_long *pkt = i3dLayerBeginSet(layer, 0.0f);
    pkt = i3dPktSetReg(pkt, SCE_GS_FRAME_1, 0x40138);
    pkt = i3dPktSetReg(pkt, SCE_GS_XYOFFSET_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEX0_1, (u_long)((fbp & 0x7ffff) << 5) | 0x264020000ULL);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEXFLUSH, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_CLAMP_1, 5);
    pkt = i3dPktSetReg(pkt, SCE_GS_FBA_1, 1);
    pkt = i3dPktSetReg(pkt, SCE_GS_RGBAQ, 0x80808080);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEX1_1, 0x60);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x31001);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0);
    pkt = magPktUVSprite(pkt, 1, 0, 0, 0x2000, 0x1c00, 0, 0, 0x1000, 0x1000);
    pkt = i3dPktSetReg(pkt, SCE_GS_FBA_1, 0);
    i3dLayerEndSet(layer, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// Through the scene's view (m_view[0], the camera) and a white ambient
// light: efffocus as one VU1 sprite at dist along the camera's z, scaled
// to the view's width and height there, colour (1, 1, 1, m_val[0]),
// drawn into the frame at fbp (XYOFFSET 1792, 1824) with z test GEQUAL
// and blend mode 0x44.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT); the camera's transform is EE vtable slot 2 (callTransform)
// ROR 0x00141184

void MagScene::drawFocusPlane(float dist, i3dContext *ctx, u_int fbp)
{
    DogCamera *cam = &m_camera_entity;
    I3dLayer *layer = i3dCtxCreateLayer(ctx, m_focus.m_bias + 860.0f);
    m_focus.m_bias += 0.01f;

    I3dView *view = i3dCtxCreateView(ctx, &m_view[0], cam->getMatrix());
    ROR_ASSERT(view); /* 496 */
    i3dLayerSetView(layer, view);

    I3dLightDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.m_ambient[0] = 1.0f;
    desc.m_ambient[1] = 1.0f;
    desc.m_ambient[2] = 1.0f;
    desc.m_ambient[3] = 1.0f;
    I3dLight *light = i3dCtxCreateLight(ctx, &desc);
    ROR_ASSERT(light); /* 511 */
    i3dLayerSetLight(layer, light);

    int tex_no = m_focus.m_texindex;
    I3dTexSetDesc set_desc;
    set_desc.m_depth = 1.0f;
    set_desc.m_tex_no = &tex_no;
    set_desc.m_tex_count = 1;
    set_desc.m_sort_func = 0;
    set_desc.m_sort_order = 0;
    set_desc.m_sort_param = 0;
    I3dDrawSet *set = i3dLayerFindDrawSet(layer, &set_desc);
    i3dLayerSetDrawSet(layer, set);

    u_long *pkt = i3dLayerBeginSet(layer, 1.0f);
    pkt = i3dPktSetReg(pkt, SCE_GS_FRAME_1, fbp | 0x80000);
    pkt = i3dPktSetReg(pkt, SCE_GS_XYOFFSET_1, 0x720000007000ULL);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEXFLUSH, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_CLAMP_1, 5);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5000d);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktCallView(pkt, layer);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(set, 0));
    pkt = i3dPktBeginVu1Sprite(pkt, layer, 1);
    const float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktVu1Rect(pkt, rect);

    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    const float color[4] = { 1.0f, 1.0f, 1.0f, m_focus.m_val[0] };
    float pos[4];
    i3dVecScale(dist, pos, m[2]);
    i3dVecAdd(pos, pos, m[3]);
    pos[3] = 1.0f;
    const MagViewParam &vp = m_view[0];
    const float scale[4] = { ((vp.m_right - vp.m_left) * dist) / vp.m_near,
                             ((vp.m_top - vp.m_bottom) * dist) / vp.m_near, 1.0f, 1.0f };
    i3dMatScale(m, m, scale);
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, pos, color);
    pkt = i3dPktEndVu1Sprite(pkt);
    i3dLayerEndSet(layer, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// The index of the first sub in s, -1 when absent.
// ROR 0x0014159c

static int magStrIndex(const char *s, const char *sub)
{
    int sub_len = (int)strlen(sub);
    int last = (int)strlen(s) - sub_len;
    for (int i = 0; i <= last; i++)
    {
        if (strncmp(s + i, sub, sub_len) == 0)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Splits s at any character of delims into NUL-terminated words packed
// one after another at out, skipping empty ones; returns the word count.
// ROR 0x00141634

static int magStrSplit(const char *s, const char *delims, char *out)
{
    int out_pos = 0;
    int count = 0;
    int len = (int)strlen(s);
    int delim_len = (int)strlen(delims);
    int start = 0;
    int i;
    for (i = 0; i < len; i++)
    {
        int k = 0;
        while (k < delim_len && s[i] != delims[k])
            k++;
        if (k == delim_len)
            continue;               /* not a delimiter */
        if (i - start < 1)
        {
            start = i + 1;          /* an empty word */
            continue;
        }
        memcpy(out + out_pos, s + start, i - start);
        out[out_pos + (i - start)] = '\0';
        out_pos += i - start + 1;
        count++;
        start = i + 1;
    }
    if (i - start > 0)
    {
        count++;
        memcpy(out + out_pos, s + start, i - start);
        out[out_pos + (i - start)] = '\0';
    }
    return count;
}

// ──────────────────────────────────────────────────────────────────────
// A command's plane word: far1far2 3, far1 1, far2 2, else 0 (the focus
// mode bits, and setFocusLerp's which).
// ROR 0x00141770

static int magFocusTarget(const char *word)
{
    if (strcmp(word, "far1far2") == 0)
        return 3;
    if (strcmp(word, "far1") == 0)
        return 1;
    if (strcmp(word, "far2") == 0)
        return 2;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Starts a focus track: focus mode 3 and size bytes of CR LF lines.
// ROR 0x001417e4

void MagScene::setFocusData(const void *data, int size)
{
    if (data == NULL)
        return;
    setFocus(3);
    m_focus.m_wait = -1;
    m_focus.m_track = (const char *)data;
    m_focus.m_track_size = size;
    m_focus.m_track_pos = 0;
    m_focus.m_frame = -1;
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the track: once the frame reaches the pending line's,
// runs line after line (lowercased; the last may lack its CR LF) until
// one waits for a later frame; an empty line ends the effect.
// PC: the ROM copies a line into its 128-byte buffer unchecked; the copy
// is clipped to it.
// callcheck: strlwr (the host's focusStrLwr)
// ROR 0x0014186c

void MagScene::runFocusTrack()
{
    char line[128];
    if (++m_focus.m_frame < m_focus.m_wait)
        return;
    for (;;)
    {
        int pos = m_focus.m_track_pos;
        int len = magStrIndex(m_focus.m_track + pos, "\r\n");
        int step;
        if (len < 0 || m_focus.m_track_size < pos + len)
        {
            len = m_focus.m_track_size - pos;
            step = len;
        }
        else
        {
            step = len + 2;
        }
        if (len > (int)sizeof(line) - 1)
            len = (int)sizeof(line) - 1;
        memcpy(line, m_focus.m_track + pos, len);
        line[len] = '\0';
        if (strlen(line) == 0)
        {
            setFocus(0);
            return;
        }
        focusStrLwr(line);
        if (execFocusCommand(line) == 0)
            return;
        m_focus.m_track_pos += step;
    }
}

// ──────────────────────────────────────────────────────────────────────
// One line, "frame command args" split at spaces and tabs. A frame still
// ahead becomes the wait and stops (0). Commands:
//   start PLANE             setFocus(magFocusTarget(PLANE))
//   term                    setFocus(0), stop
//   setalpha FROM TO N      the alpha lerps FROM (< 0: where it is) -> TO over N frames
//   setdist PLANE FROM TO N setFocusLerp(PLANE, FROM, TO, N)
//   movealpha TO N          the alpha lerps from where it is
//   movedist PLANE TO N     setFocusLerp(PLANE, -1, TO, N)
// A line of fewer than two words is skipped.
// callcheck: FUN_00215a30, dptofp (the assert handler inside ROR_ASSERT; the float conversion of atof's double)
// ROR 0x001419d8

int MagScene::execFocusCommand(char *line)
{
    char words[128];
    memset(words, 0, sizeof(words));
    int num = magStrSplit(line, " \t", words);
    if (num < 2)
        return 1;

    int frame = atoi(words);
    if (m_focus.m_frame < frame)
    {
        m_focus.m_wait = frame;
        return 0;
    }

    char *cmd = words + strlen(words) + 1;
    char *arg = cmd + strlen(cmd) + 1;
    if (strcmp(cmd, "start") == 0)
    {
        ROR_ASSERT(num>=(2+1)); /* 905 */
        setFocus(magFocusTarget(arg));
        return 1;
    }
    if (strcmp(cmd, "term") == 0)
    {
        setFocus(0);
        return 0;
    }

    float from;
    float to;
    int frames;
    if (strcmp(cmd, "setalpha") == 0)
    {
        ROR_ASSERT(num>=(2+3)); /* 917 */
        from = (float)atof(arg);
        arg += strlen(arg) + 1;
        to = (float)atof(arg);
        arg += strlen(arg) + 1;
        frames = atoi(arg);
        if (from < 0.0f)
            from = m_focus.m_val[0];
    }
    else if (strcmp(cmd, "setdist") == 0)
    {
        ROR_ASSERT(num>=(2+4)); /* 925 */
        int which = magFocusTarget(arg);
        arg += strlen(arg) + 1;
        from = (float)atof(arg);
        arg += strlen(arg) + 1;
        to = (float)atof(arg);
        arg += strlen(arg) + 1;
        frames = atoi(arg);
        setFocusLerp(which, from, to, frames);
        return 1;
    }
    else if (strcmp(cmd, "movealpha") == 0)
    {
        ROR_ASSERT(num>=(2+2)); /* 934 */
        to = (float)atof(arg);
        arg += strlen(arg) + 1;
        frames = atoi(arg);
        from = m_focus.m_val[0];
    }
    else if (strcmp(cmd, "movedist") == 0)
    {
        ROR_ASSERT(num>=(2+3)); /* 941 */
        int which = magFocusTarget(arg);
        arg += strlen(arg) + 1;
        to = (float)atof(arg);
        arg += strlen(arg) + 1;
        frames = atoi(arg);
        setFocusLerp(which, -1.0f, to, frames);
        return 1;
    }
    else
    {
        ROR_ASSERT(0); /* 949 */
        return 1;
    }
    m_focus.m_val[0] = startLerp(from, to, &m_focus.m_lerp[0], frames);
    return 1;
}
