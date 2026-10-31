#ifndef ROSEPAN_CONFIG_H
#define ROSEPAN_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Persisted host settings.
 *
 * These are the port's own knobs -- window, resolution, aspect, shading -- and
 * they deliberately live OUTSIDE the game's save data.  system/mc/dat/save_data.c
 * is a manifest walk whose callback order is literally the byte layout of a save
 * file, so adding a field there would invalidate every existing save; and none
 * of this belongs to the ROM's option screen in any case.
 *
 * The file is `rosepan.ini` in the port's user directory (see io/rosepan_paths.h),
 * next to `memcard/`, and it is plain text meant to be hand-editable: enumerated
 * settings are written as names ("borderless", "16:9", "native") and read back
 * either way, so a numeric value from an older file still loads.
 *
 * Precedence is defaults < file < environment.  The ROSEPAN_* environment
 * variables still win, so a one-off test run never has to disturb -- or
 * accidentally persist into -- the saved settings.
 */

typedef struct RosePan_ConfigRenderer
{
    int   window_mode;       /* ROSEPAN_WINDOW_MODE_*   */
    int   window_width;      /* windowed size only; fullscreen is never stored */
    int   window_height;
    int   render_mode;       /* ROSEPAN_RENDER_RES_*    */
    float render_scale;
    int   upscale_filter;    /* ROSEPAN_RENDER_FILTER_* */
    int   msaa_samples;      /* 1 (off), 2, 4 or 8     */
    int   anisotropy;        /* 1 (off), 2, 4, 8 or 16 */
    int   aspect_mode;       /* ROSEPAN_ASPECT_*        */
    float aspect_ratio;      /* used when aspect_mode is CUSTOM */
    int   lighting_mode;     /* ROSEPAN_LIGHTING_*      */
    int   animated_lighting; /* ROSEPAN_ANIMATED_LIGHTING_* */
    int   shadow_filter;     /* ROSEPAN_SHADOW_FILTER_*  */
    int   extra_presents;    /* frame smoothing: 0..3 additional presents */
    int   interpolate_presents;
    int   interpolate_geometry; /* blend the world too, not just the camera */

    /* Output transform.  See the block comment above
     * RosePan_RendererSetHdrMode() in rendering/rosepan_renderer.h. */
    int   hdr_mode;              /* ROSEPAN_HDR_*                            */
    float hdr_paper_white;       /* nits; 0 follows the OS's SDR white level */
    float hdr_peak;              /* nits; 0 follows the display's headroom   */
    float hdr_expansion;         /* highlight lift, 0..1                     */
    float hdr_expansion_knee;    /* where the lift starts, 0.5..1            */

    /* The display grade.  Applies in SDR as well; all four at 1.0 is identity
     * and costs nothing, because the present pass is then skipped. */
    float grade_brightness;
    float grade_contrast;
    float grade_gamma;
    float grade_saturation;

    /* Screen effects the player may turn off.  See the block above
     * RosePan_RendererSetFilmGrain() in rendering/rosepan_renderer.h. */
    int   film_grain;            /* ROSEPAN_FILM_GRAIN_*                      */

    /* The viewfinder surround.  See the block above
     * RosePan_RendererDrawFinderMask() in rendering/rosepan_renderer.h. */
    int   finder_mask;           /* 0 off, 1 on                              */
    float finder_mask_blur;      /* defocus outside the frame, 0..1          */
    float finder_mask_darken;    /* 0 none, 1 black                          */
    float finder_mask_tint;      /* cast toward crimson, 0..1                */
    float finder_mask_scale;     /* clear rect against the ROM's 640x448     */

    /* Cut-out edge sharpening.  See the block above
     * RosePan_RendererSetAlphaSharpen() in rendering/rosepan_renderer.h. */
    float alpha_sharpen;         /* 0 off .. 1 full                          */
    float alpha_cutoff;          /* hard discard threshold, 0 off            */

    /* Static geometry kept on the GPU.  See the block above
     * RosePan_RendererCreateResidentMesh() in rendering/rosepan_renderer.h. */
    int   resident_meshes;       /* 0 off, 1 on                              */
    /* Model textures resolved once, their GS uploads then skipped.  See
     * RosePan_RendererSetResidentTextures(). */
    int   resident_textures;     /* 0 off, 1 on                              */
} RosePan_ConfigRenderer;

/*
 * Where the game's own files are.
 *
 * This is the one setting that is not a preference: without it the port has
 * nothing to load.  It is here rather than compiled in because on a phone or a
 * console the data cannot be beside the executable and there is no shell to set
 * an environment variable in -- pointing this key at a folder is the whole
 * mechanism.  Empty means "discover it", and what discovery finds is written
 * back, so the file always names the folder actually in use.
 */
typedef struct RosePan_ConfigPaths
{
    char data_folder[1024];
} RosePan_ConfigPaths;

/*
 * The port's changes to how the game runs rather than how it looks.
 *
 * skip_intro: the title's first-launch logos (SCEI, the Atlus notice) and its
 * two movies (CP_LOGO.PSS, the MV00_AD.PSS attract film) are skipped once the
 * memory card check is done, so a launch opens on "Press START button".  Start
 * skips them either way; this is for not having to press it.
 */
typedef struct RosePan_ConfigGame
{
    int skip_intro;          /* 0 off, 1 on */
} RosePan_ConfigGame;

/*
 * The player's volume, on top of the game's own mix: percent, 0 silent .. 100
 * the level the game asks for.  The host mixer applies them (sdk/iop_voice.h,
 * RosePan_VoiceSetUserVolume), so they reach every sound without the game
 * knowing.  `music` is everything the game streams or sequences -- music, but
 * also speech and ambience, which share its streams -- and movie audio;
 * `effects` is the sound effects.
 */
typedef struct RosePan_ConfigAudio
{
    int master;
    int music;
    int effects;
} RosePan_ConfigAudio;

typedef struct RosePan_Config
{
    RosePan_ConfigRenderer renderer;
    RosePan_ConfigPaths    paths;
    RosePan_ConfigGame     game;
    RosePan_ConfigAudio    audio;
} RosePan_Config;

extern RosePan_Config rosepan_config;

/* [game] skip_intro, or ROSEPAN_SKIP_INTRO when that is set (0 or 1).  The
 * environment is read here rather than stored in the struct, so a one-off run
 * never writes its value into the file. */
int RosePan_ConfigSkipIntro(void);

/* Absolute path of the settings file, or NULL if the user directory could not be
 * created. */
const char *RosePan_ConfigPath(void);

/* Read the file over the defaults.  Idempotent -- the paths module calls it as
 * soon as anything asks where the data is, which is well before the renderer
 * exists, and the second call is a no-op.  Safe to call when no file exists:
 * that is the first run, and it leaves the defaults in place and writes the file
 * out so there is something to edit.  Does not touch the renderer; call
 * RosePan_ConfigApply() for that. */
void RosePan_ConfigLoad(void);

/* Record the data folder and persist it.  Called by the paths module with what
 * discovery found, so an unconfigured first run still ends up with the key
 * filled in, and by the settings UI when the player chooses one.  Returns
 * non-zero when the value is stored and on disk -- an unchanged value counts,
 * a refused write does not, which is what lets the UI say which happened. */
int RosePan_ConfigSetDataFolder(const char *folder);

/* Write the current struct out.  Returns non-zero on success. */
int RosePan_ConfigSave(void);

/* Push the struct into the renderer, and pull the renderer's live state back
 * into the struct, respectively.  Capture reads the *windowed* size even while
 * the window is fullscreen, so toggling fullscreen never overwrites the size the
 * window should return to. */
void RosePan_ConfigApply(void);
void RosePan_ConfigCapture(void);

/* Capture then save -- what the host UI calls after a settings change. */
int RosePan_ConfigCaptureAndSave(void);

/* Push rosepan_config.audio into the mixer, clamped to 0..100 first.  Part of
 * RosePan_ConfigApply(); the settings UI calls it alone as a slider moves. */
void RosePan_ConfigApplyAudio(void);

#ifdef __cplusplus
}
#endif

#endif /* ROSEPAN_CONFIG_H */
