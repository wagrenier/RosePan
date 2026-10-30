/* ==========================================================================
 *  rx/rxsys.cpp
 *
 *  The rx middleware's system layer: the console bring-up main runs before
 *  the game (rxSysInit), the IOP reboot and module list, the boot path,
 *  system.ini, and rxSysPrintf. See rxsys.h.
 *
 *  Not reconstructed yet in this file: the System Monitor, the text console
 *  the exception handler prints on (0x242c10..0x2432d8: rxSysInitExceptionScreen
 *  is a stub; it draws through GS image uploads, which the PC must replace
 *  with a host draw).
 * ======================================================================== */
#include "common/rosepan_stub.h"

#include "rx/rxsys.h"

#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "rx/rxfile.h"
#include "rx/rxthread.h"
#include "rosepan/rosepan_memory.h"
#include "sdk/eekernel.h"
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

/* The exception handler's register rows: a label, then count registers
 * from first (main has its own copy for mainExceptionHandler).
 * layout: host -- a pointer table, never read from files. */
struct RxSysRegRow
{
    const char *name;
    int         first;
    int         count;
    int         pad;
};
static const RxSysRegRow s_regRows[] = {                                  /* data 0x2dbda0 */
    { "\nat/v0-1    ",  1, 3, 0 },
    { "\na0-3       ",  4, 4, 0 },
    { "\nt0-3       ",  8, 4, 0 },
    { "\nt4-9       ", 12, 4, 0 },
    { "",              24, 2, 0 },
    { "\ns0-8       ", 16, 4, 0 },
    { "\ns4-7/k0-1  ", 20, 4, 0 },
    { "",              26, 2, 0 },
    { "\ngp/sp/fp/ra", 28, 4, 0 },
    { NULL,             0, 0, 0 },
};
static int s_exceptionOpen;                     /* data 0x2dbe40, set by the handler, read by nothing found */
static int s_inException;                       /* data 0x2dbe44 */
static RosePanDebugHandler s_oldHandlersLow[7]; /* data 0x2dbe4c, causes 1..7   */
static RosePanDebugHandler s_oldHandlersHigh[4];/* data 0x2dbe70, causes 10..13 */
static int s_debugHandlersSet;                  /* data 0x2dbe88 */

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

/* ---- ini parser ------------------------------------------------------------ */

static inline int rxSysIsEol(char c)
{
    return c == '\r' || c == '\n';
}

// ──────────────────────────────────────────────────────────────────────
// The line after "[section]"; NULL when there is none. A NULL section
// returns the text itself. Only a '[' in a line's first column counts.
// ROR 0x002410b8

char *rxSysIniFindSection(char *text, const char *section)
{
    if (section == NULL)
        return text;
    int len = strlen(section);
    while (*text != '\0')
    {
        char *p = text;
        if (*text == '[')
        {
            // The ROM's compare leaves p one past the first mismatch.
            p = text + 1;
            int i = 0;
            if (len > 0)
            {
                char want = section[0];
                char have = *p++;
                while (have == want)
                {
                    i++;
                    if (i >= len)
                        break;
                    want = section[i];
                    have = *p++;
                }
            }
            if (i == len && *p == ']')
                return rxSysSkipEol(p + rxSysStrLenToEol(p));
        }
        text = rxSysSkipEol(p + rxSysStrLenToEol(p));
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Copy a line's key -- up to '=' or the end of the line -- into out, with
// the blanks before '=' trimmed; returns where it stopped ('=', EOL or 0).
// ROR 0x002411d0

char *rxSysIniReadKey(char *out, char *line)
{
    int n = 0;
    out[0] = '\0';
    for (;;)
    {
        char c = *line;
        if (c == '\0' || rxSysIsEol(c))
            return line;
        if (c == '=')
        {
            while (n > 0 && (out[n - 1] == ' ' || out[n - 1] == '\t'))
                n--;
            out[n] = '\0';
            return line;
        }
        out[n] = c;
        out[n + 1] = '\0';
        line++;
        n++;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The value of key in a section's body (after '=' and blanks); NULL at
// the next section or the end of the text.
// ROR 0x00241298

char *rxSysIniFindKey(char *body, const char *key)
{
    char name[0x100];
    int len = strlen(key);
    for (;;)
    {
        char *p = rxSysSkipSpace(body);
        char c = *p;
        if (c == '\0' || c == '[')
            return NULL;
        if (c != '#' && !rxSysIsEol(c))
        {
            p = rxSysIniReadKey(name, p);
            if ((int)strlen(name) == len)
            {
                int i = 0;
                while (i < len && name[i] == key[i])
                    i++;
                if (i == len)
                {
                    p = rxSysSkipSpace(p);
                    return rxSysSkipSpace(p + 1);
                }
            }
        }
        body = rxSysSkipEol(p + rxSysStrLenToEol(p));
    }
}

// ──────────────────────────────────────────────────────────────────────
// A key's value: the rest of its line, a '\' before the end of the line
// joining the next one (comment lines between are skipped; the next line
// must start with a blank, which is kept). Returns the length.
// ROR 0x002413d8

int rxSysIniFind(char *out, const char *text, const char *section, const char *key)
{
    *out = '\0';
    char *body = rxSysIniFindSection((char *)text, section);
    if (body == NULL)
        return 0;
    char *p = rxSysIniFindKey(body, key);
    if (p == NULL)
        return 0;
    int n = 0;
    for (char c = *p; c != '\0' && !rxSysIsEol(c); c = *++p)
    {
        if (c == '\\' && rxSysIsEol(p[1]))
        {
            for (;;)
            {
                p = rxSysSkipEol(p + 1);
                if (*p != '#')
                    break;
                while (*p != '\0' && !rxSysIsEol(*p))
                    p++;
                if (*p == '\0')
                {
                    *out = '\0';
                    return n;
                }
            }
            if (*p != ' ' && *p != '\t')
            {
                *out = '\0';
                return n;
            }
            *out = *p;
        }
        else
        {
            *out = c;
        }
        out++;
        n++;
    }
    *out = '\0';
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// The key lines of a section (comments and blank lines not counted).
// ROR 0x00241558

int rxSysIniCountKeys(char *body)
{
    int n = 0;
    for (;;)
    {
        char *p = rxSysSkipSpace(body);
        char c = *p;
        if (c == '\0' || c == '[')
            return n;
        if (c != '#' && !rxSysIsEol(c))
            n++;
        body = rxSysSkipEol(p + rxSysStrLenToEol(p));
    }
}

// ──────────────────────────────────────────────────────────────────────
// The index-th key of a section into out; returns its length, 0 past the
// last.
// ROR 0x00241620

int rxSysIniKeyAt(char *out, char *body, int index)
{
    int n = 0;
    *out = '\0';
    for (;;)
    {
        char *p = rxSysSkipSpace(body);
        char c = *p;
        if (c == '\0' || c == '[')
            return 0;
        if (c != '#' && !rxSysIsEol(c))
        {
            if (n == index)
            {
                rxSysIniReadKey(out, p);
                return strlen(out);
            }
            n++;
        }
        body = rxSysSkipEol(p + rxSysStrLenToEol(p));
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241720

int rxSysIniFindKeyCount(char *text, const char *section)
{
    char *body = rxSysIniFindSection(text, section);
    if (body == NULL)
        return 0;
    return rxSysIniCountKeys(body);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241758

int rxSysIniFindKeyAt(char *out, char *text, const char *section, int index)
{
    char *body = rxSysIniFindSection(text, section);
    if (body == NULL)
        return 0;
    return rxSysIniKeyAt(out, body, index);
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
// The number of keys in a system.ini section; 0 when no system.ini was read.
// ROR 0x00241808

int rxSysIniGetKeyCount(const char *section)
{
    if (s_iniLoaded != 0)
        return rxSysIniFindKeyCount(s_confBuf, section);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241848

int rxSysIniGetKeyAt(char *out, const char *section, int index)
{
    if (s_iniLoaded != 0)
        return rxSysIniFindKeyAt(out, s_confBuf, section, index);
    return 0;
}

/* ---- tty ------------------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// Bind the tty client, up to ten tries, each waiting a moment for the IOP
// to serve it; 1 when bound.
// ROR 0x00241890

int rxSysBindTtyRpc(sceSifClientData *client, unsigned int id)
{
    rxSysPrintf("rxsys: binding IOP module %08X ...\n", id);
    for (int i = 0; i < 10; i++)
    {
        client->serve = 0;
        if (sceSifBindRpc(client, id, 0) < 0)
            break;
        // (a short busy-wait loop in the ROM)
        if (client->serve != 0)
            break;
    }
    FlushCache(0);
    int bound = client->serve != 0;
    if (bound)
        rxSysPrintf("rxsys : binding completed\n");
    else
        rxSysPrintf("rxsys : bind error\n");
    return bound;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241968

void rxSysBindTty(void)
{
    sceSifInitRpc(0);
    if (rxSysBindTtyRpc(&s_ttyClient, 0x80000220))
        s_ttyRpcBound = 1;
}

// ──────────────────────────────────────────────────────────────────────
// With a boot device, send rxSysPrintf's output to the IOP tty service;
// when it does not bind, to the device's tty file ("host0:>"). The PC
// boot path leaves s_bootDev empty, so nothing opens.
// ROR 0x00241a00

void rxSysOpenTty(void)
{
    char path[0x80];
    if (s_bootDev[0] == '\0')
        return;
    rxSysBindTty();
    if (s_ttyRpcBound != 0)
        return;
    strcpy(path, s_bootDev);
    strcpy(path + strlen(path), ">");
    s_ttyFd = rxSysOpen(path, SCE_WRONLY);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241a80

void rxSysCloseTty(void)
{
    if (s_ttyFd >= 0)
    {
        rxSysClose(s_ttyFd);
        s_ttyFd = -1;
    }
    s_ttyRpcBound = 0;
}

// ──────────────────────────────────────────────────────────────────────
// rxSysInit's teardown, in reverse.
// ROR 0x00241ac8

void rxSysTerm(void)
{
    rxSysCloseTty();
    rxSysTermEnd();
    rxSysTermModules();
    rxSysTermUser();
    rxSysTermSema();
    rxThExitOnce();
    rxSysRestoreDebugHandlers();
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
// system.ini VMODE (0 NTSC, 1 PAL).
// ROR 0x00241b50

int rxSysGetVMode(void)
{
    return s_vmode;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00241b60

int rxSysGetFrameRate(void)
{
    return rxSysGetVMode() != 0 ? 50 : 60;
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
// Empty in the retail build; rxSysTerm's step after rxSysCloseTty.
// ROR 0x00242158

void rxSysTermEnd(void)
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

/* ---- IOP RPC clients ------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// Create the client's call semaphore and bind it to an IOP service,
// retrying until it is served; 0 on a bind error. No caller in the retail
// ELF.
// ROR 0x00242188

int rxSysBindRpc(RxSysRpc *rpc, unsigned int id)
{
    rpc->sema = rxSysCreateSema(1, 1);
    rxSysPrintf("rxsys : binding IOP module %08X ...\n", id);
    do
    {
        if (sceSifBindRpc(&rpc->client, id, 0) < 0)
        {
            rxSysPrintf("rxsys : bind error '%08X'\n", id);
            return 0;
        }
        // (a short busy-wait loop in the ROM)
    } while (rpc->client.serve == 0);
    FlushCache(0);
    rxSysPrintf("rxsys : binding completed\n");
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242268

void rxSysUnbindRpc(RxSysRpc *rpc)
{
    if (rpc->sema >= 0)
    {
        DeleteSema(rpc->sema);
        rpc->sema = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// One call at a time per client: take its semaphore (rxSysRpcDone gives
// it back when the IOP answers), post the call, wait for the answer.
// ROR 0x002422a8

void rxSysCallRpc(RxSysRpc *rpc, int fno, void *send, int sendSize, void *recv, int recvSize)
{
    rxThDisableDispatch();
    WaitSema(rpc->sema);
    FlushCache(0);
    sceSifCallRpc(&rpc->client, fno, SIF_RPC_M_NOWAIT, send, sendSize, recv, recvSize,
                  (sceSifEndFunc)rxSysRpcDone, rpc);
    rxThEnableDispatch();
    while (sceSifCheckStatRpc(&rpc->client.rpcd) == 1)
        ;
    FlushCache(0);
}

// ──────────────────────────────────────────────────────────────────────
// The call's end function (interrupt context).
// ROR 0x00242368

void rxSysRpcDone(void *rpc)
{
    iSignalSema(((RxSysRpc *)rpc)->sema);
}

// ──────────────────────────────────────────────────────────────────────
// rxSysInitSema's twin.
// ROR 0x002423d0

void rxSysTermSema(void)
{
    DeleteSema(s_sema1);
    DeleteSema(s_printSema);
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

// ──────────────────────────────────────────────────────────────────────
// sceIoctl under the print lock.
// ROR 0x002426b8

int rxSysIoctl(int fd, int req, void *arg)
{
    rxSysLockPrint();
    int ret = sceIoctl(fd, req, arg);
    rxSysUnlockPrint();
    return ret;
}

/* ---- exception handler ------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242710

void rxSysLockConsole(void)
{
    rxSysLockPrint();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242728

void rxSysUnlockConsole(void)
{
    rxSysUnlockPrint();
}

// ──────────────────────────────────────────────────────────────────────
// The debug exception handler rxSysSetDebugHandlers installs: once, stop
// the vblank interrupt, reset the VIFs, VUs, GS path and DMA, open the
// exception screen, print epc / status / cause, the bad addresses, the
// registers by row and 14 lines of the stack, then hang.
// mainExceptionHandler (0x204a00) is its twin. The host kernel never
// raises a debug exception, so it does not run on the PC.
// ROR 0x00242740

void rxSysExceptionHandler(u_int stat, u_int cause, u_int epc, u_int bva, u_int bpa,
                           u_long128 *gpr)
{
    if (s_inException != 0)
        return;
    s_inException = 1;
    s_exceptionOpen = 1;
    iDisableIntc(2);        // INTC_VBLANK_S
    sceDevVif0Reset();
    sceDevVu0Reset();
    sceDevVif1Reset();
    sceDevVu1Reset();
    sceGsResetPath();
    sceDmaReset(1);
    rxSysInitExceptionScreen();
    rxSysPrintf("epc     = %08x  status  = %08x  cause   = %08x\n", epc, stat, cause);
    rxSysPrintf("badvadr = %08x  badpadr = %08x\n", bva, bpa);
    for (const RxSysRegRow *row = s_regRows; row->name != NULL; row++)
    {
        rxSysPrintf("%s", row->name);
        for (int i = 0; i < row->count; i++)
            rxSysPrintf(" %08x", *(u_int *)&gpr[row->first + i]);
    }
    rxSysPrintf("\n\n");
    // PC: the stack pointer is an EE address; read it through emulated RAM.
    u_int sp = *(u_int *)&gpr[29];
    for (int line = 13; line >= 0; line--, sp += 0x10)
    {
        if (sp > 0xfffff && (int)sp <= rxSysGetMemorySize() - 0x11)
        {
            u_int *p = (u_int *)RosePan_GetHostPointer(sp);
            rxSysPrintf("%08x: %08x %08x %08x %08x\n", sp, p[0], p[1], p[2], p[3]);
        }
    }
    for (;;)
    {
    }
}

// ──────────────────────────────────────────────────────────────────────
// Once (main calls it at boot): rxSysExceptionHandler for causes 1-7 and
// 10-13, keeping the old handlers for rxSysRestoreDebugHandlers.
// ROR 0x00242960

void rxSysSetDebugHandlers(void)
{
    if (s_debugHandlersSet != 0)
        return;
    s_debugHandlersSet = 1;
    for (int cause = 1; cause < 8; cause++)
        s_oldHandlersLow[cause - 1] = (RosePanDebugHandler)(intptr_t)SetDebugHandler(cause, rxSysExceptionHandler);
    for (int cause = 10; cause < 14; cause++)
        s_oldHandlersHigh[cause - 10] = (RosePanDebugHandler)(intptr_t)SetDebugHandler(cause, rxSysExceptionHandler);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00242a10

void rxSysRestoreDebugHandlers(void)
{
    if (s_debugHandlersSet == 0)
        return;
    s_debugHandlersSet = 0;
    for (int cause = 1; cause < 8; cause++)
        SetDebugHandler(cause, s_oldHandlersLow[cause - 1]);
    for (int cause = 10; cause < 14; cause++)
        SetDebugHandler(cause, s_oldHandlersHigh[cause - 10]);
}

// ──────────────────────────────────────────────────────────────────────
// Print memory 16 bytes a line: the address, the bytes in hex, then the
// printable ones (letters, digits, punctuation, space; '.' for others).
// No caller in the retail ELF.
// ROR 0x00242aa8

void rxSysHexDump(const char *p, int size)
{
    while (size > 0)
    {
        int n = size < 0x10 ? size : 0x10;
        rxSysPrintf("%08X: ", (u_int)(uintptr_t)p);  // PC: the host address's low bits
        size -= n;
        int i;
        for (i = 0; i < n; i++)
            rxSysPrintf("%02X ", p[i]);
        for (; i < 0x10; i++)
            rxSysPrintf("   ");
        for (i = 0; i < n; i++)
        {
            unsigned char c = (unsigned char)p[i];
            if (!(isalnum(c) || ispunct(c) || c == ' '))
                c = '.';
            rxSysPrintf("%c", c);
        }
        for (; i < 0x10; i++)
            rxSysPrintf(" ");
        rxSysPrintf("\n");
        p += n;
    }
}

/* ---- subsystems ----------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Empty in the retail build.
// ROR 0x002433d8

void rxSysInitUser(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the retail build.
// ROR 0x002433e0

void rxSysTermUser(void)
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
// rxSysInitModules' twin: close every rx file, drop the per-tick
// callback, stop rxfile's thread and shut rxfile down.
// ROR 0x002434f0

void rxSysTermModules(void)
{
    for (int i = 0; i < 64; i++)
        rxFileClose(i);
    rxThRemoveCallback(rxSysThreadCallback);
    rxFileUpdate();
    rxFileStopThread();
    rxFileTerm();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002433e8

void rxSysThreadCallback(int yielding)
{
    rxSysSetFileContext(yielding);
}

// ──────────────────────────────────────────────────────────────────────
// rxfile's context follows the running thread: 1 while the main thread
// yields to the rx thread (rxThYield), 0 when it runs again.
// ROR 0x00243560

int rxSysSetFileContext(int index)
{
    return rxFileSetContext(index);
}

// ──────────────────────────────────────────────────────────────────────
// A device driver's control call. No caller in the retail ELF.
// ROR 0x00243578

int rxSysDeviceControl(int device, int cmd, int arg)
{
    RxFileDriver *driver = rxFileGetDriver(device);
    if (driver != NULL)
        return driver->control(cmd, arg);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A device driver's status. No caller in the retail ELF.
// ROR 0x002435c8

int rxSysDeviceStatus(int device, int what)
{
    RxFileDriver *driver = rxFileGetDriver(device);
    if (driver != NULL)
        return driver->getStatus(what);
    return 0;
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
// ROR 0x00243620

int rxSysGetFileDevice(void)
{
    return rxFileGetCurrent();
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

// stub of ROR 0x002431c0
void rxSysInitExceptionScreen(void)
{
    ROSEPAN_STUB("rxSysInitExceptionScreen", 0x002431c0);
}

