// mag/maglifepcmode.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/maglifepcmode.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00122c9c
void MagLifePCModeRide::message(MagLifePC *pc, int msg)
{
    ROSEPAN_STUB("MagLifePCModeRide::message", 0x00122c9c);
}

// ---- the modes' virtuals (hand-written stubs: their vtables need every
// out-of-line slot defined, which stubgen cannot see) ----

// stub of ROR 0x001208b8
void MagLifePCMode::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCMode::live", 0x001208b8);
}

// stub of ROR 0x00120900
void MagLifePCModeNull::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeNull::init", 0x00120900);
}

// stub of ROR 0x001209b0
void MagLifePCModeNull::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeNull::live", 0x001209b0);
}

// stub of ROR 0x001209e0
void MagLifePCModeWait::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeWait::init", 0x001209e0);
}

// stub of ROR 0x00120b9c
void MagLifePCModeWait::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeWait::live", 0x00120b9c);
}

// stub of ROR 0x00120bdc
int MagLifePCModeWait::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeWait::pad", 0x00120bdc);
    return {};
}

// stub of ROR 0x00120f60
void MagLifePCModeWalk2D::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeWalk2D::init", 0x00120f60);
}

// stub of ROR 0x00121070
void MagLifePCModeWalk2D::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeWalk2D::live", 0x00121070);
}

// stub of ROR 0x00121078
int MagLifePCModeWalk2D::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeWalk2D::pad", 0x00121078);
    return {};
}

// stub of ROR 0x00121168
void MagLifePCModeRun2D::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRun2D::init", 0x00121168);
}

// stub of ROR 0x001211c8
void MagLifePCModeRun2D::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRun2D::live", 0x001211c8);
}

// stub of ROR 0x001211e4
int MagLifePCModeRun2D::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeRun2D::pad", 0x001211e4);
    return {};
}

// stub of ROR 0x001212c8
void MagLifePCModeTurn::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeTurn::init", 0x001212c8);
}

// stub of ROR 0x00121364
void MagLifePCModeTurn::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeTurn::live", 0x00121364);
}

// stub of ROR 0x0012136c
int MagLifePCModeTurn::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeTurn::pad", 0x0012136c);
    return {};
}

// stub of ROR 0x001214b4
void MagLifePCModeAtkWait2D::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkWait2D::init", 0x001214b4);
}

// stub of ROR 0x001216a4
void MagLifePCModeAtkWait2D::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkWait2D::live", 0x001216a4);
}

// stub of ROR 0x001217c0
int MagLifePCModeAtkWait2D::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeAtkWait2D::pad", 0x001217c0);
    return {};
}

// stub of ROR 0x00121858
void MagLifePCModeAtkMove2D::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkMove2D::init", 0x00121858);
}

// stub of ROR 0x00121890
void MagLifePCModeAtkMove2D::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkMove2D::live", 0x00121890);
}

// stub of ROR 0x00121898
int MagLifePCModeAtkMove2D::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeAtkMove2D::pad", 0x00121898);
    return {};
}

// stub of ROR 0x001218fc
void MagLifePCModeAttacking::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAttacking::init", 0x001218fc);
}

// stub of ROR 0x00121934
void MagLifePCModeAttacking::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAttacking::live", 0x00121934);
}

// stub of ROR 0x00121c64
int MagLifePCModeAttacking::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeAttacking::pad", 0x00121c64);
    return {};
}

// stub of ROR 0x00121cd8
void MagLifePCModeDamage::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDamage::init", 0x00121cd8);
}

// stub of ROR 0x00121fc8
void MagLifePCModeDamage::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDamage::live", 0x00121fc8);
}

// stub of ROR 0x001220f0
void MagLifePCModeItem::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeItem::init", 0x001220f0);
}

// stub of ROR 0x001221ec
void MagLifePCModeItem::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeItem::live", 0x001221ec);
}

// stub of ROR 0x00122338
void MagLifePCModeDogGo::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogGo::init", 0x00122338);
}

// stub of ROR 0x001223c0
void MagLifePCModeDogGo::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogGo::live", 0x001223c0);
}

// stub of ROR 0x0012245c
void MagLifePCModeDogCome::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogCome::init", 0x0012245c);
}

// stub of ROR 0x00122668
void MagLifePCModeDogCome::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogCome::live", 0x00122668);
}

// stub of ROR 0x001226d0
void MagLifePCModeDogStay::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogStay::init", 0x001226d0);
}

// stub of ROR 0x00122758
void MagLifePCModeDogStay::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogStay::live", 0x00122758);
}

// stub of ROR 0x001227e4
void MagLifePCModeHold::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHold::init", 0x001227e4);
}

// stub of ROR 0x00122828
int MagLifePCModeHold::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeHold::pad", 0x00122828);
    return {};
}

// stub of ROR 0x001228fc
void MagLifePCModeHoldAttack::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHoldAttack::init", 0x001228fc);
}

// stub of ROR 0x00122930
void MagLifePCModeUnhold::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeUnhold::init", 0x00122930);
}

// stub of ROR 0x00122954
void MagLifePCModeRide::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRide::init", 0x00122954);
}

// stub of ROR 0x00122ad4
void MagLifePCModeRide::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRide::live", 0x00122ad4);
}

// stub of ROR 0x00122bd4
int MagLifePCModeRide::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeRide::pad", 0x00122bd4);
    return {};
}

// stub of ROR 0x00122d70
void MagLifePCModeRoped::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRoped::init", 0x00122d70);
}

// stub of ROR 0x00122dec
void MagLifePCModeRoped::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRoped::live", 0x00122dec);
}

// stub of ROR 0x00122e2c
int MagLifePCModeRoped::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeRoped::pad", 0x00122e2c);
    return {};
}

// stub of ROR 0x00122fc4
void MagLifePCModeReload::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeReload::init", 0x00122fc4);
}

// stub of ROR 0x00123004
void MagLifePCModeReload::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeReload::live", 0x00123004);
}

// stub of ROR 0x001230a8
void MagLifePCModeHug::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHug::init", 0x001230a8);
}

// stub of ROR 0x001230fc
void MagLifePCModeHug::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHug::live", 0x001230fc);
}
