// sequence/seqvm.cpp -- the sequence script VM (0x20ff34..0x2155a0).
//
// The sequence scripts are MIPS code the VM interprets one word at a time:
// seqVmRun decodes the word and calls a handler from one of three tables
// (primary opcode data 0x2d9958, SPECIAL funct 0x2d9a58, COP1 funct
// 0x2d9b08) with the word's fields. The handlers keep the registers in
// g_seqGpr / g_seqFpr. Their semantics are the script compiler's, not the
// R5900's: every load and store moves a whole word (lb / lbu scale the
// offset by 4), lui 0x41 / 0x45 (0x42 / 0x46) load the image's relocated
// data address, jal takes 16 bits of target, mthi / mtlo read rd, mult is
// unsigned, the unary FPU operations read ft.
//
// Script addresses: code is linked at 0x8160 (a jump target t runs the
// image's word at code + t - 0x8160); a target below 0x8160 is a native
// call, entry (t - 0x144) / 0x40 of g_seqFuncTable (sequence/seqcall.cpp).
// Data addresses are the host addresses themselves.
//
// Threads: a thread's registers and its copy of the image's 0x780-byte gp
// area (small data with the stack at its top) are saved into its record
// and a 2 KB stack slot when a native call makes it wait (seqVmSaveThread),
// and loaded back before it runs (seqVmLoadThread).
//
// RoR does not name this file (no assert, no source path): it follows
// sequence/sequence.cpp and holds the rest of Sequence's methods, so it may
// be part of that file; seqvm is a guess.
//
// PC: registers are intptr_t (open-items "Decisions"). Memory words stay
// 32 bits, so a host pointer a script stores (sw) would lose its top half:
// seqVmLoadWord / seqVmStoreWord keep the full value of every word that did
// not fit in 32 bits beside memory (s_wideWords) and give it back when the
// word still holds its low half (hazards.md "Script pointers").
//
// Complete: the handlers, seqVmRun, the image / thread state functions,
// every Sequence thread method (the starters execThread / execImmediate /
// startThread / queueThread / queueSingle / queueTrigger*, the entry calls,
// the end / find / has queries) and the save data of the globals and bits.
#include "common/rosepan_stub.h"

#include <string.h>

#include <unordered_map>

#include "lib/unassigned.h"
#include "sequence/seqcall.h"
#include "sequence/sequence.h"

// The VM's state (see sequence.h).
SeqNode<SeqThread> *g_seqThread;
SeqNode<SeqThread> *g_seqThreadEnd;
u_int    g_seqFpr[32];
intptr_t g_seqGpr[32];
intptr_t g_seqPc;
int      g_seqHi;
int      g_seqLo;
intptr_t g_seqJump;
u_int    g_seqBranch;
intptr_t g_seqImageCode;
int      g_seqDataLink;
intptr_t g_seqData2;
intptr_t g_seqData;
intptr_t g_seqDataReloc;
int      g_seqHdr20;
int      g_seqHdr24;
int      g_seqFpuCond;
SeqVmImage g_seqImageState[3];
int     *g_seqGlobal;
u_char  *g_seqStackMid;
intptr_t g_seqCallSave[0x100];
u_char  *g_seqStack;
int      g_seqStackUsed[32];
intptr_t g_seqReturnPc;
int      g_seqCallNowKind;
int      g_seqImage;
int      g_seqStackNext;
intptr_t g_seqImageRegs[3][32];
u_char  *g_seqImageSave[3];

#define R(i)  g_seqGpr[i]

// The size of an image's gp area (stack included) a thread keeps.
#define SEQ_GP_AREA 0x780

// ──────────────────────────────────────────────────────────────────────
// PC: script memory words (see the file comment).

static std::unordered_map<intptr_t, intptr_t> s_wideWords;

static int seqVmRawLoad(intptr_t addr)
{
    int value;
    memcpy(&value, (const void *)addr, sizeof(value));
    return value;
}

static void seqVmRawStore(intptr_t addr, int value)
{
    memcpy((void *)addr, &value, sizeof(value));
}

intptr_t seqVmLoadWord(intptr_t addr)
{
    int value = seqVmRawLoad(addr);
    if (!s_wideWords.empty()) {
        std::unordered_map<intptr_t, intptr_t>::const_iterator it = s_wideWords.find(addr);
        if (it != s_wideWords.end() && (int)it->second == value)
            return it->second;
    }
    return value;
}

void seqVmStoreWord(intptr_t addr, intptr_t value)
{
    seqVmRawStore(addr, (int)value);
    if (value != (intptr_t)(int)value)
        s_wideWords[addr] = value;
    else if (!s_wideWords.empty())
        s_wideWords.erase(addr);
}

// A gp area copy (memcpy in the ROM) that takes the wide words along.
static void seqVmCopyArea(u_char *dst, const u_char *src)
{
    memcpy(dst, src, SEQ_GP_AREA);
    if (s_wideWords.empty())
        return;
    for (int i = 0; i < SEQ_GP_AREA; i += 4) {
        std::unordered_map<intptr_t, intptr_t>::const_iterator it = s_wideWords.find((intptr_t)(src + i));
        if (it != s_wideWords.end())
            s_wideWords[(intptr_t)(dst + i)] = it->second;
        else
            s_wideWords.erase((intptr_t)(dst + i));
    }
}

// PC: addu / subu on two 32-bit values wrap at 32 bits as on the EE; with
// a host pointer the sum is the pointer's.
static intptr_t seqVmAdd(intptr_t a, intptr_t b)
{
    if (a == (intptr_t)(int)a && b == (intptr_t)(int)b)
        return (int)((u_int)a + (u_int)b);
    return a + b;
}

static intptr_t seqVmSub(intptr_t a, intptr_t b)
{
    if (a == (intptr_t)(int)a && b == (intptr_t)(int)b)
        return (int)((u_int)a - (u_int)b);
    return a - b;
}

static float seqVmGetF(int i)
{
    float value;
    memcpy(&value, &g_seqFpr[i], sizeof(value));
    return value;
}

static void seqVmSetF(int i, float value)
{
    memcpy(&g_seqFpr[i], &value, sizeof(value));
}

// A branch's target in script addresses: the delay slot's + offset * 4.
static void seqVmBranch(u_int imm)
{
    g_seqBranch = (((int)(imm << 18) >> 16) + (u_int)(g_seqPc - g_seqImageCode) + 0x8164) | 0x80000000;
}

// The return address of a jal / jalr: the word after the delay slot.
static int seqVmLink(void)
{
    return (int)(g_seqPc - g_seqImageCode) + 0x8168;
}

// ──────────────────────────────────────────────────────────────────────
// The handlers. Primary and SPECIAL: (rs, rt, imm, word) and (rs, rt, rd,
// sa); COP1 functions: (fs, ft, fd, fmt).

typedef void (*SeqOp)(u_int a, u_int b, u_int c, u_int d);

// ROR 0x0020ff34

void seqOpNop(u_int, u_int, u_int, u_int)
{
}

// rt = 0 is a move.
// ROR 0x0020ff3c

static void seqOpAdd(u_int rs, u_int rt, u_int rd, u_int)
{
    if (rt == 0)
        R(rd) = R(rs);
    else
        R(rd) = seqVmAdd(R(rs), R(rt));
}

// ROR 0x0020ff8c

static void seqOpAddu(u_int rs, u_int rt, u_int rd, u_int)
{
    if (rt == 0)
        R(rd) = R(rs);
    else
        R(rd) = seqVmAdd(R(rs), R(rt));
}

// rs = 0 loads the immediate.
// ROR 0x0020ffdc

static void seqOpAddi(u_int rs, u_int rt, u_int imm, u_int)
{
    if (rs == 0)
        R(rt) = (short)imm;
    else
        R(rt) = seqVmAdd(R(rs), (short)imm);
}

// ROR 0x00210024

static void seqOpAddiu(u_int rs, u_int rt, u_int imm, u_int)
{
    if (rs == 0)
        R(rt) = (short)imm;
    else
        R(rt) = seqVmAdd(R(rs), (short)imm);
}

// ROR 0x0021006c

static void seqOpAnd(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = R(rs) & R(rt);
}

// ROR 0x002100a0

static void seqOpAndi(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = R(rs) & imm;
}

// Both divides are signed; the ROM's divide by zero breaks (break 7).
// ROR 0x002100c8

static void seqOpDiv(u_int rs, u_int rt, u_int, u_int)
{
    int d = (int)R(rt);
    int n = (int)R(rs);
    // PC: no trap on the host -- a zero divisor (break 7 in the ROM) and
    // the overflowing INT_MIN / -1 leave what the EE leaves.
    if (d == 0)
        return;
    if (d == -1 && n == (int)0x80000000) {
        g_seqLo = n;
        g_seqHi = 0;
        return;
    }
    g_seqLo = n / d;
    g_seqHi = n % d;
}

// ROR 0x00210110

static void seqOpDivu(u_int rs, u_int rt, u_int, u_int)
{
    int d = (int)R(rt);
    int n = (int)R(rs);
    // PC: as seqOpDiv.
    if (d == 0)
        return;
    if (d == -1 && n == (int)0x80000000) {
        g_seqLo = n;
        g_seqHi = 0;
        return;
    }
    g_seqLo = n / d;
    g_seqHi = n % d;
}

// Both multiplies are unsigned.
// ROR 0x00210158

static void seqOpMult(u_int rs, u_int rt, u_int, u_int)
{
    uint64_t p = (uint64_t)(u_int)R(rs) * (u_int)R(rt);
    g_seqLo = (int)p;
    g_seqHi = (int)(p >> 32);
}

// ROR 0x002101b4

static void seqOpMultu(u_int rs, u_int rt, u_int, u_int)
{
    uint64_t p = (uint64_t)(u_int)R(rs) * (u_int)R(rt);
    g_seqLo = (int)p;
    g_seqHi = (int)(p >> 32);
}

// ROR 0x00210210

static void seqOpNor(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = ~(R(rs) | R(rt));
}

// ROR 0x00210244

static void seqOpOr(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = R(rs) | R(rt);
}

// ROR 0x00210278

static void seqOpOri(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = R(rs) | imm;
}

// ROR 0x002102a0

static void seqOpSll(u_int, u_int rt, u_int rd, u_int sa)
{
    R(rd) = (int)((u_int)R(rt) << (sa & 0x1f));
}

// ROR 0x002102c8

static void seqOpSllv(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = (int)((u_int)R(rt) << ((u_int)R(rs) & 0x1f));
}

// ROR 0x002102fc

static void seqOpSra(u_int, u_int rt, u_int rd, u_int sa)
{
    R(rd) = (int)R(rt) >> (sa & 0x1f);
}

// ROR 0x00210324

static void seqOpSrav(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = (int)R(rt) >> ((u_int)R(rs) & 0x1f);
}

// ROR 0x00210358

static void seqOpSrl(u_int, u_int rt, u_int rd, u_int sa)
{
    R(rd) = (int)((u_int)R(rt) >> (sa & 0x1f));
}

// ROR 0x00210380

static void seqOpSrlv(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = (int)((u_int)R(rt) >> ((u_int)R(rs) & 0x1f));
}

// ROR 0x002103b4

static void seqOpSub(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = seqVmSub(R(rs), R(rt));
}

// ROR 0x002103e8

static void seqOpSubu(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = seqVmSub(R(rs), R(rt));
}

// ROR 0x0021041c

static void seqOpXor(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = R(rs) ^ R(rt);
}

// ROR 0x00210450

static void seqOpXori(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = R(rs) ^ imm;
}

// 0x41 / 0x45 load the image's relocated data address, 0x42 / 0x46 the
// next 64 KB.
// ROR 0x00210478

static void seqOpLui(u_int, u_int rt, u_int imm, u_int)
{
    if (imm == 0x45 || imm == 0x41)
        R(rt) = g_seqDataReloc;
    else if (imm == 0x46 || imm == 0x42)
        R(rt) = g_seqDataReloc + 0x10000;
    else
        R(rt) = (int)(imm << 16);
}

// ROR 0x002104ec

static void seqOpSlt(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = R(rs) < R(rt) ? 1 : 0;
}

// ROR 0x00210534

static void seqOpSlti(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = R(rs) < (short)imm ? 1 : 0;
}

// ROR 0x00210578

static void seqOpSltu(u_int rs, u_int rt, u_int rd, u_int)
{
    R(rd) = (uintptr_t)R(rs) < (uintptr_t)R(rt) ? 1 : 0;
}

// The immediate is not sign-extended.
// ROR 0x002105c0

static void seqOpSltiu(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = (uintptr_t)R(rs) < (uintptr_t)imm ? 1 : 0;
}

// rs = 0 branches whatever rt holds.
// ROR 0x002105fc

static void seqOpBeq(u_int rs, u_int rt, u_int imm, u_int)
{
    if (rs == 0) {
        seqVmBranch(imm);
        return;
    }
    if (R(rs) == R(rt))
        seqVmBranch(imm);
}

// ROR 0x002106a0

static void seqOpBgtz(u_int rs, u_int, u_int imm, u_int)
{
    if (R(rs) > 0)
        seqVmBranch(imm);
}

// ROR 0x002106f4

static void seqOpBlez(u_int rs, u_int, u_int imm, u_int)
{
    if (R(rs) < 1)
        seqVmBranch(imm);
}

// ROR 0x00210748

static void seqOpBne(u_int rs, u_int rt, u_int imm, u_int)
{
    if (R(rs) != R(rt))
        seqVmBranch(imm);
}

// bltz (rt 0) and bgez (rt 1); the others do nothing.
// ROR 0x002107ac

static void seqOpRegimm(u_int rs, u_int rt, u_int imm, u_int)
{
    if (rt == 1) {
        if (R(rs) < 0)
            return;
    } else {
        if (rt != 0)
            return;
        if (R(rs) >= 0)
            return;
    }
    seqVmBranch(imm);
}

// ROR 0x0021082c

static void seqOpJ(u_int, u_int, u_int, u_int word)
{
    // PC: the ROM ors in pc & 0xf8000000, 0 for the EE's addresses.
    g_seqBranch = (word & 0x3ffffff) << 2 | 0x80000000;
}

// The target has the 16 bits of the immediate only.
// ROR 0x00210864

static void seqOpJal(u_int, u_int, u_int imm, u_int)
{
    // PC: the ROM ors in pc & 0xf0000000, 0 for the EE's addresses.
    g_seqBranch = (imm & 0xffffff) << 2 | 0x80000000;
    R(31) = seqVmLink();
}

// ROR 0x002108b8

static void seqOpJalr(u_int rs, u_int, u_int rd, u_int)
{
    R(rd) = seqVmLink();
    g_seqBranch = (u_int)R(rs) | 0x80000000;
}

// ROR 0x00210908

static void seqOpJr(u_int rs, u_int, u_int, u_int)
{
    g_seqBranch = (u_int)R(rs) | 0x80000000;
}

// lb: a word at offset * 4.
// ROR 0x0021092c

static void seqOpLb(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = seqVmLoadWord(R(rs) + (short)imm * 4);
}

// lwl: the word << 8 with the base register's low byte.
// ROR 0x00210960

static void seqOpLwl(u_int rs, u_int rt, u_int imm, u_int)
{
    u_int base = (u_int)R(rs);
    R(rt) = (int)((u_int)seqVmRawLoad(R(rs) + (short)imm) << 8 | (base & 0xff));
}

// ROR 0x002109a0

static void seqOpLh(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = seqVmLoadWord(R(rs) + (short)imm);
}

// lbu: a word at offset * 4.
// ROR 0x002109d4

static void seqOpLbu(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = seqVmLoadWord(R(rs) + (short)imm * 4);
}

// ROR 0x00210a08

static void seqOpLw(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = seqVmLoadWord(R(rs) + (short)imm);
}

// ROR 0x00210a3c

static void seqOpLhu(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = seqVmLoadWord(R(rs) + (short)imm);
}

// ROR 0x00210a70

static void seqOpLwr(u_int rs, u_int rt, u_int imm, u_int)
{
    R(rt) = seqVmLoadWord(R(rs) + (short)imm);
}

// ROR 0x00210aa4

static void seqOpSb(u_int rs, u_int rt, u_int imm, u_int)
{
    seqVmStoreWord(R(rs) + (short)imm, R(rt));
}

// ROR 0x00210ad8

static void seqOpSh(u_int rs, u_int rt, u_int imm, u_int)
{
    seqVmStoreWord(R(rs) + (short)imm, R(rt));
}

// swl: the register >> 8 under the word's top byte.
// ROR 0x00210b0c

static void seqOpSwl(u_int rs, u_int rt, u_int imm, u_int)
{
    intptr_t addr = R(rs) + (short)imm;
    seqVmRawStore(addr, (int)((u_int)R(rt) >> 8 | ((u_int)seqVmRawLoad(addr) & 0xff000000)));
}

// swr: the register << 24 over the low three bytes of the word at +1.
// ROR 0x00210b54

static void seqOpSwr(u_int rs, u_int rt, u_int imm, u_int)
{
    intptr_t addr = R(rs) + (short)imm + 1;
    seqVmRawStore(addr, (int)((u_int)R(rt) << 24 | ((u_int)seqVmRawLoad(addr) & 0xffffff)));
}

// ROR 0x00210ba4

static void seqOpSw(u_int rs, u_int rt, u_int imm, u_int)
{
    seqVmStoreWord(R(rs) + (short)imm, R(rt));
}

// ROR 0x00210bd8

static void seqOpMfhi(u_int, u_int, u_int rd, u_int)
{
    R(rd) = g_seqHi;
}

// ROR 0x00210bf8

static void seqOpMflo(u_int, u_int, u_int rd, u_int)
{
    R(rd) = g_seqLo;
}

// mthi / mtlo read rd.
// ROR 0x00210c18

static void seqOpMthi(u_int, u_int, u_int rd, u_int)
{
    g_seqHi = (int)R(rd);
}

// ROR 0x00210c34

static void seqOpMtlo(u_int, u_int, u_int rd, u_int)
{
    g_seqLo = (int)R(rd);
}

// ROR 0x00210c50

static void seqOpSyscall(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210c58

static void seqOpBreak(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210c60

static void seqOpLl(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210c68

static void seqOpLwc1(u_int rs, u_int rt, u_int imm, u_int)
{
    g_seqFpr[rt] = (u_int)seqVmRawLoad(R(rs) + (short)imm);
}

// ROR 0x00210ca0

static void seqOpLwc2(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210ca8

static void seqOpPref(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210cb0

static void seqOpSc(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210cb8

static void seqOpSwc1(u_int rs, u_int rt, u_int imm, u_int)
{
    seqVmRawStore(R(rs) + (short)imm, (int)g_seqFpr[rt]);
}

// ROR 0x00210cec

static void seqOpSwc2(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210cf4

static void seqOpOp3b(u_int, u_int, u_int, u_int)
{
}

// Opcode 0x3d loads an FPU register as lwc1 does.
// ROR 0x00210cfc

static void seqOpSdc1(u_int rs, u_int rt, u_int imm, u_int)
{
    g_seqFpr[rt] = (u_int)seqVmRawLoad(R(rs) + (short)imm);
}

// ROR 0x00210d34

static void seqOpCop0(u_int, u_int, u_int, u_int)
{
}

/* data 0x2d9b08 (defined after the handlers) */
extern const SeqOp g_seqCop1Ops[64];

// The FPU functions (fmt S and W through one table), mfc1 / mtc1, bc1f /
// bc1t, and 0x45800000 which ends the thread (a branch to 0).
// ROR 0x00210d3c

static void seqOpCop1(u_int, u_int, u_int imm, u_int word)
{
    if ((word & 0xfe000000) == 0x46000000) {
        SeqOp op = g_seqCop1Ops[word & 0x3f];
        // PC: the ROM calls the empty entries (address 0).
        if (op != NULL)
            op(word >> 11 & 0x1f, word >> 16 & 0x1f, word >> 6 & 0x1f, word >> 21 & 0xf);
        return;
    }
    u_int kind = word & 0xffe00000;
    if (kind == 0x44800000) {
        g_seqFpr[word >> 11 & 0x1f] = (u_int)R(word >> 16 & 0x1f);
    } else if (kind == 0x44000000) {
        R(word >> 16 & 0x1f) = (int)g_seqFpr[word >> 11 & 0x1f];
    } else if (kind == 0x45000000) {
        if (word & 0x10000) {
            if (g_seqFpuCond == 0)
                return;
        } else {
            if (g_seqFpuCond != 0)
                return;
        }
        seqVmBranch(imm);
    } else if (kind == 0x45800000) {
        g_seqBranch = 0x80000000;
    }
}

// ROR 0x00210ebc

static void seqOpCop2(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210ec4

static void seqOpCop3(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00210ecc

static void seqOpFAdd(u_int fs, u_int ft, u_int fd, u_int)
{
    seqVmSetF(fd, seqVmGetF(fs) + seqVmGetF(ft));
}

// ROR 0x00210f00

static void seqOpFSub(u_int fs, u_int ft, u_int fd, u_int)
{
    seqVmSetF(fd, seqVmGetF(fs) - seqVmGetF(ft));
}

// ROR 0x00210f34

static void seqOpFMul(u_int fs, u_int ft, u_int fd, u_int)
{
    seqVmSetF(fd, seqVmGetF(fs) * seqVmGetF(ft));
}

// ROR 0x00210f68

static void seqOpFDiv(u_int fs, u_int ft, u_int fd, u_int)
{
    seqVmSetF(fd, seqVmGetF(fs) / seqVmGetF(ft));
}

// ROR 0x00210fa4

static void seqOpFLt(u_int fs, u_int ft, u_int, u_int)
{
    g_seqFpuCond = seqVmGetF(fs) < seqVmGetF(ft) ? 1 : 0;
}

// ROR 0x00210fe4

static void seqOpFEq(u_int fs, u_int ft, u_int, u_int)
{
    g_seqFpuCond = seqVmGetF(fs) == seqVmGetF(ft) ? 1 : 0;
}

// ROR 0x00211024

static void seqOpFLe(u_int fs, u_int ft, u_int, u_int)
{
    g_seqFpuCond = seqVmGetF(fs) <= seqVmGetF(ft) ? 1 : 0;
}

// The unary operations read ft.
// ROR 0x00211064
// callcheck: fptodp, dpcmp, dpsub, dptofp (the EE's software double compare and negate)

static void seqOpFAbs(u_int, u_int ft, u_int fd, u_int)
{
    double value = seqVmGetF(ft);
    if (value < 0)
        value = 0 - value;
    seqVmSetF(fd, (float)value);
}

// ROR 0x002110e4

static void seqOpFMov(u_int, u_int ft, u_int fd, u_int)
{
    g_seqFpr[fd] = g_seqFpr[ft];
}

// ROR 0x00211108

static void seqOpFNeg(u_int, u_int ft, u_int fd, u_int)
{
    seqVmSetF(fd, -seqVmGetF(ft));
}

// The word in fs to a float.
// ROR 0x00211130

static void seqOpFCvtS(u_int fs, u_int, u_int fd, u_int)
{
    seqVmSetF(fd, (float)(int)g_seqFpr[fs]);
}

// ROR 0x00211158

static void seqOpFCvtD(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00211160

static void seqOpFCvtW(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00211168

static void seqOpFCf(u_int, u_int, u_int, u_int)
{
    g_seqFpuCond = 0;
}

// ROR 0x00211174

static void seqOpFCun(u_int, u_int, u_int, u_int)
{
}

// ROR 0x0021117c

static void seqOpFCeq(u_int fs, u_int ft, u_int, u_int)
{
    g_seqFpuCond = seqVmGetF(fs) == seqVmGetF(ft) ? 1 : 0;
}

// ROR 0x002111bc

static void seqOpFCueq(u_int, u_int, u_int, u_int)
{
}

// ROR 0x002111c4

static void seqOpFColt(u_int, u_int, u_int, u_int)
{
}

// ROR 0x002111cc

static void seqOpFCult(u_int, u_int, u_int, u_int)
{
}

// ROR 0x002111d4

static void seqOpFCole(u_int, u_int, u_int, u_int)
{
}

// ROR 0x002111dc

static void seqOpFCule(u_int, u_int, u_int, u_int)
{
}

// ROR 0x002111e4

static void seqOpFCsf(u_int, u_int, u_int, u_int)
{
    g_seqFpuCond = 0;
}

// ROR 0x002111f0

static void seqOpFCngle(u_int, u_int, u_int, u_int)
{
}

// ROR 0x002111f8

static void seqOpFCseq(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00211200

static void seqOpFCngl(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00211208

static void seqOpFClt(u_int fs, u_int ft, u_int, u_int)
{
    g_seqFpuCond = seqVmGetF(fs) < seqVmGetF(ft) ? 1 : 0;
}

// ROR 0x00211248

static void seqOpFCnge(u_int, u_int, u_int, u_int)
{
}

// ROR 0x00211250

static void seqOpFCle(u_int fs, u_int ft, u_int, u_int)
{
    g_seqFpuCond = seqVmGetF(fs) <= seqVmGetF(ft) ? 1 : 0;
}

// ROR 0x00211290

static void seqOpFCngt(u_int, u_int, u_int, u_int)
{
}

// In no table.
// ROR 0x00211298

void seqOpUnused211298(void)
{
}

// Truncates ft to a word (cvt.w.s: towards zero on the EE).
// ROR 0x002112a0

static void seqOpFTrunc(u_int, u_int ft, u_int fd, u_int)
{
    g_seqFpr[fd] = (u_int)(int)seqVmGetF(ft);
}

// Every MMI word is rd = rs * rt.
// ROR 0x002112d8

static void seqOpMmi(u_int, u_int, u_int, u_int word)
{
    R(word >> 11 & 0x1f) = (int)((u_int)R(word >> 21 & 0x1f) * (u_int)R(word >> 16 & 0x1f));
}

/* data 0x2d9958: by opcode (0 is SPECIAL, decoded by seqVmRun) */
static const SeqOp s_seqPrimaryOps[64] = {
    /* 00 */ NULL,        seqOpRegimm, seqOpJ,      seqOpJal,
    /* 04 */ seqOpBeq,    seqOpBne,    seqOpBlez,   seqOpBgtz,
    /* 08 */ seqOpAddi,   seqOpAddiu,  seqOpSlti,   seqOpSltiu,
    /* 0c */ seqOpAndi,   seqOpOri,    seqOpXori,   seqOpLui,
    /* 10 */ seqOpCop0,   seqOpCop1,   seqOpCop2,   seqOpCop3,
    /* 14 */ NULL,        NULL,        NULL,        NULL,
    /* 18 */ NULL,        NULL,        NULL,        NULL,
    /* 1c */ seqOpMmi,    NULL,        NULL,        NULL,
    /* 20 */ seqOpLb,     seqOpLh,     seqOpLwl,    seqOpLw,
    /* 24 */ seqOpLbu,    seqOpLhu,    seqOpLwr,    NULL,
    /* 28 */ seqOpSb,     seqOpSh,     seqOpSwl,    seqOpSw,
    /* 2c */ NULL,        NULL,        seqOpSwr,    NULL,
    /* 30 */ seqOpLl,     seqOpLwc1,   seqOpLwc2,   seqOpPref,
    /* 34 */ NULL,        NULL,        NULL,        NULL,
    /* 38 */ seqOpSc,     seqOpSwc1,   seqOpSwc2,   seqOpOp3b,
    /* 3c */ NULL,        seqOpSdc1,   NULL,        NULL,
};

/* data 0x2d9a58: SPECIAL by funct. The table is 0x2c entries long: the
 * COP1 table follows it, so funct 0x2c..0x3f runs COP1 entries 0..0x13
 * with the SPECIAL fields (seqVmRun keeps that). */
static const SeqOp s_seqSpecialOps[0x2c] = {
    /* 00 */ seqOpSll,    NULL,        seqOpSrl,    seqOpSra,
    /* 04 */ seqOpSllv,   NULL,        seqOpSrlv,   seqOpSrav,
    /* 08 */ seqOpJr,     seqOpJalr,   NULL,        NULL,
    /* 0c */ seqOpSyscall, seqOpBreak, NULL,        NULL,
    /* 10 */ seqOpMfhi,   seqOpMthi,   seqOpMflo,   seqOpMtlo,
    /* 14 */ NULL,        NULL,        NULL,        NULL,
    /* 18 */ seqOpMult,   seqOpMultu,  seqOpDiv,    seqOpDivu,
    /* 1c */ NULL,        NULL,        NULL,        NULL,
    /* 20 */ seqOpAdd,    seqOpAddu,   seqOpSub,    seqOpSubu,
    /* 24 */ seqOpAnd,    seqOpOr,     seqOpXor,    seqOpNor,
    /* 28 */ NULL,        NULL,        seqOpSlt,    seqOpSltu,
};

/* data 0x2d9b08: COP1 by funct */
const SeqOp g_seqCop1Ops[64] = {
    /* 00 */ seqOpFAdd,   seqOpFSub,   seqOpFMul,   seqOpFDiv,
    /* 04 */ NULL,        seqOpFAbs,   seqOpFMov,   seqOpFNeg,
    /* 08 */ NULL,        NULL,        NULL,        NULL,
    /* 0c */ NULL,        seqOpFTrunc, NULL,        NULL,
    /* 10 */ NULL,        NULL,        seqOpFEq,    NULL,
    /* 14 */ NULL,        NULL,        NULL,        NULL,
    /* 18 */ NULL,        NULL,        NULL,        NULL,
    /* 1c */ seqOpFLt,    NULL,        seqOpFLe,    NULL,
    /* 20 */ seqOpFCvtS,  seqOpFCvtD,  NULL,        NULL,
    /* 24 */ seqOpFCvtW,  NULL,        NULL,        NULL,
    /* 28 */ NULL,        NULL,        NULL,        NULL,
    /* 2c */ NULL,        NULL,        NULL,        NULL,
    /* 30 */ seqOpFCf,    seqOpFCun,   seqOpFCeq,   seqOpFCueq,
    /* 34 */ seqOpFColt,  seqOpFCult,  seqOpFCole,  seqOpFCule,
    /* 38 */ seqOpFCsf,   seqOpFCngle, seqOpFCseq,  seqOpFCngl,
    /* 3c */ seqOpFClt,   seqOpFCnge,  seqOpFCle,   seqOpFCngt,
};

// ──────────────────────────────────────────────────────────────────────
// The running thread waits n frames. Called by the wait natives each
// frame: the first call starts the count, the call that brings it to 0
// returns 0.
// ROR 0x00211318

int Sequence::waitFrames(int n)
{
    SeqThread &thread = g_seqThread->m_data;
    if (thread.m_wait != -1 && (m_flags & 0x400) == 0) {
        thread.m_wait--;
        if (thread.m_wait == 0) {
            thread.m_wait = -1;
            return 0;
        }
        return 1;
    }
    m_flags &= ~0x400u;
    thread.m_wait = n;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002113a4

void Sequence::jumpTo(intptr_t target)
{
    g_seqJump = target + g_seqImageCode - m_base[g_seqImage];
}

// A thread starts at the first "addiu sp, sp, ..." (its prologue) among
// the four words at pc and the four before it, else at pc (inline in
// startThread and execThread).
static intptr_t seqFindEntry(intptr_t pc)
{
    static const int s_order[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };
    const u_int *word = (const u_int *)pc;
    for (int i = 0; i < 8; i++) {
        if ((word[s_order[i]] & 0xffff0000) == 0x27bd0000)
            return (intptr_t)&word[s_order[i]];
    }
    return pc;
}

// A script address of image idx (as m_base counts it) as a host address
// in its code (inline in every thread starter).
// PC: the script's base (native 018 case 0x1e) is a host pointer, and an
// entry the script stored in a 32-bit record word (GamAreaTrg::m_entry) has
// lost its top half; the offset into the image is taken in 32 bits, as
// the EE's addresses are, so both forms give the same pc.
static intptr_t seqImagePc(const Sequence *seq, u_int idx, intptr_t entry)
{
    return g_seqImageState[idx].m_code + (int)((u_int)entry - (u_int)seq->m_base[idx]);
}

// ──────────────────────────────────────────────────────────────────────
// entry is an address in id's image (as m_base counts it). The caller's
// registers are kept around the run.
// ROR 0x002113dc

void Sequence::execThread(u_int id, intptr_t entry, int arg, u_int flags, int resume,
                          int area, const char *name)
{
    u_int image = id >> 30;
    intptr_t pc = seqImagePc(this, image, entry);
    if (hasThread(id, arg) != 0)
        return;

    intptr_t regs[32];
    memcpy(regs, g_seqGpr, sizeof(regs));

    m_single.m_flags = flags;
    m_single.m_id = id;
    m_single.m_pc = seqFindEntry(pc);
    m_single.m_wait = -1;
    m_single.m_area = area;
    m_single.m_24 = 1;
    m_single.m_arg = arg;
    m_single.m_stack = 0;
    strncpy(m_single.m_name, name, 8);
    if (resume != 0)
        m_single.m_resume = seqVmAllocCallSave();
    seqVmSelectImage(image);
    if (resume != 0)
        seqVmResume(m_single.m_resume);

    m_flags |= 0x100;
    intptr_t next = seqVmRun(m_single.m_pc);
    if (next == 0) {
        releaseThread(&m_single);
    } else {
        m_single.m_pc = next;
        m_start.add(m_single);
    }
    m_flags &= ~0x100u;

    seqVmSelectImage(g_seqImage);
    memcpy(g_seqGpr, regs, sizeof(regs));
}

// ──────────────────────────────────────────────────────────────────────
// execThread's sibling for callEntryNow: no registers kept (the caller
// keeps them), m_area 0, and image 2 selected at the end.
// ROR 0x00211830

void Sequence::execImmediate(u_int id, intptr_t entry, int arg, u_int flags, int callSave)
{
    u_int image = id >> 30;
    intptr_t pc = seqImagePc(this, image, entry);
    if (hasThread(id, arg) != 0)
        return;

    m_single.m_flags = flags;
    m_single.m_id = id;
    m_single.m_pc = seqFindEntry(pc);
    m_single.m_wait = -1;
    m_single.m_24 = 1;
    m_single.m_arg = arg;
    m_single.m_area = 0;
    m_single.m_stack = 0;
    strncpy(m_single.m_name, "imdy", 8);
    if (callSave != 0)
        m_single.m_resume = seqVmAllocCallSave();
    seqVmSelectImage(image);
    if (callSave != 0)
        seqVmResume(m_single.m_resume);

    m_flags |= 0x100;
    intptr_t next = seqVmRun(m_single.m_pc);
    if (next == 0) {
        releaseThread(&m_single);
    } else {
        m_single.m_pc = next;
        m_start.add(m_single);
    }
    m_flags &= ~0x100u;
    seqVmSelectImage(2);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00211be4

void Sequence::queueSingle(u_int id, intptr_t entry, int arg, u_int flags)
{
    u_int image = id >> 30;
    intptr_t pc = seqImagePc(this, image, entry);
    if (hasThread(id, arg) != 0)
        return;

    // PC: the ROM leaves m_resume and m_name as stack garbage.
    SeqThread thread;
    memset(&thread, 0, sizeof(thread));
    thread.m_pc = seqFindEntry(pc);
    thread.m_24 = 1;
    thread.m_wait = -1;
    thread.m_arg = arg;
    thread.m_area = 0;
    thread.m_stack = 0;
    thread.m_flags = flags;
    thread.m_id = id;
    // The ROM copies the name into m_single, as startThread does.
    strncpy(m_single.m_name, "single", 8);
    m_start.add(thread);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00211e84

void Sequence::startThread(u_int id, intptr_t entry)
{
    // PC: the ROM leaves the fields it does not set as stack garbage.
    SeqThread thread;
    memset(&thread, 0, sizeof(thread));
    thread.m_flags = 0;
    thread.m_id = id;
    thread.m_pc = seqFindEntry(entry + 0xa4);
    thread.m_wait = -1;
    thread.m_area = 0;
    thread.m_24 = 1;
    thread.m_arg = 0;
    thread.m_stack = 0;
    // The ROM copies the name into m_single, not into the new thread.
    strncpy(m_single.m_name, "mst", 8);
    m_start.add(thread);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002120ac

void Sequence::queueThread(u_int id, intptr_t entry, int arg, u_int flags, int callSave,
                           const char *name)
{
    u_int image = id >> 30;
    // PC: the ROM leaves m_resume (without a call save) and _18 as stack
    // garbage.
    SeqThread thread;
    memset(&thread, 0, sizeof(thread));
    thread.m_pc = seqFindEntry(seqImagePc(this, image, entry));
    thread.m_wait = -1;
    thread.m_24 = 1;
    thread.m_area = 0;
    thread.m_stack = 0;
    thread.m_flags = flags;
    thread.m_id = id;
    thread.m_arg = arg;
    strncpy(thread.m_name, name, 8);
    if (callSave != 0) {
        thread.m_flags |= 0x80;
        thread.m_resume = seqVmAllocCallSave();
    }
    m_start.add(thread);
}

// ──────────────────────────────────────────────────────────────────────
// Unreferenced. m_area holds the record's address (the GamArea trigger
// functions compare it with a rect's id word).
// ROR 0x00212330

void Sequence::queueTrigger(const SeqTrgRec *rec, int arg, u_int flags)
{
    u_int image = rec->m_image >> 30;
    SeqThread thread;
    memset(&thread, 0, sizeof(thread));
    thread.m_id = rec->m_id;
    thread.m_pc = seqFindEntry(seqImagePc(this, image, rec->m_entry));
    thread.m_24 = 1;
    thread.m_wait = -1;
    thread.m_stack = 0;
    thread.m_flags = flags;
    // PC: a host pointer does not fit the 32-bit id; nothing calls this.
    thread.m_area = (int)(intptr_t)rec;
    thread.m_arg = arg;
    strncpy(thread.m_name, "trg", 8);
    m_start.add(thread);
}

// ──────────────────────────────────────────────────────────────────────
// Unreferenced.
// ROR 0x00212598

void Sequence::queueTriggerThread(u_int area, u_int id, intptr_t entry, int arg, u_int flags)
{
    SeqThread thread;
    memset(&thread, 0, sizeof(thread));
    thread.m_pc = seqFindEntry(seqImagePc(this, area >> 30, entry));
    thread.m_24 = 1;
    thread.m_wait = -1;
    thread.m_stack = 0;
    thread.m_flags = flags;
    thread.m_id = id;
    thread.m_area = (int)area;
    thread.m_arg = arg;
    strncpy(thread.m_name, "trg", 8);
    m_start.add(thread);
}

// ──────────────────────────────────────────────────────────────────────
// The stage image's entries (id >> 30 == 1) go back to the free list.
// ROR 0x002127f4

void Sequence::freeStageEntries()
{
    SeqList<SeqEntry> &used = m_entries.m_used;
    SeqNode<SeqEntry> *it = used.begin();
    while (it != used.end()) {
        SeqNode<SeqEntry> *next = it->m_next;
        if ((it->m_data.m_id >> 30) == 1)
            m_entries.remove(it);
        it = next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00212920

void Sequence::addEntry(u_int id, intptr_t value)
{
    // PC: the ROM's last 8 bytes are stack garbage.
    SeqEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.m_id = id;
    entry.m_value = value;
    m_entries.add(entry);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002129b8

void Sequence::callEntry(u_int id, int arg, int callSave)
{
    SeqList<SeqEntry> &used = m_entries.m_used;
    for (SeqNode<SeqEntry> *it = used.begin(); it != used.end(); it = it->m_next) {
        if (it->m_data.m_id == id) {
            queueThread(id, it->m_data.m_value, arg, 2, callSave, "none");
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The thread runs inside the native call: the caller's registers and gp
// area are kept around it, and SEQ_RET_RETURN continues the caller after
// the call (g_seqReturnPc).
// ROR 0x00212a5c

void Sequence::callEntryNow(u_int id, int arg, int callSave)
{
    SeqList<SeqEntry> &used = m_entries.m_used;
    SeqNode<SeqEntry> *it = used.begin();
    while (true) {
        if (it == used.end())
            return;
        if (it->m_data.m_id == id)
            break;
        it = it->m_next;
    }

    g_seqReturnPc = g_seqPc;
    g_seqCallNowKind = 2;
    intptr_t regs[32];
    u_char area[SEQ_GP_AREA];
    memcpy(regs, g_seqGpr, sizeof(regs));
    seqVmCopyArea(area, (const u_char *)R(28));

    execImmediate(id, it->m_data.m_value, arg, 2, callSave);

    memcpy(g_seqGpr, regs, sizeof(regs));
    seqVmCopyArea((u_char *)R(28), area);
}

// ──────────────────────────────────────────────────────────────────────
// The thread is copied to m_ended (reapThreads removes it from its pool
// at the next exec).
// ROR 0x00212d08

void Sequence::endThread(u_int id, int arg)
{
    SeqNode<SeqThread> *node = NULL;
    if (findThread(id, &node, arg) != 0)
        m_ended.add(node->m_data);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00212e64

void Sequence::endThreadOf(const char *name)
{
    SeqNode<SeqThread> *node = NULL;
    if (findThreadNamed(name, &node) != 0)
        m_ended.add(node->m_data);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00212fbc

void Sequence::endThreadsWithArg(int arg)
{
    for (int idx = 0; idx < 3; idx++) {
        SeqList<SeqThread> &used = m_threads[idx]->m_used;
        for (SeqNode<SeqThread> *it = used.begin(); it != used.end(); it = it->m_next) {
            if (it->m_data.m_arg == arg)
                m_ended.add(it->m_data);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213448

void seqUnused213448(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// Images 1 and 2: every thread of the pool is copied to the ended list.
// Image 0's loop has no body in the ROM: it copies the list's end (the
// sentinel) once.
// ROR 0x00213450
// callcheck: memset (the image clears below)

void Sequence::killThreads(int idx)
{
    static const int s_size[3] = { 0x20000, 0x18000, 0x30000 };

    if (idx == 1 || idx == 2) {
        SeqList<SeqThread> &used = m_threads[idx]->m_used;
        for (SeqNode<SeqThread> *it = used.begin(); it != used.end(); it = it->m_next)
            m_ended.add(it->m_data);
    } else if (idx == 0) {
        SeqList<SeqThread> &used = m_threads[0]->m_used;
        SeqNode<SeqThread> *it = used.begin();
        while (it != used.end())
            it = it->m_next;
        m_ended.add(it->m_data);
    }
    reapThreads();
    if (idx < 0 || idx > 2)
        return;

    m_threads[idx]->clear();
    memset(m_image[idx], 0, s_size[idx]);
    memset(g_seqImageRegs[idx], 0, sizeof(g_seqImageRegs[idx]));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213adc

int Sequence::hasThreadIn(u_int id, SeqNode<SeqThread> *it, SeqNode<SeqThread> *end, int arg)
{
    for (; it != end; it = it->m_next) {
        if (it->m_data.m_id == id && it->m_data.m_arg == arg)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213b28

int Sequence::findThreadIn(u_int id, SeqNode<SeqThread> *it, SeqNode<SeqThread> *end,
                           SeqNode<SeqThread> **node, int arg)
{
    for (; it != end; it = it->m_next) {
        if (it->m_data.m_id == id && it->m_data.m_arg == arg) {
            *node = it;
            return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213b7c

int Sequence::findThreadNamedIn(const char *name, SeqNode<SeqThread> *it,
                                SeqNode<SeqThread> *end, SeqNode<SeqThread> **node)
{
    for (; it != end; it = it->m_next) {
        if (strcmp(it->m_data.m_name, name) == 0) {
            *node = it;
            return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213bfc

int Sequence::hasThreadNamedIn(const char *name, SeqNode<SeqThread> *it, SeqNode<SeqThread> *end)
{
    for (; it != end; it = it->m_next) {
        if (strcmp(it->m_data.m_name, name) == 0)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213c6c

int Sequence::findThread(u_int id, SeqNode<SeqThread> **node, int arg)
{
    u_int image = id >> 30;
    if (image > 2)
        return 0;
    SeqList<SeqThread> &used = m_threads[image]->m_used;
    return findThreadIn(id, used.begin(), used.end(), node, arg);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213d00

int Sequence::hasThread(u_int id, int arg)
{
    u_int image = id >> 30;
    if (image > 2)
        return 0;
    SeqList<SeqThread> &used = m_threads[image]->m_used;
    return hasThreadIn(id, used.begin(), used.end(), arg);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213d90

int Sequence::findThreadNamed(const char *name, SeqNode<SeqThread> **node)
{
    for (int idx = 0; idx < 3; idx++) {
        SeqList<SeqThread> &used = m_threads[idx]->m_used;
        if (findThreadNamedIn(name, used.begin(), used.end(), node) == 1)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00213e78

int Sequence::hasThreadNamed(const char *name)
{
    for (int idx = 0; idx < 3; idx++) {
        SeqList<SeqThread> &used = m_threads[idx]->m_used;
        if (hasThreadNamedIn(name, used.begin(), used.end()) == 1)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The image header: +0xc, +0x14, +0x18 sizes (the gp area follows them
// + 0x8000), +0x10 the gp area's link address, +0x1c, +0x20, +0x24. Two
// words before the data's end get the global variables and the bitset.
// ROR 0x00213f48

void Sequence::setupImage(u_char *image, int idx)
{
    g_seqImageCode = (intptr_t)(image + 0x30);
    int size0c = seqVmRawLoad((intptr_t)(image + 0x0c));
    int size14 = seqVmRawLoad((intptr_t)(image + 0x14));
    int size18 = seqVmRawLoad((intptr_t)(image + 0x18));
    intptr_t end = (intptr_t)(image + size14 + size18);
    seqVmStoreWord(end - 0x20, (intptr_t)m_global);
    seqVmStoreWord(end - 0x10, (intptr_t)m_bits);
    g_seqDataLink = seqVmRawLoad((intptr_t)(image + 0x10));
    g_seqData = (intptr_t)(image + size14);
    g_seqData2 = g_seqData + seqVmRawLoad((intptr_t)(image + 0x1c)) + 8;
    g_seqHdr20 = seqVmRawLoad((intptr_t)(image + 0x20));
    g_seqHdr24 = seqVmRawLoad((intptr_t)(image + 0x24));
    intptr_t area = (intptr_t)(image + size0c + size18 + size14);
    R(29) = area + 0x8780;
    R(28) = area + 0x8000;
    memcpy(g_seqImageRegs[idx], g_seqGpr, sizeof(g_seqImageRegs[idx]));
    seqVmCopyArea(g_seqImageSave[idx], (const u_char *)R(28));

    SeqVmImage &state = g_seqImageState[idx];
    state.m_code = g_seqImageCode;
    state.m_link = g_seqDataLink;
    g_seqDataReloc = R(28) + (0x450000 - g_seqDataLink);
    state.m_data = g_seqData;
    state.m_data2 = g_seqData2;
    state.m_hdr20 = g_seqHdr20;
    state.m_hdr24 = g_seqHdr24;
    state.m_gp = R(28);
    state.m_sp = R(29);
    state.m_reloc = g_seqDataReloc;
    R(1) = g_seqDataReloc;
}

// ──────────────────────────────────────────────────────────────────────
// Unreferenced: two empty count-down loops.
// ROR 0x002142c0

void seqVmIdleLoop(void)
{
    for (volatile int i = 0x1ff; i >= 0; i -= 0x20) {
    }
    for (volatile int i = 0x1ff; i >= 0; i -= 0x20) {
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00214310

void seqVmSelectImage(int idx)
{
    const SeqVmImage &state = g_seqImageState[idx];
    g_seqImageCode = state.m_code;
    g_seqDataLink = state.m_link;
    g_seqData = state.m_data;
    g_seqData2 = state.m_data2;
    g_seqHdr20 = state.m_hdr20;
    g_seqHdr24 = state.m_hdr24;
    R(28) = state.m_gp;
    R(29) = state.m_sp;
    g_seqDataReloc = state.m_reloc;
    memcpy(g_seqGpr, g_seqImageRegs[idx], sizeof(g_seqGpr));
    R(31) = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002144c4

void seqVmSaveImage(int idx)
{
    (void)idx;
}

// ──────────────────────────────────────────────────────────────────────
// Unreferenced: seqVmLoadThread for the record m_record, which it clears.
// ROR 0x002144cc

void seqVmLoadRecord(Sequence *seq)
{
    SeqThread *record = seq->m_record;
    if (record != NULL) {
        if (record->m_flags & 0x200)
            memcpy(g_seqGpr, record->m_reg, sizeof(g_seqGpr));
        if (seq->m_record->m_stack != 0)
            seqVmCopyArea((u_char *)R(28), (const u_char *)seq->m_record->m_stack);
    }
    seq->m_record = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00214664

void seqVmLoadThread(void)
{
    SeqThread &thread = g_seqThread->m_data;
    if (thread.m_flags & 0x200)
        memcpy(g_seqGpr, thread.m_reg, sizeof(g_seqGpr));
    if (thread.m_stack != 0)
        seqVmCopyArea((u_char *)R(28), (const u_char *)thread.m_stack);
}

// ──────────────────────────────────────────────────────────────────────
// Into m_single while execThread runs (flag 0x100).
// ROR 0x002147e0

void seqVmSaveThread(Sequence *seq)
{
    SeqThread *thread = &g_seqThread->m_data;
    if (seq->m_flags & 0x100)
        thread = &seq->m_single;
    memcpy(thread->m_reg, g_seqGpr, sizeof(thread->m_reg));
    thread->m_flags |= 0x200;
    if (thread->m_stack == 0)
        thread->m_stack = (intptr_t)seqVmAllocStack();
    seqVmCopyArea((u_char *)thread->m_stack, (const u_char *)R(28));
}

// ──────────────────────────────────────────────────────────────────────
// No check for a full block: with all 32 used the search does not end, as
// in the ROM.
// ROR 0x002149a8
// callcheck: memset (the slot clear)

u_char *seqVmAllocStack(void)
{
    u_char *slot = g_seqStack + g_seqStackNext * 0x800;
    g_seqStackUsed[g_seqStackNext] = 1;
    memset(slot, 0, 0x800);
    do {
        g_seqStackNext++;
        if (g_seqStackNext > 0x1f)
            g_seqStackNext = 0;
    } while (g_seqStackUsed[g_seqStackNext] != 0);
    return slot;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00214a94

void seqVmFreeStack(intptr_t stack)
{
    g_seqStackNext = (u_int)(stack - (intptr_t)g_seqStack) >> 11;
    g_seqStackUsed[g_seqStackNext] = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The first free slot (word 0 0xcccccccc) of 8 gets the registers; with
// none free the ROM writes a ninth past the table.
// ROR 0x00214ac4

int seqVmAllocCallSave(void)
{
    int i = 0;
    while (i < 8 && (int)g_seqCallSave[i * 32] != (int)0xcccccccc)
        i++;
    // PC: kept inside the table (the ROM's ninth slot is g_seqStack ...).
    if (i == 8)
        i = 7;
    memcpy(&g_seqCallSave[i * 32], g_seqGpr, sizeof(g_seqGpr));
    return i;
}

// ──────────────────────────────────────────────────────────────────────
// a0..a3 from the slot's a2..t1 (the call's arguments as the native
// function saw them shifted by two), and the slot is free again.
// ROR 0x00214bcc

void seqVmResume(int slot)
{
    const intptr_t *save = &g_seqCallSave[slot * 32];
    R(4) = save[6];
    R(5) = save[7];
    R(6) = save[8];
    R(7) = save[9];
    g_seqCallSave[slot * 32] = (int)0xcccccccc;
}

// ──────────────────────────────────────────────────────────────────────
// The word 0x0c102031 (jal 0x4080c4) ends the thread, as does a branch to
// 0 (jr ra with ra 0, the entry function's return). A native function's
// SEQ_RET_* code decides what happens after its call: the thread state is
// saved and the run returns where the thread continues next frame.
// ROR 0x00214c24
// callcheck: seqOpNop (the word 0)

intptr_t seqVmRun(intptr_t pc)
{
    g_seqBranch = 0;
    g_seqJump = 0;
    for (;;) {
        u_int word = *(const u_int *)pc;
        g_seqPc = pc;
        if (word == 0x0c102031)
            return 0;

        // PC: the ROM calls the empty entries (address 0).
        if ((word >> 26) == 0) {
            if (word == 0) {
                seqOpNop(0, 0, 0, 0);
            } else {
                SeqOp op = (word & 0x3f) < 0x2c ? s_seqSpecialOps[word & 0x3f] : g_seqCop1Ops[(word & 0x3f) - 0x2c];
                if (op != NULL)
                    op(word >> 21 & 0x1f, word >> 16 & 0x1f, word >> 11 & 0x1f, word >> 6 & 0x1f);
            }
        } else {
            SeqOp op = s_seqPrimaryOps[word >> 26];
            if (op != NULL)
                op(word >> 21 & 0x1f, word >> 16 & 0x1f, word & 0xffff, word);
        }

        if (g_seqJump == 0) {
            pc += 4;
        } else {
            if ((uintptr_t)g_seqJump < 0x8160) {
                u_int n = ((u_int)(g_seqJump - 0x144) >> 6) & 0xffff;
                // PC: the ROM reads past the table for n >= 512.
                R(2) = n < SEQ_FUNC_COUNT ? g_seqFuncTable[n](R(4), R(5), R(6), R(7), R(8), R(9)) : 0;
                g_seqBranch = 0;
                // PC: the ROM writes through a NULL g_seqThread too.
                if (g_seqThread != NULL)
                    g_seqThread->m_data.m_24 = 1;
                int ret = (int)R(2);
                if (ret == 0) {
                    pc += 4;
                } else if (ret == (int)SEQ_RET_WAIT) {
                    R(31) = 0;
                    seqVmSaveThread(&g_sequence);
                    return pc - 4;
                } else if (ret == (int)SEQ_RET_YIELD) {
                    R(31) = 0;
                    seqVmSaveThread(&g_sequence);
                    return pc;
                } else if (ret == (int)SEQ_RET_JUMP) {
                    seqVmSaveThread(&g_sequence);
                    return g_seqJump;
                } else if (ret == (int)SEQ_RET_END) {
                    g_seqJump = 0;
                    return 0;
                } else if (ret == (int)SEQ_RET_RETURN) {
                    pc = g_seqReturnPc + 4;
                } else {
                    pc += 4;
                }
            } else {
                pc = g_seqJump + g_seqImageCode - 0x8160;
            }
            g_seqJump = 0;
        }

        if (g_seqBranch != 0) {
            g_seqJump = g_seqBranch & 0x7fffffff;
            if (g_seqJump == 0)
                return 0;
            g_seqBranch = 0;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The save slot index (inline in the four save functions).
static int seqSaveSlot(void)
{
    int slot = g_bss_7a3210;
    if ((u_int)(slot - 1) > 0xc)
        slot = 9;
    return slot;
}

// The save data are the raw words: a host pointer a script stored in the
// globals (seqVmStoreWord) keeps only its low half.
// ROR 0x00214e98

int Sequence::saveGlobals(void *dst)
{
    const u_char *global = (const u_char *)m_global;
    u_char buf[0x544];
    memcpy(buf, global, sizeof(buf));
    memcpy(buf + 0x504, global + SEQ_GLOBAL_SLOT(seqSaveSlot()), 0x40);
    memcpy(dst, buf, sizeof(buf));
    return 0x544;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002150b0

int Sequence::saveBits(void *dst)
{
    const u_char *bits = (const u_char *)m_bits;
    u_char buf[0x400];
    memcpy(buf, bits, sizeof(buf));
    memcpy(buf + 0x360, bits + SEQ_BITS_SLOT(seqSaveSlot()), 0x40);
    memcpy(dst, buf, sizeof(buf));
    return 0x400;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002152b8

int Sequence::loadGlobals(const void *src)
{
    const u_char *in = (const u_char *)src;
    int slot = seqSaveSlot();
    memcpy(m_global, in, 0x544);
    memcpy((u_char *)m_global + SEQ_GLOBAL_SLOT(slot), in + 0x504, 0x40);
    return 0x544;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215430

int Sequence::loadBits(const void *src)
{
    const u_char *in = (const u_char *)src;
    int slot = seqSaveSlot();
    memcpy(m_bits, in, 0x360);
    memcpy((u_char *)m_bits + SEQ_BITS_SLOT(slot), in + 0x360, 0x40);
    return 0x400;
}
