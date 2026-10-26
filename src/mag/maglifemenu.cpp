// mag/maglifemenu.cpp -- the scene's menus (MagLifeMenuBase and its 14
// classes): the type table lookup, the wrappers' init / processPad / render
// slots that hand the scene to the menu object at +0x40 and forward to it,
// the personal books' choice of book, and the 3D board.

#include "mag/maglifemenu.h"

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "mag/magscene.h"

/* data 0x298318: menu type -> value, 267 rows and a -1 end. 0x40001..
 * map to themselves, the 3D boards 0x80001.. to model ids 0x10c5..,
 * 0x400001.. to 0, the books 0x100001.. / 0x200001.. to page ids. */
struct MagLifeMenuTypeRec
{
    int m_type;                 // ROR 0x00
    int m_value;                // ROR 0x04
};
static_assert(sizeof(MagLifeMenuTypeRec) == 0x8, "EE 0x8");
static_assert(offsetof(MagLifeMenuTypeRec, m_value) == 0x4, "m_value");

// 268 x 0x8 bytes at 0x00298318 (datadump.py)
static const MagLifeMenuTypeRec s_menuTypeTable[268] = {
    { 0x40001, 0x40001 }, { 0x40002, 0x40002 }, { 0x40003, 0x40003 }, { 0x40004, 0x40004 }, // 0x00298318
    { 0x40005, 0x40005 }, { 0x40006, 0x40006 }, { 0x40007, 0x40007 }, { 0x40008, 0x40008 }, // 0x00298338
    { 0x40009, 0x40009 }, { 0x4000a, 0x4000a }, { 0x4000b, 0x4000b }, { 0x4000c, 0x4000c }, // 0x00298358
    { 0x4000d, 0x4000d }, { 0x4000e, 0x4000e }, { 0x4000f, 0x4000f }, { 0x40010, 0x40010 }, // 0x00298378
    { 0x40011, 0x40011 }, { 0x40012, 0x40012 }, { 0x40013, 0x40013 }, { 0x40014, 0x40014 }, // 0x00298398
    { 0x40015, 0x40015 }, { 0x40016, 0x40016 }, { 0x40017, 0x40017 }, { 0x40018, 0x40018 }, // 0x002983b8
    { 0x40019, 0x40019 }, { 0x4001a, 0x4001a }, { 0x4001b, 0x4001b }, { 0x4001c, 0x4001c }, // 0x002983d8
    { 0x4001d, 0x4001d }, { 0x4001e, 0x4001e }, { 0x4001f, 0x4001f }, { 0x40020, 0x40020 }, // 0x002983f8
    { 0x40021, 0x40021 }, { 0x40022, 0x40022 }, { 0x40023, 0x40023 }, { 0x40024, 0x40024 }, // 0x00298418
    { 0x40025, 0x40025 }, { 0x40026, 0x40026 }, { 0x40027, 0x40027 }, { 0x40028, 0x40028 }, // 0x00298438
    { 0x40029, 0x40029 }, { 0x4002a, 0x4002a }, { 0x4002b, 0x4002b }, { 0x4002c, 0x4002c }, // 0x00298458
    { 0x4002d, 0x4002d }, { 0x4002e, 0x4002e }, { 0x4002f, 0x4002f }, { 0x40030, 0x40030 }, // 0x00298478
    { 0x40031, 0x40031 }, { 0x40032, 0x40032 }, { 0x40033, 0x40033 }, { 0x40034, 0x40034 }, // 0x00298498
    { 0x40035, 0x40035 }, { 0x40036, 0x40036 }, { 0x40037, 0x40037 }, { 0x40038, 0x40038 }, // 0x002984b8
    { 0x40039, 0x40039 }, { 0x4003a, 0x4003a }, { 0x4003b, 0x4003b }, { 0x4003c, 0x4003c }, // 0x002984d8
    { 0x4003d, 0x4003d }, { 0x4003e, 0x4003e }, { 0x4003f, 0x4003f }, { 0x40040, 0x40040 }, // 0x002984f8
    { 0x40041, 0x40041 }, { 0x40042, 0x40042 }, { 0x40043, 0x40043 }, { 0x40044, 0x40044 }, // 0x00298518
    { 0x40045, 0x40045 }, { 0x40046, 0x40046 }, { 0x40047, 0x40047 }, { 0x40048, 0x40048 }, // 0x00298538
    { 0x40049, 0x40049 }, { 0x4004a, 0x4004a }, { 0x4004b, 0x4004b }, { 0x4004c, 0x4004c }, // 0x00298558
    { 0x4004d, 0x4004d }, { 0x4004e, 0x4004e }, { 0x4004f, 0x4004f }, { 0x40050, 0x40050 }, // 0x00298578
    { 0x40051, 0x40051 }, { 0x40052, 0x40052 }, { 0x40053, 0x40053 }, { 0x40054, 0x40054 }, // 0x00298598
    { 0x40055, 0x40055 }, { 0x40056, 0x40056 }, { 0x40057, 0x40057 }, { 0x40058, 0x40058 }, // 0x002985b8
    { 0x40059, 0x40059 }, { 0x4005a, 0x4005a }, { 0x4005b, 0x4005b }, { 0x4005c, 0x4005c }, // 0x002985d8
    { 0x4005d, 0x4005d }, { 0x4005e, 0x4005e }, { 0x4005f, 0x4005f }, { 0x40060, 0x40060 }, // 0x002985f8
    { 0x40061, 0x40061 }, { 0x40062, 0x40062 }, { 0x40063, 0x40063 }, { 0x40064, 0x40064 }, // 0x00298618
    { 0x40065, 0x40065 }, { 0x40066, 0x40066 }, { 0x40067, 0x40067 }, { 0x40068, 0x40068 }, // 0x00298638
    { 0x40069, 0x40069 }, { 0x4006a, 0x4006a }, { 0x4006b, 0x4006b }, { 0x4006c, 0x4006c }, // 0x00298658
    { 0x4006d, 0x4006d }, { 0x4006e, 0x4006e }, { 0x4006f, 0x4006f }, { 0x40070, 0x40070 }, // 0x00298678
    { 0x40071, 0x40071 }, { 0x40072, 0x40072 }, { 0x40073, 0x40073 }, { 0x40074, 0x40074 }, // 0x00298698
    { 0x40075, 0x40075 }, { 0x40076, 0x40076 }, { 0x40077, 0x40077 }, { 0x40078, 0x40078 }, // 0x002986b8
    { 0x40079, 0x40079 }, { 0x4007a, 0x4007a }, { 0x4007b, 0x4007b }, { 0x4007c, 0x4007c }, // 0x002986d8
    { 0x4007d, 0x4007d }, { 0x4007e, 0x4007e }, { 0x4007f, 0x4007f }, { 0x40080, 0x40080 }, // 0x002986f8
    { 0x40081, 0x40081 }, { 0x40082, 0x40082 }, { 0x40083, 0x40083 }, { 0x40084, 0x40084 }, // 0x00298718
    { 0x40085, 0x40085 }, { 0x40086, 0x40086 }, { 0x40087, 0x40087 }, { 0x40088, 0x40088 }, // 0x00298738
    { 0x40089, 0x40089 }, { 0x4008a, 0x4008a }, { 0x4008b, 0x4008b }, { 0x4008c, 0x4008c }, // 0x00298758
    { 0x4008d, 0x4008d }, { 0x4008e, 0x4008e }, { 0x4008f, 0x4008f }, { 0x40090, 0x40090 }, // 0x00298778
    { 0x40091, 0x40091 }, { 0x40092, 0x40092 }, { 0x40093, 0x40093 }, { 0x40094, 0x40094 }, // 0x00298798
    { 0x40095, 0x40095 }, { 0x40096, 0x40096 }, { 0x40097, 0x40097 }, { 0x40098, 0x40098 }, // 0x002987b8
    { 0x40099, 0x40099 }, { 0x4009a, 0x4009a }, { 0x4009b, 0x4009b }, { 0x4009c, 0x4009c }, // 0x002987d8
    { 0x4009d, 0x4009d }, { 0x4009e, 0x4009e }, { 0x4009f, 0x4009f }, { 0x400a0, 0x400a0 }, // 0x002987f8
    { 0x400a1, 0x400a1 }, { 0x400a2, 0x400a2 }, { 0x400a3, 0x400a3 }, { 0x400a4, 0x400a4 }, // 0x00298818
    { 0x400a5, 0x400a5 }, { 0x400a6, 0x400a6 }, { 0x400a7, 0x400a7 }, { 0x400a8, 0x400a8 }, // 0x00298838
    { 0x400a9, 0x400a9 }, { 0x400aa, 0x400aa }, { 0x400ab, 0x400ab }, { 0x400ac, 0x400ac }, // 0x00298858
    { 0x400ad, 0x400ad }, { 0x400ae, 0x400ae }, { 0x400af, 0x400af }, { 0x400b0, 0x400b0 }, // 0x00298878
    { 0x400b1, 0x400b1 }, { 0x400b2, 0x400b2 }, { 0x400b3, 0x400b3 }, { 0x400b4, 0x400b4 }, // 0x00298898
    { 0x400b5, 0x400b5 }, { 0x400b6, 0x400b6 }, { 0x400b7, 0x400b7 }, { 0x400b8, 0x400b8 }, // 0x002988b8
    { 0x400b9, 0x400b9 }, { 0x400ba, 0x400ba }, { 0x400bb, 0x400bb }, { 0x400bc, 0x400bc }, // 0x002988d8
    { 0x400bd, 0x400bd }, { 0x400be, 0x400be }, { 0x400bf, 0x400bf }, { 0x400c0, 0x400c0 }, // 0x002988f8
    { 0x400c1, 0x400c1 }, { 0x400c2, 0x400c2 }, { 0x400c3, 0x400c3 }, { 0x400c4, 0x400c4 }, // 0x00298918
    { 0x400c5, 0x400c5 }, { 0x400c6, 0x400c6 }, { 0x400c7, 0x400c7 }, { 0x400c8, 0x400c8 }, // 0x00298938
    { 0x400c9, 0x400c9 }, { 0x400ca, 0x400ca }, { 0x400cb, 0x400cb }, { 0x400cc, 0x400cc }, // 0x00298958
    { 0x400cd, 0x400cd }, { 0x400ce, 0x400ce }, { 0x400cf, 0x400cf }, { 0x400d0, 0x400d0 }, // 0x00298978
    { 0x400d1, 0x400d1 }, { 0x400d2, 0x400d2 }, { 0x400d3, 0x400d3 }, { 0x400d4, 0x400d4 }, // 0x00298998
    { 0x400d5, 0x400d5 }, { 0x400d6, 0x400d6 }, { 0x400d7, 0x400d7 }, { 0x400d8, 0x400d8 }, // 0x002989b8
    { 0x400d9, 0x400d9 }, { 0x400da, 0x400da }, { 0x400db, 0x400db }, { 0x400dc, 0x400dc }, // 0x002989d8
    { 0x400dd, 0x400dd }, { 0x400de, 0x400de }, { 0x400df, 0x400df }, { 0x400e0, 0x400e0 }, // 0x002989f8
    { 0x400e1, 0x400e1 }, { 0x400e2, 0x400e2 }, { 0x400e3, 0x400e3 }, { 0x80001, 0x10c5 }, // 0x00298a18
    { 0x80002, 0x10c6 }, { 0x80003, 0x10c7 }, { 0x80004, 0x10c8 }, { 0x80005, 0x10c9 }, // 0x00298a38
    { 0x80006, 0x10ca }, { 0x80007, 0x10cb }, { 0x80008, 0x10cc }, { 0x80009, 0x10cd }, // 0x00298a58
    { 0x8000a, 0x10ce }, { 0x8000b, 0x10cf }, { 0x8000c, 0x10d0 }, { 0x8000d, 0x10d1 }, // 0x00298a78
    { 0x8000e, 0x10d2 }, { 0x8000f, 0x10d3 }, { 0x80010, 0x10d4 }, { 0x80011, 0x10d5 }, // 0x00298a98
    { 0x80012, 0x10d6 }, { 0x80013, 0x10d7 }, { 0x80014, 0x10d8 }, { 0x400001, 0x0 }, // 0x00298ab8
    { 0x400002, 0x0 }, { 0x400003, 0x0 }, { 0x400004, 0x0 }, { 0x400005, 0x0 }, // 0x00298ad8
    { 0x100001, 0x1000 }, { 0x100002, 0x1024 }, { 0x100003, 0x102a }, { 0x200001, 0x1038 }, // 0x00298af8
    { 0x200002, 0x1059 }, { 0x200003, 0x1063 }, { 0x200004, 0x106b }, { 0x200005, 0x1074 }, // 0x00298b18
    { 0x200006, 0x107e }, { 0x200007, 0x1088 }, { 0x200008, 0x1093 }, { 0x200009, 0x109d }, // 0x00298b38
    { 0x20000a, 0x10a8 }, { 0x20000b, 0x10af }, { 0x20000c, 0x10ba }, { -1, -1 }, // 0x00298b58
};

// ──────────────────────────────────────────────────────────────────────
// The value of m_type's row; the end row's -1 when m_type is not in the
// table (asserting when m_type is 0).
// ROR 0x0012cf3c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

int MagLifeMenuBase::getTypeValue()
{
    int i = 0;
    for (; s_menuTypeTable[i].m_type != -1; i++)
    {
        if (m_type == s_menuTypeTable[i].m_type)
            return s_menuTypeTable[i].m_value;
    }
    ROR_ASSERT(m_type); /* 329 */
    return s_menuTypeTable[i].m_value;
}

// ──────────────────────────────────────────────────────────────────────
// The wrappers' slots 9 / 12 / 6: init hands the menu object the scene
// (Mcd's takes none) and the texture table, processPad and render pass the
// pad and the layer straight on.
// ROR 0x0012cfe0

void MagLifeMenuSelLang::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d00c

int MagLifeMenuSelLang::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d024

void MagLifeMenuSelLang::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d040

void MagLifeMenuItem::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d06c

int MagLifeMenuItem::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d084

void MagLifeMenuItem::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d0a0

void MagLifeMenuMap::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d0cc

int MagLifeMenuMap::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d0e4

void MagLifeMenuMap::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d100

void MagLifeMenuStock::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d12c

int MagLifeMenuStock::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d144

void MagLifeMenuStock::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d160

void MagLifeMenuSave::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d18c

int MagLifeMenuSave::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d1a8

void MagLifeMenuSave::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d1c4

void MagLifeMenuLoad::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d1f0

int MagLifeMenuLoad::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d20c

void MagLifeMenuLoad::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d228

void MagLifeMenuOpt::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d254

int MagLifeMenuOpt::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d26c

void MagLifeMenuOpt::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d288

void MagLifeMenuMovie::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d2b4

int MagLifeMenuMovie::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d2cc

void MagLifeMenuMovie::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d2e8

void MagLifeMenuTitle::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d314

int MagLifeMenuTitle::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d32c

void MagLifeMenuTitle::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ROR 0x0012d348

void MagLifeMenuMcd::init(DogTexInfo *tex)
{
    m_menu.init(tex);
}

// ROR 0x0012d364

int MagLifeMenuMcd::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d37c

void MagLifeMenuMcd::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ──────────────────────────────────────────────────────────────────────
// The MagLife0 / MagLife words, the two books, no book chosen.
// ROR 0x0012d398
// callcheck: MnuPBook, MnuDiary (member constructors, not written: the members start zeroed in the global scene)

MagLifeMenuPBook::MagLifeMenuPBook()
{
    m_menu_scene = NULL;
    m_cur = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Picks the book whose page table holds the type's page (the personal
// book first, then the diary), hands it the scene and inits it.
// ROR 0x0012d400 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagLifeMenuPBook::init(DogTexInfo *tex)
{
    int type = getTypeValue();
    if (m_pbook.setType(type))
        m_cur = &m_pbook;
    else if (m_diary.setType(type))
        m_cur = &m_diary;
    else
        ROR_ASSERT(0); /* 664 */
    /* PC: the ROM goes on with whatever m_cur held (NULL the first time). */
    if (!m_cur)
        return;
    m_cur->m_scene = m_menu_scene;
    m_cur->init(tex);
}

// ROR 0x0012d4cc

int MagLifeMenuPBook::processPad(BedPad *pad)
{
    if (!m_cur)
        return 0; /* PC: no book chosen (init's assert) */
    return m_cur->processPad(pad);
}

// ROR 0x0012d4fc

void MagLifeMenuPBook::render(I3dLayer *layer)
{
    if (!m_cur)
        return; /* PC: no book chosen (init's assert) */
    m_cur->draw(layer);
}

// ROR 0x0012d52c

void MagLifeMenuNow::init(DogTexInfo *tex)
{
    m_menu.m_scene = m_menu_scene;
    m_menu.init(tex);
}

// ROR 0x0012d558

int MagLifeMenuNow::processPad(BedPad *pad)
{
    return m_menu.processPad(pad);
}

// ROR 0x0012d570

void MagLifeMenuNow::render(I3dLayer *layer)
{
    m_menu.draw(layer);
}

// ──────────────────────────────────────────────────────────────────────
// The MagLife0 / MagLife words, no actor, no type, then the reset the
// ROM inlines (no scene, step 0).
// ROR 0x0012d58c

MagLifeMenuBoard3D::MagLifeMenuBoard3D()
{
    m_menu_scene = NULL;
    m_step = 0;
    m_wait = 0;
    m_48 = 0;
    m_4c = 0;
    m_type = -1;
    m_actor_no = -1;
    m_menu_scene = NULL;
    m_wait = -1;
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 1: releases the actor (term).
// ROR 0x0012d5e0 // callcheck: operator_delete (the deleting variant: delete does it on the host)

MagLifeMenuBoard3D::~MagLifeMenuBoard3D()
{
    term();
}

// ──────────────────────────────────────────────────────────────────────
// Loads the type's model as a scene actor and saves the scene's camera
// (render places its own), step 0.
// ROR 0x0012d648 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagLifeMenuBoard3D::init(DogTexInfo *tex)
{
    ROR_ASSERT(m_menu_scene); /* 740: the ROM's text is m_scene */
    int model = getTypeValue();
    m_actor_no = m_menu_scene->loadModel(NULL, model, NULL);
    m_menu_scene->saveCamera();
    m_step = 0;
    m_wait = -1;
}

// ──────────────────────────────────────────────────────────────────────
// Releases the actor init loaded and gives the scene its camera back.
// ROR 0x0012d6c0 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagLifeMenuBoard3D::term()
{
    if (m_actor_no < 0)
        return;
    ROR_ASSERT(m_menu_scene); /* 768: the ROM's text is m_scene */
    m_menu_scene->releaseActor(m_actor_no);
    m_menu_scene->restoreCamera();
    m_actor_no = -1;
}

// ROR 0x0012d72c

int MagLifeMenuBoard3D::isInit()
{
    return m_actor_no >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// Done (1) once render's fade out has ended; while shown, pad bit 13
// starts the fade out (step 3).
// ROR 0x0012d73c

int MagLifeMenuBoard3D::processPad(BedPad *pad)
{
    if (m_step == 4)
        return 1;
    if (m_step == 2)
    {
        if (pad->m_repeat >> 13 & 1)
        {
            m_step = 3;
            m_wait = -1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 0 puts the camera at (0, -200, 500) turned (-0.1, pi, 0), the actor
// at the origin unrotated, and fades in over 30 frames; step 1 waits for
// the fade, step 3 fades out and back in (closing the menu). Every step
// draws the actor into the layer.
// ROR 0x0012d788 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagLifeMenuBoard3D::render(I3dLayer *theRenderStage)
{
    ROR_ASSERT(theRenderStage); /* 810 */
    if (!m_menu_scene)
        return;
    if (m_actor_no < 0)
        return;
    DogActor *actor = &m_menu_scene->m_actors[m_actor_no];
    ROR_ASSERT(actor); /* 820 */
    switch (m_step)
    {
    case 0:
    {
        DogCamera *camera = &m_menu_scene->m_camera_entity;
        camera->m_trans[1] = -200.0f;
        camera->m_trans[2] = 500.0f;
        camera->m_trans[0] = 0.0f;
        camera->m_dirty = 1;
        camera->setRotate(-0x1.999998p-4f, 0x1.921fb8p+1f, 0.0f); /* 0xbdcccccc, 0x40490fdc */
        actor->m_trans[0] = 0.0f;
        actor->m_trans[1] = 0.0f;
        actor->m_trans[2] = 0.0f;
        actor->m_dirty = 1;
        actor->setRotate(0.0f, 0.0f, 0.0f);
        actor->transform(); /* EE slot 2 */
        m_step = 1;
        m_wait = -1;
        m_menu_scene->fadeIn(30, 7, 0);
        break;
    }
    case 1:
        if (m_menu_scene->getFadeState() >= 5)
        {
            m_step = 2;
            m_wait = -1;
        }
        break;
    case 2:
    case 4:
        break;
    case 3:
        if (m_wait < 1)
        {
            m_menu_scene->fadeOut(30, 7, 0);
            m_wait = 1;
        }
        if (m_menu_scene->getFadeState() >= 5)
        {
            m_step = 4;
            m_wait = -1;
            m_menu_scene->fadeIn(30, 7, 0);
        }
        break;
    }
    actor->drawModel(theRenderStage, 0, NULL); /* EE slot 19 */
}
