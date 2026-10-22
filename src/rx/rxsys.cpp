/* ==========================================================================
 *  rx/rxsys.cpp
 *
 *  The rx middleware's system layer: the console bring-up main runs before
 *  the game (rxSysInit), the IOP reboot and module list, the boot path,
 *  system.ini, and rxSysPrintf. See rxsys.h.
 *
 *  Not reconstructed yet in this file: the ini parser (0x2410b8..0x241848),
 *  the tty / IOP RPC binding (0x241890..0x241ac8, 0x242188..0x2424e0), the
 *  rxSysIoctl (0x2426b8), the exception handler and system
 *  monitor (0x242710..0x2432d8) and the rxfile wrappers after 0x2436a0.
 * ======================================================================== */
#include "common/rosepan_stub.h"

#include "rx/rxsys.h"

#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>

#include "rx/rxfile.h"
#include "rx/rxthread.h"
#include "sdk/libcdvd.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"
#include "sdk/sifdev.h"
#include "sdk/sifrpc.h"
#include "sdk/fileio.h"

/* ---- data ------------------------------------------------------------- */

/* "0300" followed by "$RX_SCE_LIBRARY_VERSION:..." in the ROM: the SDK
 * version the middleware was built against. rxSysInit rewrites its first
 * character with the same '0'. */
static char s_libVersion[] = "0300";            /* data 0x2db9d8 */

static RxSysPrintHook s_printHook;              /* data 0x2db9d0 */

static char s_productNumber[16] = "SCPS-00000"; /* data 0x2dba00, system.ini NUMBER   */
static int  s_version = 100;                    /* data 0x2dba10, system.ini VERSION  */
static int  s_country;                          /* data 0x2dba14, system.ini COUNTRY  */
static int  s_language;                         /* data 0x2dba18, system.ini LANGUAGE */
static int  s_vmode;                            /* data 0x2dba1c, system.ini VMODE    */
static int  s_warning;                          /* data 0x2dba20, system.ini WARNING  */
static int  s_bootDevice = -1;                  /* data 0x2dba24, RX_SYS_DEVICE       */
static int  s_iniLoaded;                        /* data 0x2dba28 */
static int  s_iopRebooted;                      /* data 0x2dba2c */
static int  s_romDate;                          /* data 0x2dba30, 0xYYYYMMDD in BCD   */
static int  s_ttyFd = -1;                       /* data 0x2dba34, host tty file       */
static int  s_ttyRpcBound;                      /* data 0x2dba38 */
static char s_bootDev[0x80];                    /* data 0x2dba40, "host0:" -> ""     */
static char s_bootDir[0x80];                    /* data 0x2dbac0 */
static char s_bootName[0x80];                   /* data 0x2dbb40 */

/* Save-data prefix per COUNTRY (JAPAN, USA, EUROPE, ASIA). */
static const char *const s_regionPrefix[4] = { "BI", "BA", "BE", "BI" };  /* data 0x2dbbc0 */
static int s_moduleCount;                       /* data 0x2dbbd0 */

static const char *const s_vmodeNames[] = { "NTSC", "PAL", 0 };          /* data 0x2dbbd8 */
static const char *const s_countryNames[] = { "JAPAN", "USA", "EUROPE", "ASIA", 0 };  /* data 0x2dbbe8 */
static const char *const s_languageNames[] = {                           /* data 0x2dbc00 */
    "JAPANESE", "ENGLISH", "GERMANY", "FRENCH", "ITALIAN", "SPANISH", "KOREAN", "CHINESE", 0,
};
static const char *const s_warningNames[] = { "NO", "YES", 0 };          /* data 0x2dbc28 */
static const char *const s_deviceNames[RX_SYS_DEVICE_NUM] = {            /* data 0x2dbc38 */
    "HOST ", "CDROM", "MCARD", "HDD  ",
};

static int  s_ttyBufLen;                        /* data 0x2dbc48 */
static char s_iopImage[0x100] = "ioprp300.img"; /* data 0x2dbc50 */

static const char *const s_moduleListBase[] = {                          /* data 0x2dbd58 */
    "sifcmd.irx", "fileio.irx", "cdvdman.irx", "cdvdfsv.irx", 0,
};
static const char *const s_moduleListPad[] = {                           /* data 0x2dbd70 */
    "sio2man.irx", "padman.irx", "mcman.irx", "mcserv.irx", 0,
};
static const char *const s_moduleListUsb[] = { "usbd.irx", 0 };          /* data 0x2dbd88 */

static int s_hardwareReset;                     /* data 0x2dbd90 */
static int s_sema1 = -1;                        /* data 0x2dbd94 */
static int s_printSema = -1;                    /* data 0x2dbd98 */

RxSysMemFunc g_rxSysMemFunc[4];                 /* data 0x2dbe8c..0x2dbe98 */

static char s_saveName[0x40];                   /* bss 0x7cc340: region prefix + NUMBER  */
static char s_modules[32][32];                  /* bss 0x7cc380: loaded IOP modules      */
static int  s_deviceReady[RX_SYS_DEVICE_NUM];   /* bss 0x7cc780 */
static char s_confBuf[0x1000];                  /* bss 0x7cc7c0: iopconf.cnf, system.ini */
static char s_ttyBuf[0x1000];                   /* bss 0x7cd7c0 */
static sceSifClientData s_ttyClient;            /* bss 0x7ce7c0 */

static inline int rxSysIsPathSep(char c)
{
    return c == '/' || c == '\\' || c == ':' || c == ',';
}

/* ---- memory hooks ----------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// What every unset memory hook points at.
// ROR 0x0023ff58

int rxSysNoMemFunc(void)
{
    rxSysPrintf("rxSys:No Memory Allocate Func !\n");
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Install memory hook 0 (null = rxSysNoMemFunc), returning the old one.
// ROR 0x0023ff80

RxSysMemFunc rxSysSetMemFunc0(RxSysMemFunc func)
{
    RxSysMemFunc old = g_rxSysMemFunc[0];
    g_rxSysMemFunc[0] = func;
    if (func == 0)
        g_rxSysMemFunc[0] = (RxSysMemFunc)rxSysNoMemFunc;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0023ffb0

RxSysMemFunc rxSysSetMemFunc1(RxSysMemFunc func)
{
    RxSysMemFunc old = g_rxSysMemFunc[1];
    g_rxSysMemFunc[1] = func;
    if (func == 0)
        g_rxSysMemFunc[1] = (RxSysMemFunc)rxSysNoMemFunc;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0023ffe0

RxSysMemFunc rxSysSetMemFunc2(RxSysMemFunc func)
{
    RxSysMemFunc old = g_rxSysMemFunc[2];
    g_rxSysMemFunc[2] = func;
    if (func == 0)
        g_rxSysMemFunc[2] = (RxSysMemFunc)rxSysNoMemFunc;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00240010

RxSysMemFunc rxSysSetMemFunc3(RxSysMemFunc func)
{
    RxSysMemFunc old = g_rxSysMemFunc[3];
    g_rxSysMemFunc[3] = func;
    if (func == 0)
        g_rxSysMemFunc[3] = (RxSysMemFunc)rxSysNoMemFunc;
    return old;
}

/* ---- string and path helpers ----------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Length up to the end of the line.
// ROR 0x00240040

int rxSysStrLenToEol(const char *s)
{
    int n = 0;
    for (char c; (c = *s++) != '\0'; n++)
    {
        if (c == '\r' || c == '\n')
            break;
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// Length up to the next blank.
// ROR 0x00240080

int rxSysStrLenToSpace(const char *s)
{
    int n = 0;
    for (char c; (c = *s++) != '\0'; n++)
    {
        if (c == ' ' || c == '\t')
            break;
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// Skip line ends.
// ROR 0x002400c0

char *rxSysSkipEol(char *s)
{
    while (*s != '\0' && (*s == '\r' || *s == '\n'))
        s++;
    return s;
}

// ──────────────────────────────────────────────────────────────────────
// Skip blanks.
// ROR 0x00240100

char *rxSysSkipSpace(char *s)
{
    while (*s != '\0' && (*s == ' ' || *s == '\t'))
        s++;
    return s;
}

// ──────────────────────────────────────────────────────────────────────
// Copy at most n characters and terminate.
// ROR 0x00240140

void rxSysStrCopyN(char *dst, const char *src, int n)
{
    int i = 0;
    if (n > 0)
    {
        for (char c = *src++; c != '\0'; c = *src++)
        {
            i++;
            *dst++ = c;
            if (i >= n)
                break;
        }
    }
    *dst = '\0';
}

// ──────────────────────────────────────────────────────────────────────
// Copy at most n characters of a path, '\' to '/' and lower case: rx's
// paths are case-folded the way the disc's ISO names are matched.
// ROR 0x00240180

void rxSysPathCopy(char *dst, const char *src, int n)
{
    int i = 0;
    if (n > 0)
    {
        for (int c = *src; c != 0; c = *src)
        {
            src++;
            i++;
            if (c == '\\')
                c = '/';
            if (isupper((u_char)c))  // newlib's ctype table 0x3079a9, bit _U
                c += 0x20;
            *dst++ = (char)c;
            if (i >= n)
                break;
        }
    }
    *dst = '\0';
}

// ──────────────────────────────────────────────────────────────────────
// Is this module name in the loaded-module table?
// ROR 0x002401e8

int rxSysIsModuleLoaded(const char *name)
{
    if (name != 0)
    {
        for (int i = 0; i < s_moduleCount; i++)
        {
            if (strcmp(name, s_modules[i]) == 0)
                return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The file name: what follows the last '/', '\', ':' or ','.
// ROR 0x00240280

char *rxSysPathFileName(char *path)
{
    int i = (int)strlen(path) - 1;
    for (; i >= 0; i--)
    {
        if (rxSysIsPathSep(path[i]))
            break;
    }
    return path + i + 1;
}

// ──────────────────────────────────────────────────────────────────────
// The file name's extension (after its last '.'), or its end.
// ROR 0x00240338

char *rxSysPathExt(char *path)
{
    char *name = rxSysPathFileName(path);
    int len = (int)strlen(name);
    for (int i = len - 1; i >= 0; i--)
    {
        if (name[i] == '.')
            return name + i + 1;
    }
    return name + len;
}

// ──────────────────────────────────────────────────────────────────────
// Empty lines and lines starting with '-' or '#' are skipped.
// ROR 0x002403c0

int rxSysIsCommentLine(const char *line)
{
    char c = *line;
    return c == '\0' || c == '-' || c == '#';
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002403f0

int rxSysSkipModule(char *path)
{
    if (rxSysIsCommentLine(path))
        return 1;
    return rxSysIsModuleLoaded(rxSysPathFileName(path)) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Record a module as loaded (32 at most).
// ROR 0x00240440

void rxSysAddModule(const char *name)
{
    if (s_moduleCount < 32)
    {
        int i = s_moduleCount++;
        rxSysPathCopy(s_modules[i], name, 31);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00240490

void rxSysAddModuleList(const char *const *names)
{
    for (; *names != 0; names++)
        rxSysAddModule(*names);
}

/* ---- IOP ---------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Media mode for the disc in the drive: DVD for a PS2 DVD (or DVD video),
// CD otherwise.
// ROR 0x002404d8

void rxSysSetCdMode(void)
{
    int type = sceCdGetDiskType();
    if (type != SCECdPS2DVD && type != SCECdDVDV)
    {
        sceCdMmode(SCECdCD);
        return;
    }
    sceCdMmode(SCECdDVD);
}

// ──────────────────────────────────────────────────────────────────────
// Reboot the IOP with an image (booted from the disc only), then bring
// SIF RPC, the IOP heap, the CD and the file system back up.
// ROR 0x00240520

void rxSysRebootIop(const char *img)
{
    if (s_bootDevice == RX_SYS_DEVICE_CDROM)
    {
        rxSysPrintf("Iop Rebooting : %s\n", img);
        rxSysSetCdMode();
        while (rxFileRebootIop(img) == 0)
            ;
        sceSifInitRpc(0);
        sceSifInitIopHeap();
        while (sceCdInit(SCECdINIT) == 0)
            ;
        rxSysSetCdMode();
        sceFsReset();
        s_iopRebooted = 1;
        return;
    }
    rxSysPrintf("Skip Iop Reboot: %s\n", img);
}

// ──────────────────────────────────────────────────────────────────────
// Load the modules an iopconf.cnf lists, one per line: a path then its
// arguments, '#' / '-' lines skipped, modules already loaded skipped.
// ".img" lines are the reboot image and are ignored here.
// ROR 0x002405d8

int rxSysLoadModuleList(char *conf)
{
    char name[256];
    char args[1024];
    char line[512];

    while (*conf != '\0')
    {
        char *p = rxSysSkipSpace(conf);
        int n = rxSysStrLenToEol(p);
        rxSysStrCopyN(line, p, n);
        conf = rxSysSkipEol(p + n);
        if (rxSysIsCommentLine(line))
            continue;

        name[0] = '\0';
        args[0] = '\0';
        int len = rxSysStrLenToSpace(line);
        int arglen = 0;
        rxSysPathCopy(name, line, len);
        char *a = rxSysSkipSpace(line + len);
        while (*a != '\0')
        {
            int k = rxSysStrLenToSpace(a);
            rxSysStrCopyN(args + arglen, a, k);
            arglen += k + 1;
            a = rxSysSkipSpace(a + k);
        }
        const char *argp = 0;
        if (arglen > 0)
            argp = args;

        if (rxSysSkipModule(name))
            continue;
        char *ext = rxSysPathExt(name);
        if (strcmp("irx", ext) == 0)
        {
            rxSysAddModule(rxSysPathFileName(name));
            while (rxFileLoadModule(name, arglen, argp) == 0)
                ;
        }
        else if (strcmp("img", ext) != 0)
        {
            rxSysPrintf("Illigal Module Name:%s\n", name);
        }
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Reboot the IOP with <dir><image> once, then read iopconf.cnf into buf
// (through the file layer's search directory).
// ROR 0x002407b0

int rxSysReadIopConf(char *buf, const char *dir)
{
    char path[256];

    if (s_iopRebooted == 0)
    {
        strcpy(path, dir);
        strcpy(path + strlen(path), s_iopImage);
        rxSysRebootIop(path);
    }
    int size = rxFileLoad("iopconf.cnf", buf);
    if (size > 0)
        buf[size] = '\0';
    return size > 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00240838

void rxSysCdInit(int mode)
{
    rxSysPrintf("initializing CD (%d) ...\n", mode != 0);
    while (sceCdInit(mode != 0) == 0)
        ;
    rxSysPrintf("complete\n");
    s_deviceReady[RX_SYS_DEVICE_CDROM] = 1;
}

// ──────────────────────────────────────────────────────────────────────
// The IOP bring-up: the default module table, SIF, the CD, then the IOP
// reboot and iopconf.cnf from modules/ (a devkit falls back to the SDK's
// /usr/local/sce/iop/modules/ and gives up with exit(1)).
// ROR 0x00240890

void rxSysLoadIopModules(void)
{
    s_iopRebooted = 0;
    s_moduleCount = 0;
    rxSysAddModuleList(s_moduleListBase);
    if (s_bootDev[0] != '\0')
    {
        rxSysAddModuleList(s_moduleListPad);
        if (s_bootDev[0] == 'a')
            rxSysAddModuleList(s_moduleListUsb);
    }
    sceSifInitRpc(0);
    sceSifInitIopHeap();
    if (s_bootDevice == RX_SYS_DEVICE_CDROM)
        rxSysCdInit(0);
    rxSysPrintf("IOP Modules Load Start\n");
    rxSysSetFileSearchDir(0);
    if (s_bootDevice == RX_SYS_DEVICE_CDROM)
    {
        while (rxSysReadIopConf(s_confBuf, "modules/") == 0)
            ;
        while (rxSysLoadModuleList(s_confBuf) == 0)
            ;
    }
    else if (rxSysReadIopConf(s_confBuf, "modules/") == 0 || rxSysLoadModuleList(s_confBuf) == 0)
    {
        rxSysSetFileSearchDir("/usr/local/sce/iop/modules/");
        if (rxSysReadIopConf(s_confBuf, "") == 0 || rxSysLoadModuleList(s_confBuf) == 0)
        {
            rxSysSetFileSearchDir(0);
            rxSysPrintf("IOP Modules Load Failed\n");
            exit(1);
        }
    }
    rxSysSetFileSearchDir(0);
    if (s_bootDevice != RX_SYS_DEVICE_CDROM && rxSysIsModuleLoaded("cdvdman.irx"))
        rxSysCdInit(1);
    if (rxSysIsModuleLoaded("mcman.irx"))
        s_deviceReady[RX_SYS_DEVICE_MCARD] = 1;
    rxSysPrintf("IOP Modules Load End\n");
}

/* ---- devices ------------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00240a88

int rxSysIsDeviceReady(int device)
{
    int i = s_bootDevice;
    if (device >= 0)
        i = device;
    return s_deviceReady[i];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00240ab0

int rxSysGetBootDevice(void)
{
    return s_bootDevice;
}

// ──────────────────────────────────────────────────────────────────────
// No boot path given: a devkit's memory (more than 32 MB) means HOST, a
// retail console's means CDROM. The boot device is marked ready.
// ROR 0x00240ac0

void rxSysDetectDevice(void)
{
    if (s_bootDevice < 0)
    {
        int size = GetMemorySize();
        if (size > 0x2000000)
            s_deviceReady[RX_SYS_DEVICE_HOST] = 1;
        else
            s_deviceReady[RX_SYS_DEVICE_CDROM] = 1;
        s_bootDevice = size <= 0x2000000;
        return;
    }
    s_deviceReady[s_bootDevice] = 1;
}

/* ---- system.ini ----------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Index of s in a null-terminated table, or -1.
// ROR 0x00240b40

int rxSysFindString(const char *s, const char *const *table)
{
    for (int i = 0; table[i] != 0; i++)
    {
        if (strcmp(s, table[i]) == 0)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Read system.ini's [SYSTEM] section, when the file exists.
// ROR 0x00240bb0

void rxSysReadSystemIni(void)
{
    char value[32];

    s_iniLoaded = 0;
    int size = rxFileLoad("system.ini", s_confBuf);
    if (size <= 0)
        return;
    s_iniLoaded = 1;
    s_confBuf[size] = '\0';

    if (rxSysIniGetString(s_productNumber, "SYSTEM", "NUMBER"))
        rxSysPrintf(" Number  :%s\n", s_productNumber);
    if (rxSysIniGetString(value, "SYSTEM", "VERSION"))
    {
        int version = atoi(value);
        s_version = version;
        rxSysPrintf(" Version :%d\n", version);
    }
    if (rxSysIniGetString(value, "SYSTEM", "VMODE"))
    {
        int i = rxSysFindString(value, s_vmodeNames);
        if (i >= 0)
        {
            s_vmode = i;
            rxSysPrintf(" VMode   :%s\n", s_vmodeNames[i]);
        }
    }
    if (rxSysIniGetString(value, "SYSTEM", "COUNTRY"))
    {
        int i = rxSysFindString(value, s_countryNames);
        if (i >= 0)
        {
            s_country = i;
            rxSysPrintf(" Country :%s\n", s_countryNames[i]);
        }
    }
    if (rxSysIniGetString(value, "SYSTEM", "LANGUAGE"))
    {
        int i = rxSysFindString(value, s_languageNames);
        if (i >= 0)
        {
            s_language = i;
            rxSysPrintf(" Language:%s\n", s_languageNames[i]);
        }
    }
    if (rxSysIniGetString(value, "SYSTEM", "WARNING"))
    {
        int i = rxSysFindString(value, s_warningNames);
        if (i >= 0)
        {
            s_warning = i;
            rxSysPrintf(" Warning :%s\n", s_warningNames[i]);
        }
    }
}

/* ---- init ----------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Once: Sony's standard reset of VIF0 / VU0 / VIF1 / VU1, the GS path and
// DMA, then the rx thread layer and the print semaphores.
// ROR 0x00240df0

void rxSysResetHardware(void)
{
    if (s_hardwareReset != 0)
        return;
    s_hardwareReset = 1;
    sceDevVif0Reset();
    sceDevVu0Reset();
    sceDevVif1Reset();
    sceDevVu1Reset();
    sceGsResetPath();
    sceDmaReset(1);
    rxThInitOnce();
    rxSysInitSema();
}

// ──────────────────────────────────────────────────────────────────────
// The BIOS date word (0x1fc00100) when the ROM carries Sony's signature.
// ROR 0x00240e60

void rxSysReadRomDate(void)
{
    // PC: there is no BIOS image on the host; an all-zero ROM stands in for
    // 0x1fc00100, so the signature check fails and the date stays 0.
    static const char s_hostRom[0x30] = { 0 };
    const char *signature = "Sony Computer Entertainment Inc.";

    int len = (int)strlen(signature);
    if (strncmp(s_hostRom + 8, signature, len) == 0)
    {
        int date;
        memcpy(&date, s_hostRom, sizeof(date));
        s_romDate = date;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The console bring-up main runs before the game: hardware, memory hooks,
// the boot device, the rx subsystems, the IOP modules, system.ini, and the
// save-data name (region prefix + product number, "BASLUS-..." style).
// ROR 0x00240eb8

void rxSysInit(void)
{
    s_libVersion[0] = '0';
    rxSysResetHardware();
    s_romDate = 0;
    rxSysReadRomDate();
    rxSysSetMemFunc0(0);
    rxSysSetMemFunc1(0);
    rxSysSetMemFunc2(0);
    rxSysSetMemFunc3(0);
    for (int i = RX_SYS_DEVICE_NUM - 1; i >= 0; i--)
        s_deviceReady[i] = 0;
    rxSysDetectDevice();
    rxSysPrintf("BootDevice = %d\n", s_bootDevice);
    rxSysPrintf("Rom Date = %04X/%02X/%02X\n", s_romDate >> 16, (s_romDate >> 8) & 0xff,
                s_romDate & 0xff);
    rxSysInitUser();
    rxSysInitModules();
    if (s_bootDev[0] != '\0')
        rxFileCallDevice(s_bootDevice, s_bootDev);
    rxSysSetFileDevice(0, s_bootDevice);
    rxSysSetFileDevice(1, s_bootDevice);
    rxSysSetFileDir(s_bootDir);
    rxSysLoadIopModules();
    for (int i = 0; i < RX_SYS_DEVICE_NUM; i++)
        rxSysPrintf("Device[%s] %s\n", s_deviceNames[i], s_deviceReady[i] != 0 ? "OK" : "--");
    rxSysReadSystemIni();
    strcpy(s_saveName, s_regionPrefix[rxSysGetCountry()]);
    const char *number = rxSysGetProductNumber();
    strcpy(s_saveName + strlen(s_saveName), number);
    rxSysInitEnd();
}

// ──────────────────────────────────────────────────────────────────────
// A system.ini value; 0 when no system.ini was read.
// ROR 0x002417c0

int rxSysIniGetString(char *out, const char *section, const char *key)
{
    if (s_iniLoaded != 0)
        return rxSysIniFind(out, s_confBuf, section, key);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Send buffered output to the IOP tty service and wait for it.
// ROR 0x002419a8

void rxSysTtyWrite(const char *buf, int size)
{
    sceSifCallRpc(&s_ttyClient, 0x10, SIF_RPC_M_NOWAIT, (void *)buf, size, 0, 0, 0, 0);
    while (sceSifCheckStatRpc(&s_ttyClient.rpcd) != 0)
        ;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241b20

void rxSysSetIopImage(const char *img)
{
    strcpy(s_iopImage, img);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241b90

const char *rxSysGetProductNumber(void)
{
    return s_productNumber;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241bb0

int rxSysGetCountry(void)
{
    return s_country;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241c00

int rxSysGetMemorySize(void)
{
    return GetMemorySize();
}

// ──────────────────────────────────────────────────────────────────────
// A disc path: "cdrom0:..." or an ISO name ending ";1".
// ROR 0x00241c18

int rxSysIsCdromPath(const char *path)
{
    int len = (int)strlen(path);
    if (strncmp(path, "cdrom0:", 7) == 0)
        return 1;
    return len > 2 && path[len - 1] == '1' && path[len - 2] == ';';
}

// ──────────────────────────────────────────────────────────────────────
// Skip a path's device: a drive letter ("x:"), a host's "...," prefix, or
// everything up to the first ':'.
// ROR 0x00241ca0

char *rxSysPathSkipDevice(char *path)
{
    if (path[1] == ':')
        return rxSysPathFileName(path);
    if (s_bootDevice == RX_SYS_DEVICE_HOST)
    {
        for (char *p = path; *p != '\0'; p++)
        {
            if (*p == ',')
                return p + 1;
        }
    }
    for (char *p = path; *p != '\0'; p++)
    {
        if (*p == ':')
            return p + 1;
    }
    return path;
}

// ──────────────────────────────────────────────────────────────────────
// Split the boot path (argv[0] style) into device, directory and name; a
// boot from a host device also opens the host tty.
// ROR 0x00241d68

void rxSysSetBootPath(char *path)
{
    if (path == 0)
        return;
    rxSysResetHardware();
    s_bootDevice = rxSysIsCdromPath(path) != 0;
    char *rest = rxSysPathSkipDevice(path);
    int n = (int)(rest - path);
    if (path[1] == ':')
        n = 0;
    if (s_bootDevice != RX_SYS_DEVICE_CDROM)
    {
        rxSysPathCopy(s_bootDev, path, n);
        if (strcmp(s_bootDev, "host0:") == 0)
            s_bootDev[0] = '\0';
        rxSysOpenTty();
    }
    rxSysPrintf("BootDev = %s\n", s_bootDev);
    char *file = rxSysPathFileName(rest);
    rxSysPathCopy(s_bootDir, rest, (int)(file - rest));
    rxSysPrintf("BootDir = %s\n", s_bootDir);
    file = rxSysPathFileName(rest);
    int len = (int)strlen(file);
    if (len > 2 && file[len - 1] == '1' && file[len - 2] == ';')
        len -= 2;
    rxSysPathCopy(s_bootName, file, len);
    rxSysPrintf("BootName = %s\n", s_bootName);
}

/* ---- printing ------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241f08

RxSysPrintHook rxSysSetPrintHook(RxSysPrintHook hook)
{
    RxSysPrintHook old = s_printHook;
    s_printHook = hook;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// The request rxSysLockPrint hands the rx thread while it waits.
// ROR 0x00241f20

int rxSysPrintWait(intptr_t *args)
{
    (void)args;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241f28

int rxSysIsTtyOpen(void)
{
    return s_ttyRpcBound != 0 || s_ttyFd >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// The middleware's printf: format into 1 KB, ask the print hook, then
// scePrintf, or buffer for the host tty (a file, or the IOP tty RPC).
// ROR 0x00241f58

int rxSysPrintf(const char *fmt, ...)
{
    char buf[1024];
    int busy = 0;

    rxSysResetHardware();
    if (s_ttyRpcBound == 0 && s_ttyFd >= 0)
    {
        busy = rxThIsDispatchDisabled();
        if (s_ttyFd >= 0 && busy == 0)
            rxSysLockPrint();
    }
    rxThDisableDispatch();
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    int print = 1;
    if (s_printHook != 0)
        print = s_printHook(buf);
    int len = 0;
    if (print != 0)
    {
        len = (int)strlen(buf);
        if (rxSysIsTtyOpen() == 0)
        {
            // PC: the ROM passes buf as scePrintf's format; "%s" keeps a '%'
            // in a formatted message from being read as a conversion.
            scePrintf("%s", buf);
        }
        else
        {
            if (s_ttyBufLen + len < 0x1000)
            {
                strcpy(s_ttyBuf + s_ttyBufLen, buf);
                s_ttyBufLen += len;
            }
            if (s_ttyBufLen > 0)
            {
                FlushCache(0);
                if (s_ttyRpcBound == 0)
                {
                    if (busy == 0)
                    {
                        sceWrite(s_ttyFd, s_ttyBuf, s_ttyBufLen + 1);
                        s_ttyBufLen = 0;
                    }
                }
                else
                {
                    rxSysTtyWrite(s_ttyBuf, s_ttyBufLen + 1);
                    s_ttyBufLen = 0;
                }
            }
        }
    }
    rxThEnableDispatch();
    if (s_ttyRpcBound == 0 && s_ttyFd >= 0 && busy == 0)
        rxSysUnlockPrint();
    return len;
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the retail build.
// ROR 0x00242150

void rxSysInitEnd(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// The EE SemaParam takes max_count at +4 and init_count at +8, in that
// order of the arguments.
// ROR 0x00242160

int rxSysCreateSema(int max, int init)
{
    struct SemaParam param;
    memset(&param, 0, sizeof(param));
    param.maxCount = max;
    param.initCount = init;
    return CreateSema(&param);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242390

void rxSysInitSema(void)
{
    s_printSema = rxSysCreateSema(1, 1);
    s_sema1 = rxSysCreateSema(1, 1);
}

// ──────────────────────────────────────────────────────────────────────
// Take the print semaphore, letting the rx thread run while it is held.
// ROR 0x00242400

void rxSysLockPrint(void)
{
    while (PollSema(s_printSema) == -1)
    {
        rxThRequest(rxSysPrintWait, 0, 0, 0);
        rxThYield();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242460

void rxSysUnlockPrint(void)
{
    SignalSema(s_printSema);
}

// ──────────────────────────────────────────────────────────────────────
// rxSysLockPrint's twin for the second semaphore.
// ROR 0x00242480

void rxSysLockSema1(void)
{
    while (PollSema(s_sema1) == -1)
    {
        rxThRequest(rxSysPrintWait, 0, 0, 0);
        rxThYield();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002424e0

void rxSysUnlockSema1(void)
{
    SignalSema(s_sema1);
}

/* ---- file wrappers ------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// A cdrom0: path switches the drive to CD mode first.
// ROR 0x00242500

void rxSysCheckCdPath(const char *path)
{
    if (rxSysIsCdromPath(path) != 0)
        rxSysSetCdMode();
}

// ──────────────────────────────────────────────────────────────────────
// The file calls run under the print lock (the host tty shares fileio).
// ROR 0x00242528

int rxSysOpen(const char *path, int mode)
{
    rxSysLockPrint();
    rxSysCheckCdPath(path);
    int fd = sceOpen(path, mode);
    rxSysUnlockPrint();
    return fd;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242578

int rxSysClose(int fd)
{
    rxSysLockPrint();
    int ret = sceClose(fd);
    rxSysUnlockPrint();
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002425b0

int rxSysRead(int fd, void *buf, int size)
{
    rxSysLockPrint();
    int ret = sceRead(fd, buf, size);
    rxSysUnlockPrint();
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242608

int rxSysWrite(int fd, const void *buf, int size)
{
    rxSysLockPrint();
    int ret = sceWrite(fd, buf, size);
    rxSysUnlockPrint();
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242660

int rxSysLseek(int fd, int offset, int whence)
{
    rxSysLockPrint();
    int ret = sceLseek(fd, offset, whence);
    rxSysUnlockPrint();
    return ret;
}

/* ---- subsystems ----------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Empty in the retail build.
// ROR 0x002433d8

void rxSysInitUser(void)
{
}

/* The four pools it threads into free lists (bss 0x7dabb0 .. 0x7df9b8) are
 * rxfile's: its file slots, markers, ring segments and tags (rxfile.h). */

// ──────────────────────────────────────────────────────────────────────
// Reset the rx pools, start the file layer and its thread, and register
// the per-tick callback with the thread layer.
// ROR 0x00243400

void rxSysInitModules(void)
{
    for (int i = 0; i < 64; i++)
        rxFileSlots[i].used = 0;
    rxFileMarkerFree = 0;
    for (int i = 0; i < 64; i++)
    {
        rxFileMarkerPool[i].next = rxFileMarkerFree;
        rxFileMarkerFree = &rxFileMarkerPool[i];
    }
    rxFileSegmentFree = 0;
    for (int i = 0; i < 256; i++)
    {
        rxFileSegmentPool[i].next = rxFileSegmentFree;
        rxFileSegmentFree = &rxFileSegmentPool[i];
    }
    rxFileTagFree = 0;
    for (int i = 0; i < 64; i++)
    {
        rxFileTagPool[i].next = rxFileTagFree;
        rxFileTagFree = &rxFileTagPool[i];
    }
    rxFileInit();
    rxFileStartThread();
    rxFileInitSlots();
    rxThAddCallback(rxSysThreadCallback);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002433e8

void rxSysThreadCallback(int yielding)
{
    (void)yielding;             // rxThRunCallbacks' argument, unused
    rxSysUpdateDevices();
}

// ──────────────────────────────────────────────────────────────────────
// The current file context's device (main passes 1, CDROM); returns the
// previous one.
// ROR 0x00243608

int rxSysInitFileDevice(int device)
{
    return rxFileSetCurrent(device);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243638

int rxSysSetFileDevice(int slot, int device)
{
    return rxFileSetDevice(slot, device);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243650

void rxSysSetFileDir(const char *dir)
{
    rxFileSetDir(rxFileGetCurrent(), dir);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243680

char *rxSysGetFileDir(void)
{
    return rxFileGetDir(rxFileGetCurrent());
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002436a0

void rxSysSetFileSearchDir(const char *dir)
{
    rxFileSetSearchDir(rxFileGetCurrent(), dir);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002436d0

char *rxSysGetFileSearchDir(void)
{
    return rxFileGetSearchDir(rxFileGetCurrent());
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002436f0

void rxSysPushSearchDir(void)
{
    rxFilePushSearchDir(rxFileGetCurrent());
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243710

void rxSysPopSearchDir(void)
{
    rxFilePopSearchDir(rxFileGetCurrent());
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243730

char *rxSysGetLastName(void)
{
    return rxFileGetLastName(rxFileGetCurrent());
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243750

int rxSysSetDeviceParam(int value)
{
    return rxFileSetDeviceParam(rxFileGetCurrent(), value);
}

// ──────────────────────────────────────────────────────────────────────
// Open an rxfile slot on the current device (returns the fd, -1).
// ROR 0x00243788

int rxSysStreamOpen(const char *name, int flags)
{
    return rxFileOpenEx(name, flags, 0, 0, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Open a slot that streams into a ring buffer, refilled while it holds
// threshold bytes or fewer.
// ROR 0x002437a8

int rxSysStreamOpenRing(const char *name, void *ring, int ringSize, int threshold)
{
    return rxFileOpenEx(name, SCE_RDONLY | RX_FILE_NOWAIT | RX_FILE_RING, ring, ringSize, threshold);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002437d8

int rxSysStreamOpenPath(int device, const char *path, int flags, void *ring, int ringSize,
                        int threshold)
{
    return rxFileOpenSlot(device, path, flags, ring, ringSize, threshold);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x002413d8
int rxSysIniFind(char *out, const char *text, const char *section, const char *key)
{
    ROSEPAN_STUB("rxSysIniFind", 0x002413d8);
    return {};
}

// stub of ROR 0x00241a00
void rxSysOpenTty(void)
{
    ROSEPAN_STUB("rxSysOpenTty", 0x00241a00);
}

// stub of ROR 0x00243560
void rxSysUpdateDevices(void)
{
    ROSEPAN_STUB("rxSysUpdateDevices", 0x00243560);
}

// stub of ROR 0x002431c0
void rxSysInitExceptionScreen(void)
{
    ROSEPAN_STUB("rxSysInitExceptionScreen", 0x002431c0);
}

// stub of ROR 0x00242960
void rxSysSetDebugHandlers(void)
{
    ROSEPAN_STUB("rxSysSetDebugHandlers", 0x00242960);
}

// stub of ROR 0x00241ac8
void rxSysTerm(void)
{
    ROSEPAN_STUB("rxSysTerm", 0x00241ac8);
}
