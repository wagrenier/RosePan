/* ==========================================================================
 *  mag/magsystem.h
 *
 *  MagSystem (mag/magsystem.cpp, 0x117654..0x1194b4): the game's system
 *  object, one global at bss 0x6e9840 right after the game heap. It owns
 *  the frame: the two pads, the VIF1 DMA channel, the 512x448 double
 *  buffer, the i3d draw context and its packet buffer, the scene at
 *  +0x12540, the V-blank semaphore its callback posts every m_v_interval
 *  V-blanks, and the per-frame update main's loop calls (pads, scene
 *  update and draw, V-blank wait, buffer swap, packet kick). Most other
 *  methods forward to the scene.
 * ======================================================================== */
#ifndef _MAG_MAGSYSTEM_H
#define _MAG_MAGSYSTEM_H

#include <stddef.h>

#include "bed/bedpad.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "sdk/libdma.h"
#include "sdk/sce_gs.h"

/* Frame statistics (asserts name m_info.m_trash_overrun; the i3d packet
 * buffer is the "trash"). EE 0x28, cleared by the constructor. Times are
 * RCNT0 (H-blank) counts from the start of the frame. */
struct MagSystemInfo
{
    int m_00;                   // ROR 0x00: role unknown
    int m_04;                   // ROR 0x04: role unknown
    int m_heap_max_free;        // ROR 0x08: the game heap's largest free block, after the frame
    int m_heap_total_free;      // ROR 0x0c: the game heap's free bytes
    int m_trash_size;           // ROR 0x10: one frame's packet buffer (0x1ae000)
    int m_trash_rest;           // ROR 0x14: what the frame left unused
    int m_trash_overrun;        // ROR 0x18: what the frame wrote past the end (asserted 0)
    int m_t_dma_end;            // ROR 0x1c: when the last frame's DMA was done
    int m_t_cpu;                // ROR 0x20: scene update + draw time
    int m_t_vwait;              // ROR 0x24: when the V-blank wait started
};
static_assert(sizeof(MagSystemInfo) == 0x28, "MagSystemInfo is 0x28 bytes on the EE");
static_assert(offsetof(MagSystemInfo, m_heap_max_free) == 0x08, "m_heap_max_free");
static_assert(offsetof(MagSystemInfo, m_trash_size) == 0x10, "m_trash_size");
static_assert(offsetof(MagSystemInfo, m_trash_overrun) == 0x18, "m_trash_overrun");
static_assert(offsetof(MagSystemInfo, m_t_vwait) == 0x24, "m_t_vwait");

/* A 16-byte slot per pad at +0x97a40; only its first word, the pad, is
 * known (the constructor clears it, initGraph points it at m_pad[i]).
 * layout: host -- holds a pointer. */
struct MagSystemPadRef
{
    BedPad *m_pad;              // ROR 0x00
    int     _04[3];
};

/* EE 0x97a80 (to GamMan at 0x7812c0). layout: host -- holds the scene, the
 * pads, the i3d context and pointers; the EE offsets are in the comments. */
struct MagSystem
{
    MagSystem();                                                         // ROR 0x001176fc
    ~MagSystem();                                                        // ROR 0x00117808

    static int vSyncCallback(int cause);                                 // ROR 0x00117654
    static int vSyncCallbackEach(int cause);                             // ROR 0x001176bc

    bool setLanguage(int lang);                                          // ROR 0x001178b8
    void startVibration(int frames);                                     // ROR 0x00117924
    void setActorLightID(int no, int id);                                // ROR 0x00117974
    void initGraph();                                                    // ROR 0x00117990
    void resetDBuff();                                                   // ROR 0x00117c58
    void resetDBuff384();                                                // ROR 0x00117d14
    void init();                                                         // ROR 0x00117dc8
    void reset();                                                        // ROR 0x00117dfc
    void start();                                                        // ROR 0x00117e18
    void clearScene22cc();                                               // ROR 0x00117ec0
    void term();                                                         // ROR 0x00117ed0
    void setSceneFlags(int a, int b, int c, int d, int e, int f, int g); // ROR 0x00117f80
    void addLight(int type, unsigned int mask, const float *v0, const float *v1); // ROR 0x00117fc4
    void setPC(int no, int code);                                        // ROR 0x001180ec
    void setBwn(int no, int arg);                                        // ROR 0x0011825c
    void flushFrame();                                                   // ROR 0x0011828c
    void update();                                                       // ROR 0x001182dc
    void beginPacket();                                                  // ROR 0x00118440
    void storeImage();                                                   // ROR 0x0011845c
    void endPacket();                                                    // ROR 0x00118640
    void checkPacketRest();                                              // ROR 0x001186f0
    void setDispCircuit2(int on);                                        // ROR 0x0011871c
    void waitVSync();                                                    // ROR 0x00118754
    void kickPacket();                                                   // ROR 0x00118840
    void setCameraMode(int mode);                                        // ROR 0x00118880
    void setLightColor(int type, int idx, int which, float r, float g, float b, float a); // ROR 0x001188a8
    void getLightColor(int type, int idx, int which, float *r, float *g, float *b, float *a); // ROR 0x001188e4
    int  getCameraMode(int *out);                                        // ROR 0x00118960
    void setViewParam(int no, int a, int b, int c, int d, int e);        // ROR 0x00118980
    void drawView(int no, int a, int b, const void *p, int c);           // ROR 0x001189bc
    int  loadScene(int a, int b, int c);                                 // ROR 0x001189e4
    void setActorParent(int theNo, int parentNo, int node);              // ROR 0x00118a18
    void getActorNodeMatrix(int no, I3dMatrixF4x4 *out, int n);          // ROR 0x00118adc
    void getActorNodeMatrix(int no, sceVu0FMATRIX out, int n);           // ROR 0x00118c14
    int  getActorNodeMatrixByName(int no, I3dMatrixF4x4 *out, const char *name); // ROR 0x00118ccc
    int  getActorNodeMatrixByName(int no, sceVu0FMATRIX out, const char *name);  // ROR 0x00118d9c
    int  getActorMotionMove(int no, float t0, float t1, float *move, float *rot,
                            float *pos);                                 // ROR 0x00118e6c
    void addActorTrans(int no, const float *d, float trans_ofs_y, float rot_y_aar); // ROR 0x00118ea4
    void setActorAnim(int no, int a, int b);                             // ROR 0x00118ed8
    int  loadActorAnim(int no, const char *name, int arg);               // ROR 0x00118f04
    void releaseActorMotion(int no, int theId);                          // ROR 0x00118f30
    void setCameraTarget(const float *pos, const float *at);             // ROR 0x00118f64
    int  getPCWordE4();                                                  // ROR 0x00118fdc
    int  getPCStateClass();                                              // ROR 0x00118fec
    int  getBwnState();                                                  // ROR 0x00119090
    void loadModel(int id, int chapter);                                 // ROR 0x00119138
    void loadModelByName(const char *name, int arg, int chapter);        // ROR 0x001191e0
    void setActorTrans(int no, float x, float y, float z);               // ROR 0x00119234
    void setActorRotate(int no, float x, float y, float z);              // ROR 0x001192c8
    void setActorRotate(int no, const float *v);                         // ROR 0x0011939c
    void clearPC();                                                     // ROR 0x0011948c

    int             m_frame;                // ROR 0x00: frames updated; bit 0 picks the draw buffer
    int             m_vib_time;             // ROR 0x04: frames until update stops pad 0's motor
    int             m_vib_on;               // ROR 0x08: the vibration option (utlIsVibrationOn)
    MagSystemInfo   m_info;                 // ROR 0x0c
    const char     *m_store_image_filename; // ROR 0x34: storeImage's BMP name
    int             m_store_request;        // ROR 0x38: set -> endPacket stores the next frame
    BedPad          m_pad[2];               // ROR 0x40
    sceDmaChan     *m_dma;                  // ROR 0x160: channel 1 (VIF1)
    sceGsDBuff      m_dbuff;                // ROR 0x170
    i3dContext      m_i3d;                  // ROR 0x3a0
    void           *m_trash_buf;            // ROR 0x12508: the packet buffer (bss 0x308580)
    MagScene        m_scene;                // ROR 0x12540 (bss 0x6fbd80)
    MagSystemPadRef m_pad_ref[2];           // ROR 0x97a40
    int             m_pc_no;                // ROR 0x97a60: the actor bound to the MagLifePC, < 0 none
    int             m_bwn_no;               // ROR 0x97a64: the actor bound to the MagLifeBwn
    int             m_pc_no_last;           // ROR 0x97a68: the last m_pc_no >= 0
    int             m_language;             // ROR 0x97a6c
    int             m_language_free;        // ROR 0x97a70: 1 until setLanguage takes one

    static MagSystem *m_current_system;     /* sdata 0x296190 */
    static int        m_v_count;            /* sdata 0x296194: V-blanks since the last signal */
    static int        m_v_interval;         /* data 0x296198: V-blanks per frame (2: 30 fps) */
    static int        m_v_sema;             /* data 0x29619c: -1 until start */
};

extern MagSystem g_magSystem;   /* bss 0x6e9840 */

#endif /* _MAG_MAGSYSTEM_H */
