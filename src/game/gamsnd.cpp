// game/gamsnd.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "game/gamsnd.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001e435c
int GamSndMan::isSndBankLoaded(int id)
{
    ROSEPAN_STUB("GamSndMan::isSndBankLoaded", 0x001e435c);
    return {};
}

// stub of ROR 0x001e17b4
void GamSndMan::stopBgm(int mode)
{
    ROSEPAN_STUB("GamSndMan::stopBgm", 0x001e17b4);
}

// stub of ROR 0x001e6570
void GamSndMan::clear_snd_lst(int arg)
{
    ROSEPAN_STUB("GamSndMan::clear_snd_lst", 0x001e6570);
}

// stub of ROR 0x001e46b8
void GamSndMan::freeSePack(int n)
{
    ROSEPAN_STUB("GamSndMan::freeSePack", 0x001e46b8);
}

// stub of ROR 0x001e6734
void GamSndMan::funcSnd(int no, int a, int b, int c, int d)
{
    ROSEPAN_STUB("GamSndMan::funcSnd", 0x001e6734);
}

// stub of ROR 0x001e0f1c
void GamSndMan::init()
{
    ROSEPAN_STUB("GamSndMan::init", 0x001e0f1c);
}

// stub of ROR 0x001e4458
void GamSndMan::loadSePack(const char *name, void *data, int arg)
{
    ROSEPAN_STUB("GamSndMan::loadSePack", 0x001e4458);
}

// stub of ROR 0x001e4790
void GamSndMan::registSnd(int state, void *data)
{
    ROSEPAN_STUB("GamSndMan::registSnd", 0x001e4790);
}

// stub of ROR 0x001e484c
void GamSndMan::registStrSnd(int state, void *data)
{
    ROSEPAN_STUB("GamSndMan::registStrSnd", 0x001e484c);
}

// stub of ROR 0x001e100c
void GamSndMan::update(const float *pos)
{
    ROSEPAN_STUB("GamSndMan::update", 0x001e100c);
}

// stub of ROR 0x001e1920
void GamSndMan::resumeBgm(int a, int mode, int reset)
{
    ROSEPAN_STUB("GamSndMan::resumeBgm", 0x001e1920);
}

// stub of ROR 0x001e3698
void GamSndMan::del_str_3d_pos_chr(u_int oid)
{
    ROSEPAN_STUB("GamSndMan::del_str_3d_pos_chr", 0x001e3698);
}

// stub of ROR 0x001e2c40
void GamSndMan::del_se_3d_pos_chr(u_int oid)
{
    ROSEPAN_STUB("GamSndMan::del_se_3d_pos_chr", 0x001e2c40);
}

// stub of ROR 0x001e107c
void GamSndMan::resetVolume()
{
    ROSEPAN_STUB("GamSndMan::resetVolume", 0x001e107c);
}

// stub of ROR 0x001e2aa0
void GamSndMan::del_se_3d_pos(u_int se, int arg)
{
    ROSEPAN_STUB("GamSndMan::del_se_3d_pos", 0x001e2aa0);
}

// stub of ROR 0x001e34ec
void GamSndMan::del_str_3d_pos(int str, int arg)
{
    ROSEPAN_STUB("GamSndMan::del_str_3d_pos", 0x001e34ec);
}

// stub of ROR 0x001e5e58
int GamSndMan::funcSE(u_int se, int no, void *arg)
{
    ROSEPAN_STUB("GamSndMan::funcSE", 0x001e5e58);
    return {};
}

// stub of ROR 0x001e5660
int GamSndMan::funcStr(int str, int no, void *arg)
{
    ROSEPAN_STUB("GamSndMan::funcStr", 0x001e5660);
    return {};
}

// stub of ROR 0x001e4ab8
int GamSndMan::getStrNO(const char *name)
{
    ROSEPAN_STUB("GamSndMan::getStrNO", 0x001e4ab8);
    return {};
}

// stub of ROR 0x001e5be0
int GamSndMan::getStrNoByChan(int chan)
{
    ROSEPAN_STUB("GamSndMan::getStrNoByChan", 0x001e5be0);
    return {};
}

// stub of ROR 0x001e4d04
int GamSndMan::get_se_oid(const char *name)
{
    ROSEPAN_STUB("GamSndMan::get_se_oid", 0x001e4d04);
    return {};
}

// stub of ROR 0x001e6cac
int GamSndMan::isPlayEnvStr()
{
    ROSEPAN_STUB("GamSndMan::isPlayEnvStr", 0x001e6cac);
    return {};
}

// stub of ROR 0x001e4ebc
int GamSndMan::playSE(u_int se, bool loop, float *pos, int arg)
{
    ROSEPAN_STUB("GamSndMan::playSE", 0x001e4ebc);
    return {};
}

// stub of ROR 0x001e50c8
int GamSndMan::playSE(u_int se, int a, int b, float c, int d, int e, int arg)
{
    ROSEPAN_STUB("GamSndMan::playSE", 0x001e50c8);
    return {};
}

// stub of ROR 0x001e5250
int GamSndMan::playStr(int str, int a, float *pos, bool b, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::playStr", 0x001e5250);
    return {};
}

// stub of ROR 0x001e5428
int GamSndMan::playStr(int str, int a, int b, float c, int d, int e, int arg)
{
    ROSEPAN_STUB("GamSndMan::playStr", 0x001e5428);
    return {};
}

// stub of ROR 0x001e239c
void GamSndMan::put_se_3d_pos(u_int se, float *pos, int wrd, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_se_3d_pos", 0x001e239c);
}

// stub of ROR 0x001e2704
void GamSndMan::put_se_3d_pos_chr(u_int se, u_int oid, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_se_3d_pos_chr", 0x001e2704);
}

// stub of ROR 0x001e2db4
void GamSndMan::put_str_3d_pos(int str, float *pos, int wrd, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_str_3d_pos", 0x001e2db4);
}

// stub of ROR 0x001e3138
void GamSndMan::put_str_3d_pos_chr(int str, u_int oid, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_str_3d_pos_chr", 0x001e3138);
}

// stub of ROR 0x001e4cc4
void GamSndMan::setReverb(int a, int b, int c)
{
    ROSEPAN_STUB("GamSndMan::setReverb", 0x001e4cc4);
}

// stub of ROR 0x001e5de4
void GamSndMan::set_se_3d_snd_prm(u_int se, int a, int b, int c, int d)
{
    ROSEPAN_STUB("GamSndMan::set_se_3d_snd_prm", 0x001e5de4);
}

// stub of ROR 0x001e5540
void GamSndMan::set_str_3d_snd_prm(int str, int a, int b, int c, int d)
{
    ROSEPAN_STUB("GamSndMan::set_str_3d_snd_prm", 0x001e5540);
}

// stub of ROR 0x001e1620
void GamSndMan::unpush_snd(int mode, u_int se, int str)
{
    ROSEPAN_STUB("GamSndMan::unpush_snd", 0x001e1620);
}

// stub of ROR 0x001e4540
void GamSndMan::loadSePack(const char *name, int arg)
{
    ROSEPAN_STUB("GamSndMan::loadSePack", 0x001e4540);
}

// stub of ROR 0x001e15d0
void GamSndMan::setBgmRec(const GamBgmRec *rec)
{
    ROSEPAN_STUB("GamSndMan::setBgmRec", 0x001e15d0);
}

// stub of ROR 0x001e1fa4
void GamSndMan::reset()
{
    ROSEPAN_STUB("GamSndMan::reset", 0x001e1fa4);
}

// stub of ROR 0x001e2178
void GamSndMan::stopObjSnds()
{
    ROSEPAN_STUB("GamSndMan::stopObjSnds", 0x001e2178);
}

// stub of ROR 0x001e4a0c
int GamSndMan::getSeID(const char *name)
{
    ROSEPAN_STUB("GamSndMan::getSeID", 0x001e4a0c);
    return {};
}

// stub of ROR 0x001e4c1c
int GamSndMan::getStrSnd(int str, GamSndEntry **entry)
{
    ROSEPAN_STUB("GamSndMan::getStrSnd", 0x001e4c1c);
    return {};
}

// stub of ROR 0x001e4d18
void GamSndMan::stopSE(int se)
{
    ROSEPAN_STUB("GamSndMan::stopSE", 0x001e4d18);
}
