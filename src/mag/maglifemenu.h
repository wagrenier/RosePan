/* ==========================================================================
 *  mag/maglifemenu.h
 *
 *  MagLifeMenuBase (mag/maglifemenu.cpp; base MagLife, vptr at +0x14): the
 *  base of the scene's menus. The scene embeds one of each (MagScene
 *  +0x2be60..) and runs one at a time through m_pMenuBase (+0x2be5c):
 *  MagScene::openMenu sets m_type / the scene and runs init, the scene's
 *  update feeds processPad the pad until it returns non-zero, closeMenu
 *  runs term, MagLifeMan draws the open one through render (slot 6).
 *
 *  Most menus are thin wrappers: the menu object at +0x40 (lib/unassigned.h,
 *  mnu/) does the work and the wrapper forwards init / term / isInit /
 *  processPad / render to it, handing it the scene first. MagLifeMenuPBook
 *  picks one of two books by type; MagLifeMenuBoard3D is the only menu with
 *  code of its own (a model shown by the scene's camera).
 *
 *  EE virtual slots (all menus): 1 ~dtor, 2 MagLife::live, 3 attached,
 *  4 getLevel, 5 getName (0 in every menu), 6 render, 7 setHide, 8 getHide,
 *  9 init, 10 term, 11 isInit, 12 processPad. MagLifeMenuBase has no vtable
 *  of its own in the ELF (abstract): 9..12 are pure here. getName is a plain
 *  member (MagLife does not declare slot 5 yet). The wrappers' constructors,
 *  destructors, term, isInit and getName are linkonce in the ELF (defined
 *  in the class body): they are inline below. Their __tf type-info
 *  functions are the host compiler's RTTI.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEMENU_H
#define _MAG_MAGLIFEMENU_H

#include <stddef.h>

#include "lib/unassigned.h"
#include "mag/maglife.h"
#include "mnu/mnuSelectLanguage.h"
#include "mnu/mnusvld.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;

/* EE 0x40 (the menus' own members start at +0x40). layout: host --
 * polymorphic in the ROM (vptr +0x14); EE offsets in the comments. */
struct MagLifeMenuBase : MagLife
{
    /* The MagLife0 / MagLife / MagLifeMenuBase words every menu
     * constructor stores (the ROM inlines them). */
    MagLifeMenuBase()
    {
        m_state = 0;
        m_04 = 0;
        m_08 = 0;
        m_0c = 0;
        m_level = 1;
        m_04 = -1;
        m_state = 0;
        m_hide = 0;
        m_scene = NULL;
        m_menu_scene = NULL;
    }

    /* EE slot 9 (+0x48): loads the menu's textures into tex. */
    virtual void init(DogTexInfo *tex) = 0;
    /* EE slot 10 (+0x50) */
    virtual void term() = 0;
    /* EE slot 11 (+0x58): non-zero once init has run. */
    virtual int  isInit() = 0;
    /* EE slot 12 (+0x60): one frame of input; non-zero closes the menu. */
    virtual int  processPad(BedPad *pad) = 0;

    /* The value of m_type's row in the menu type table (data 0x298318):
     * the board model / page id; -1 when m_type is not in it. */
    int  getTypeValue();                                                 // ROR 0x0012cf3c

    u_char    _28[0x30 - 0x28];
    /* ROR 0x30: the ROM's m_scene (the asserts' name), renamed on the host
     * since MagLife has its own m_scene at +0x20. openMenu stores it
     * before init. */
    MagScene *m_menu_scene;
    int       m_type;           // ROR 0x34: the asserts' m_type: openMenu's id for the board / pbook families and 0x1000c..; getTypeValue's key
    int       m_menu_arg;       // ROR 0x38: requestMenu's argument
    u_char    _3c[0x40 - 0x3c];
};
static_assert(offsetof(MagLifeMenuBase, m_menu_scene) >= 0x30, "m_menu_scene");
static_assert(offsetof(MagLifeMenuBase, m_type) >= 0x34, "m_type");
static_assert(offsetof(MagLifeMenuBase, m_menu_arg) >= 0x38, "m_menu_arg");

/* The language select, menu 0x10010 (vtable 0x2e1b58). EE 0x10d0. */
struct MagLifeMenuSelLang : MagLifeMenuBase
{
    // ROR 0x00283ea4; callcheck: MnuSelectLanguage::MnuSelectLanguage (not written; the member starts zeroed)
    MagLifeMenuSelLang() {}
    // ROR 0x00283ef4; callcheck: MnuSelectLanguage::~MnuSelectLanguage (the member's)
    ~MagLifeMenuSelLang() {}
    const char *getName() { return NULL; }                               // ROR 0x00283f98
    void render(I3dLayer *layer) override;                               // ROR 0x0012d024
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012cfe0
    // ROR 0x00283f64
    void term() override { m_menu.term(); }
    // ROR 0x00283f80
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d00c

    MnuSelectLanguage m_menu;   // ROR 0x40
};

/* The item menu, 0x10000 (vtable 0x2e1ae8). EE 0x2750. */
struct MagLifeMenuItem : MagLifeMenuBase
{
    // ROR 0x00283ff0; callcheck: MnuItem::MnuItem (not written; the member starts zeroed)
    MagLifeMenuItem() {}
    // ROR 0x00284040; callcheck: MnuItem::~MnuItem (the member's)
    ~MagLifeMenuItem() {}
    const char *getName() { return NULL; }                               // ROR 0x002840e4
    void render(I3dLayer *layer) override;                               // ROR 0x0012d084
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d040
    // ROR 0x002840b0
    void term() override { m_menu.term(); }
    // ROR 0x002840cc
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d06c

    MnuItem   m_menu;           // ROR 0x40
};

/* The map, 0x10001 (vtable 0x2e1a78). EE 0x920. */
struct MagLifeMenuMap : MagLifeMenuBase
{
    // ROR 0x0028413c; callcheck: MnuMap::MnuMap (not written; the member starts zeroed but for its vptr)
    MagLifeMenuMap() {}
    // ROR 0x0028418c; callcheck: MnuMap::~MnuMap (the member's)
    ~MagLifeMenuMap() {}
    const char *getName() { return NULL; }                               // ROR 0x00284224
    void render(I3dLayer *layer) override;                               // ROR 0x0012d0e4
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d0a0
    // ROR 0x002841fc
    void term() override { m_menu.term(); }
    /* mnuBase::isInit inlined: bit 0 of the map's flags. */
    // ROR 0x00284218
    int  isInit() override { return m_menu.m_flags & 1; }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d0cc

    MnuMap    m_menu;           // ROR 0x40
};

/* The stock (item box), 0x10002 (vtable 0x2e1a08). EE 0x15f0. */
struct MagLifeMenuStock : MagLifeMenuBase
{
    // ROR 0x0028427c; callcheck: MnuStock::MnuStock (not written; the member starts zeroed)
    MagLifeMenuStock() {}
    // ROR 0x002842cc; callcheck: MnuStock::~MnuStock (the member's)
    ~MagLifeMenuStock() {}
    const char *getName() { return NULL; }                               // ROR 0x00284370
    void render(I3dLayer *layer) override;                               // ROR 0x0012d144
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d100
    // ROR 0x0028433c
    void term() override { m_menu.term(); }
    // ROR 0x00284358
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d12c

    MnuStock  m_menu;           // ROR 0x40
};

/* The save screen, 0x10003 (vtable 0x2e1998). EE 0x27c0. */
struct MagLifeMenuSave : MagLifeMenuBase
{
    // ROR 0x002843c8; callcheck: MnuSvld::MnuSvld (not written; the member starts zeroed)
    MagLifeMenuSave() { m_menu.m_save = 1; }
    // ROR 0x00284434; callcheck: MnuSvld::~MnuSvld (the member's)
    ~MagLifeMenuSave() {}
    const char *getName() { return NULL; }                               // ROR 0x002844dc
    void render(I3dLayer *layer) override;                               // ROR 0x0012d1a8
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d160
    // ROR 0x002844a4
    void term() override { m_menu.term(); }
    // ROR 0x002844c0
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d18c

    MnuSvld   m_menu;           // ROR 0x40
};

/* The load screen, 0x10004 (vtable 0x2e1928): the save screen's class
 * with m_save 0. EE 0x27c0. */
struct MagLifeMenuLoad : MagLifeMenuBase
{
    // ROR 0x00284534; callcheck: MnuSvld::MnuSvld (not written; the member starts zeroed)
    MagLifeMenuLoad() { m_menu.m_save = 0; }
    // ROR 0x00284598; callcheck: MnuSvld::~MnuSvld (the member's)
    ~MagLifeMenuLoad() {}
    const char *getName() { return NULL; }                               // ROR 0x00284640
    void render(I3dLayer *layer) override;                               // ROR 0x0012d20c
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d1c4
    // ROR 0x00284608
    void term() override { m_menu.term(); }
    // ROR 0x00284624
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d1f0

    MnuSvld   m_menu;           // ROR 0x40
};

/* The personal books, the type families 0x100000 / 0x200000 (vtable
 * 0x2e16f8): init asks each book whether the type's page is its own and
 * runs that one through mnuBase's virtuals. EE 0x1200. */
struct MagLifeMenuPBook : MagLifeMenuBase
{
    MagLifeMenuPBook();                                                  // ROR 0x0012d398
    // ROR 0x00284bc8; callcheck: MnuDiary::~MnuDiary, MnuPBook::~MnuPBook (the members', in that order)
    ~MagLifeMenuPBook() {}
    const char *getName() { return NULL; }                               // ROR 0x00284cbc
    void render(I3dLayer *layer) override;                               // ROR 0x0012d4fc
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d400
    // ROR 0x00284c44
    void term() override
    {
        if (m_cur)
            m_cur->term();
    }
    // ROR 0x00284c7c
    int  isInit() override
    {
        if (!m_cur)
            return 0;
        return m_cur->isInit();
    }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d4cc

    mnuBase  *m_cur;            // ROR 0x40: the book init chose, NULL before
    u_char    _44[0x50 - 0x44];
    MnuPBook  m_pbook;          // ROR 0x50
    MnuDiary  m_diary;          // ROR 0x910
};

/* The title screen, 0x10005 (vtable 0x2e17d8). EE 0x5740. */
struct MagLifeMenuTitle : MagLifeMenuBase
{
    // ROR 0x00284930; callcheck: MnuTitle::MnuTitle (not written; the member starts zeroed)
    MagLifeMenuTitle() {}
    // ROR 0x00284980; callcheck: MnuTitle::~MnuTitle (the member's)
    ~MagLifeMenuTitle() {}
    const char *getName() { return NULL; }                               // ROR 0x00284a24
    void render(I3dLayer *layer) override;                               // ROR 0x0012d32c
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d2e8
    // ROR 0x002849f0
    void term() override { m_menu.term(); }
    // ROR 0x00284a0c
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d314

    MnuTitle  m_menu;           // ROR 0x40
};

/* The options, 0x10006 (vtable 0x2e18b8). EE 0x1130. */
struct MagLifeMenuOpt : MagLifeMenuBase
{
    // ROR 0x00284698; callcheck: MnuOpt::MnuOpt (not written; the member starts zeroed)
    MagLifeMenuOpt() {}
    // ROR 0x002846e8; callcheck: MnuOpt::~MnuOpt (the member's)
    ~MagLifeMenuOpt() {}
    const char *getName() { return NULL; }                               // ROR 0x0028478c
    void render(I3dLayer *layer) override;                               // ROR 0x0012d26c
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d228
    // ROR 0x00284758
    void term() override { m_menu.term(); }
    // ROR 0x00284774
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d254

    MnuOpt    m_menu;           // ROR 0x40
};

/* The movie theatre, 0x10007 (vtable 0x2e1848). EE 0x9d0. */
struct MagLifeMenuMovie : MagLifeMenuBase
{
    // ROR 0x002847e4; callcheck: MnuMovie::MnuMovie (not written; the member starts zeroed)
    MagLifeMenuMovie() {}
    // ROR 0x00284834; callcheck: MnuMovie::~MnuMovie (the member's)
    ~MagLifeMenuMovie() {}
    const char *getName() { return NULL; }                               // ROR 0x002848d8
    void render(I3dLayer *layer) override;                               // ROR 0x0012d2cc
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d288
    // ROR 0x002848a4
    void term() override { m_menu.term(); }
    // ROR 0x002848c0
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d2b4

    MnuMovie  m_menu;           // ROR 0x40
};

/* The memory card check, 0x10008 (vtable 0x2e1768); its init does not
 * hand the scene over. EE 0x1440. */
struct MagLifeMenuMcd : MagLifeMenuBase
{
    // ROR 0x00284a7c; callcheck: MnuMcd::MnuMcd (not written; the member starts zeroed)
    MagLifeMenuMcd() {}
    // ROR 0x00284acc; callcheck: MnuMcd::~MnuMcd (the member's)
    ~MagLifeMenuMcd() {}
    const char *getName() { return NULL; }                               // ROR 0x00284b70
    void render(I3dLayer *layer) override;                               // ROR 0x0012d37c
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d348
    // ROR 0x00284b3c
    void term() override { m_menu.term(); }
    // ROR 0x00284b58
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d364

    MnuMcd    m_menu;           // ROR 0x40
};

/* The status ("now") screen, 0x10009 (vtable 0x2e1688). EE 0x960. */
struct MagLifeMenuNow : MagLifeMenuBase
{
    // ROR 0x00284d14; callcheck: MnuNow::MnuNow (not written; the member starts zeroed)
    MagLifeMenuNow() {}
    // ROR 0x00284d64; callcheck: MnuNow::~MnuNow (the member's)
    ~MagLifeMenuNow() {}
    const char *getName() { return NULL; }                               // ROR 0x00284e08
    void render(I3dLayer *layer) override;                               // ROR 0x0012d570
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d52c
    // ROR 0x00284dd4
    void term() override { m_menu.term(); }
    // ROR 0x00284df0
    int  isInit() override { return m_menu.isInit(); }
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d558

    MnuNow    m_menu;           // ROR 0x40
};

/* The 2D board, the type family 0x40000 (vtable 0x2e1f00). Every method is
 * linkonce (0x2856a0..0x2857c0). EE 0x1100. */
struct MagLifeMenuBoard2D : MagLifeMenuBase
{
    // ROR 0x002856a0; callcheck: MnuBoard2D::~MnuBoard2D (the member's)
    ~MagLifeMenuBoard2D() {}
    const char *getName() { return NULL; }                               // ROR 0x002857b8
    // ROR 0x0028579c
    void render(I3dLayer *layer) override { m_menu.draw(layer); }
    // ROR 0x00285710
    void init(DogTexInfo *tex) override
    {
        m_menu.m_board = getTypeValue();
        m_menu.init(tex);
    }
    // ROR 0x00285750
    void term() override { m_menu.term(); }
    // ROR 0x0028576c
    int  isInit() override { return m_menu.isInit(); }
    // ROR 0x00285784
    int  processPad(BedPad *pad) override { return m_menu.processPad(pad); }

    MnuBoard2D m_menu;          // ROR 0x40
};

/* The 3D board, the type family 0x80000 (vtable 0x2e1618): loads the
 * type's model as a scene actor, shows it under a fixed camera between a
 * fade in and (pad bit 13) a fade out. EE 0x60. */
struct MagLifeMenuBoard3D : MagLifeMenuBase
{
    MagLifeMenuBoard3D();                                                // ROR 0x0012d58c
    ~MagLifeMenuBoard3D();                                               // ROR 0x0012d5e0
    const char *getName() { return NULL; }                               // ROR 0x00284e60
    void render(I3dLayer *theRenderStage) override;                      // ROR 0x0012d788
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d648
    void term() override;                                                // ROR 0x0012d6c0
    int  isInit() override;                                              // ROR 0x0012d72c
    int  processPad(BedPad *pad) override;                               // ROR 0x0012d73c

    int       m_step;           // ROR 0x40: 0 set up, 1 fading in, 2 shown, 3 fading out, 4 done
    int       m_wait;           // ROR 0x44: -1 on each step change; 1 once step 3 started its fade
    int       m_48;             // ROR 0x48: the constructor clears it; role unknown
    int       m_4c;             // ROR 0x4c: the constructor clears it; role unknown
    int       m_actor_no;       // ROR 0x50: the scene actor init loaded, -1 = none
    u_char    _54[0x60 - 0x54];
};
static_assert(offsetof(MagLifeMenuBoard3D, m_step) >= 0x40, "m_step");
static_assert(offsetof(MagLifeMenuBoard3D, m_actor_no) >= 0x50, "m_actor_no");

static_assert(offsetof(MagLifeMenuSelLang, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuItem, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuMap, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuStock, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuSave, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuLoad, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuPBook, m_cur) >= 0x40, "m_cur");
static_assert(offsetof(MagLifeMenuPBook, m_pbook) >= 0x50, "m_pbook");
static_assert(offsetof(MagLifeMenuPBook, m_diary) >= 0x910, "m_diary");
static_assert(offsetof(MagLifeMenuTitle, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuOpt, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuMovie, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuMcd, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuNow, m_menu) >= 0x40, "m_menu");
static_assert(offsetof(MagLifeMenuBoard2D, m_menu) >= 0x40, "m_menu");

#endif /* _MAG_MAGLIFEMENU_H */
