/* ==========================================================================
 *  spr/sprman.cpp
 *
 *  The sprite manager (see spr/sprman.h): 0x18bb8c..0x18f5b0, pinned in
 *  the sprman session (the one assert names the file; SprTexture's
 *  constructor at 0x18f5b0 opens spr/sprtexture.cpp).
 * ======================================================================== */
#include "spr/sprman.h"

#include "common/ror_assert.h"
#include "game/gamman.h"
#include "i3d/i3dctx.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dtex.h"
#include "mag/magsystem.h"
#include "sdk/libgraph.h"
#include "utl/utlmisc.h"

/* ---- data ---- */

const int g_sprColorOpaque[4] = { 0x80, 0x80, 0x80, 0x80 };    /* data 0x299990 */
const int g_sprColorClear[4]  = { 0x80, 0x80, 0x80, 0 };       /* data 0x2999b0 */

static const char s_file[] = "spr/sprman.cpp";                  /* rodata 0x2f7930 */

/* The last font file setFont took from a pack: the next SprMan without a
 * pack shares it. */
static const void *s_sprFontData;                               /* bss 0x6e93d4 */

/* The GS drawing area's offset from the primitive coordinates (pixels with
 * 4 fraction bits): 2048 - 256, 2048 - 224. */
#define SPR_GS_OFFSET_X 0x7000
#define SPR_GS_OFFSET_Y 0x7200

/* ALPHA_1 (Cs - Cd) * As + Cd, the blend every sprite uses. */
#define SPR_ALPHA_BLEND SCE_GS_SET_ALPHA(0, 1, 0, 1, 0)

/* PC: the frames / keys an anim / move points at (EE addresses in the
 * table image). */
static inline SprFrame *sprFrames(const SprMan *man, const SprAnim *anim)
{
    return (SprFrame *)man->eePtr(anim->m_frames);
}

static inline SprMoveKey *sprKeys(const SprMan *man, const SprMove *move)
{
    return (SprMoveKey *)man->eePtr(move->m_keys);
}

static inline bool sprIsSjisLead(u_int c)
{
    return (c - 0x81) < 0x1f || (c - 0xe0) < 0x1d;
}

// ──────────────────────────────────────────────────────────────────────
// Clears the words the manager reads before init; the SprTexture and the
// font are built by their own constructors (C++ runs them first).
// ROR 0x0018bb8c
// callcheck: SprTexture, GamFont (the members' constructors, run before this body)

SprMan::SprMan()
{
    m_font_in_pack = 0;
    m_back_color = 0x80;
    m_00 = 0;
    m_sprite = NULL;
    m_pattern = NULL;
    m_active = 0;
    m_font_data[0] = NULL;
    m_volume_on = 0;
    m_4cc = 0;
    m_draw_set[0] = NULL;
    m_draw_set[1] = NULL;
    m_pattern_ee = 0;   // PC
}

// ──────────────────────────────────────────────────────────────────────
// Forgets the font and the draw sets; the members' destructors follow.
// ROR 0x0018bc18
// callcheck: ~GamFont, ~SprTexture, operator_delete (member destructors and the deleting variant)

SprMan::~SprMan()
{
    if (m_font_data[0] != NULL)
        m_font_data[0] = NULL;
    m_draw_set[0] = NULL;
    m_draw_set[1] = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// init with 256 sprites (no caller found).
// ROR 0x0018bca8

int SprMan::initDefault()
{
    return init(0x100);
}

// ──────────────────────────────────────────────────────────────────────
// Takes num sprite records from the game heap and clears them; the loader
// and the layers start empty. 0 on success, 1 when the heap is full.
// ROR 0x0018bcc0

int SprMan::init(int num)
{
    m_num = num;
    m_active = 0;
    m_tex.clear();
    m_layer[0] = NULL;
    m_layer[1] = NULL;
    // PC: the host record is bigger than the EE's 0x154 bytes.
    m_sprite = (SprSprite *)g_utlMemory.allocDebug(m_num * (int)sizeof(SprSprite), s_file, 112, 0, 2);
    if (m_sprite == NULL)
        return 1;
    m_draw_set[0] = NULL;
    m_draw_set[1] = NULL;
    for (int i = 0; i < m_num; i++)
        initSprite(&m_sprite[i]);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Steps and draws the sprites into layer 0: the top-level ones first, then
// the children (placed on their parent's position of this frame), skipping
// those whose processing is off; then layer 1's frame-buffer copy.
// ROR 0x0018bd94

int SprMan::draw()
{
    if (m_active == 0)
        return 0;
    if (beginDraw(0) != 0)
        return 1;
    for (int i = 0; i < m_num; i++) {
        SprSprite *spr = &m_sprite[i];
        if (spr->m_parent == 0 && (spr->m_flags & 2) == 0) {
            updateMove(spr);
            updateAnim(spr);
            updateFade(spr);
            drawSprite(spr, 0);
        }
    }
    for (int i = 0; i < m_num; i++) {
        SprSprite *spr = &m_sprite[i];
        if (spr->m_parent > 0 && (spr->m_flags & 2) == 0) {
            updateMove(spr);
            updateAnim(spr);
            updateFade(spr);
            drawSprite(spr, 0);
        }
    }
    endDraw(0);
    drawFrameCopy();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Layer 1 (when set): the frame buffer the GS is not drawing into (base 0
// or 0xe00 by MagSystem's frame parity) as a 512 x 448 sprite over the
// screen, in the grey m_back_color.
// ROR 0x0018bef8

void SprMan::drawFrameCopy()
{
    if (m_layer[1] == NULL || beginDraw(1) != 0)
        return;
    u_long tbp = 0xe00;
    if (MagSystem::m_current_system->m_frame & 1)
        tbp = 0;
    m_pkt = i3dPktSetReg(m_pkt, SCE_GS_ALPHA_1, SPR_ALPHA_BLEND);
    m_pkt = i3dPktCallView(m_pkt, m_layer[1]);
    /* TEX0: TBW 8, PSMCT32, 512 x 512 */
    m_pkt = i3dPktSetReg(m_pkt, SCE_GS_TEX0_1, tbp | 0x264020000ull);
    m_pkt = i3dPktSetReg(m_pkt, SCE_GS_TEX1_1, SCE_GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    m_pkt = i3dPktSetReg(m_pkt, SCE_GS_ALPHA_1, SPR_ALPHA_BLEND);
    u_long xyz0 = i3dPackXYZ(0x7000, 0x7200, 0x80000);
    u_long xyz1 = i3dPackXYZ(0x9000, 0x8e00, 0x80000);
    u_long uv0 = i3dPackUV(8, 7);
    u_long uv1 = i3dPackUV(0x2008, 0x1c07);
    u_long rgbaq = i3dPackRGBAQ(1.0f, m_back_color, m_back_color, m_back_color, 0x80);
    m_pkt = i3dPktTexSpriteUV(m_pkt, xyz0, xyz1, uv0, uv1, rgbaq);
    endDraw(1);
}

// ──────────────────────────────────────────────────────────────────────
// The CLUT animations of the shown sprites, the top-level ones first.
// ROR 0x0018c068

int SprMan::update()
{
    if (m_active != 0) {
        for (int i = 0; i < m_num; i++) {
            if (m_sprite[i].m_parent == 0)
                updateClut(&m_sprite[i]);
        }
        for (int i = 0; i < m_num; i++) {
            if (m_sprite[i].m_parent > 0)
                updateClut(&m_sprite[i]);
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Frees the records and the loaded textures. -1 when not initialised.
// ROR 0x0018c130

int SprMan::release()
{
    if (m_sprite == NULL)
        return -1;
    m_tex.release();
    utlMemFree(m_sprite);
    m_sprite = NULL;
    if (m_font_data[0] != NULL)
        m_font_data[0] = NULL;
    m_active = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sets the sprites a display list names: anim table, flags, parent,
// offset, then the start anim.
// ROR 0x0018c190

void SprMan::setDispList(const SprDispDef *def)
{
    for (; def->m_no != -1; def++) {
        setAnimTable(def->m_no, (SprAnim *)eePtr(def->m_anim_ee));
        SprSprite *spr = &m_sprite[def->m_no];
        spr->m_disp = def->m_disp;
        spr->m_state = def->m_state;
        spr->m_pat_base = def->m_pat_base;
        spr->m_parent = def->m_parent;
        spr->m_off[0] = def->m_off[0];
        spr->m_off[1] = def->m_off[1];
        spr->m_off[2] = def->m_off[2];
        setAnim(def->m_no, def->m_anim);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Font number no (1; 0 fails) for the text sprites: once, from the
// texture pack by name when m_font_in_pack, else the last pack font. 0 on
// success.
// ROR 0x0018c2e4
// callcheck: FUN_00215a30 x2 (BedPfh::getFile's asserts, bed/bedpfh.h lines 85 / 86)

int SprMan::setFont(const char *theName, int no)
{
    if (no == 0)
        return 1;
    if (no == 1)
        no = 0;
    if (m_font_data[no] != NULL)
        return 0;
    if (m_font_in_pack == 1) {
        int n = m_tex.getPackIndex(theName);
        if (n < 0)
            return 1;
        s_sprFontData = m_tex.m_pfh->getFile(n);
        m_font_data[no] = (void *)s_sprFontData;
        m_font[no].setFont(m_font_data[no]);
        return 0;
    }
    m_font_data[no] = (void *)s_sprFontData;
    m_font[no].setFont(s_sprFontData);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Gives sprite no an anim table: anim 0, frame 0, the count. 1 when there
// are no records or no is out of range.
// ROR 0x0018c448

int SprMan::setAnimTable(int no, SprAnim *anims)
{
    if (m_sprite == NULL || no >= m_num)
        return 1;
    SprSprite *spr = &m_sprite[no];
    spr->m_anim = anims;
    spr->m_anim_no = 0;
    spr->m_frame = 0;
    const SprFrame *frames = sprFrames(this, anims);
    spr->m_anim_timer = SPR_FRAME_TIME(frames[0].m_ctrl);
    spr->m_frame_flags = frames[0].m_ctrl & SPR_FRAME_FLAGS;
    spr->m_anim_num = countAnims(anims);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Starts anim number anim of sprite no from its first frame.
// ROR 0x0018c4d8

int SprMan::setAnim(int no, int anim)
{
    SprSprite *spr = &m_sprite[no];
    int bad = spr->m_anim_num <= anim;
    if (!bad) {
        spr->m_anim_no = anim;
        spr->m_frame = 0;
        const SprFrame *frames = sprFrames(this, &spr->m_anim[anim]);
        spr->m_anim_timer = SPR_FRAME_TIME(frames[0].m_ctrl);
        spr->m_frame_flags = frames[0].m_ctrl & SPR_FRAME_FLAGS;
    }
    return bad;
}

// ──────────────────────────────────────────────────────────────────────
// setAnim on a record.
// ROR 0x0018c54c

int SprMan::setSprAnim(SprSprite *spr, int anim)
{
    int bad = spr->m_anim_num <= anim;
    if (!bad) {
        spr->m_anim_no = anim;
        spr->m_frame = 0;
        const SprFrame *frames = sprFrames(this, &spr->m_anim[anim]);
        spr->m_anim_timer = SPR_FRAME_TIME(frames[0].m_ctrl);
        spr->m_frame_flags = frames[0].m_ctrl & SPR_FRAME_FLAGS;
    }
    return bad;
}

// ──────────────────────────────────────────────────────────────────────
// Gives sprite no a move table and starts move 0's first key (the caller
// sets the mode). 1 when there are no records or no is out of range.
// ROR 0x0018c630

int SprMan::setMoveTable(int no, SprMove *moves)
{
    if (m_sprite == NULL || no >= m_num)
        return 1;
    SprSprite *spr = &m_sprite[no];
    spr->m_move = moves;
    spr->m_move_no = 0;
    spr->m_move_key = 0;
    spr->m_move_timer = sprKeys(this, moves)[0].m_time;
    spr->m_move_num = countMoves(moves);
    startMoveKey(spr);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Starts move number move of sprite no from its first key; non-zero when
// move is past the table.
// ROR 0x0018c6c4

int SprMan::setMove(int no, int move)
{
    SprSprite *spr = &m_sprite[no];
    int bad = spr->m_move_num <= move;
    if (!bad) {
        spr->m_move_no = move;
        spr->m_move_key = 0;
        spr->m_move_timer = sprKeys(this, &spr->m_move[move])[0].m_time;
        startMoveKey(spr);
    }
    return bad;
}

// ──────────────────────────────────────────────────────────────────────
// setMove on a record.
// ROR 0x0018c738

int SprMan::setSprMove(SprSprite *spr, int move)
{
    int bad = spr->m_move_num <= move;
    if (!bad) {
        spr->m_move_no = move;
        spr->m_move_key = 0;
        spr->m_move_timer = sprKeys(this, &spr->m_move[move])[0].m_time;
        startMoveKey(spr);
    }
    return bad;
}

// ──────────────────────────────────────────────────────────────────────
// A cleared record: unused (parent -1), processing on, white, 16 x 16, no
// anim, move, texture or text.
// ROR 0x0018c814

void SprMan::initSprite(SprSprite *spr)
{
    spr->m_draw_h = 0x10;
    spr->m_flags = 1;
    spr->m_parent = -1;
    spr->m_move_mode = SPR_MOVE_NONE;
    spr->m_disp = 0;
    spr->m_state = SPR_STATE_OFF;
    spr->m_fade_mode = 0;
    for (int i = 0; i < 3; i++) {
        spr->m_std[i] = 0;
        spr->m_off[i] = 0;
        spr->m_pos[i] = 0;
    }
    for (int i = 0; i < 3; i++) {
        spr->m_move_speed[i] = 0;
        spr->m_move_accel[i] = 0;
        spr->m_move_from[i] = 0;
        spr->m_move_to[i] = 0;
    }
    spr->m_u = 0;
    spr->m_v = 0;
    spr->m_w = 0;
    spr->m_h = 0;
    spr->m_draw_w = 0x10;
    spr->m_fix_w = -1;
    spr->m_color[3] = 0x80;
    spr->m_move_key = -1;
    spr->m_clear_rect = NULL;
    spr->m_fix_h = -1;
    spr->m_color[0] = 0x80;
    spr->m_color[1] = 0x80;
    spr->m_color[2] = 0x80;
    spr->m_clut = 0;
    spr->m_clut_timer = 0;
    spr->m_clut_step = 0;
    spr->_128 = 0;
    spr->m_clut_max = 0;
    spr->m_clut_interval = 0;
    spr->m_frame_flags = 0;
    spr->m_pat_base = 0;
    spr->m_anim_timer = 0;
    spr->m_move_timer = 0;
    spr->m_tex = SPR_TEX_NONE;
    spr->m_anim_no = -1;
    spr->m_frame = -1;
    spr->m_anim = NULL;
    spr->m_move_no = -1;
    spr->m_move = NULL;
    spr->m_text = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// The anims of a table (to m_id -1), counting the frames of those whose
// count is < 0 into the table.
// ROR 0x0018c910

int SprMan::countAnims(SprAnim *anims)
{
    int n = 0;
    if (anims != NULL) {
        for (; anims->m_id != -1; anims++, n++) {
            if (anims->m_num < 0)
                anims->m_num = countFrames(sprFrames(this, anims));
        }
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// The frames before the end code.
// ROR 0x0018c990

int SprMan::countFrames(const SprFrame *frames)
{
    int n = 0;
    if (frames != NULL) {
        for (; (frames->m_ctrl & SPR_FRAME_END) == 0; frames++)
            n++;
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// The moves of a table (to m_id -1), counting the keys of those whose
// count is < 0 into the table.
// ROR 0x0018c9d8

int SprMan::countMoves(SprMove *moves)
{
    int n = 0;
    if (moves != NULL) {
        for (; moves->m_id != -1; moves++, n++) {
            if (moves->m_num < 0)
                moves->m_num = countKeys(sprKeys(this, moves));
        }
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// The keys before the first one of type < 0.
// ROR 0x0018ca58

int SprMan::countKeys(const SprMoveKey *keys)
{
    int n = 0;
    if (keys != NULL) {
        for (; keys->m_type >= 0; keys++)
            n++;
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// The acceleration that adds up to dist over n frames (1 + 2 + .. + n
// steps).
// ROR 0x0018ca98

float SprMan::calcAccel(float dist, int n)
{
    float f = (float)n;
    return dist / ((f * f + f) * 0.5f);
}

// ──────────────────────────────────────────────────────────────────────
// Sprite no eases from from to to (pixels with 4 fraction bits) over time
// frames: speeding up for the first half, slowing down for the second. No
// caller found.
// ROR 0x0018cac8

void SprMan::startEaseMove(int no, int time, const int *from, const int *to)
{
    int half = time >> 1;
    SprSprite *spr = &m_sprite[no];
    spr->m_move_timer = time;
    spr->m_move_time = time;
    spr->m_move_mode = SPR_MOVE_EASE;
    for (int i = 0; i < 3; i++)
        spr->m_move_pos[i] = (float)(from[i] >> 4);
    for (int i = 0; i < 3; i++)
        spr->m_move_accel[i] = calcAccel((float)((to[i] >> 4) - (from[i] >> 4)) * 0.5f, half);
    for (int i = 0; i < 3; i++) {
        spr->m_move_speed[i] = 0;
        spr->m_move_from[i] = from[i];
        spr->m_move_to[i] = to[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// An ease from from to to (pixels) over time frames.
// ROR 0x0018cd88

void SprMan::setEaseMove(SprSprite *spr, int time, const float *from, const float *to)
{
    spr->m_move_timer = time;
    spr->m_move_time = time;
    int half = time >> 1;
    for (int i = 0; i < 3; i++)
        spr->m_move_pos[i] = from[i];
    for (int i = 0; i < 3; i++)
        spr->m_move_accel[i] = calcAccel((to[i] - from[i]) * 0.5f, half);
    for (int i = 0; i < 3; i++) {
        spr->m_move_speed[i] = 0;
        spr->m_move_from[i] = (int)from[i] << 4;
        spr->m_move_to[i] = (int)to[i] << 4;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A constant speed from from to to (pixels) over time frames.
// ROR 0x0018ceec

void SprMan::setLinearMove(SprSprite *spr, int time, const float *from, const float *to)
{
    spr->m_move_timer = time;
    float t = (float)time;
    spr->m_move_time = time;
    for (int i = 0; i < 3; i++)
        spr->m_move_pos[i] = from[i];
    for (int i = 0; i < 3; i++)
        spr->m_move_speed[i] = (to[i] - from[i]) / t;
    for (int i = 0; i < 3; i++) {
        spr->m_move_accel[i] = 0;
        spr->m_move_from[i] = (int)from[i] << 4;
        spr->m_move_to[i] = (int)to[i] << 4;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Sprite no moves from its offset with a speed and an acceleration (pixels
// per frame) for time frames.
// ROR 0x0018cff0

void SprMan::startSpeedMove(int no, int time, const float *speed, const float *accel)
{
    SprSprite *spr = &m_sprite[no];
    spr->m_move_mode = SPR_MOVE_SPEED;
    spr->m_move_time = time;
    for (int i = 0; i < 3; i++)
        spr->m_move_pos[i] = (float)(spr->m_off[i] >> 4);
    spr->m_move_timer = time;
    for (int i = 0; i < 3; i++) {
        spr->m_move_accel[i] = accel[i];
        spr->m_move_speed[i] = speed[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// Starts the sprite's current move key: an ease or a constant speed, from
// the key's start or from where the sprite is.
// ROR 0x0018d08c

void SprMan::startMoveKey(SprSprite *spr)
{
    const SprMoveKey *key = &sprKeys(this, &spr->m_move[spr->m_move_no])[spr->m_move_key];
    float here[3];
    switch (key->m_type) {
    case SPR_KEY_EASE:
        setEaseMove(spr, key->m_time, key->m_from, key->m_to);
        break;
    case SPR_KEY_EASE_HERE:
        here[0] = (float)(spr->m_off[0] >> 4);
        here[1] = (float)(spr->m_off[1] >> 4);
        here[2] = (float)(spr->m_off[2] >> 4);
        setEaseMove(spr, key->m_time, here, key->m_to);
        break;
    case SPR_KEY_LINEAR:
        setLinearMove(spr, key->m_time, key->m_from, key->m_to);
        break;
    case SPR_KEY_LINEAR_HERE:
        here[0] = (float)(spr->m_off[0] >> 4);
        here[1] = (float)(spr->m_off[1] >> 4);
        here[2] = (float)(spr->m_off[2] >> 4);
        setLinearMove(spr, key->m_time, here, key->m_to);
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the sprite's motion, written to m_off: a free speed move
// (each speed capped at 8 pixels), an ease, or the move table's keys
// (the next key when the timer runs out; -2 repeats, -1 ends).
// ROR 0x0018d1e8
// callcheck: fptodp, dpcmp x3 (the EE's software double compare of the speed cap)

void SprMan::updateMove(SprSprite *spr)
{
    switch (spr->m_move_mode) {
    case SPR_MOVE_SPEED: {
        int timer = spr->m_move_timer;
        if (timer > 0) {
            spr->m_move_timer = timer - 1;
            timer = spr->m_move_timer;
        }
        if (timer == 0)
            spr->m_move_mode = SPR_MOVE_NONE;
        for (int i = 0; i < 3; i++) {
            spr->m_move_speed[i] += spr->m_move_accel[i];
            if ((double)spr->m_move_speed[i] > 8.0)
                spr->m_move_speed[i] = 8.0f;
        }
        for (int i = 0; i < 3; i++) {
            spr->m_move_pos[i] += spr->m_move_speed[i];
            spr->m_off[i] = (int)spr->m_move_pos[i] << 4;
        }
        return;
    }
    case SPR_MOVE_STOP:
        spr->m_move_mode = SPR_MOVE_NONE;
        return;
    case SPR_MOVE_EASE:
        break;
    case SPR_MOVE_TABLE: {
        if (spr->m_move_timer < 1) {
            /* ROM: the flag a halt key sets is never cleared, so after one
             * the loop runs through every following key in this frame. */
            bool again = false;
            do {
                const SprMove *move = &spr->m_move[spr->m_move_no];
                const SprMoveKey *keys = sprKeys(this, move);
                int next = spr->m_move_key + 1;
                if (spr->m_move_key < move->m_num - 1) {
                    spr->m_move_key = next;
                    spr->m_move_timer = keys[next].m_time;
                    if (keys[next].m_type == SPR_KEY_HALT) {
                        for (int i = 0; i < 3; i++) {
                            spr->m_move_accel[i] = 0;
                            spr->m_move_speed[i] = 0;
                        }
                        again = true;
                    }
                } else if (keys[next].m_type == SPR_KEY_REPEAT) {
                    /* ROM: the move to repeat is read at the anim's frame
                     * + 1 (m_frame), not at the end key. */
                    setSprMove(spr, keys[spr->m_frame + 1].m_time);
                    spr->m_move_key = 0;
                    spr->m_move_timer = sprKeys(this, &spr->m_move[spr->m_move_no])[0].m_time;
                } else if (keys[next].m_type == SPR_KEY_END) {
                    spr->m_move_mode = SPR_MOVE_NONE;
                }
            } while (again);
            if (spr->m_move_mode == SPR_MOVE_NONE)
                return;
            startMoveKey(spr);
        }
        int type = sprKeys(this, &spr->m_move[spr->m_move_no])[spr->m_move_key].m_type;
        if (type > SPR_KEY_LINEAR_HERE || type < 0) {
            if (spr->m_move_timer > 0)
                spr->m_move_timer--;
            return;
        }
        break;
    }
    default:
        return;
    }

    /* an ease (or a constant speed: accel 0) */
    int timer = spr->m_move_timer;
    int half = spr->m_move_time / 2;
    if (half < timer) {
        for (int i = 0; i < 3; i++)
            spr->m_move_speed[i] += spr->m_move_accel[i];
    } else if (timer < half) {
        for (int i = 0; i < 3; i++)
            spr->m_move_speed[i] -= spr->m_move_accel[i];
    }
    for (int i = 0; i < 3; i++) {
        spr->m_move_pos[i] += spr->m_move_speed[i];
        spr->m_off[i] = (int)spr->m_move_pos[i] << 4;
    }
    if (timer > 0) {
        spr->m_move_timer = timer - 1;
        return;
    }
    for (int i = 0; i < 3; i++)
        spr->m_off[i] = spr->m_move_to[i];
    spr->m_move_mode = SPR_MOVE_NONE;
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the sprite's anim (unless frozen): the next frame when the
// timer runs out, and at the end the end code's stop, hold, loop or jump.
// ROR 0x0018d640

void SprMan::updateAnim(SprSprite *spr)
{
    if (spr->m_disp == 0)
        return;
    if (spr->m_disp != 2)
        spr->m_anim_timer--;
    if (spr->m_anim_timer >= 1)
        return;
    const SprAnim *anim = &spr->m_anim[spr->m_anim_no];
    const SprFrame *frames = sprFrames(this, anim);
    int next = spr->m_frame + 1;
    u_int ctrl;
    if (spr->m_frame < anim->m_num - 1) {
        spr->m_frame = next;
        spr->m_anim_timer = SPR_FRAME_TIME(frames[next].m_ctrl);
        ctrl = frames[next].m_ctrl;
    } else {
        spr->m_frame_flags = 0;
        const SprFrame *end = &frames[next];
        switch (end->m_ctrl & SPR_FRAME_END) {
        case SPR_END_STOP:
            spr->m_disp = 0;
            return;
        case SPR_END_LOOP:
            spr->m_frame = 0;
            spr->m_anim_timer = SPR_FRAME_TIME(frames[0].m_ctrl);
            ctrl = frames[0].m_ctrl;
            break;
        case SPR_END_JUMP:
            setSprAnim(spr, end->m_pos[0]);
            spr->m_frame = 0;
            frames = sprFrames(this, &spr->m_anim[spr->m_anim_no]);
            spr->m_anim_timer = SPR_FRAME_TIME(frames[0].m_ctrl);
            ctrl = frames[0].m_ctrl;
            break;
        default:    /* SPR_END_HOLD */
            return;
        }
    }
    spr->m_frame_flags = ctrl & SPR_FRAME_FLAGS;
}

// ──────────────────────────────────────────────────────────────────────
// Starts a colour fade of sprite no over time frames: mode 1 / 2 from from
// to to and back (to - step * n / from + step * n), 3 from the colour to
// from, 4 from the colour to to, 5 a step of 128 / time; 0x100 | mode
// ping-pongs (0x101 <-> 0x102).
// ROR 0x0018d7ec

void SprMan::setFade(int no, int mode, int time, const int *from, const int *to)
{
    SprSprite *spr = &m_sprite[no];
    spr->m_fade_mode = mode;
    spr->m_fade_count = time;
    spr->m_fade_time = time;
    for (int i = 0; i < 4; i++)
        spr->m_fade_step[i] = 0;
    for (int i = 0; i < 4; i++) {
        spr->m_fade_from[i] = from[i];
        spr->m_fade_to[i] = to[i];
    }
    if ((spr->m_fade_mode & 0xff00) == 0x100)
        spr->m_fade_mode = 0x101;
    u_int low = spr->m_fade_mode & 0xff;
    if (low - 1 >= 5)
        return;
    float t = (float)time;
    switch (low) {
    case 1:
    case 2:
        for (int i = 0; i < 4; i++)
            spr->m_fade_step[i] = (float)(to[i] - from[i]) / t;
        break;
    case 3:
        for (int i = 0; i < 4; i++)
            spr->m_fade_step[i] = (float)(spr->m_color[i] - from[i]) / t;
        break;
    case 4:
        for (int i = 0; i < 4; i++)
            spr->m_fade_step[i] = (float)(to[i] - spr->m_color[i]) / t;
        break;
    case 5: {
        float step = 128.0f / t;
        for (int i = 0; i < 4; i++)
            spr->m_fade_step[i] = step;
        break;
    }
    }
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the colour fade (not while a fade-in waits): the colour at
// the count, then at the end of the count the next phase (5: a fade from
// from to to; 0x101 / 0x102: the other way) or the end; colours clamped to
// 0..255.
// ROR 0x0018dafc

void SprMan::updateFade(SprSprite *spr)
{
    u_int mode = spr->m_fade_mode;
    if (mode == 0)
        return;
    if (spr->m_state == SPR_STATE_WAIT_IN || spr->m_state == SPR_STATE_SHOW_IN)
        return;
    u_int low = mode & 0xff;
    if (low - 1 < 5) {
        float n = (float)spr->m_fade_count;
        switch (low) {
        case 2:
        case 3:
            for (int i = 0; i < 4; i++)
                spr->m_color[i] = spr->m_fade_from[i] + (int)(spr->m_fade_step[i] * n);
            break;
        default:
            for (int i = 0; i < 4; i++)
                spr->m_color[i] = spr->m_fade_to[i] - (int)(spr->m_fade_step[i] * n);
            break;
        }
    }
    if (spr->m_fade_count < 1) {
        if (low != 0 && low <= 5) {
            if ((mode & 0xff00) == 0) {
                if (low == 5) {
                    float t = (float)spr->m_fade_time;
                    for (int i = 0; i < 4; i++)
                        spr->m_fade_step[i] = (float)(spr->m_fade_to[i] - spr->m_fade_from[i]) / t;
                    spr->m_fade_mode = 0x102;
                    spr->m_fade_count = spr->m_fade_time;
                    for (int i = 0; i < 4; i++)
                        spr->m_color[i] = spr->m_fade_from[i] + (int)(spr->m_fade_step[i] * t);
                } else {
                    spr->m_fade_mode = 0;
                }
            } else if ((mode & 0xff00) == 0x100) {
                spr->m_fade_count = spr->m_fade_time;
                float t = (float)spr->m_fade_count;
                if (low == 1) {
                    spr->m_fade_mode = 0x102;
                    for (int i = 0; i < 4; i++)
                        spr->m_color[i] = spr->m_fade_from[i] + (int)(spr->m_fade_step[i] * t);
                } else if (low == 2) {
                    spr->m_fade_mode = 0x101;
                    for (int i = 0; i < 4; i++)
                        spr->m_color[i] = spr->m_fade_to[i] - (int)(spr->m_fade_step[i] * t);
                }
            }
        }
    } else {
        spr->m_fade_count--;
    }
    for (int i = 0; i < 4; i++) {
        if (spr->m_color[i] < 0)
            spr->m_color[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
        if (spr->m_color[i] >= 0x100)
            spr->m_color[i] = 0xff;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The CLUT animation of a shown sprite with a texture: every m_clut_interval
// frames the CLUT index steps on, until it reaches m_clut_max.
// ROR 0x0018df20

void SprMan::updateClut(SprSprite *spr)
{
    int state = spr->m_state;
    if (state == SPR_STATE_OFF)
        return;
    if (state != SPR_STATE_ON && (state < SPR_STATE_SHOW_IN || state > SPR_STATE_TEXT))
        return;
    if (spr->m_tex == SPR_TEX_NONE || spr->m_clut_interval == 0)
        return;
    if (m_tex.getTexRecord(spr->m_tex) == NULL)
        return;
    if (spr->m_clut_timer < 1) {
        spr->m_clut += spr->m_clut_step;
        spr->m_clut_timer = spr->m_clut_interval;
        if (spr->m_clut >= spr->m_clut_max)
            spr->m_clut_interval = 0;
    } else {
        spr->m_clut_timer--;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The pixels of sprite no's texture (NULL without one). No caller found.
// ROR 0x0018dfdc

void *SprMan::getTexImage(int no)
{
    I3dTexEntry *rec = (I3dTexEntry *)m_tex.getTexRecord(m_sprite[no].m_tex);
    if (rec == NULL)
        return NULL;
    return ((I3dTexture *)rec->pPicture)->m_level[0].m_data;
}

// ──────────────────────────────────────────────────────────────────────
// 0 when the rect lies off the 640 x 480 screen (left / above, or right /
// below).
// ROR 0x0018e028

int SprMan::isOnScreen(int x0, int y0, int x1, int y1)
{
    int on = 1;
    if (x1 < 0 || y1 < 0)
        on = 0;
    if (x0 > 0x2800)
        on = 0;
    else if (y0 > 0x1e00)
        on = 0;
    return on;
}

// ──────────────────────────────────────────────────────────────────────
// Places one sprite (the parent's position + the standard point + the
// offset + the anim frame's) and takes the frame's pattern, then writes
// it into the open draw set: a plain colour sprite, a frame-buffer copy
// or a textured sprite (flipped by the frame flags); pending text is
// rendered into its texture first. The state's waits count here.
// ROR 0x0018e05c

void SprMan::drawSprite(SprSprite *spr, int layer)
{
    switch (spr->m_state) {
    case SPR_STATE_OFF:
        return;
    case SPR_STATE_SHOW_IN:
    case SPR_STATE_WAIT_IN:
        if (spr->m_fade_wait < 1)
            spr->m_state = SPR_STATE_ON;
        else
            spr->m_fade_wait--;
        break;
    case SPR_STATE_WAIT_OUT:
        if (spr->m_fade_wait > 0)
            spr->m_fade_wait--;
        else
            spr->m_state = SPR_STATE_OFF;
        break;
    case SPR_STATE_TEXT:
        if (spr->m_text == NULL)
            return;
        break;
    }
    spr->m_pos[0] = 0;
    spr->m_pos[1] = 0;
    spr->m_pos[2] = 0;

    switch (spr->m_state) {
    case SPR_STATE_ON:
    case SPR_STATE_SHOW_IN:
    case SPR_STATE_WAIT_OUT:
    case SPR_STATE_TEXT:
    case 8: {
        if (spr->m_parent >= 1) {
            const SprSprite *parent = &m_sprite[spr->m_parent];
            spr->m_pos[0] = parent->m_pos[0];
            spr->m_pos[1] = parent->m_pos[1];
            spr->m_pos[2] = parent->m_pos[2];
        }
        for (int i = 0; i < 3; i++)
            spr->m_pos[i] += spr->m_std[i] + spr->m_off[i];
        if (spr->m_disp != 0) {
            const SprFrame *frame = &sprFrames(this, &spr->m_anim[spr->m_anim_no])[spr->m_frame];
            const SprPattern *pat = &((const SprPattern *)m_pattern)[spr->m_pat_base + frame->m_pat];
            spr->m_tex = pat->m_tex;
            for (int i = 0; i < 3; i++)
                spr->m_pos[i] += frame->m_pos[i];
            if (frame->m_w != SPR_SIZE_KEEP)
                spr->m_draw_w = frame->m_w;
            if (frame->m_h != SPR_SIZE_KEEP)
                spr->m_draw_h = frame->m_h;
            if (spr->m_tex != SPR_TEX_FRAME || spr->m_u == 0) {
                spr->m_u = pat->m_u;
                spr->m_v = pat->m_v;
            }
            if ((spr->m_frame_flags & SPR_FRAME_SIZE) == 0) {
                spr->m_w = pat->m_w;
                spr->m_h = pat->m_h;
            } else {
                if (spr->m_draw_w == SPR_SIZE_PATTERN) {
                    spr->m_w = pat->m_w;
                    spr->m_draw_w = pat->m_w;
                } else {
                    spr->m_w = frame->m_w;
                }
                if (spr->m_draw_h != SPR_SIZE_PATTERN) {
                    spr->m_h = frame->m_h;
                } else {
                    spr->m_h = pat->m_h;
                    spr->m_draw_h = pat->m_h;
                }
            }
            if (spr->m_draw_w == SPR_SIZE_PATTERN)
                spr->m_draw_w = spr->m_w;
            if (spr->m_draw_h == SPR_SIZE_PATTERN)
                spr->m_draw_h = spr->m_h;
        }
        if (spr->m_fix_w != -1)
            spr->m_draw_w = spr->m_fix_w;
        if (spr->m_fix_h != -1)
            spr->m_draw_h = spr->m_fix_h;
        if (spr->m_w == 0 || spr->m_h == 0)
            return;

        int x = spr->m_pos[0];
        int y = spr->m_pos[1];
        int z = spr->m_pos[2];
        u_long xyz0, xyz1, uv0, uv1;
        if (spr->m_tex == SPR_TEX_COLOR) {
            if (isOnScreen(x, y, x + spr->m_draw_w, y + spr->m_draw_h) == 1) {
                m_pkt = i3dPktSetReg(m_pkt, SCE_GS_ALPHA_1, SPR_ALPHA_BLEND);
                /* TEST_1: the alpha test never passes and writes the frame
                 * buffer only (no Z), depth test ALWAYS */
                m_pkt = i3dPktSetReg(m_pkt, SCE_GS_TEST_1, SCE_GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 1));
                m_pkt = i3dPktCallView(m_pkt, m_layer[layer]);
                xyz0 = i3dPackXYZ(x + SPR_GS_OFFSET_X, y + SPR_GS_OFFSET_Y, z);
                xyz1 = i3dPackXYZ(x + spr->m_draw_w + SPR_GS_OFFSET_X, y + spr->m_draw_h + SPR_GS_OFFSET_Y, z);
                u_long rgbaq = i3dPackRGBAQ(1.0f, spr->m_color[0], spr->m_color[1], spr->m_color[2], spr->m_color[3]);
                m_pkt = i3dPktSprite(m_pkt, xyz0, xyz1, rgbaq);
            }
            return;
        }
        if (spr->m_tex == SPR_TEX_FRAME) {
            if (isOnScreen(x, y, x + spr->m_draw_w, y + spr->m_draw_h) != 1)
                return;
            m_pkt = i3dPktSetReg(m_pkt, SCE_GS_ALPHA_1, SPR_ALPHA_BLEND);
            m_pkt = i3dPktCallView(m_pkt, m_layer[layer]);
            /* TEX0: TBP0 m_u << 4, TBW from the width, PSMCT32, 128 x 128 */
            int64_t tbw = (spr->m_w >> 4) / 64;
            m_pkt = i3dPktSetReg(m_pkt, SCE_GS_TEX0_1,
                                 (u_long)(int64_t)(spr->m_u << 4) | (u_long)(tbw << 14) | 0x1dc000000ull);
            m_pkt = i3dPktSetReg(m_pkt, SCE_GS_ALPHA_1, SPR_ALPHA_BLEND);
            xyz0 = i3dPackXYZ(x + SPR_GS_OFFSET_X, y + SPR_GS_OFFSET_Y, z);
            xyz1 = i3dPackXYZ(x + spr->m_draw_w + SPR_GS_OFFSET_X, y + spr->m_draw_h + SPR_GS_OFFSET_Y, z);
            uv0 = i3dPackUV(0, 0);
            uv1 = i3dPackUV(spr->m_w, spr->m_h);
        } else {
            if (spr->m_tex == SPR_TEX_NONE)
                return;
            if (isOnScreen(x, y, x + spr->m_draw_w, y + spr->m_draw_h) != 1)
                return;
            int slot = m_tex.getSlot(spr->m_tex);
            I3dTexEntry *rec = (I3dTexEntry *)m_tex.getTexRecord(spr->m_tex);
            if (rec == NULL)
                return;
            rec->m_0c = spr->m_clut;
            /* ROM: the texture is always taken in layer 0's draw set */
            u_long *regs = i3dDrawSetUseTex(m_draw_set[0], slot);
            if (spr->m_text != NULL || spr->m_clear_rect != NULL) {
                /* the record's picture is the I3dTexture (its handle first) */
                I3dTexture *pic = (I3dTexture *)rec->pPicture;
                void *image = pic->m_level[0].m_data;
                int w = pic->m_width;
                int h = pic->m_height;
                if (spr->m_clear_rect != NULL) {
                    clearTextRect(spr, image, w, h);
                    spr->m_clear_rect = NULL;
                    if (spr->m_text == NULL)
                        i3dTexRelease(pic);
                }
                if (spr->m_text != NULL) {
                    drawText(spr, image, w, h);
                    i3dTexRelease(pic);
                    spr->m_text = NULL;
                }
            }
            if (spr->m_state == SPR_STATE_TEXT)
                return;
            m_pkt = i3dPktSetReg(m_pkt, SCE_GS_TEXA, SCE_GS_SET_TEXA(0, 1, 0));
            m_pkt = i3dPktSetReg(m_pkt, SCE_GS_ALPHA_1, SPR_ALPHA_BLEND);
            m_pkt = i3dPktRefTexRegs(m_pkt, regs);
            int u0 = spr->m_u;
            int v0 = spr->m_v;
            int u1 = u0 + spr->m_w;
            int v1 = v0 + spr->m_h;
            if (spr->m_disp != 0) {
                if (spr->m_frame_flags & SPR_FRAME_FLIP_U) {
                    int t = u0;
                    u0 = u1;
                    u1 = t;
                }
                if (spr->m_frame_flags & SPR_FRAME_FLIP_V) {
                    int t = v0;
                    v0 = v1;
                    v1 = t;
                }
            }
            xyz0 = i3dPackXYZ(x + SPR_GS_OFFSET_X, y + SPR_GS_OFFSET_Y, z);
            xyz1 = i3dPackXYZ(x + spr->m_draw_w + SPR_GS_OFFSET_X, y + spr->m_draw_h + SPR_GS_OFFSET_Y, z);
            uv0 = i3dPackUV(u0, v0);
            uv1 = i3dPackUV(u1, v1);
        }
        u_long rgbaq = i3dPackRGBAQ(1.0f, spr->m_color[0], spr->m_color[1], spr->m_color[2], spr->m_color[3]);
        m_pkt = i3dPktTexSpriteUV(m_pkt, xyz0, xyz1, uv0, uv1, rgbaq);
        break;
    }
    case SPR_STATE_PLACE: {
        if (spr->m_parent >= 1) {
            const SprSprite *parent = &m_sprite[spr->m_parent];
            spr->m_pos[0] = parent->m_pos[0];
            spr->m_pos[1] = parent->m_pos[1];
            spr->m_pos[2] = parent->m_pos[2];
        }
        for (int i = 0; i < 3; i++)
            spr->m_pos[i] += spr->m_std[i] + spr->m_off[i];
        if (spr->m_disp != 0) {
            const SprFrame *frame = &sprFrames(this, &spr->m_anim[spr->m_anim_no])[spr->m_frame];
            const SprPattern *pat = &((const SprPattern *)m_pattern)[spr->m_pat_base + frame->m_pat];
            spr->m_tex = pat->m_tex;
            for (int i = 0; i < 3; i++)
                spr->m_pos[i] += frame->m_pos[i];
            spr->m_draw_w = frame->m_w;
            spr->m_draw_h = frame->m_h;
            spr->m_u = pat->m_u;
            spr->m_v = pat->m_v;
            spr->m_w = pat->m_w;
            spr->m_h = pat->m_h;
        }
        break;
    }
    case SPR_STATE_STOP:
        spr->m_state = SPR_STATE_OFF;
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Clears the sprite's text rect (m_clear_rect, from its pattern's corner)
// in its texture image.
// ROR 0x0018ea9c

void SprMan::clearTextRect(SprSprite *spr, void *image, int w, int h)
{
    int u = spr->m_u;
    int v = spr->m_v;
    const int *rect = spr->m_clear_rect;
    m_font[0].setTarget(image, w, h, 4);
    m_font[0].fillRect((u >> 4) + rect[0], (v >> 4) + rect[1], rect[2], rect[3]);
}

// ──────────────────────────────────────────────────────────────────────
// Copies line n of a text ("#0" ends a line; Shift-JIS pairs kept whole)
// to line, at most 0x3f bytes.
// ROR 0x0018eba4

void SprMan::getTextLine(const char *text, char *line, int n)
{
    const u_char *p = (const u_char *)text;
    int no = 0;
    int len = 0;
    while (*p != 0) {
        u_int c = *p;
        if (n == no)
            *line = (char)c;
        const u_char *q = p + 1;
        if (c == '#' && *q == '0') {
            q = p + 2;
            if (n == no)
                break;
            no++;
        } else {
            if (sprIsSjisLead(c)) {
                if (n == no) {
                    line++;
                    len++;
                    *line = (char)*q;
                }
                q = p + 2;
            }
            if (n == no) {
                line++;
                len++;
            }
            if (len > 0x3e)
                break;
        }
        p = q;
    }
    *line = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The width of a text in the font (proportional fonts: 2 pixels between
// glyphs); "#0" and newlines add nothing.
// ROR 0x0018ec58

int SprMan::getTextWidth(const char *text)
{
    const u_char *p = (const u_char *)text;
    int w = 0;
    while (*p != 0) {
        u_int c = *p++;
        if (sprIsSjisLead(c))
            c = (c << 8) | *p++;
        if (c == '#' && *p == '0') {
            p++;
            continue;
        }
        if (c == '\n')
            continue;
        GamFont *font = &m_font[0];
        if (font->m_prop != 0)
            w += font->getGlyphWidth(font->getGlyph(c)) + 2;
        else
            w += font->getGlyphWidth(c);
    }
    return w;
}

// ──────────────────────────────────────────────────────────────────────
// Renders the sprite's text into its texture image in colour 15, from its
// pattern's corner + the text origin: "#0" and newlines start a line, a
// space moves half a cell, a full-width space a cell.
// ROR 0x0018ed44

void SprMan::drawText(SprSprite *spr, void *image, int w, int h)
{
    const u_char *p = (const u_char *)spr->m_text;
    int y = (spr->m_v >> 4) + spr->m_text_y;
    int x0 = (spr->m_u >> 4) + spr->m_text_x;
    GamFont *font = &m_font[0];
    font->setTarget(image, w, h, 4);
    font->setColor(0xf);
    int x = x0;
    while (*p != 0) {
        u_int c = *p++;
        if (sprIsSjisLead(c))
            c = (c << 8) | *p++;
        if (c == '#' && *p == '0') {
            p++;
            y += font->m_char_h;
            x = x0;
        } else if (c == '\n') {
            y += font->m_char_h;
            x = x0;
        } else if (c == ' ') {
            x += font->m_char_w / 2;
        } else if (c == 0x8140) {
            x += font->m_char_w;
        } else if (font->m_prop == 0) {
            /* ROM: the code goes where getGlyphWidth wants a glyph (fixed
             * fonts ignore it) */
            int adv = font->getGlyphWidth(c);
            font->drawChar(x, y, c);
            x += adv;
        } else {
            int glyph = font->getGlyph(c);
            int left = font->getGlyphLeft(glyph);
            int gw = font->getGlyphWidth(glyph);
            x -= left;
            font->drawChar(x, y, c);
            x += left + gw + 2;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Opens layer n's draw set for the loaded textures (found or made by
// i3dLayerFindDrawSet); 0 when m_pkt is the open packet.
// ROR 0x0018ef40

int SprMan::beginDraw(int layer)
{
    I3dLayer *l = m_layer[layer];
    if (l == NULL)
        return 1;
    I3dTexSetDesc desc;
    desc.m_tex_no = m_tex.m_tex_no;
    desc.m_tex_count = m_tex.m_count;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_depth = 1.0f;
    desc.m_sort_param = 0;
    I3dDrawSet *set = i3dLayerFindDrawSet(l, &desc);
    m_draw_set[layer] = set;
    if (set == NULL)
        return 1;
    i3dLayerSetDrawSet(l, set);
    m_pkt = i3dLayerBeginSet(l, 0.0f);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Closes layer n's draw set at m_pkt.
// ROR 0x0018eff0

void SprMan::endDraw(int layer)
{
    I3dLayer *l = m_layer[layer];
    if (l != NULL)
        i3dLayerEndSet(l, m_pkt);
}

// ──────────────────────────────────────────────────────────────────────
// Restores the game volume when one is saved.
// ROR 0x0018f024

void SprMan::volumeOn()
{
    if (g_gamMan.m_saved_volume != -1)
        g_gamMan.switchVolume(1);
}

// ──────────────────────────────────────────────────────────────────────
// Silences the game (saving its volume) unless already silenced.
// ROR 0x0018f060

void SprMan::volumeOff()
{
    if (g_gamMan.m_saved_volume == -1)
        g_gamMan.switchVolume(0);
}

// ──────────────────────────────────────────────────────────────────────
// Remembers whether the volume is on.
// ROR 0x0018f09c

void SprMan::saveVolume()
{
    m_volume_on = g_gamMan.m_saved_volume == -1;
}

// ──────────────────────────────────────────────────────────────────────
// Puts the volume back to what saveVolume saw.
// ROR 0x0018f0b4

void SprMan::restoreVolume()
{
    if (m_volume_on == 0)
        volumeOff();
    else if (m_volume_on == 1)
        volumeOn();
}

// ──────────────────────────────────────────────────────────────────────
// Turns sprite no on with anim anim (full colour) and / or move move from
// the move table; -1 leaves either alone.
// ROR 0x0018f0ec

void SprMan::showSprite(int no, int anim, int move)
{
    if (anim != -1) {
        setAnim(no, anim);
        SprSprite *spr = &m_sprite[no];
        spr->m_disp = 1;
        spr->m_color[0] = 0x80;
        spr->m_color[1] = 0x80;
        spr->m_color[2] = 0x80;
        spr->m_color[3] = 0x80;
    }
    if (move != -1) {
        setMove(no, move);
        m_sprite[no].m_move_mode = SPR_MOVE_TABLE;
    }
    m_sprite[no].m_state = SPR_STATE_ON;
}

// ──────────────────────────────────────────────────────────────────────
// showSprite, then a fade from from to to over time frames once the
// sprite has waited wait frames hidden.
// ROR 0x0018f1f4

void SprMan::fadeInSprite(int no, int wait, int time, int anim, int move,
                          const int *from, const int *to)
{
    showSprite(no, anim, move);
    SprSprite *spr = &m_sprite[no];
    spr->m_color[0] = from[0];
    spr->m_color[1] = from[1];
    spr->m_color[2] = from[2];
    spr->m_color[3] = from[3];
    spr->m_state = SPR_STATE_WAIT_IN;
    spr->m_fade_wait = wait;
    setFade(no, 4, time, from, to);
}

// ──────────────────────────────────────────────────────────────────────
// A fade from the sprite's colour to to over time frames, after wait
// frames shown.
// ROR 0x0018f2e8

void SprMan::fadeInWait(int no, int wait, int time, const int *from, const int *to)
{
    m_sprite[no].m_state = SPR_STATE_SHOW_IN;
    m_sprite[no].m_fade_wait = wait;
    setFade(no, 4, time, from, to);
}

// ──────────────────────────────────────────────────────────────────────
// Turns an off sprite on (anim on, grey with alpha 0) and fades it to to.
// ROR 0x0018f338

void SprMan::fadeIn(int no, int time, const int *from, const int *to)
{
    SprSprite *spr = &m_sprite[no];
    if (spr->m_state != SPR_STATE_OFF)
        return;
    spr->m_state = SPR_STATE_ON;
    spr->m_disp = 1;
    spr->m_color[0] = 0x80;
    spr->m_color[1] = 0x80;
    spr->m_color[2] = 0x80;
    spr->m_color[3] = 0;
    setFade(no, 4, time, from, to);
}

// ──────────────────────────────────────────────────────────────────────
// Turns all three off at once.
// ROR 0x0018f3e0

void SprMan::hideSprite(int no)
{
    m_sprite[no].m_disp = 0;
    m_sprite[no].m_move_mode = SPR_MOVE_NONE;
    m_sprite[no].m_state = SPR_STATE_OFF;
}

// ──────────────────────────────────────────────────────────────────────
// Fades the sprite to clear grey over time frames, then turns it off.
// ROR 0x0018f41c

void SprMan::fadeOutDefault(int no, int time)
{
    setFade(no, 3, time, g_sprColorClear, g_sprColorOpaque);
    m_sprite[no].m_state = SPR_STATE_WAIT_OUT;
    m_sprite[no].m_fade_wait = time;
}

// ──────────────────────────────────────────────────────────────────────
// Fades the sprite to from over time frames, then turns it off.
// ROR 0x0018f4a0

void SprMan::fadeOutColor(int no, int time, const int *from, const int *to)
{
    setFade(no, 3, time, from, to);
    m_sprite[no].m_state = SPR_STATE_WAIT_OUT;
    m_sprite[no].m_fade_wait = time;
}

// ──────────────────────────────────────────────────────────────────────
// fadeOutColor for a sprite that is on.
// ROR 0x0018f520

void SprMan::fadeOut(int no, int time, const int *from, const int *to)
{
    if (m_sprite[no].m_state == SPR_STATE_OFF)
        return;
    setFade(no, 3, time, from, to);
    m_sprite[no].m_state = SPR_STATE_WAIT_OUT;
    m_sprite[no].m_fade_wait = time;
}
