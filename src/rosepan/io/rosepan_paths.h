#ifndef ROSEPAN_PATHS_H
#define ROSEPAN_PATHS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Where everything lives.
 *
 * The port needs three unrelated kinds of file and the only thing they have in
 * common is that none of them is "next to the executable":
 *
 *   USER    what the port writes -- rosepan.ini, memcard/, logs, debug dumps.
 *           SDL_GetPrefPath("Mikompilation", "RosePan"), i.e.
 *             %APPDATA%\Mikompilation\RosePan\            (Windows)
 *             ~/.local/share/Mikompilation/RosePan/       (Linux, XDG)
 *             ~/Library/Application Support/Mikompilation/RosePan/  (macOS)
 *           Deliberately NOT the executable's directory: an installed copy
 *           lives somewhere the player cannot write (Program Files, /usr/bin,
 *           a signed .app bundle), and a build tree is the only layout where
 *           writing beside the binary happens to work.
 *
 *   DATA    the game's own files -- the disc's root (the folder holding DATA), copied file by file.
 *           Set by `[paths] data_folder` in rosepan.ini so it can point
 *           anywhere, which is the only practical answer on a phone or a
 *           console where the data is not next to the binary and cannot be.
 *           Auto-discovered when the key is empty, and the discovered value is
 *           written back so the file always shows where the game is reading
 *           from.
 *
 *   ASSETS  the port's own shipped resources/ -- compiled shader bytecode and
 *           gamecontrollerdb.txt.  Read-only, installed with the executable,
 *           and on Android read straight out of the APK.
 *
 * Every path the game asks for goes through one of the three Resolve calls
 * below.  There is no fourth mechanism and nothing else calls SDL_GetBasePath.
 */

/* The three roots, each with a trailing separator.  Never NULL: a root that
 * cannot be determined falls back to "./" so the caller still has something to
 * open rather than a special case to handle. */
const char *RosePan_PathUserDir(void);
const char *RosePan_PathDataDir(void);
const char *RosePan_PathAssetsDir(void);

/* Which of the four sources the data root came from, for the settings UI to
 * show: "rosepan.ini", "ROSEPAN_DATA_DIR", "found automatically" or "not found".
 * Never NULL; resolves the root if that has not happened yet. */
const char *RosePan_PathDataDirSource(void);

/* Does `dir` hold the game's files -- the disc's root, with DATA/CMNTEX in it?
 * This is the same test discovery uses, exposed so the settings
 * UI can tell the player a folder is wrong while they are choosing it rather
 * than after the next restart.  Returns 1 if it does. */
int RosePan_PathLooksLikeDataDir(const char *dir);

/*
 * Resolve a PS2-style game-data path ("host0:\dir\file;1", "../bin/data/x.pk2")
 * against the data root: strip the device prefix, leading slashes and the
 * ";version" suffix, turn '\' into '/', then look under the root.  A leading
 * "../" is dropped if the direct join misses, and "bin/" is retried as "bin2/",
 * because both extracted-tree layouts are in circulation.  Each candidate is
 * also tried with the ISO ";1" suffix kept on the host file name ("SLUS_214.48;1"),
 * which is how a disc copied file by file arrives.
 *
 * Returns 1 with `out` filled, 0 if the file is not there.
 */
int RosePan_PathResolveData(const char *ps2_name, char *out, size_t out_size);

/* Resolve a shipped resource ("resources/shaders/spirv/mesh.vert.spv").
 * Returns 1 on success, 0 when the file is missing from the install. */
int RosePan_PathResolveAsset(const char *relative, char *out, size_t out_size);

/*
 * Build a path under the user directory.  `relative` is taken as relative: any
 * device prefix, leading separator or ".." component is discarded, so a caller
 * cannot escape the user directory with a name the ROM handed it.  The file need
 * not exist and nothing is created.  Returns 1 on success, 0 only if the name is
 * empty or the result does not fit.
 */
int RosePan_PathResolveUser(const char *relative, char *out, size_t out_size);

/*
 * As above, and additionally create the parent directories so a writer can just
 * open the result.  This is the one to use before writing; the plain resolve is
 * for reading, where creating a directory to then find nothing in it would leave
 * a mirror of every path that missed.
 */
int RosePan_PathPrepareUser(const char *relative, char *out, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif /* ROSEPAN_PATHS_H */
