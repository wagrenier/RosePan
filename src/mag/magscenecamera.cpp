/* ==========================================================================
 *  mag/magscenecamera.cpp
 *
 *  The scene's camera (RoR's "mag/magscenecamera.cpp", 0x13c664..0x13f07c):
 *  MagScene's camera modes over its DogCamera (+0x2ab70) -- free, tied to
 *  an actor, or a map camera standing on one of a sky pack's _c.bin paths
 *  (MagSceneCRec) and looking at the actor. A path camera slides along its
 *  path so that the actor stays between m_cam_dist_min and m_cam_dist_max
 *  (scoreCameraPath); changeCamera sets the path's goal and the move bit
 *  0x10 that updateCamera plays out over 30 frames. A path record's look
 *  letter turns the camera within limits of its own rotation instead of
 *  looking straight at the actor. Saving and restoring the camera brackets
 *  a menu's own camera.
 * ======================================================================== */
#include <math.h>

#include "mag/magscene.h"

#include "utl/utlmath.h"

/* The ROM's pi in this file (0x40490fdc, one ulp over the float of pi). */
static const float CAM_PI = 3.141593f;

/* The actor's position the path camera aims at: its m_trans, 120 up. */
static inline void camTarget(const DogActor &a, float *out)
{
    out[0] = a.m_trans[0];
    out[1] = a.m_trans[1] - 120.0f;
    out[2] = a.m_trans[2];
    out[3] = a.m_trans[3];
}

/* Modes 3..5 with no path in sky slot 0: the camera rides the actor
 * (param[2], if given) 150 up and 500 behind, tilted down 0.17. Inlined
 * three times in setCameraMode. */
static void camFollowActor(MagScene *s, const int *param)
{
    if (param)
        s->m_cam_actor_no = param[2];
    DogCamera &cam = s->m_camera_entity;
    cam.setParent(&s->m_actors[s->m_cam_actor_no]);
    cam.m_trans[1] = -150.0f;
    cam.m_trans[2] = -500.0f;
    cam.m_dirty = 1;
    cam.m_trans[0] = 0.0f;
    cam.setRotate(-0.17f, 0.0f, 0.0f);
}

/* modff with the whole part cut to an int, as the ROM's callers do. */
static inline float camModf(float t, int *ip)
{
    float whole;
    float frac = modff(t, &whole);
    *ip = (int)whole;
    return frac;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013c664

void MagScene::getCameraPathWork(MagCameraPathWork *theWork, MagSceneCRec *path)
{
    int numpos = path->m_pos_num;
    ROR_ASSERT(numpos > 0 && numpos < 256); /* 66 */
    if (numpos < 2)
        return;

    int pa = path->m_seg;
    ROR_ASSERT(pa >= 0 && pa <= numpos-1); /* 71 */
    float pb = path->m_seg_t;
    ROR_ASSERT(pb >= 0.0f && pb < 1.0f); /* 74 */

    int pa1 = pa;
    int pa2;
    if (path->m_loop == 0)
    {
        if (pa == numpos - 1)
        {
            /* At the open path's end: its last segment. */
            ROR_ASSERT(pb == 0.0f); /* 85 */
            pa2 = numpos - 1;
            pa1 = numpos - 2;
        }
        else
            pa2 = pa + 1;
    }
    else
        pa2 = (pa + 1) % numpos;

    const float *pts = path->points();
    const float *p1 = &pts[pa1 * 4];
    const float *p2 = &pts[pa2 * 4];
    sceVu0FVECTOR a = { p1[0], p1[1], p1[2], 0.0f };
    sceVu0FVECTOR b = { p2[0], p2[1], p2[2], 0.0f };

    theWork->m_distance = p2[3];
    ROR_ASSERT(theWork->m_distance > 0.0f); /* 107 */

    if (pb == 0.0f)
    {
        const float *p = &pts[pa * 4];
        theWork->m_pos[0] = p[0];
        theWork->m_pos[1] = p[1];
        theWork->m_pos[2] = p[2];
        theWork->m_pos[3] = 1.0f;
    }
    else
    {
        ROR_ASSERT(pa == pa1); /* 119 */
        utlLerp4(pb, theWork->m_pos, a, b);
    }

    theWork->m_dir[0] = (b[0] - a[0]) / theWork->m_distance;
    theWork->m_dir[1] = (b[1] - a[1]) / theWork->m_distance;
    theWork->m_dir[2] = (b[2] - a[2]) / theWork->m_distance;
    theWork->m_dir[3] = 0.0f;
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the ROM (mainInitGame calls it).
// ROR 0x0013c928

void MagScene::initCamera()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013c930

void MagScene::saveCamera()
{
    m_camera_entity.saveCamera();
    m_cam_save_count++;
    m_saved_mode = m_camera_mode;
    m_saved_actor_no = m_cam_actor_no;
    m_saved_dist_max = m_cam_dist_max;
    m_saved_dist_min = m_cam_dist_min;
    m_saved_path_no = m_cam_path_no;
    m_saved_map_no = m_cam_map_no;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013ca10

void MagScene::restoreCamera()
{
    m_camera_entity.restoreCamera();
    m_cam_save_count--;
    m_camera_mode = m_saved_mode;
    m_cam_actor_no = m_saved_actor_no;
    m_cam_dist_max = m_saved_dist_max;
    m_cam_dist_min = m_saved_dist_min;
    m_cam_path_no = m_saved_path_no;
    m_cam_map_no = m_saved_map_no;
}

// ──────────────────────────────────────────────────────────────────────
// mode 0..5 (see m_camera_mode); arg is the actor (1, 2) or the path as
// map << 8 | path (4, 5). param, when given: [0] / [1] the path camera's
// nearest / farthest distance (modes 3, 4: when > 0), [2] the target actor
// (mode 3: when > 0, 4 / 5: when >= 0); mode 5 takes [0] as the path's
// point and [1] / 1000 as its fraction instead, and with no param puts the
// camera on the path's first point.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT and the inline getCameraMaxPath)
// ROR 0x0013caf0

void MagScene::setCameraMode(int mode, int arg, const int *param)
{
    m_camera_mode = mode;
    switch (mode)
    {
    case 0:
        m_camera_entity.setParent(NULL);
        return;

    case 1:
        m_camera_entity.setParent(NULL);
        m_cam_actor_no = arg;
        return;

    case 2:
    {
        int theParam = arg;
        ROR_ASSERT(0 <= theParam && theParam < 128); /* 190 */
        DogCamera &cam = m_camera_entity;
        cam.setParent(&m_actors[theParam]);
        cam.m_dirty = 1;
        cam.m_trans[2] = -50.0f;
        cam.m_trans[0] = 0.0f;
        cam.m_trans[1] = -50.0f;
        cam.setRotate(-0.5f, 0.0f, 0.0f);
        m_cam_actor_no = arg;
        return;
    }

    case 3:
        if (m_tenkyu[0].m_path_data == NULL)
        {
            camFollowActor(this, param);
            return;
        }
        m_camera_entity.setParent(NULL);
        m_cam_path_no = 0;
        m_cam_map_no = 0;
        m_camera_entity.m_angle = m_tenkyu[0].m_path_data[0]->m_fov * CAM_PI / 180.0f;
        if (param == NULL)
            return;
        if (param[0] > 0)
            m_cam_dist_min = (float)param[0];
        if (param[1] > 0)
            m_cam_dist_max = (float)param[1];
        if (param[2] > 0)
            m_cam_actor_no = param[2];
        return;

    case 4:
    {
        if (m_tenkyu[0].m_path_data == NULL)
        {
            camFollowActor(this, param);
            return;
        }
        int map = arg >> 8;
        int no = arg & 0xff;
        if (!(no < getCameraMaxPath(map)))
            return;
        m_camera_entity.setParent(NULL);
        m_cam_path_no = no;
        m_cam_map_no = map;
        ROR_ASSERT(0 <= m_cam_map_no && m_cam_map_no < MAX_MAP_MODEL); /* 269 */
        ROR_ASSERT(0 <= m_cam_path_no && m_cam_path_no < getCameraMaxPath( m_cam_map_no )); /* 270 */
        m_camera_entity.m_angle =
            m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no]->m_fov * CAM_PI / 180.0f;
        if (param == NULL)
            return;
        if (param[0] > 0)
            m_cam_dist_min = (float)param[0];
        if (param[1] > 0)
            m_cam_dist_max = (float)param[1];
        if (param[2] >= 0)
            m_cam_actor_no = param[2];
        return;
    }

    case 5:
    {
        if (m_tenkyu[0].m_path_data == NULL)
        {
            camFollowActor(this, param);
            return;
        }
        m_camera_entity.setParent(NULL);
        m_cam_path_no = arg & 0xff;
        m_cam_map_no = arg >> 8;
        MagSceneCRec *path = m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no];
        m_camera_entity.m_angle = path->m_fov * CAM_PI / 180.0f;
        if (param == NULL)
        {
            path->m_seg_t = 0.0f;
            path->m_seg = 0;
            return;
        }
        if (param[0] > 0)
            path->m_seg = param[0];
        if (param[1] > 0)
            path->m_seg_t = (float)param[1] / 1000.0f;
        if (param[2] >= 0)
            m_cam_actor_no = param[2];
        return;
    }

    default:
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, I3rRoot__CheckIdString (the assert handler; inside the inline DogModel::isAlive)
// ROR 0x0013d240

void MagScene::seekCameraPath(float pos)
{
    if ((u_int)(m_camera_mode - 3) >= 2 || m_cam_actor_no < 0)
        return;
    if (!m_actors[m_cam_actor_no].isAlive())
        return;

    sceVu0FVECTOR target;
    camTarget(m_actors[m_cam_actor_no], target);

    MagSceneCRec *ppath = m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no];
    ROR_ASSERT(ppath->m_pos_num > 0); /* 375 */
    if (ppath->m_pos_num == 1)
    {
        setCameraOnPath(0.0f, ppath->points(), 0, 1);
        m_camera_entity.lookAt(target, 0);
        return;
    }

    int start = findCameraPathStart(pos, ppath, target);
    if (start < ppath->m_pos_num - 1)
    {
        ppath->m_seg = start;
        ppath->m_seg_t = 0.0f;
    }
    else
    {
        ppath->m_seg = ppath->m_pos_num - 2;
        ppath->m_seg_t = 0.9f;
    }

    /* Step until the score changes by less than 5 (at most 1000 steps). */
    float last = 0.0f;
    for (int i = 0; i < 1000; i++)
    {
        float score = scoreCameraPath(ppath, target, 0);
        float d = last - score;
        if (d < 5.0f && -5.0f < d)
            break;
        setCameraOnPath(ppath->m_seg_t, ppath->points(), ppath->m_seg, ppath->m_pos_num);
        last = score;
    }
    m_camera_entity.lookAt(target, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013d4d0

float MagScene::getCameraPathPos()
{
    const MagSceneCRec *path = m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no];
    return (float)path->m_seg + path->m_seg_t;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: modff (inside camModf)
// ROR 0x0013d52c

void MagScene::setCameraPathPos(float pos)
{
    MagSceneCRec *path = m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no];
    int whole;
    path->m_seg_t = camModf(pos, &whole);
    path->m_seg = whole;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013d5ac

int MagScene::getCameraMode(int *out)
{
    if (out)
    {
        out[0] = m_camera_mode;
        out[1] = m_cam_actor_no;
    }
    return m_camera_mode;
}

// ──────────────────────────────────────────────────────────────────────
// A path camera's next goal (MagScene::update): a one-point path with
// m_alt == 1 flips its view; a longer one projects the actor onto the
// camera's segment and aims two segment lengths of that ahead. A new goal
// sets the move bit 0x10 and clears m_cam_move_frame.
// callcheck: FUN_00215a30, I3rRoot__CheckIdString, modff (the assert handler; inside the inline DogModel::isAlive; modff inside camModf)
// ROR 0x0013d5e4

void MagScene::changeCamera()
{
    if ((u_int)(m_camera_mode - 3) >= 2 || m_cam_actor_no < 0)
        return;
    if (!m_actors[m_cam_actor_no].isAlive())
        return;

    ROR_ASSERT(m_cam_map_no >= 0); /* 476 */
    ROR_ASSERT(m_cam_path_no >= 0); /* 477 */
    MagSceneCRec *path = m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no];
    int numpos = path->m_pos_num;
    if (numpos == 1)
    {
        if (path->m_alt != 1)
            return;
        path->m_alt_side = (path->m_alt_side + 1) & 1;
    }
    else
    {
        sceVu0FVECTOR target;
        camTarget(m_actors[m_cam_actor_no], target);
        /* PC: m_distance starts at 0; the ROM leaves it unset (the call
         * fills it for any path of two or more points). */
        MagCameraPathWork work = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f };
        getCameraPathWork(&work, path);

        sceVu0FVECTOR v = { target[0] - work.m_pos[0], target[1] - work.m_pos[1],
                            target[2] - work.m_pos[2], 0.0f };
        float along = utlVecDot(v, work.m_dir) / work.m_distance;
        float t = (float)path->m_seg + path->m_seg_t + (along + along);

        int seg;
        float frac = 0.0f;
        if (t < 0.0f)
            seg = 0;
        else
        {
            seg = numpos - 1;
            if (!((float)seg < t))
                frac = camModf(t, &seg);
        }
        if (seg == path->m_seg && frac == path->m_seg_t)
            return;
        path->m_goal_t = frac;
        path->m_goal_seg = seg;
    }
    m_cam_move_frame = 0;
    m_camera_mode |= 0x10;
}

// ──────────────────────────────────────────────────────────────────────
// thePath's tag letters: [0] 'a'..'d' full steps, 'e'..'h' a tenth (no
// letter: a tenth too), [1] 'a'..'z' both distances to 100 + 20 per letter,
// [2] 'a'..'z' widens the band by 10 per letter each way. flags bit 1 adds
// 1e6 when a ray from the camera point toward target hits.
// callcheck: FUN_00215a30, modff (the assert handler inside ROR_ASSERT; modff inside camModf)
// ROR 0x0013d8f8

float MagScene::scoreCameraPath(MagSceneCRec *thePath, const float *target, u_int flags)
{
    ROR_ASSERT(thePath); /* 568 */

    int slow = -1;
    if ((u_int)(thePath->m_tag[0] - 'a') < 8)
        slow = ((thePath->m_tag[0] - 'a') >> 2) & 1;
    float dmax = m_cam_dist_max;
    float dmin = m_cam_dist_min;
    if ((u_int)(thePath->m_tag[1] - 'a') < 26)
    {
        dmax = (float)((thePath->m_tag[1] - 'a') * 20 + 100);
        dmin = dmax;
    }
    if ((u_int)(thePath->m_tag[2] - 'a') < 26)
    {
        float w = (float)((thePath->m_tag[2] - 'a') * 10);
        dmax += w;
        dmin -= w;
    }

    int numpos = thePath->m_pos_num;
    ROR_ASSERT(numpos > 0 && numpos < 256); /* 636 */
    const float *pts = thePath->points();
    float distSq;

    if (numpos < 2)
    {
        if (thePath->m_alt != 0)
            pts += (thePath->m_alt_side & 1) * 16;  /* the second view, 4 points on */
        float dx = pts[0] - target[0];
        float dy = pts[1] - target[1];
        float dz = pts[2] - target[2];
        distSq = dx * dx + dy * dy + dz * dz;
    }
    else
    {
        MagCameraPathWork work = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f };
        getCameraPathWork(&work, thePath);
        sceVu0FVECTOR v = { target[0] - work.m_pos[0], target[1] - work.m_pos[1],
                            target[2] - work.m_pos[2], 0.0f };
        float len = utlVecLength(v);

        float excess = 0.0f;
        if (dmax < len)
            excess = (len - dmax) / len;
        else if (len < dmin)
            excess = (len - dmin) / len;

        if (excess == 0.0f)
            distSq = len * len;
        else
        {
            float step = utlVecDot(v, work.m_dir) * excess / work.m_distance;
            if (slow != 0)
                step *= 0.099999994f;
            float t = (float)thePath->m_seg + thePath->m_seg_t + step;

            int na;
            float frac = 0.0f;
            if (thePath->m_loop == 0)
            {
                na = 0;
                if (!(t < 0.0f))
                {
                    na = numpos - 1;
                    if (!((float)na < t))
                        frac = camModf(t, &na);
                }
            }
            else
            {
                if (t < 0.0f)
                    t += (float)numpos;
                else if ((float)numpos <= t)
                    t -= (float)numpos;
                frac = camModf(t, &na);
            }
            ROR_ASSERT(na >= 0 && na < 256); /* 721 */
            ROR_ASSERT(na >= 0 && na < numpos); /* 722 */

            const float *pa = &pts[na * 4];
            const float *pb = &pts[((na + 1) % numpos) * 4];
            sceVu0FVECTOR a = { pa[0], pa[1], pa[2], 0.0f };
            sceVu0FVECTOR b = { pb[0], pb[1], pb[2], 0.0f };
            sceVu0FVECTOR q = { 0.0f, 0.0f, 0.0f, 1.0f };
            utlLerp4(frac, q, a, b);
            v[0] = target[0] - q[0];
            v[1] = target[1] - q[1];
            v[2] = target[2] - q[2];
            v[3] = 0.0f;
            float len2 = utlVecLength(v);

            /* Take the step when it brings the distance into the band or
             * nearer to it. */
            bool better = false;
            if (dmax < len)
                better = !(dmax < len2) || len2 < len;
            else if (len < dmin)
                better = !(len2 < dmin) || len < len2;
            if (better)
            {
                thePath->m_seg = na;
                distSq = len2 * len2;
                thePath->m_seg_t = frac;
            }
            else
                distSq = len * len;
        }
    }

    float score = 1e6f;
    if (dmax * dmax <= distSq)
        score = 1e6f - (distSq - dmax * dmax);
    else if (distSq < dmin * dmin)
        score = 1e6f - (dmin * dmin - distSq);

    if (flags & 1)
    {
        /* The camera point (the next point taken unwrapped, as the ROM
         * does). */
        const float *p = &pts[thePath->m_seg * 4];
        sceVu0FVECTOR c = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (thePath->m_seg_t == 0.0f)
        {
            c[0] = p[0];
            c[1] = p[1];
            c[2] = p[2];
            c[3] = 0.0f;
        }
        else
        {
            sceVu0FVECTOR a = { p[0], p[1], p[2], 0.0f };
            sceVu0FVECTOR b = { p[4], p[5], p[6], 0.0f };
            utlLerp4(thePath->m_seg_t, c, a, b);
        }
        MagSceneRayHit hit;
        for (int i = 0; i < 5; i++)
        {
            hit.m_vec[i][0] = 0.0f;
            hit.m_vec[i][1] = 0.0f;
            hit.m_vec[i][2] = 0.0f;
            hit.m_vec[i][3] = 1.0f;
        }
        hit.m_50 = -1;
        /* The ROM passes target itself as castRay's dir (not target - c). */
        float t = castRay(&hit, c, target, -1);
        if (0.0f <= t && t <= 1.0f)
            score -= -1e6f;
    }
    return score;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013dfe0

int MagScene::findCameraPathStart(float pos, const MagSceneCRec *path, const float *target)
{
    int numpos = path->m_pos_num;
    if (numpos < 2)
        return 0;
    if (0.0f <= pos && pos <= 100.0f)
        return (int)(pos * ((float)numpos - 1.0f) / 100.0f);

    const float *pts = path->points();
    int best = 0;
    float bestDist = 1e8f;
    for (int i = 0; i < numpos - 1; i++)
    {
        sceVu0FVECTOR v = { target[0] - pts[i * 4], target[1] - pts[i * 4 + 1],
                            target[2] - pts[i * 4 + 2], 0.0f };
        float d = utlVecLength(v);
        if (d < bestDist && 500.0f < d)
        {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013e190

void MagScene::setCameraOnPath(float theF, const float *thePos, int theI, int theMax)
{
    ROR_ASSERT(thePos); /* 920 */
    ROR_ASSERT(theI >= 0 && theI < theMax); /* 921 */
    ROR_ASSERT(theF >= 0.0f && theF <= 1.0f); /* 922 */
    const float *a = &thePos[theI * 4];
    const float *b = &thePos[((theI + 1) % theMax) * 4];
    float x = a[0];
    float y = a[1];
    float z = a[2];
    if (0.0f < theF)
    {
        z = z + (b[2] - z) * theF;
        x = x + (b[0] - x) * theF;
        y = y + (b[1] - y) * theF;
    }
    m_camera_entity.m_dirty = 1;
    m_camera_entity.m_trans[0] = x;
    m_camera_entity.m_trans[1] = y;
    m_camera_entity.m_trans[2] = z;
}

// ──────────────────────────────────────────────────────────────────────
// The rotation rot wrapped to within pi of base and kept within lim of it.
// Inlined twice in clampCameraLook.
static float camLimitRot(float rot, float base, float lim)
{
    float d = rot - base;
    if (d < -CAM_PI)
        d += 2.0f * CAM_PI;
    if (CAM_PI <= d)
        d -= 2.0f * CAM_PI;
    if (d < -lim)
        return base - lim;
    if (lim < d)
        return base + lim;
    return rot;
}

// The ROM normalises target itself, the actor's position in the world, not
// its direction from the camera (kept as it is; open-items.md "Path camera
// look").
// ROR 0x0013e2ec

void MagScene::clampCameraLook(float limX, float limY, const float *target, const float *base)
{
    sceVu0FVECTOR v = { target[0], target[1], target[2], target[3] };
    utlNormalize3(v);

    float rot;
    if (0.0f < limX)
        rot = camLimitRot(utlVecToRotX(v), base[0], limX);
    else
        rot = base[0];
    m_camera_entity.m_rotate[0] = rot;
    m_camera_entity.normalizeRotate();
    m_camera_entity.m_dirty = 1;

    if (0.0f < limY)
        rot = camLimitRot(utlVecToRotY(v), base[1], limY);
    else
        rot = base[1];
    m_camera_entity.m_rotate[1] = rot;
    m_camera_entity.normalizeRotate();
    m_camera_entity.m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// n / 8 in steps of 30 degrees, n % 8 in steps of 10.
// ROR 0x0013e4c0

void MagScene::getCameraLookLimit(int n, float *limX, float *limY)
{
    if (n == 0)
    {
        *limY = 0.0f;
        *limX = 0.0f;
        return;
    }
    *limX = ((float)(n / 8) * 1.5707963f) / 3.0f;
    *limY = ((float)(n % 8) * 1.5707963f) / 9.0f;
}

/* A one-point path with two views (m_alt 1) is two records back to back,
 * each its header and its point (0x40): the second is the view while
 * m_alt_side is set. scoreCameraPath reads the second point the same way. */
static MagSceneCRec *camViewRec(MagSceneCRec *path)
{
    if (path->m_pos_num == 1 && path->m_alt == 1 && path->m_alt_side != 0)
        return (MagSceneCRec *)((u_char *)path + 0x40);
    return path;
}

/* The camera's actor has a live model. PORT: the ROM reads actor -1's
 * words before m_actors when m_cam_actor_no is -1 (harmless on the EE,
 * a stray pointer on the host); none counts as not alive. */
static bool camActorAlive(MagScene *s)
{
    return s->m_cam_actor_no >= 0 && s->m_actors[s->m_cam_actor_no].isAlive();
}

/* Sky slots 0..3 hold a camera path at all. */
static bool camAnyPath(const MagScene *s)
{
    int total = 0;
    for (int i = 0; i < MagScene::MAX_MAP_MODEL; i++)
        total += s->m_tenkyu[i].m_c_num;
    return total > 0;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, I3rRoot__CheckIdString, modff (the assert handler; inside the inline DogModel::isAlive; modff inside camModf)
// ROR 0x0013e548

void MagScene::updateCamera()
{
    int mode = m_camera_mode;
    int map_no = m_cam_map_no;
    int path_no = m_cam_path_no;
    sceVu0FVECTOR target;

    if (mode & 0x10)
    {
        /* A move to the path's goal (changeCamera): 30 frames from
         * (m_seg, m_seg_t) to (m_goal_seg, m_goal_t). */
        int base = mode & 0xf;
        if (base > 4 || base < 3)
        {
            ROR_ASSERT(0); /* 1157 */
            return;
        }
        MagSceneCRec *path = m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no];
        if (path->m_pos_num == 1)
        {
            m_camera_mode = base;
            m_cam_move_frame = 0;
            return;
        }
        m_cam_move_frame++;
        if (camActorAlive(this))
        {
            camTarget(m_actors[m_cam_actor_no], target);
            scoreCameraPath(path, target, 0);
            float f = (float)m_cam_move_frame / 30.0f;
            float t = ((float)path->m_seg + path->m_seg_t) * (1.0f - f)
                    + ((float)path->m_goal_seg + path->m_goal_t) * f;
            int seg;
            float frac = camModf(t, &seg);
            setCameraOnPath(frac, path->points(), seg, path->m_pos_num);
            m_camera_entity.lookAt(target, 0);
        }
        if (m_cam_move_frame < 30)
            return;
        path->m_seg = path->m_goal_seg;
        path->m_seg_t = path->m_goal_t;
        m_cam_move_frame = 0;
        m_camera_mode &= 0xf;
        return;
    }

    switch (mode)
    {
    case 1:
        if (camActorAlive(this))
        {
            camTarget(m_actors[m_cam_actor_no], target);
            m_camera_entity.lookAt(target, 0);
        }
        break;

    case 3:
    {
        if (!camAnyPath(this) || !camActorAlive(this))
            break;
        camTarget(m_actors[m_cam_actor_no], target);

        /* Every path's score goes into its m_goal_t (the ROM reuses the
         * word), the current path's raised by 200000 to keep it. */
        for (int i = 0; i < MAX_MAP_MODEL; i++)
        {
            for (int j = 0; j < m_tenkyu[i].m_c_num; j++)
            {
                MagSceneCRec *p = m_tenkyu[i].m_path_data[j];
                p->m_goal_t = scoreCameraPath(p, target, 0);
                if (j == m_cam_path_no && i == m_cam_map_no)
                    p->m_goal_t += 200000.0f;
            }
        }
        /* The highest score wins; the ROM starts from a NaN, so the first
         * path (and any NaN score) always takes the lead. */
        int best = -1;
        int bestMap = -1;
        float bestScore = NAN;
        for (int i = 0; i < MAX_MAP_MODEL; i++)
        {
            for (int j = 0; j < m_tenkyu[i].m_c_num; j++)
            {
                float score = m_tenkyu[i].m_path_data[j]->m_goal_t;
                if (!(score <= bestScore))
                {
                    bestScore = score;
                    best = j;
                    bestMap = i;
                }
            }
        }
        if (best >= 0)
        {
            m_cam_path_no = best;
            m_cam_map_no = bestMap;
        }
        MagSceneCRec *path = m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no];
        setCameraOnPath(path->m_seg_t, path->points(), path->m_seg, path->m_pos_num);
        m_camera_entity.lookAt(target, 0);
        break;
    }

    case 4:
    case 5:
    {
        if (!camAnyPath(this) || !camActorAlive(this))
            break;
        camTarget(m_actors[m_cam_actor_no], target);

        /* 4 slides along the path (scoreCameraPath steps m_seg / m_seg_t),
         * 5 stays where it was placed. */
        MagSceneCRec *path = camViewRec(m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no]);
        if (mode == 4)
            scoreCameraPath(path, target, 0);
        setCameraOnPath(path->m_seg_t, path->points(), path->m_seg, path->m_pos_num);
        if (path->m_look_tag != 0)
        {
            float limX, limY;
            getCameraLookLimit(path->m_look_tag - 'a', &limX, &limY);
            clampCameraLook(limX, limY, target, path->m_look_rot);
        }
        else
            m_camera_entity.lookAt(target, 0);
        break;
    }

    case 6:
        m_camera_entity.updateMove();
        break;

    default:
        break;
    }

    /* A path camera that changed paths takes the new path's lens angle. */
    if (m_camera_mode < 6 && 2 < m_camera_mode
        && (map_no != m_cam_map_no || path_no != m_cam_path_no))
        m_camera_entity.m_angle =
            m_tenkyu[m_cam_map_no].m_path_data[m_cam_path_no]->m_fov * CAM_PI / 180.0f;
}
