#ifndef ROSEPAN_PROFILER_H
#define ROSEPAN_PROFILER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RosePanProfilePhase
{
    ROSEPAN_PROFILE_SGD_CPU = 0,
    ROSEPAN_PROFILE_BILLBOARD_CPU,
    ROSEPAN_PROFILE_BILLBOARD_HOST_BRIDGE,
    ROSEPAN_PROFILE_SKY_CPU,
    ROSEPAN_PROFILE_MAPPUT_SORT,
    ROSEPAN_PROFILE_MAPPUT_CALLBACK,
    ROSEPAN_PROFILE_ROOM_LOAD,
    ROSEPAN_PROFILE_ROOM_INIT,
    ROSEPAN_PROFILE_ROOM_FURN,
    ROSEPAN_PROFILE_ROOM_MODEL_INIT,
    ROSEPAN_PROFILE_ROOM_REGISTRATION,
    ROSEPAN_PROFILE_MESH_CPU,
    ROSEPAN_PROFILE_MESH_LIGHTING,
    ROSEPAN_PROFILE_MESH_SKINNING,
    ROSEPAN_PROFILE_GS_UPLOAD,
    ROSEPAN_PROFILE_TEXTURE_INVALIDATE,
    ROSEPAN_PROFILE_TEXTURE_MISS,
    ROSEPAN_PROFILE_RENDERER_UPLOAD,
    ROSEPAN_PROFILE_RENDERER_RECORD,
    ROSEPAN_PROFILE_RENDERER_ACQUIRE,
    ROSEPAN_PROFILE_RENDERER_SUBMIT,
    ROSEPAN_PROFILE_PHASE_COUNT
} RosePanProfilePhase;

typedef enum RosePanProfilerCounter
{
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_REQUESTS = 0,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_QUEUED,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_REJECTED,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_SUPPRESSED,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_COMMANDS,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_VERTICES,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_STREAM_BYTES,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_COMPATIBLE_JOINS,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_TEXTURE_LOOKUPS,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_TEXTURE_MISSES,
    ROSEPAN_PROFILER_COUNTER_BILLBOARD_LEGACY_SKIPS,
    ROSEPAN_PROFILER_COUNTER_SKY_COMMANDS,
    ROSEPAN_PROFILER_COUNTER_SKY_VERTICES,
    ROSEPAN_PROFILER_COUNTER_SKY_STREAM_BYTES,
    ROSEPAN_PROFILER_COUNTER_SKY_COMPATIBLE_JOINS,
    ROSEPAN_PROFILER_COUNTER_SKY_POINTS_TRANSFORMED,
    ROSEPAN_PROFILER_COUNTER_SKY_CELLS_TESTED,
    ROSEPAN_PROFILER_COUNTER_SKY_VISIBLE_CELLS,
    ROSEPAN_PROFILER_COUNTER_SKY_HORIZON_QUADS,
    ROSEPAN_PROFILER_COUNTER_SKY_DOME_SUPPRESSED,
    ROSEPAN_PROFILER_COUNTER_SKY_HORIZON_SUPPRESSED,
    ROSEPAN_PROFILER_COUNTER_MAPPUT_CANDIDATES,
    ROSEPAN_PROFILER_COUNTER_MAPPUT_DRAWABLE,
    ROSEPAN_PROFILER_COUNTER_MAPPUT_COMPARISONS,
    ROSEPAN_PROFILER_COUNTER_MAPPUT_CALLBACKS,
    ROSEPAN_PROFILER_COUNTER_MAPPUT_OBJECT_DRAWS,
    ROSEPAN_PROFILER_COUNTER_COUNT
} RosePanProfilerCounter;

typedef struct RosePanProfilerCounterStats
{
    double per_frame;
    uint64_t max;
} RosePanProfilerCounterStats;

typedef struct RosePanProfilerStats
{
    double frame_ms;
    double frame_max_ms;
    double workload_ms;
    double workload_max_ms;
    double game_cpu_ms;
    double game_cpu_max_ms;
    double vblank_wait_ms;
    double vblank_wait_max_ms;
    double sgd_cpu_ms;
    double sgd_cpu_max_ms;
    double billboard_cpu_ms;
    double billboard_cpu_max_ms;
    double billboard_host_bridge_ms;
    double billboard_host_bridge_max_ms;
    double sky_cpu_ms;
    double sky_cpu_max_ms;
    double mapput_sort_ms;
    double mapput_sort_max_ms;
    double mapput_callback_ms;
    double mapput_callback_max_ms;
    double room_load_ms;
    double room_load_max_ms;
    double room_init_ms;
    double room_init_max_ms;
    double room_furn_ms;
    double room_furn_max_ms;
    double room_model_init_ms;
    double room_model_init_max_ms;
    double room_registration_ms;
    double room_registration_max_ms;
    double mesh_cpu_ms;
    double mesh_cpu_max_ms;
    double mesh_lighting_ms;
    double mesh_lighting_max_ms;
    double mesh_skinning_ms;
    double mesh_skinning_max_ms;
    double gs_upload_ms;
    double gs_upload_max_ms;
    double texture_invalidate_ms;
    double texture_invalidate_max_ms;
    double texture_miss_ms;
    double texture_miss_max_ms;
    double renderer_ms;
    double renderer_max_ms;
    double renderer_upload_ms;
    double renderer_upload_max_ms;
    double renderer_record_ms;
    double renderer_record_max_ms;
    double renderer_acquire_ms;
    double renderer_acquire_max_ms;
    double renderer_submit_ms;
    double renderer_submit_max_ms;
    double gpu_idle_wait_ms;
    double gpu_idle_wait_max_ms;
    int gpu_idle_waits;
    int swapchain_acquire_attempts;
    int swapchain_unavailable;
    int vblank_drained;
    int vblank_wait_calls;
    int vblank_deadline_misses;
    int vblank_target;
    RosePanProfilerCounterStats billboard_requests;
    RosePanProfilerCounterStats billboard_queued;
    RosePanProfilerCounterStats billboard_rejected;
    RosePanProfilerCounterStats billboard_suppressed;
    RosePanProfilerCounterStats billboard_commands;
    RosePanProfilerCounterStats billboard_vertices;
    RosePanProfilerCounterStats billboard_stream_bytes;
    RosePanProfilerCounterStats billboard_compatible_joins;
    RosePanProfilerCounterStats billboard_texture_lookups;
    RosePanProfilerCounterStats billboard_texture_misses;
    RosePanProfilerCounterStats billboard_legacy_skips;
    RosePanProfilerCounterStats sky_commands;
    RosePanProfilerCounterStats sky_vertices;
    RosePanProfilerCounterStats sky_stream_bytes;
    RosePanProfilerCounterStats sky_compatible_joins;
    RosePanProfilerCounterStats sky_points_transformed;
    RosePanProfilerCounterStats sky_cells_tested;
    RosePanProfilerCounterStats sky_visible_cells;
    RosePanProfilerCounterStats sky_horizon_quads;
    RosePanProfilerCounterStats sky_dome_suppressed;
    RosePanProfilerCounterStats sky_horizon_suppressed;
    RosePanProfilerCounterStats mapput_candidates;
    RosePanProfilerCounterStats mapput_drawable;
    RosePanProfilerCounterStats mapput_comparisons;
    RosePanProfilerCounterStats mapput_callbacks;
    RosePanProfilerCounterStats mapput_object_draws;
} RosePanProfilerStats;

/* The profiler follows the renderer/game loop and is intentionally
 * main-thread-only. */
void RosePan_ProfilerReset(void);
void RosePan_ProfilerBeginFrame(void);
void RosePan_ProfilerBeginRenderer(void);
void RosePan_ProfilerEndFrame(void);

uint64_t RosePan_ProfilerNow(void);
uint64_t RosePan_ProfilerBeginPhase(RosePanProfilePhase phase);
void RosePan_ProfilerEndPhase(RosePanProfilePhase phase,
                             uint64_t start_counter);
void RosePan_ProfilerAddCounter(RosePanProfilerCounter counter,
                               uint64_t amount);

/* TEMPORARY DIAGNOSTIC -- stutter attribution.
 *
 * Set ROSEPAN_STUTTER_MS in the environment to a millisecond threshold; every
 * logical frame whose wall time exceeds it appends one fully attributed line
 * to rosepan_stutter.log.  The renderer publishes its per-frame cache/texture
 * counters through RosePan_ProfilerSetFrameNote() so that the dump can say
 * *which* cold work the slow frame did, not just how long it took.  Costs one
 * comparison per frame when the variable is unset. */
void RosePan_ProfilerSetFrameNote(const char *text);

void RosePan_ProfilerRecordGpuIdleWait(uint64_t wait_ticks);

/* Time the renderer spent deliberately idle, spacing the presents of one
 * logical frame a CRTC field apart.  Subtracted from the renderer total the
 * same way the V-blank wait is subtracted from the game's, so `renderer_ms`
 * keeps meaning work done and the workload budget stays a budget. */
void RosePan_ProfilerRecordPresentWait(uint64_t wait_ticks);
void RosePan_ProfilerRecordSwapchainAcquire(int unavailable);
void RosePan_ProfilerRecordVblankPacing(uint64_t wait_ticks,
                                       int drained_vblanks,
                                       int wait_calls,
                                       int target_vblanks);

/* Converts the accumulated window to milliseconds per logical frame and
 * resets it.  Maxima are the worst individual logical frame in the window. */
void RosePan_ProfilerConsumeStats(RosePanProfilerStats *out);

#ifdef __cplusplus
}

class RosePanProfileScope
{
public:
    explicit RosePanProfileScope(RosePanProfilePhase phase,
                                bool enabled = true)
        : phase_(phase),
          start_(enabled ? RosePan_ProfilerBeginPhase(phase) : 0)
    {
    }

    ~RosePanProfileScope()
    {
        if (start_ != 0)
        {
            RosePan_ProfilerEndPhase(phase_, start_);
        }
    }

    RosePanProfileScope(const RosePanProfileScope &) = delete;
    RosePanProfileScope &operator=(const RosePanProfileScope &) = delete;

private:
    RosePanProfilePhase phase_;
    uint64_t start_;
};
#endif

#endif
