#include "rosepan_ui.h"
#include "../os/rosepan_pacing.h"
#include "../rendering/rosepan_renderer.h"
#include "../rosepan_config.h"
#include "../io/rosepan_input.h"
#include "../io/rosepan_paths.h"
#include "../os/rosepan_host.h"
#include "../rosepan_debug.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <imgui_internal.h>
#include <implot.h>

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
ImGuiContext *g_context;
ImPlotContext *g_plot_context;
SDL_Window *g_window;
bool g_platform_initialized;
bool g_renderer_initialized;
bool g_frame_started;
bool g_frame_rendered;
bool g_previous_rb;
bool g_windowing_chord_latched;
bool g_show_main_menu_bar = true;
bool g_show_performance_summary;
/* Summary trimmed to its first line.  The frame rate is the figure worth
 * leaving on screen; the rest of the block is for when something is wrong. */
bool g_summary_fps_only;
bool g_show_performance_graphs;
bool g_show_controls;
bool g_show_data_folder;
bool g_pause_performance_history;
bool g_follow_performance_history = true;
RosePanUi::PerformanceStats g_performance_stats{};

constexpr int kProfilerHistoryCapacity = 120;

struct ProfilerHistorySample
{
    double time_seconds;
    double frame_ms;
    double frame_max_ms;
    double workload_ms;
    double workload_max_ms;
    double game_cpu_ms;
    double game_cpu_max_ms;
    double vblank_wait_ms;
    double pacing_target_ms;
    double sgd_cpu_ms;
    double sgd_cpu_max_ms;
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
    double billboard_requests;
    double billboard_queued;
    double billboard_rejected;
    double billboard_suppressed;
    double billboard_commands;
    double billboard_vertices;
    double billboard_stream_bytes;
    double billboard_compatible_joins;
    double billboard_texture_lookups;
    double billboard_texture_misses;
    double billboard_legacy_skips;
    double billboard_ideal_draws;
    double sky_commands;
    double sky_vertices;
    double sky_stream_bytes;
    double sky_compatible_joins;
    double sky_points_transformed;
    double sky_cells_tested;
    double sky_visible_cells;
    double sky_horizon_quads;
    double sky_dome_suppressed;
    double sky_horizon_suppressed;
    double sky_ideal_draws;
    double mapput_candidates;
    double mapput_drawable;
    double mapput_comparisons;
    double mapput_callbacks;
    double mapput_object_draws;
    double billboard_stream_kib;
    double sky_stream_kib;
    double world_stream_kib;
};

std::array<ProfilerHistorySample, kProfilerHistoryCapacity>
    g_profiler_history{};
int g_profiler_history_count;
int g_profiler_history_next;
Uint64 g_profiler_history_origin;

bool IsInitialized()
{
    return g_context != nullptr && g_plot_context != nullptr &&
           g_platform_initialized &&
           g_renderer_initialized;
}

void ClearProfilerHistory()
{
    g_profiler_history_count = 0;
    g_profiler_history_next = 0;
    g_profiler_history_origin = 0;
}

/*
 * Frame-pacing harness.  The game waits SYSTEM_VBLANK_WAIT_NUM CRTC fields per
 * logical frame -- 2, so 30 fps -- and this forces a different count so the
 * profiler's workload figures can be taken at the rate being asked about
 * rather than inferred from a 30 Hz sample.  See rosepan_pacing.h; the engine
 * has no delta time, so game speed scales with the count.
 */
void DrawFramePacingMenu()
{
    struct PacingChoice
    {
        int fields;
        const char *label;
    };

    /* 60 divided by the field count, and the speed multiplier is the same
     * ratio against the game's own 2. */
    static const PacingChoice kChoices[] = {
        {1, "1 field  -  60 fps, 2x speed"},
        {2, "2 fields -  30 fps, normal"},
        {3, "3 fields -  20 fps, 2/3 speed"},
        {4, "4 fields -  15 fps, 1/2 speed"},
    };

    if (!ImGui::BeginMenu("Frame pacing"))
    {
        return;
    }

    const int active = RosePan_GetVBlankWaitOverride();

    if (ImGui::MenuItem("Follow game", nullptr, active == 0))
    {
        RosePan_SetVBlankWaitOverride(0);
    }
    ImGui::Separator();
    for (const PacingChoice &choice : kChoices)
    {
        if (ImGui::MenuItem(choice.label, nullptr, active == choice.fields))
        {
            RosePan_SetVBlankWaitOverride(choice.fields);
        }
    }
    ImGui::Separator();
    ImGui::TextDisabled("Measurement harness; not saved to rosepan.ini.");
    ImGui::TextDisabled("Watch 'deadline miss' in the summary: rising at");
    ImGui::TextDisabled("1 field means the workload exceeds 16.7 ms.");
    ImGui::EndMenu();
}

/*
 * Frame smoothing.  The game simulates at a fixed rate and has no delta time,
 * so extra frames cannot come from ticking it faster; they come from putting
 * the frame that is already queued on screen again through an interpolated
 * camera.  Turning interpolation off makes the extras identity copies instead,
 * which is the check that the second record pass is sound at all.
 */
void DrawFrameSmoothingMenu()
{
    if (!ImGui::BeginMenu("Frame smoothing"))
    {
        return;
    }

    static const char *kLabels[] = {
        "Off      -  1 present per frame",
        "1 extra  -  2 presents per frame",
        "2 extra  -  3 presents per frame",
        "3 extra  -  4 presents per frame",
    };

    /* Beyond the budget the tick grows instead of the frame rate rising, so
     * those rows are shown disabled rather than silently clamped. */
    const int budget = RosePan_RendererGetMaxExtraPresents();
    const int active = RosePan_RendererGetExtraPresents();
    for (int extra = 0; extra < (int)(sizeof(kLabels) / sizeof(kLabels[0]));
         extra++)
    {
        if (ImGui::MenuItem(kLabels[extra], nullptr, active == extra,
                            extra <= budget))
        {
            RosePan_RendererSetExtraPresents(extra);
        }
    }
    ImGui::Separator();
    ImGui::TextDisabled("At most %d extra: the game waits %d fields a tick",
                        budget, budget + 1);
    ImGui::TextDisabled("and vfunc() always keeps the last one for itself.");
    ImGui::Separator();
    bool interpolate = RosePan_RendererGetPresentInterpolation() != 0;
    if (ImGui::MenuItem("Interpolate camera", nullptr, &interpolate,
                        active != 0))
    {
        RosePan_RendererSetPresentInterpolation(interpolate ? 1 : 0);
    }
    bool interpolate_geometry =
        RosePan_RendererGetGeometryInterpolation() != 0;
    if (ImGui::MenuItem("Interpolate motion", nullptr, &interpolate_geometry,
                        active != 0 && interpolate))
    {
        RosePan_RendererSetGeometryInterpolation(interpolate_geometry ? 1 : 0);
    }
    ImGui::Separator();
    ImGui::TextDisabled("The simulation keeps its own rate either way -- the");
    ImGui::TextDisabled("extra fields come out of the wait the game already");
    ImGui::TextDisabled("does, so nothing speeds up.  Watch 'present/frame'");
    ImGui::TextDisabled("in the summary follow this.");
    ImGui::TextDisabled("With interpolation off the picture must not change.");
    ImGui::Separator();
    ImGui::TextDisabled("'Interpolate motion' moves the world as well as the");
    ImGui::TextDisabled("eye: characters and moving objects stop stepping at");
    ImGui::TextDisabled("30 Hz.  It re-uploads the vertex buffer once per");
    ImGui::TextDisabled("present, so 'Renderer upload' rises with the frame");
    ImGui::TextDisabled("rate -- with it off, upload must stay flat.  Watch");
    ImGui::TextDisabled("'geo' in the summary: it is the share of 3D draws");
    ImGui::TextDisabled("that found a matching draw last frame.");
    ImGui::EndMenu();
}

void AppendProfilerHistory(const RosePanUi::PerformanceStats &stats)
{
    if (g_pause_performance_history)
    {
        return;
    }

    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    if (g_profiler_history_origin == 0 || now < g_profiler_history_origin)
    {
        g_profiler_history_origin = now;
    }

    const RosePanProfilerStats &profiler = stats.profiler;
    ProfilerHistorySample sample{};
    sample.time_seconds = frequency != 0
        ? (double)(now - g_profiler_history_origin) / (double)frequency
        : 0.0;
    sample.frame_ms = profiler.frame_ms;
    sample.frame_max_ms = profiler.frame_max_ms;
    sample.workload_ms = profiler.workload_ms;
    sample.workload_max_ms = profiler.workload_max_ms;
    sample.game_cpu_ms = profiler.game_cpu_ms;
    sample.game_cpu_max_ms = profiler.game_cpu_max_ms;
    sample.vblank_wait_ms = profiler.vblank_wait_ms;
    sample.pacing_target_ms = profiler.vblank_target > 0
        ? (double)profiler.vblank_target * (1000.0 / 60.0) : 0.0;
    sample.sgd_cpu_ms = profiler.sgd_cpu_ms;
    sample.sgd_cpu_max_ms = profiler.sgd_cpu_max_ms;
    sample.room_load_ms = profiler.room_load_ms;
    sample.room_load_max_ms = profiler.room_load_max_ms;
    sample.room_init_ms = profiler.room_init_ms;
    sample.room_init_max_ms = profiler.room_init_max_ms;
    sample.room_furn_ms = profiler.room_furn_ms;
    sample.room_furn_max_ms = profiler.room_furn_max_ms;
    sample.room_model_init_ms = profiler.room_model_init_ms;
    sample.room_model_init_max_ms = profiler.room_model_init_max_ms;
    sample.room_registration_ms = profiler.room_registration_ms;
    sample.room_registration_max_ms = profiler.room_registration_max_ms;
    sample.billboard_cpu_ms = profiler.billboard_cpu_ms;
    sample.billboard_cpu_max_ms = profiler.billboard_cpu_max_ms;
    sample.billboard_host_bridge_ms = profiler.billboard_host_bridge_ms;
    sample.billboard_host_bridge_max_ms =
        profiler.billboard_host_bridge_max_ms;
    sample.sky_cpu_ms = profiler.sky_cpu_ms;
    sample.sky_cpu_max_ms = profiler.sky_cpu_max_ms;
    sample.mapput_sort_ms = profiler.mapput_sort_ms;
    sample.mapput_sort_max_ms = profiler.mapput_sort_max_ms;
    sample.mapput_callback_ms = profiler.mapput_callback_ms;
    sample.mapput_callback_max_ms = profiler.mapput_callback_max_ms;
    sample.mesh_cpu_ms = profiler.mesh_cpu_ms;
    sample.mesh_cpu_max_ms = profiler.mesh_cpu_max_ms;
    sample.mesh_lighting_ms = profiler.mesh_lighting_ms;
    sample.mesh_lighting_max_ms = profiler.mesh_lighting_max_ms;
    sample.mesh_skinning_ms = profiler.mesh_skinning_ms;
    sample.mesh_skinning_max_ms = profiler.mesh_skinning_max_ms;
    sample.gs_upload_ms = profiler.gs_upload_ms;
    sample.gs_upload_max_ms = profiler.gs_upload_max_ms;
    sample.texture_invalidate_ms = profiler.texture_invalidate_ms;
    sample.texture_invalidate_max_ms = profiler.texture_invalidate_max_ms;
    sample.texture_miss_ms = profiler.texture_miss_ms;
    sample.texture_miss_max_ms = profiler.texture_miss_max_ms;
    sample.renderer_ms = profiler.renderer_ms;
    sample.renderer_max_ms = profiler.renderer_max_ms;
    sample.renderer_upload_ms = profiler.renderer_upload_ms;
    sample.renderer_upload_max_ms = profiler.renderer_upload_max_ms;
    sample.renderer_record_ms = profiler.renderer_record_ms;
    sample.renderer_record_max_ms = profiler.renderer_record_max_ms;
    sample.renderer_acquire_ms = profiler.renderer_acquire_ms;
    sample.renderer_acquire_max_ms = profiler.renderer_acquire_max_ms;
    sample.renderer_submit_ms = profiler.renderer_submit_ms;
    sample.renderer_submit_max_ms = profiler.renderer_submit_max_ms;
    sample.gpu_idle_wait_ms = profiler.gpu_idle_wait_ms;
    sample.gpu_idle_wait_max_ms = profiler.gpu_idle_wait_max_ms;
    sample.billboard_requests = profiler.billboard_requests.per_frame;
    sample.billboard_queued = profiler.billboard_queued.per_frame;
    sample.billboard_rejected = profiler.billboard_rejected.per_frame;
    sample.billboard_suppressed = profiler.billboard_suppressed.per_frame;
    sample.billboard_commands = profiler.billboard_commands.per_frame;
    sample.billboard_vertices = profiler.billboard_vertices.per_frame;
    sample.billboard_stream_bytes =
        profiler.billboard_stream_bytes.per_frame;
    sample.billboard_compatible_joins =
        profiler.billboard_compatible_joins.per_frame;
    sample.billboard_texture_lookups =
        profiler.billboard_texture_lookups.per_frame;
    sample.billboard_texture_misses =
        profiler.billboard_texture_misses.per_frame;
    sample.billboard_legacy_skips =
        profiler.billboard_legacy_skips.per_frame;
    sample.billboard_ideal_draws = std::max(
        0.0, sample.billboard_commands - sample.billboard_compatible_joins);
    sample.sky_commands = profiler.sky_commands.per_frame;
    sample.sky_vertices = profiler.sky_vertices.per_frame;
    sample.sky_stream_bytes = profiler.sky_stream_bytes.per_frame;
    sample.sky_compatible_joins = profiler.sky_compatible_joins.per_frame;
    sample.sky_points_transformed = profiler.sky_points_transformed.per_frame;
    sample.sky_cells_tested = profiler.sky_cells_tested.per_frame;
    sample.sky_visible_cells = profiler.sky_visible_cells.per_frame;
    sample.sky_horizon_quads = profiler.sky_horizon_quads.per_frame;
    sample.sky_dome_suppressed = profiler.sky_dome_suppressed.per_frame;
    sample.sky_horizon_suppressed = profiler.sky_horizon_suppressed.per_frame;
    sample.sky_ideal_draws = std::max(
        0.0, sample.sky_commands - sample.sky_compatible_joins);
    sample.mapput_candidates = profiler.mapput_candidates.per_frame;
    sample.mapput_drawable = profiler.mapput_drawable.per_frame;
    sample.mapput_comparisons = profiler.mapput_comparisons.per_frame;
    sample.mapput_callbacks = profiler.mapput_callbacks.per_frame;
    sample.mapput_object_draws = profiler.mapput_object_draws.per_frame;
    sample.billboard_stream_kib = sample.billboard_stream_bytes / 1024.0;
    sample.sky_stream_kib = sample.sky_stream_bytes / 1024.0;
    sample.world_stream_kib =
        sample.billboard_stream_kib + sample.sky_stream_kib;

    g_profiler_history[(size_t)g_profiler_history_next] = sample;
    g_profiler_history_next =
        (g_profiler_history_next + 1) % kProfilerHistoryCapacity;
    g_profiler_history_count = std::min(g_profiler_history_count + 1,
                                        kProfilerHistoryCapacity);
}

int ProfilerHistoryOffset()
{
    return g_profiler_history_count == kProfilerHistoryCapacity
        ? g_profiler_history_next : 0;
}

const ProfilerHistorySample &OldestProfilerHistorySample()
{
    return g_profiler_history[(size_t)ProfilerHistoryOffset()];
}

const ProfilerHistorySample &NewestProfilerHistorySample()
{
    const int newest =
        (g_profiler_history_next + kProfilerHistoryCapacity - 1) %
        kProfilerHistoryCapacity;
    return g_profiler_history[(size_t)newest];
}

using ProfilerMetric = double ProfilerHistorySample::*;

void SetupProfilerHistoryAxes(const char *y_axis_label = "Milliseconds")
{
    const double first = OldestProfilerHistorySample().time_seconds;
    const double last = NewestProfilerHistorySample().time_seconds;
    ImPlot::SetupAxis(ImAxis_X1, "Elapsed time (s)");
    ImPlot::SetupAxis(ImAxis_Y1, y_axis_label,
                      ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);
    ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, HUGE_VAL);
    if (g_follow_performance_history)
    {
        ImPlot::SetupAxisLimits(ImAxis_X1, first,
                                last > first ? last : first + 1.0,
                                ImPlotCond_Always);
    }
}

void PlotProfilerHistory(const char *label, ProfilerMetric metric)
{
    const ProfilerHistorySample &first = g_profiler_history[0];
    ImPlotSpec spec{};
    spec.LineWeight = 2.0f;
    spec.Offset = ProfilerHistoryOffset();
    spec.Stride = (int)sizeof(ProfilerHistorySample);
    ImPlot::PlotLine(label, &first.time_seconds, &(first.*metric),
                     g_profiler_history_count, spec);
}

void DrawFrameTimingGraph()
{
    if (ImPlot::BeginPlot("##RosePanFrameTiming", ImVec2(-1.0f, -1.0f)))
    {
        SetupProfilerHistoryAxes();
        PlotProfilerHistory("Frame average", &ProfilerHistorySample::frame_ms);
        PlotProfilerHistory("Frame max", &ProfilerHistorySample::frame_max_ms);
        PlotProfilerHistory("Workload average",
                            &ProfilerHistorySample::workload_ms);
        PlotProfilerHistory("Workload max",
                            &ProfilerHistorySample::workload_max_ms);
        PlotProfilerHistory("Game CPU", &ProfilerHistorySample::game_cpu_ms);
        PlotProfilerHistory("Renderer", &ProfilerHistorySample::renderer_ms);
        PlotProfilerHistory("VBlank/gate",
                            &ProfilerHistorySample::vblank_wait_ms);
        PlotProfilerHistory("Pacing target",
                            &ProfilerHistorySample::pacing_target_ms);
        ImPlot::EndPlot();
    }
}

void DrawGameCpuGraph()
{
    if (ImPlot::BeginPlot("##RosePanGameCpu", ImVec2(-1.0f, -1.0f)))
    {
        SetupProfilerHistoryAxes();
        PlotProfilerHistory("SGD", &ProfilerHistorySample::sgd_cpu_ms);
        PlotProfilerHistory("Mesh bridge",
                            &ProfilerHistorySample::mesh_cpu_ms);
        PlotProfilerHistory("Colour/light",
                            &ProfilerHistorySample::mesh_lighting_ms);
        PlotProfilerHistory("VUVN/skin",
                            &ProfilerHistorySample::mesh_skinning_ms);
        PlotProfilerHistory("Texture miss",
                            &ProfilerHistorySample::texture_miss_ms);
        ImPlot::EndPlot();
    }
}

void DrawRendererGraph()
{
    if (ImPlot::BeginPlot("##RosePanRenderer", ImVec2(-1.0f, -1.0f)))
    {
        SetupProfilerHistoryAxes();
        PlotProfilerHistory("Renderer total",
                            &ProfilerHistorySample::renderer_ms);
        PlotProfilerHistory("Upload",
                            &ProfilerHistorySample::renderer_upload_ms);
        PlotProfilerHistory("Record",
                            &ProfilerHistorySample::renderer_record_ms);
        PlotProfilerHistory("Acquire",
                            &ProfilerHistorySample::renderer_acquire_ms);
        PlotProfilerHistory("Submit",
                            &ProfilerHistorySample::renderer_submit_ms);
        PlotProfilerHistory("GPU idle wait",
                            &ProfilerHistorySample::gpu_idle_wait_ms);
        ImPlot::EndPlot();
    }
}

void DrawHitchGraphs()
{
    ImGui::TextDisabled(
        "Worst logical frame in each ~1 s window. Inclusive phases overlap.");
    if (ImPlot::BeginPlot("Game/transition max##RosePanHitchGame",
                          ImVec2(-1.0f, 255.0f)))
    {
        SetupProfilerHistoryAxes();
        PlotProfilerHistory("Frame", &ProfilerHistorySample::frame_max_ms);
        PlotProfilerHistory("Game CPU",
                            &ProfilerHistorySample::game_cpu_max_ms);
        PlotProfilerHistory("Room load",
                            &ProfilerHistorySample::room_load_max_ms);
        PlotProfilerHistory("Room init",
                            &ProfilerHistorySample::room_init_max_ms);
        PlotProfilerHistory("Furniture init",
                            &ProfilerHistorySample::room_furn_max_ms);
        PlotProfilerHistory("Model remap",
                            &ProfilerHistorySample::room_model_init_max_ms);
        PlotProfilerHistory("Registration/hit",
                            &ProfilerHistorySample::room_registration_max_ms);
        PlotProfilerHistory("SGD", &ProfilerHistorySample::sgd_cpu_max_ms);
        PlotProfilerHistory("GS image replay",
                            &ProfilerHistorySample::gs_upload_max_ms);
        PlotProfilerHistory("Texture invalidation",
                            &ProfilerHistorySample::texture_invalidate_max_ms);
        PlotProfilerHistory("Texture miss",
                            &ProfilerHistorySample::texture_miss_max_ms);
        PlotProfilerHistory("Mesh bridge",
                            &ProfilerHistorySample::mesh_cpu_max_ms);
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Renderer max##RosePanHitchRenderer",
                          ImVec2(-1.0f, 190.0f)))
    {
        SetupProfilerHistoryAxes();
        PlotProfilerHistory("Renderer",
                            &ProfilerHistorySample::renderer_max_ms);
        PlotProfilerHistory("Upload",
                            &ProfilerHistorySample::renderer_upload_max_ms);
        PlotProfilerHistory("Record",
                            &ProfilerHistorySample::renderer_record_max_ms);
        PlotProfilerHistory("Acquire",
                            &ProfilerHistorySample::renderer_acquire_max_ms);
        PlotProfilerHistory("Submit",
                            &ProfilerHistorySample::renderer_submit_max_ms);
        PlotProfilerHistory("Explicit GPU idle",
                            &ProfilerHistorySample::gpu_idle_wait_max_ms);
        ImPlot::EndPlot();
    }
}

void DrawWorldObjectsGraphs()
{
    ImGui::TextDisabled(
        "CPU phases are inclusive and may overlap; do not add the curves.");
    if (ImPlot::BeginPlot("CPU timings##RosePanWorldObjectTiming",
                          ImVec2(-1.0f, 165.0f)))
    {
        SetupProfilerHistoryAxes();
        PlotProfilerHistory("Billboard packet avg",
                            &ProfilerHistorySample::billboard_cpu_ms);
        PlotProfilerHistory("Billboard packet max",
                            &ProfilerHistorySample::billboard_cpu_max_ms);
        PlotProfilerHistory(
            "Billboard host bridge avg",
            &ProfilerHistorySample::billboard_host_bridge_ms);
        PlotProfilerHistory(
            "Billboard host bridge max",
            &ProfilerHistorySample::billboard_host_bridge_max_ms);
        PlotProfilerHistory("Sky avg", &ProfilerHistorySample::sky_cpu_ms);
        PlotProfilerHistory("Sky max",
                            &ProfilerHistorySample::sky_cpu_max_ms);
        PlotProfilerHistory("MapPut sort avg",
                            &ProfilerHistorySample::mapput_sort_ms);
        PlotProfilerHistory("MapPut sort max",
                            &ProfilerHistorySample::mapput_sort_max_ms);
        PlotProfilerHistory("MapPut callbacks avg",
                            &ProfilerHistorySample::mapput_callback_ms);
        PlotProfilerHistory("MapPut callbacks max",
                            &ProfilerHistorySample::mapput_callback_max_ms);
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Activity##RosePanWorldObjectActivity",
                          ImVec2(-1.0f, 165.0f)))
    {
        SetupProfilerHistoryAxes("Objects or commands / frame");
        PlotProfilerHistory("Billboard commands",
                            &ProfilerHistorySample::billboard_commands);
        PlotProfilerHistory("Billboard ideal draws",
                            &ProfilerHistorySample::billboard_ideal_draws);
        PlotProfilerHistory("Sky commands",
                            &ProfilerHistorySample::sky_commands);
        PlotProfilerHistory("Sky ideal draws",
                            &ProfilerHistorySample::sky_ideal_draws);
        PlotProfilerHistory("MapPut candidates",
                            &ProfilerHistorySample::mapput_candidates);
        PlotProfilerHistory("MapPut drawable",
                            &ProfilerHistorySample::mapput_drawable);
        PlotProfilerHistory("MapPut callbacks",
                            &ProfilerHistorySample::mapput_callbacks);
        ImPlot::EndPlot();
    }

    ImGui::TextDisabled(
        "Ideal draws = commands - compatible adjacent joins.");
    if (ImPlot::BeginPlot("MapPut sort work##RosePanMapPutComparisons",
                          ImVec2(-1.0f, 120.0f)))
    {
        SetupProfilerHistoryAxes("Comparisons / frame");
        PlotProfilerHistory("MapPut comparisons",
                            &ProfilerHistorySample::mapput_comparisons);
        ImPlot::EndPlot();
    }
    if (ImPlot::BeginPlot("Queued stream payload##RosePanWorldObjectUpload",
                          ImVec2(-1.0f, 165.0f)))
    {
        SetupProfilerHistoryAxes("KiB / frame");
        PlotProfilerHistory("Billboards",
                            &ProfilerHistorySample::billboard_stream_kib);
        PlotProfilerHistory("Sky",
                            &ProfilerHistorySample::sky_stream_kib);
        PlotProfilerHistory("Combined",
                            &ProfilerHistorySample::world_stream_kib);
        ImPlot::EndPlot();
    }
}

void DrawRendererDebugViewItem(const char *label, unsigned int flag)
{
    unsigned int flags = RosePan_RendererGetDebugViewFlags();
    bool enabled = (flags & flag) != 0;
    if (!ImGui::MenuItem(label, nullptr, &enabled))
    {
        return;
    }

    flags = enabled ? flags | flag : flags & ~flag;
    RosePan_RendererSetDebugViewFlags(flags);
}

/*
 * Anti-aliasing and texture filtering.
 *
 * Its own menu under Render resolution because that is what both settings are
 * really about -- what happens to the picture between the geometry and the
 * pixels -- and because both carry a cost the rows have to say out loud.  MSAA
 * especially: the usual "4x is nearly free" intuition comes from renderers
 * that draw the frame in one pass, and this one does not whenever an effect
 * samples the frame buffer.
 */
void DrawAntiAliasingMenu()
{
    if (!ImGui::BeginMenu("Anti-aliasing / filtering"))
    {
        return;
    }

    const int msaa = RosePan_RendererGetMsaaSamples();
    const int msaa_active = RosePan_RendererGetMsaaActiveSamples();
    const int msaa_max = RosePan_RendererGetMsaaMaxSamples();

    /*
     * Both lists label their rows "Off", "2x", "4x", "8x", and both live in
     * this one popup -- so without a scope of their own the two sets share
     * four ImGui IDs.  1.92's conflict detector says so out loud, and behind
     * the warning the rows really do fight over the active-ID state.
     */
    ImGui::TextDisabled("MSAA (final image)");
    ImGui::PushID("msaa");
    for (int samples = 1; samples <= 8; samples *= 2)
    {
        char label[32];
        if (samples == 1)
        {
            SDL_strlcpy(label, "Off", sizeof(label));
        }
        else
        {
            SDL_snprintf(label, sizeof(label), "%dx", samples);
        }
        const bool supported = samples <= msaa_max;
        ImGui::BeginDisabled(!supported);
        if (ImGui::MenuItem(label, nullptr, msaa == samples))
        {
            RosePan_RendererSetMsaaSamples(samples);
        }
        ImGui::EndDisabled();
    }
    ImGui::PopID();
    if (msaa_active != msaa)
    {
        /* Only when the two differ, so the usual case says nothing.  They
         * differ when the device clamped the request, or when the target
         * failed to allocate and the frame fell back. */
        ImGui::TextDisabled("  running at %dx (device maximum %dx)",
                            msaa_active, msaa_max);
    }
    ImGui::TextDisabled("  smooths geometry edges in the frame the game is");
    ImGui::TextDisabled("  drawn into. Everything read back out of that frame");
    ImGui::TextDisabled("  -- photographs, the pause still, effect grabs --");
    ImGui::TextDisabled("  sees the resolved copy, so none of them changes.");
    ImGui::TextDisabled("  Cost: this game samples the frame buffer for its");
    ImGui::TextDisabled("  effects, and each such grab forces a full resolve,");
    ImGui::TextDisabled("  so an effect-heavy scene costs far more than a");
    ImGui::TextDisabled("  plain one. While it is on, the depth read-back is");
    ImGui::TextDisabled("  unavailable and a ghost can stay a camera target");
    ImGui::TextDisabled("  through a wall.");

    ImGui::Separator();

    const int aniso = RosePan_RendererGetAnisotropy();
    const int aniso_active = RosePan_RendererGetAnisotropyActive();

    ImGui::TextDisabled("Anisotropic filtering (all textures)");
    ImGui::PushID("aniso");
    for (int taps = 1; taps <= 16; taps *= 2)
    {
        char label[32];
        if (taps == 1)
        {
            SDL_strlcpy(label, "Off", sizeof(label));
        }
        else
        {
            SDL_snprintf(label, sizeof(label), "%dx", taps);
        }
        if (ImGui::MenuItem(label, nullptr, aniso == taps))
        {
            RosePan_RendererSetAnisotropy(taps);
        }
    }
    ImGui::PopID();
    if (aniso_active != aniso)
    {
        ImGui::TextDisabled("  running at %dx (device refused the request)",
                            aniso_active);
    }
    ImGui::TextDisabled("  stops tatami and corridor floors shimmering at");
    ImGui::TextDisabled("  grazing angles. No extra passes, no extra memory.");
    ImGui::TextDisabled("  Only where the PS2 asked for linear filtering in");
    ImGui::TextDisabled("  both directions, so point-sampled art is untouched.");
    ImGui::TextDisabled("  The port keeps one mip level per GS page, so this");
    ImGui::TextDisabled("  supersamples the texel footprint rather than");
    ImGui::TextDisabled("  picking a mip -- it helps the near and middle of a");
    ImGui::TextDisabled("  floor much more than the far end of one.");

    ImGui::EndMenu();
}

/*
 * Screen effects a player may want off.
 *
 * Sits beside the viewfinder surround rather than under the debug views
 * because, like it, it changes the picture on purpose -- and unlike the debug
 * views it is meant to be left where the player puts it.
 */
void DrawEffectsMenu()
{
    if (!ImGui::BeginMenu("Screen effects"))
    {
        return;
    }

    const int grain = RosePan_RendererGetFilmGrain();

    ImGui::TextDisabled("Film grain");
    if (ImGui::MenuItem("Off", nullptr, grain == ROSEPAN_FILM_GRAIN_OFF))
    {
        RosePan_RendererSetFilmGrain(ROSEPAN_FILM_GRAIN_OFF);
    }
    if (ImGui::MenuItem("Native resolution", nullptr,
                        grain == ROSEPAN_FILM_GRAIN_NATIVE))
    {
        RosePan_RendererSetFilmGrain(ROSEPAN_FILM_GRAIN_NATIVE);
    }
    ImGui::TextDisabled("  one grain per screen pixel, whatever the");
    ImGui::TextDisabled("  resolution -- same density and drift as the PS2");
    if (ImGui::MenuItem("PS2 (128x128 sheet)", nullptr,
                        grain == ROSEPAN_FILM_GRAIN_PS2))
    {
        RosePan_RendererSetFilmGrain(ROSEPAN_FILM_GRAIN_PS2);
    }
    ImGui::TextDisabled("  faithful: one grain per 640x448 pixel, so each");
    ImGui::TextDisabled("  one arrives as big as your upscale makes it");

    /*
     * Cut-out edge sharpening.  Sits here rather than under a debug view
     * because, like the grain above it, it is a playing preference whose
     * faithful setting is not the best-looking one at a modern
     * resolution -- and the row says so, since 0 is what the GS did.
     */
    ImGui::Separator();
    ImGui::TextDisabled("Cut-out edges");

    float sharpen = RosePan_RendererGetAlphaSharpen();
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::SliderFloat("Sharpen", &sharpen, 0.0f, 1.0f, "%.2f"))
    {
        RosePan_RendererSetAlphaSharpen(sharpen);
    }
    ImGui::TextDisabled("  restores the one-texel alpha edge on foliage,");
    ImGui::TextDisabled("  fences and hair that magnification softens");
    ImGui::TextDisabled("  0.00 is the hardware's own edge; no effect at");
    ImGui::TextDisabled("  or below native resolution whatever it is set to");

    float cutoff = RosePan_RendererGetAlphaCutoff();
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::SliderFloat("Hard cutoff", &cutoff, 0.0f, 1.0f, "%.2f"))
    {
        RosePan_RendererSetAlphaCutoff(cutoff);
    }
    ImGui::TextDisabled("  discards any texel below this outright -- a");
    ImGui::TextDisabled("  blunter tool than Sharpen, and independent of");
    ImGui::TextDisabled("  it. Aliases the edge, applies at every");
    ImGui::TextDisabled("  resolution, and can thin hair and far foliage.");
    ImGui::TextDisabled("  0.00 off; 0.50 is the usual cut-out midpoint.");

    ImGui::EndMenu();
}

/*
 * The viewfinder surround.
 *
 * Its own menu rather than a row under the debug views, because it is a
 * playing preference and not a diagnostic -- and it is the only entry here
 * that changes what the game looks like on purpose rather than as a side
 * effect, so the note about it not being what the PS2 drew belongs on screen
 * and not only in the source.
 */
void DrawFinderMenu()
{
    if (!ImGui::BeginMenu("Viewfinder"))
    {
        return;
    }

    int enable = 0;
    float blur = 0.0f;
    float darken = 0.0f;
    float tint = 0.0f;
    float scale = 1.0f;
    RosePan_RendererGetFinderMask(&enable, &blur, &darken, &tint, &scale);

    bool changed = false;
    if (ImGui::MenuItem("Treat the world outside 640x448", nullptr,
                        enable != 0))
    {
        enable = enable != 0 ? 0 : 1;
        changed = true;
    }
    ImGui::TextDisabled("  a port addition -- the PS2 rendered 640x448");
    ImGui::TextDisabled("  and nothing beyond it. No effect on a 4:3");
    ImGui::TextDisabled("  window, where there is nothing outside.");

    ImGui::Separator();
    /* Greyed rather than hidden while the effect is off, so switching it on
     * does not move the next row under the cursor. */
    ImGui::BeginDisabled(enable == 0);
    {
        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Blur", &blur, 0.0f, 1.0f, "%.2f");
        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Darken", &darken, 0.0f, 1.0f, "%.2f");
        ImGui::TextDisabled("  darken 1.00 with blur 0.00 is a black");
        ImGui::TextDisabled("  surround rather than a defocused one");

        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Crimson", &tint, 0.0f, 1.0f, "%.2f");

        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Frame size", &scale, 0.25f, 2.0f,
                                      "%.2f");
        ImGui::TextDisabled("  1.00 is the original 640x448 frame; below");
        ImGui::TextDisabled("  it the surround covers game composition");
    }
    ImGui::EndDisabled();

    if (changed)
    {
        RosePan_RendererSetFinderMask(enable, blur, darken, tint, scale);
    }

    ImGui::EndMenu();
}

/*
 * HDR output and the display grade.
 *
 * Laid out in the order a player should actually reach for them: pick a mode,
 * see what the display says it can do, then set paper white -- which is the
 * brightness control that matters and is not the same knob as the grade's
 * brightness further down.  The expansion pair is below that because it is the
 * only thing here that invents information the source never had, and the
 * faithful setting for it is 0.
 */
void DrawHdrMenu()
{
    if (!ImGui::BeginMenu("HDR and display"))
    {
        return;
    }

    struct HdrChoice
    {
        const char *label;
        int mode;
    };
    static const HdrChoice kModes[] = {
        {"Off (SDR)", ROSEPAN_HDR_OFF},
        {"Auto (use HDR when the display has it)", ROSEPAN_HDR_AUTO},
        {"Force scRGB (extended linear)", ROSEPAN_HDR_SCRGB},
        {"Force HDR10 (PQ)", ROSEPAN_HDR_HDR10},
    };

    const int mode = RosePan_RendererGetHdrMode();
    for (const HdrChoice &choice : kModes)
    {
        if (ImGui::MenuItem(choice.label, nullptr, mode == choice.mode))
        {
            RosePan_RendererSetHdrMode(choice.mode);
        }
    }

    /* What the swapchain actually got, which is not always what was asked
     * for: AUTO on an SDR monitor, or a forced composition the display
     * refused, both land here as "SDR". */
    const bool active = RosePan_RendererGetHdrActive() != 0;
    float display_sdr_white = 0.0f;
    float display_peak = 0.0f;
    RosePan_RendererGetHdrDisplayInfo(&display_sdr_white, &display_peak);

    ImGui::TextDisabled("  output: %s", active ? "HDR" : "SDR");
    if (display_sdr_white > 0.0f)
    {
        ImGui::TextDisabled("  display: SDR white %.0f nits, peak %.0f nits",
                            (double)display_sdr_white, (double)display_peak);
    }
    else
    {
        ImGui::TextDisabled("  display: no HDR headroom reported");
    }
    if (active && RosePan_RendererHdrIsFullPrecision() == 0)
    {
        /* The one thing a live toggle cannot fix: the scene target's format is
         * decided when the pipelines are built. */
        ImGui::TextDisabled("  8-bit scene; restart with HDR on for 10-bit");
    }

    /* Every HDR control below stays on screen whatever it is set to, greyed
     * rather than hidden.  A menu whose rows appear and disappear as you click
     * them moves the next row under the cursor, which is how one click becomes
     * two -- and the auto toggles are exactly the rows that would do it. */
    float effective_paper = 0.0f;
    float effective_peak = 0.0f;
    RosePan_RendererGetHdrEffective(&effective_paper, &effective_peak);

    ImGui::Separator();
    ImGui::TextDisabled("Brightness (HDR)");
    ImGui::BeginDisabled(!active);
    {
        const float paper = RosePan_RendererGetHdrPaperWhite();
        const bool paper_auto = paper <= 0.0f;

        /* On auto the slider shows the value the shader is actually using, so
         * "follow OS" reads as a number rather than as a blank. */
        float paper_value = paper_auto ? effective_paper : paper;
        ImGui::BeginDisabled(paper_auto);
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderFloat("Paper white", &paper_value, 50.0f, 1000.0f,
                               "%.0f nits") &&
            !paper_auto)
        {
            RosePan_RendererSetHdrPaperWhite(paper_value);
        }
        ImGui::EndDisabled();
        /* MenuItem's by-value overload, and the toggle written out: the
         * bool* overload flips the caller's variable after drawing the tick
         * from its old value, which reads one frame stale right here where
         * the same variable also chooses what is drawn below. */
        if (ImGui::MenuItem("   follow the OS SDR white level", nullptr,
                            paper_auto))
        {
            RosePan_RendererSetHdrPaperWhite(paper_auto ? 200.0f : 0.0f);
        }

        const float peak = RosePan_RendererGetHdrPeak();
        const bool peak_auto = peak <= 0.0f;

        float peak_value = peak_auto ? effective_peak : peak;
        ImGui::BeginDisabled(peak_auto);
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderFloat("Peak", &peak_value, 100.0f, 4000.0f,
                               "%.0f nits") &&
            !peak_auto)
        {
            RosePan_RendererSetHdrPeak(peak_value);
        }
        ImGui::EndDisabled();
        if (ImGui::MenuItem("   follow the display's headroom", nullptr,
                            peak_auto))
        {
            RosePan_RendererSetHdrPeak(
                peak_auto ? (display_peak > 0.0f ? display_peak : 1000.0f)
                          : 0.0f);
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("Highlights (HDR)");
    {
        float strength = 0.0f;
        float knee = 0.0f;
        RosePan_RendererGetHdrExpansion(&strength, &knee);

        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderFloat("Expansion", &strength, 0.0f, 1.0f, "%.2f"))
        {
            RosePan_RendererSetHdrExpansion(strength, knee);
        }
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderFloat("Knee", &knee, 0.5f, 1.0f, "%.2f"))
        {
            RosePan_RendererSetHdrExpansion(strength, knee);
        }
        ImGui::TextDisabled("  0 shows the SDR image at paper white and");
        ImGui::TextDisabled("  invents nothing -- the faithful setting.");
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextDisabled("Display grade (SDR and HDR)");
    {
        float brightness = 1.0f;
        float contrast = 1.0f;
        float gamma = 1.0f;
        float saturation = 1.0f;
        RosePan_RendererGetGrade(&brightness, &contrast, &gamma, &saturation);

        bool changed = false;
        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Brightness", &brightness, 0.25f, 2.0f,
                                      "%.2f");
        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Contrast", &contrast, 0.5f, 2.0f,
                                      "%.2f");
        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Gamma", &gamma, 0.5f, 2.5f, "%.2f");
        ImGui::SetNextItemWidth(180.0f);
        changed |= ImGui::SliderFloat("Saturation", &saturation, 0.0f, 2.0f,
                                      "%.2f");
        if (changed)
        {
            RosePan_RendererSetGrade(brightness, contrast, gamma, saturation);
        }

        if (ImGui::MenuItem("Reset grade"))
        {
            RosePan_RendererSetGrade(1.0f, 1.0f, 1.0f, 1.0f);
        }
        ImGui::TextDisabled("  all four at 1.00 skips the output pass");
    }

    ImGui::EndMenu();
}

/*
 * Persist settings that changed, wherever they changed.
 *
 * Comparing the captured struct rather than hooking each menu item means one
 * place covers all of them, including changes that never went through a menu at
 * all -- Alt+Enter is the case that matters today.
 *
 * Debounced: dragging a slider changes the value every frame, and writing the
 * file 60 times a second for the length of a drag would be silly.  Half a second
 * of quiet is the settle point.
 */
constexpr Uint64 kSettingsSettleMs = 500;
Uint64 g_settings_dirty_since;

/* Arm the debounce by hand.  The compare below only watches
 * RosePan_ConfigRenderer, and the control bindings deliberately do not live
 * there -- rosepan_input owns them -- so a rebinding has to say so itself. */
void MarkSettingsDirty()
{
    const Uint64 now = SDL_GetTicks();
    g_settings_dirty_since = now != 0 ? now : 1;
}

void PersistSettingsIfChanged()
{
    /* Every field is 4 bytes, so the struct has no padding and memcmp is a
     * valid comparison.  The assert is here to fail loudly if a future field
     * breaks that rather than letting the compare read indeterminate bytes. */
    static_assert(sizeof(RosePan_ConfigRenderer) == 35 * sizeof(int),
                  "RosePan_ConfigRenderer gained padding; compare by field");

    RosePan_ConfigRenderer before = rosepan_config.renderer;
    RosePan_ConfigCapture();

    const Uint64 now = SDL_GetTicks();
    if (std::memcmp(&before, &rosepan_config.renderer, sizeof(before)) != 0)
    {
        g_settings_dirty_since = now != 0 ? now : 1;
        return;
    }

    if (g_settings_dirty_since != 0 &&
        now - g_settings_dirty_since >= kSettingsSettleMs)
    {
        g_settings_dirty_since = 0;
        RosePan_ConfigSave();
    }
}

/* ---------------------------------------------------------------------------
 *  Controls -- rebinding.
 *
 *  The 16 PS2 buttons and 4 stick axes are the whole vocabulary the game reads
 *  through, so a change made here moves that control everywhere at once:
 *  gameplay, menus, and the raw pad bitmask tests alike.  Nothing above
 *  rosepan_input knows a rebinding happened.
 * ------------------------------------------------------------------------ */

const char *const kControlButtonLabels[ROSEPAN_PAD_BUTTON_COUNT] = {
    "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right",
    "Triangle", "Cross",      "Square",     "Circle",
    "L1",       "L2",         "R1",         "R2",
    "Start",    "Select",     "L3",         "R3",
};

const char *const kControlStickLabels[ROSEPAN_PAD_STICK_COUNT] = {
    "Left stick X", "Left stick Y", "Right stick X", "Right stick Y",
};

enum CaptureKind
{
    kCaptureNone = 0,
    kCaptureButton,
    kCaptureStickAnalog,
    kCaptureStickNeg,
    kCaptureStickPos,
};

int g_capture_kind = kCaptureNone;
int g_capture_index;
int g_capture_slot;

bool CaptureActive()
{
    return g_capture_kind != kCaptureNone;
}

void CancelCapture()
{
    g_capture_kind = kCaptureNone;
    RosePan_InputSetSuppressed(0);
}

void BeginCapture(int kind, int index, int slot)
{
    g_capture_kind = kind;
    g_capture_index = index;
    g_capture_slot = slot;

    /* The game polls SDL directly rather than reading the event queue, so
     * without this the key being chosen is also pressed in-game while the
     * player is choosing it. */
    RosePan_InputSetSuppressed(1);
}

void ApplyCapture(const RosePan_InputSource &src)
{
    if (g_capture_kind == kCaptureButton)
    {
        RosePan_InputButtonBinding binding;
        if (RosePan_InputGetButtonBinding(g_capture_index, &binding))
        {
            binding.src[g_capture_slot] = src;
            RosePan_InputSetButtonBinding(g_capture_index, &binding);
        }
    }
    else
    {
        RosePan_InputStickBinding binding;
        if (RosePan_InputGetStickBinding(g_capture_index, &binding))
        {
            if (g_capture_kind == kCaptureStickAnalog)
            {
                binding.analog = src;
            }
            else if (g_capture_kind == kCaptureStickNeg)
            {
                binding.neg = src;
            }
            else
            {
                binding.pos = src;
            }
            RosePan_InputSetStickBinding(g_capture_index, &binding);
        }
    }

    MarkSettingsDirty();
    CancelCapture();
}

/*
 * Take one event as a binding.  Returns true when it was consumed -- the
 * caller must then keep it away from ImGui, which has keyboard and gamepad nav
 * enabled and would otherwise re-activate the very button that armed the
 * capture the moment Space or A is pressed.
 */
bool HandleCaptureEvent(const SDL_Event *event)
{
    if (!CaptureActive() || event == nullptr)
    {
        return false;
    }

    RosePan_InputSource src;
    std::memset(&src, 0, sizeof(src));

    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat)
    {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE)
        {
            CancelCapture();
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_BACKSPACE)
        {
            ApplyCapture(src); /* kind NONE -- clears the slot */
            return true;
        }
        src.kind = ROSEPAN_SRC_KEY;
        src.code = (int)event->key.scancode;
    }
    else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        src.kind = ROSEPAN_SRC_PAD_BUTTON;
        src.code = (int)event->gbutton.button;
    }
    else if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
    {
        /* Half travel, so a stick resting off-centre or a trigger brushed on
         * the way to a button does not bind itself. */
        const int value = (int)event->gaxis.value;
        if (value > -16000 && value < 16000)
        {
            return false;
        }
        src.kind = ROSEPAN_SRC_PAD_AXIS;
        src.code = (int)event->gaxis.axis;
        src.dir = value < 0 ? -1 : 1;
        src.threshold = 8000;
    }
    else
    {
        return false;
    }

    ApplyCapture(src);
    return true;
}

void ControlSourceText(const RosePan_InputSource &src, char *buf, int len)
{
    if (src.kind == ROSEPAN_SRC_NONE ||
        RosePan_InputSourceToString(&src, buf, len) == 0)
    {
        SDL_snprintf(buf, (size_t)len, "unbound");
    }
}

bool ControlSourcesEqual(const RosePan_InputSource &a,
                         const RosePan_InputSource &b)
{
    return a.kind == b.kind && a.code == b.code && a.dir == b.dir;
}

/*
 * Which other button already answers to this source, or -1.  Sharing one is
 * legal -- a button's sources are OR'd, so both would simply fire -- but it is
 * almost always a mistake, so it is worth pointing at rather than refusing.
 */
int ControlConflict(const RosePan_InputSource &src, int except_button)
{
    if (src.kind == ROSEPAN_SRC_NONE)
    {
        return -1;
    }
    for (int i = 0; i < ROSEPAN_PAD_BUTTON_COUNT; i++)
    {
        if (i == except_button)
        {
            continue;
        }
        RosePan_InputButtonBinding other;
        if (!RosePan_InputGetButtonBinding(i, &other))
        {
            continue;
        }
        for (int s = 0; s < ROSEPAN_INPUT_MAX_SOURCES; s++)
        {
            if (ControlSourcesEqual(other.src[s], src))
            {
                return i;
            }
        }
    }
    return -1;
}

void DrawCaptureCell(const char *id, const RosePan_InputSource &src, bool waiting,
                     int conflict_with, int kind, int index, int slot)
{
    ImGui::PushID(id);

    if (waiting)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.16f, 0.18f, 1.0f));
        if (ImGui::Button("press...", ImVec2(-FLT_MIN, 0.0f)))
        {
            CancelCapture();
        }
        ImGui::PopStyleColor();
    }
    else
    {
        char text[96];
        ControlSourceText(src, text, sizeof(text));

        const bool unbound = src.kind == ROSEPAN_SRC_NONE;
        if (unbound)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }
        else if (conflict_with >= 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.58f, 0.36f, 1.0f));
        }

        if (ImGui::Button(text, ImVec2(-FLT_MIN, 0.0f)))
        {
            BeginCapture(kind, index, slot);
        }

        if (unbound || conflict_with >= 0)
        {
            ImGui::PopStyleColor();
        }
        if (conflict_with >= 0 && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Also bound to %s -- both will fire",
                              kControlButtonLabels[conflict_with]);
        }
    }

    ImGui::PopID();
}

void DrawControlsWindow()
{
    if (!g_show_controls)
    {
        /* Closing the window mid-capture must not leave the game deaf. */
        if (CaptureActive())
        {
            CancelCapture();
        }
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(640.0f, 560.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("RosePan Controls", &g_show_controls))
    {
        ImGui::TextWrapped(
            "These 16 buttons and 4 stick axes are what the whole game reads "
            "through, so rebinding one here moves it everywhere at once. Click "
            "a binding, then press the key, gamepad button or trigger you want.");
        ImGui::TextDisabled(
            "Esc cancels   Backspace clears   up to %d sources per button",
            ROSEPAN_INPUT_MAX_SOURCES);

        ImGui::Separator();

        const bool defaults = RosePan_InputBindingsAreDefault() != 0;
        ImGui::BeginDisabled(defaults);
        if (ImGui::Button("Reset all to defaults"))
        {
            CancelCapture();
            RosePan_InputResetBindings();
            MarkSettingsDirty();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", defaults ? "unchanged"
                                           : "changed -- saved to rosepan.ini");

        if (CaptureActive())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.94f, 0.58f, 0.36f, 1.0f),
                               "  game input paused while binding");
        }

        ImGui::Separator();

        /* Device picker.  SDL lists every device carrying a gamepad mapping,
         * which on a desk with a wheel or a second pad is not necessarily the
         * one the player means -- and the list order is whatever the OS
         * enumerated this boot, so "the first" is not stable either. */
        {
            const char *pref = RosePan_InputGetPreferredGamepad();
            const bool  autopick = pref == nullptr || pref[0] == '\0';
            char        active_name[128];
            char        active_guid[ROSEPAN_GAMEPAD_GUID_LEN];
            const bool  have_active = RosePan_InputGetActiveGamepad(
                active_name, sizeof(active_name),
                active_guid, sizeof(active_guid)) != 0;

            char preview[160];
            if (autopick)
            {
                SDL_snprintf(preview, sizeof(preview), "Automatic%s%s",
                             have_active ? " -- " : "",
                             have_active ? active_name : "");
            }
            else
            {
                SDL_snprintf(preview, sizeof(preview), "%s",
                             have_active ? active_name : "(not connected)");
            }

            ImGui::SetNextItemWidth(320.0f);
            if (ImGui::BeginCombo("Gamepad", preview))
            {
                if (ImGui::Selectable("Automatic (first connected)", autopick))
                {
                    RosePan_InputSetPreferredGamepad(nullptr);
                    MarkSettingsDirty();
                }

                const int n = RosePan_InputGetGamepadCount();
                for (int i = 0; i < n; i++)
                {
                    char name[128];
                    char guid[ROSEPAN_GAMEPAD_GUID_LEN];
                    if (!RosePan_InputGetGamepadInfo(i, name, sizeof(name), guid,
                                                    sizeof(guid)))
                    {
                        continue;
                    }
                    ImGui::PushID(i);
                    const bool chosen =
                        !autopick && SDL_strcasecmp(guid, pref) == 0;
                    if (ImGui::Selectable(name, chosen))
                    {
                        RosePan_InputSetPreferredGamepad(guid);
                        MarkSettingsDirty();
                    }
                    ImGui::PopID();
                }

                if (n == 0)
                {
                    ImGui::TextDisabled("no gamepads connected");
                }
                ImGui::EndCombo();
            }

            if (!autopick && !have_active)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.94f, 0.58f, 0.36f, 1.0f),
                                   "waiting for it");
            }
        }

        ImGui::Separator();

        constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_BordersInnerV |
                                                ImGuiTableFlags_RowBg |
                                                ImGuiTableFlags_SizingStretchProp;

        if (ImGui::BeginTable("##controls_buttons",
                              1 + ROSEPAN_INPUT_MAX_SOURCES, kTableFlags))
        {
            ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed,
                                    104.0f);
            for (int s = 0; s < ROSEPAN_INPUT_MAX_SOURCES; s++)
            {
                char head[24];
                SDL_snprintf(head, sizeof(head), "Source %d", s + 1);
                ImGui::TableSetupColumn(head);
            }
            ImGui::TableHeadersRow();

            for (int i = 0; i < ROSEPAN_PAD_BUTTON_COUNT; i++)
            {
                RosePan_InputButtonBinding binding;
                if (!RosePan_InputGetButtonBinding(i, &binding))
                {
                    continue;
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(kControlButtonLabels[i]);

                ImGui::PushID(i);
                for (int s = 0; s < ROSEPAN_INPUT_MAX_SOURCES; s++)
                {
                    ImGui::TableNextColumn();
                    const bool waiting = g_capture_kind == kCaptureButton &&
                                         g_capture_index == i &&
                                         g_capture_slot == s;
                    char id[8];
                    SDL_snprintf(id, sizeof(id), "s%d", s);
                    DrawCaptureCell(id, binding.src[s], waiting,
                                    ControlConflict(binding.src[s], i),
                                    kCaptureButton, i, s);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Sticks");
        ImGui::TextWrapped(
            "Ramp is how many updates a held key takes to reach full "
            "deflection: 1 snaps, which is what movement wants, while a higher "
            "value makes keyboard aiming steerable instead of all-or-nothing.");

        if (ImGui::BeginTable("##controls_sticks", 5, kTableFlags))
        {
            ImGui::TableSetupColumn("Stick", ImGuiTableColumnFlags_WidthFixed,
                                    104.0f);
            ImGui::TableSetupColumn("Analog");
            ImGui::TableSetupColumn("Key toward 0");
            ImGui::TableSetupColumn("Key toward 255");
            ImGui::TableSetupColumn("Ramp", ImGuiTableColumnFlags_WidthFixed,
                                    72.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < ROSEPAN_PAD_STICK_COUNT; i++)
            {
                RosePan_InputStickBinding binding;
                if (!RosePan_InputGetStickBinding(i, &binding))
                {
                    continue;
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(kControlStickLabels[i]);

                ImGui::PushID(1000 + i);

                ImGui::TableNextColumn();
                DrawCaptureCell("an", binding.analog,
                                g_capture_kind == kCaptureStickAnalog &&
                                    g_capture_index == i,
                                -1, kCaptureStickAnalog, i, 0);

                ImGui::TableNextColumn();
                DrawCaptureCell("ng", binding.neg,
                                g_capture_kind == kCaptureStickNeg &&
                                    g_capture_index == i,
                                -1, kCaptureStickNeg, i, 0);

                ImGui::TableNextColumn();
                DrawCaptureCell("ps", binding.pos,
                                g_capture_kind == kCaptureStickPos &&
                                    g_capture_index == i,
                                -1, kCaptureStickPos, i, 0);

                ImGui::TableNextColumn();
                int ramp = binding.ramp_frames;
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragInt("##ramp", &ramp, 0.2f, 1, 30))
                {
                    binding.ramp_frames = ramp < 1 ? 1 : ramp;
                    RosePan_InputSetStickBinding(i, &binding);
                    MarkSettingsDirty();
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

/* ---------------------------------------------------------------------------
 *  Settings -- where the game's files are.
 *
 *  The one setting here that is not a preference: without a data folder the
 *  port has nothing to load.  It is worth a window rather than a menu row
 *  because it is the setting most likely to be wrong on a machine where the
 *  game's files are not beside the executable -- a phone, a console, a copy
 *  installed somewhere tidy -- and the player needs to be told which folder is
 *  in use and whether the one they picked will actually work.
 *
 *  Changing it takes effect at the next start, and says so.  By the time this
 *  window can be opened the data root has been resolved once and thousands of
 *  files have been read through it (see rosepan/io/rosepan_paths.h); repointing
 *  it mid-session would leave half the loaded game from one folder and half
 *  from another.
 * ------------------------------------------------------------------------ */

char g_data_folder_edit[1024];
bool g_data_folder_primed;
bool g_data_folder_saved;
bool g_data_folder_save_failed;

/*
 * The native folder picker answers on whichever thread the OS runs its dialog
 * on -- SDL says so explicitly -- so the callback only parks the result and
 * sets a flag.  The UI thread picks it up on its next frame; nothing is touched
 * from two threads at once.
 */
std::atomic<bool> g_folder_dialog_open;
std::atomic<bool> g_folder_pick_ready;
char              g_folder_pick[1024];

void SDLCALL DataFolderPicked(void *userdata, const char *const *filelist,
                              int filter)
{
    (void)userdata;
    (void)filter;

    /* A null list is an error and an empty one is a cancel; neither should
     * disturb what the player has typed. */
    if (filelist != nullptr && filelist[0] != nullptr)
    {
        SDL_strlcpy(g_folder_pick, filelist[0], sizeof(g_folder_pick));
        g_folder_pick_ready.store(true, std::memory_order_release);
    }

    g_folder_dialog_open.store(false, std::memory_order_release);
}

/* Hand a directory to the desktop's file manager.  Percent-encoded because a
 * path with a space in it is not a URL until it is. */
void OpenFolderInShell(const char *dir)
{
    if (dir == nullptr || dir[0] == '\0')
    {
        return;
    }

    std::string url = "file:///";
    for (const char *p = dir; *p != '\0'; p++)
    {
        const unsigned char ch = (unsigned char)*p;

        if (ch == '\\' || ch == '/')
        {
            /* One separator, and never a doubled one after the "file:///". */
            if (url.back() != '/')
            {
                url += '/';
            }
        }
        else if (ch <= 0x20 || ch >= 0x7f || std::strchr("\"#%<>?[]^`{|}", ch) != nullptr)
        {
            static const char hex[] = "0123456789ABCDEF";
            url += '%';
            url += hex[ch >> 4];
            url += hex[ch & 0x0f];
        }
        else
        {
            url += (char)ch;
        }
    }

    if (!SDL_OpenURL(url.c_str()))
    {
        SDL_Log("RosePan: could not open %s: %s", url.c_str(), SDL_GetError());
    }
}

void DrawDataFolderWindow()
{
    if (!g_show_data_folder)
    {
        g_data_folder_primed = false;
        /* Drop a pick that arrived after the window was closed rather than
         * letting it appear in the field the next time it is opened. */
        g_folder_pick_ready.store(false, std::memory_order_relaxed);
        return;
    }

    if (!g_data_folder_primed)
    {
        g_data_folder_primed = true;
        g_data_folder_saved = false;
        g_data_folder_save_failed = false;
        SDL_strlcpy(g_data_folder_edit, rosepan_config.paths.data_folder,
                    sizeof(g_data_folder_edit));
    }

    if (g_folder_pick_ready.exchange(false, std::memory_order_acquire))
    {
        SDL_strlcpy(g_data_folder_edit, g_folder_pick,
                    sizeof(g_data_folder_edit));
        g_data_folder_saved = false;
        g_data_folder_save_failed = false;
    }

    ImGui::SetNextWindowSize(ImVec2(660.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("RosePan Game Data", &g_show_data_folder))
    {
        ImGui::TextWrapped(
            "Where RosePan reads the game's files from: the disc's root, the "
            "folder with DATA in it. It can be any folder on this device -- "
            "nothing requires it to be next to RosePan.");

        ImGui::Separator();

        ImGui::TextDisabled("In use this session");
        ImGui::TextWrapped("%s", RosePan_PathDataDir());
        ImGui::TextDisabled("(%s)", RosePan_PathDataDirSource());

        const char *env = std::getenv("ROSEPAN_DATA_DIR");
        if (env != nullptr && env[0] != '\0')
        {
            ImGui::TextColored(ImVec4(0.94f, 0.58f, 0.36f, 1.0f),
                               "ROSEPAN_DATA_DIR is set in the environment and "
                               "overrides whatever is saved here.");
        }

        ImGui::Separator();
        ImGui::TextDisabled("Folder");

        const float browse_width = 100.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_width -
                                ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::InputText("##data_folder", g_data_folder_edit,
                             sizeof(g_data_folder_edit)))
        {
            g_data_folder_saved = false;
            g_data_folder_save_failed = false;
        }

        ImGui::SameLine();
        const bool picking = g_folder_dialog_open.load(std::memory_order_acquire);
        ImGui::BeginDisabled(picking);
        if (ImGui::Button(picking ? "Choosing..." : "Browse...",
                          ImVec2(browse_width, 0.0f)))
        {
            g_folder_dialog_open.store(true, std::memory_order_release);
            /* OS dialogs belong to the window's thread (rosepan_host.h); the
             * call only opens the dialog, so waiting for it costs nothing. */
            const char *start = g_data_folder_edit[0] != '\0'
                                    ? g_data_folder_edit
                                    : RosePan_PathDataDir();
            RosePan_HostRunOnMainThread(
                [](void *location) {
                    SDL_ShowOpenFolderDialog(DataFolderPicked, nullptr,
                                             g_window,
                                             (const char *)location, false);
                },
                (void *)start);
        }
        ImGui::EndDisabled();

        /* Say whether the folder holds the game before it is saved, rather than
         * letting the player find out at the next start. */
        if (g_data_folder_edit[0] == '\0')
        {
            ImGui::TextDisabled(
                "Empty: look for the game beside RosePan and above it, and save "
                "whatever is found here.");
        }
        else if (RosePan_PathLooksLikeDataDir(g_data_folder_edit))
        {
            ImGui::TextColored(ImVec4(0.45f, 0.80f, 0.45f, 1.0f),
                               "The game's files are there.");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.94f, 0.58f, 0.36f, 1.0f),
                               "No DATA/CMNTEX there. RosePan "
                               "will fall back to looking for itself.");
        }

        ImGui::Separator();

        if (ImGui::Button("Save"))
        {
            g_data_folder_saved =
                RosePan_ConfigSetDataFolder(g_data_folder_edit) != 0;
            g_data_folder_save_failed = !g_data_folder_saved;
        }
        ImGui::SameLine();
        if (ImGui::Button("Use automatic"))
        {
            g_data_folder_edit[0] = '\0';
            g_data_folder_saved = RosePan_ConfigSetDataFolder("") != 0;
            g_data_folder_save_failed = !g_data_folder_saved;
        }
        ImGui::SameLine();
        if (ImGui::Button("Open this folder"))
        {
            OpenFolderInShell(g_data_folder_edit[0] != '\0'
                                  ? g_data_folder_edit
                                  : RosePan_PathDataDir());
        }

        if (g_data_folder_saved)
        {
            ImGui::TextColored(ImVec4(0.45f, 0.80f, 0.45f, 1.0f),
                               "Saved. RosePan reads it the next time it starts.");
        }
        else if (g_data_folder_save_failed)
        {
            ImGui::TextColored(ImVec4(0.94f, 0.58f, 0.36f, 1.0f),
                               "Could not write rosepan.ini -- see the log.");
        }

        ImGui::Separator();
        ImGui::TextDisabled("Settings and saves");
        ImGui::TextWrapped("%s", RosePan_PathUserDir());
        if (ImGui::Button("Open user folder"))
        {
            OpenFolderInShell(RosePan_PathUserDir());
        }
    }
    ImGui::End();
}

/*
 * Volume.  RosePan's own, on top of the game's mix: the host mixer applies it
 * (RosePan_ConfigApplyAudio), so a drag is heard at once, and the settle in
 * PersistSettingsIfChanged writes it to rosepan.ini once the slider stops.
 */
void DrawAudioMenu()
{
    if (!ImGui::BeginMenu("Audio"))
    {
        return;
    }

    RosePan_ConfigAudio *a = &rosepan_config.audio;
    bool changed = false;

    ImGui::TextDisabled("Volume");
    ImGui::SetNextItemWidth(180.0f);
    changed |= ImGui::SliderInt("Master", &a->master, 0, 100, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetNextItemWidth(180.0f);
    changed |= ImGui::SliderInt("Music and voices", &a->music, 0, 100, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Everything the game streams: its music, and the\n"
                          "speech and ambience that share those streams;\n"
                          "movie audio too.");
    }
    ImGui::SetNextItemWidth(180.0f);
    changed |= ImGui::SliderInt("Sound effects", &a->effects, 0, 100, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextDisabled("  100%% is the level the game asks for");

    if (ImGui::MenuItem("Reset to 100%", nullptr, false,
                        a->master != 100 || a->music != 100 || a->effects != 100))
    {
        a->master = a->music = a->effects = 100;
        changed = true;
    }

    if (changed)
    {
        RosePan_ConfigApplyAudio();
        MarkSettingsDirty();
    }

    ImGui::EndMenu();
}

void DrawMainMenuBar()
{
    if (!g_show_main_menu_bar)
    {
        return;
    }
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("Profiler"))
    {
        const bool visible =
            g_show_performance_summary || g_show_performance_graphs;
        if (ImGui::MenuItem("Show profiler", nullptr, visible))
        {
            const bool show = !visible;
            g_show_performance_summary = show;
            g_show_performance_graphs = show;
        }
        ImGui::Separator();
        ImGui::MenuItem("Summary", nullptr, &g_show_performance_summary);
        ImGui::MenuItem("   FPS only", nullptr, &g_summary_fps_only,
                        g_show_performance_summary);
        ImGui::MenuItem("Graphs", nullptr, &g_show_performance_graphs);
        ImGui::Separator();
        if (ImGui::MenuItem("Pause graph history", nullptr,
                            &g_pause_performance_history) &&
            g_pause_performance_history)
        {
            g_follow_performance_history = false;
        }
        ImGui::MenuItem("Follow latest", nullptr,
                        &g_follow_performance_history);
        if (ImGui::MenuItem("Clear graph history", nullptr, false,
                            g_profiler_history_count != 0))
        {
            ClearProfilerHistory();
        }
        ImGui::Separator();
        DrawFramePacingMenu();
        DrawFrameSmoothingMenu();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Controls"))
    {
        ImGui::MenuItem("Rebind keyboard and gamepad...", nullptr,
                        &g_show_controls);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset all bindings", nullptr, false,
                            RosePan_InputBindingsAreDefault() == 0))
        {
            CancelCapture();
            RosePan_InputResetBindings();
            MarkSettingsDirty();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Renderer"))
    {
        ImGui::TextDisabled("Window");
        {
            const int window_mode = RosePan_RendererGetWindowMode();
            if (ImGui::MenuItem("Windowed", "Alt+Enter",
                                window_mode == ROSEPAN_WINDOW_MODE_WINDOWED))
            {
                RosePan_RendererSetWindowMode(ROSEPAN_WINDOW_MODE_WINDOWED);
            }
            if (ImGui::MenuItem("Borderless fullscreen", "Alt+Enter",
                                window_mode == ROSEPAN_WINDOW_MODE_BORDERLESS))
            {
                RosePan_RendererSetWindowMode(ROSEPAN_WINDOW_MODE_BORDERLESS);
            }
        }
        ImGui::Separator();

        ImGui::TextDisabled("Aspect ratio");
        {
            int aspect_mode = ROSEPAN_ASPECT_AUTO;
            float custom = 16.0f / 9.0f;
            RosePan_RendererGetAspectMode(&aspect_mode, &custom);

            struct AspectChoice
            {
                const char *label;
                int mode;
            };
            static const AspectChoice kAspects[] = {
                {"Auto (window)", ROSEPAN_ASPECT_AUTO},
                {"Original (PS2 4:3)", ROSEPAN_ASPECT_ORIGINAL},
                {"4:3", ROSEPAN_ASPECT_4_3},
                {"16:9", ROSEPAN_ASPECT_16_9},
                {"16:10", ROSEPAN_ASPECT_16_10},
                {"Custom", ROSEPAN_ASPECT_CUSTOM},
            };
            for (const AspectChoice &choice : kAspects)
            {
                if (ImGui::MenuItem(choice.label, nullptr,
                                    aspect_mode == choice.mode))
                {
                    RosePan_RendererSetAspectMode(choice.mode, custom);
                }
            }

            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::SliderFloat("Custom ratio", &custom, 0.5f, 5.0f, "%.3f"))
            {
                RosePan_RendererSetAspectMode(ROSEPAN_ASPECT_CUSTOM, custom);
            }

            int px = 0, py = 0, pw = 0, ph = 0;
            RosePan_RendererGetPresentRect(&px, &py, &pw, &ph);
            ImGui::TextDisabled("  presented at %dx%d +%d+%d", pw, ph, px, py);
        }
        ImGui::Separator();

        ImGui::TextDisabled("Render resolution");
        {
            int mode = ROSEPAN_RENDER_RES_MATCH_WINDOW;
            float scale = 1.0f;
            RosePan_RendererGetRenderResolution(&mode, &scale);

            if (ImGui::MenuItem("Match window", nullptr,
                                mode == ROSEPAN_RENDER_RES_MATCH_WINDOW))
            {
                RosePan_RendererSetRenderResolution(
                    ROSEPAN_RENDER_RES_MATCH_WINDOW, scale);
            }
            if (ImGui::MenuItem("Native PS2 (640x448 x scale)", nullptr,
                                mode == ROSEPAN_RENDER_RES_NATIVE_PS2))
            {
                RosePan_RendererSetRenderResolution(
                    ROSEPAN_RENDER_RES_NATIVE_PS2, scale);
            }
            if (ImGui::MenuItem("Window x scale", nullptr,
                                mode == ROSEPAN_RENDER_RES_WINDOW_SCALE))
            {
                RosePan_RendererSetRenderResolution(
                    ROSEPAN_RENDER_RES_WINDOW_SCALE, scale);
            }

            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::SliderFloat("Scale", &scale, 0.25f, 4.0f, "%.2fx"))
            {
                RosePan_RendererSetRenderResolution(mode, scale);
            }

            const int filter = RosePan_RendererGetUpscaleFilter();
            if (ImGui::MenuItem("Upscale: nearest (sharp)", nullptr,
                                filter == ROSEPAN_RENDER_FILTER_NEAREST))
            {
                RosePan_RendererSetUpscaleFilter(ROSEPAN_RENDER_FILTER_NEAREST);
            }
            if (ImGui::MenuItem("Upscale: linear (smooth)", nullptr,
                                filter == ROSEPAN_RENDER_FILTER_LINEAR))
            {
                RosePan_RendererSetUpscaleFilter(ROSEPAN_RENDER_FILTER_LINEAR);
            }

            int render_w = 0;
            int render_h = 0;
            RosePan_RendererGetRenderSize(&render_w, &render_h);
            ImGui::TextDisabled("  rasterised at %dx%d", render_w, render_h);
        }
        ImGui::Separator();

        DrawAntiAliasingMenu();
        ImGui::Separator();

        DrawHdrMenu();
        ImGui::Separator();

        DrawEffectsMenu();
        ImGui::Separator();

        DrawFinderMenu();
        ImGui::Separator();

        ImGui::TextDisabled("Debug views");
        DrawRendererDebugViewItem("Wireframe",
                                  ROSEPAN_RENDERER_DEBUG_WIREFRAME);
        DrawRendererDebugViewItem("Disable depth test/write",
                                  ROSEPAN_RENDERER_DEBUG_DISABLE_DEPTH);
        DrawRendererDebugViewItem("Flat colours",
                                  ROSEPAN_RENDERER_DEBUG_FLAT_COLOUR);
        DrawRendererDebugViewItem("Disable lighting",
                                  ROSEPAN_RENDERER_DEBUG_DISABLE_LIGHTING);
        ImGui::Separator();

        ImGui::TextDisabled("Diagnostics / A-B");
        DrawRendererDebugViewItem(
            "Disable host 3D quads (simulation + PS2 packets continue)",
            ROSEPAN_RENDERER_DEBUG_DISABLE_BILLBOARD_HOST);
        DrawRendererDebugViewItem(
            "Skip legacy PS2 billboard packets (host visuals remain)",
            ROSEPAN_RENDERER_DEBUG_SKIP_BILLBOARD_LEGACY_PACKETS);
        DrawRendererDebugViewItem(
            "Disable sky dome grid (flat fog/background remains)",
            ROSEPAN_RENDERER_DEBUG_DISABLE_SKY_DOME);
        DrawRendererDebugViewItem(
            "Disable sky horizon quad (flat fog/background remains)",
            ROSEPAN_RENDERER_DEBUG_DISABLE_SKY_HORIZON);
        DrawRendererDebugViewItem(
            "Disable 2D depth (photo/tray depth-stencil passes unmasked)",
            ROSEPAN_RENDERER_DEBUG_DISABLE_2D_DEPTH);
        DrawRendererDebugViewItem(
            "Shadow projector view (red occluded / green lit / blue unprojected)",
            ROSEPAN_RENDERER_DEBUG_SHADOW_VIEW);
        {
            int shadows = 0, casters = 0, receivers = 0, valid = 0;
            RosePan_RendererGetShadowStats(&shadows, &casters, &receivers,
                                          &valid);
            ImGui::TextDisabled(
                "  shadows %d / caster draws %d / receiver draws %d / map %s",
                shadows, casters, receivers, valid ? "yes" : "no");
        }
        ImGui::Separator();

        ImGui::TextDisabled("Animated character lighting");
        const int animated_backend =
            RosePan_RendererGetAnimatedLightingBackend();
        if (ImGui::MenuItem("GPU vertex (fast)", nullptr,
                            animated_backend == ROSEPAN_ANIMATED_LIGHTING_GPU))
        {
            RosePan_RendererSetAnimatedLightingBackend(
                ROSEPAN_ANIMATED_LIGHTING_GPU);
        }
        if (ImGui::MenuItem("CPU reference (A/B)", nullptr,
                            animated_backend == ROSEPAN_ANIMATED_LIGHTING_CPU))
        {
            RosePan_RendererSetAnimatedLightingBackend(
                ROSEPAN_ANIMATED_LIGHTING_CPU);
        }
        ImGui::Separator();

        ImGui::TextDisabled("Static meshes");
        bool resident = RosePan_RendererGetResidentMeshes() != 0;
        if (ImGui::MenuItem("Keep geometry on the GPU", nullptr, &resident))
        {
            RosePan_RendererSetResidentMeshes(resident ? 1 : 0);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Rooms, furniture and doors are decoded once and drawn from\n"
                "GPU buffers, with neighbouring parts merged into one draw.\n"
                "Off re-decodes and re-uploads every mesh every frame, as the\n"
                "PS2-shaped path always has -- the A/B if anything differs.");
        }
        bool resident_textures = RosePan_RendererGetResidentTextures() != 0;
        if (ImGui::MenuItem("Stop re-sending their textures", nullptr,
                            &resident_textures))
        {
            RosePan_RendererSetResidentTextures(resident_textures ? 1 : 0);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Each room, piece of furniture and door resolves its textures\n"
                "once, and then stops uploading them to the emulated GS every\n"
                "frame. Off re-sends and re-hashes them every draw, as the PS2\n"
                "did -- the A/B if a texture looks wrong.");
        }
        ImGui::Separator();

        ImGui::TextDisabled("Static mesh shading");
        const int lighting_mode = RosePan_RendererGetLightingMode();
        if (ImGui::MenuItem("Vertex lighting (PS2)", nullptr,
                            lighting_mode == ROSEPAN_LIGHTING_VERTEX))
        {
            RosePan_RendererSetLightingMode(ROSEPAN_LIGHTING_VERTEX);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Every light on the CPU, once per vertex. What the PS2 did.");
        }
        if (ImGui::MenuItem("Fragment lighting (spots)", nullptr,
                            lighting_mode == ROSEPAN_LIGHTING_FRAGMENT))
        {
            RosePan_RendererSetLightingMode(ROSEPAN_LIGHTING_FRAGMENT);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Spotlights per pixel, the rest per vertex on the CPU.");
        }
        if (ImGui::MenuItem("Fragment lighting (all lights)", nullptr,
                            lighting_mode == ROSEPAN_LIGHTING_FRAGMENT_ALL))
        {
            RosePan_RendererSetLightingMode(ROSEPAN_LIGHTING_FRAGMENT_ALL);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "The whole light image per pixel; no CPU vertex lighting at "
                "all.\nSharper speculars and cone edges than the PS2 had.");
        }
        ImGui::Separator();

        ImGui::TextDisabled("Projected shadows");
        const int shadow_filter = RosePan_RendererGetShadowFilter();
        if (ImGui::MenuItem("Hard edge (PS2)", nullptr,
                            shadow_filter == ROSEPAN_SHADOW_FILTER_NONE))
        {
            RosePan_RendererSetShadowFilter(ROSEPAN_SHADOW_FILTER_NONE);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "One tap of the silhouette, as the projected sprite gave.");
        }
        if (ImGui::MenuItem("Soft edge", nullptr,
                            shadow_filter == ROSEPAN_SHADOW_FILTER_SOFT))
        {
            RosePan_RendererSetShadowFilter(ROSEPAN_SHADOW_FILTER_SOFT);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Average a 3x3 of the silhouette.");
        }
        ImGui::EndMenu();
    }
    DrawAudioMenu();
    if (ImGui::BeginMenu("Settings"))
    {
        ImGui::MenuItem("Game data folder...", nullptr, &g_show_data_folder);
        ImGui::Separator();
        bool skip_intro = rosepan_config.game.skip_intro != 0;
        if (ImGui::MenuItem("Skip intro at launch", nullptr, &skip_intro))
        {
            rosepan_config.game.skip_intro = skip_intro ? 1 : 0;
            MarkSettingsDirty();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Open on \"Press START button\", without the logos\n"
                              "and the two opening movies. Takes effect at the\n"
                              "next launch; Start skips them either way.");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Open user data folder"))
        {
            OpenFolderInShell(RosePan_PathUserDir());
        }
        if (ImGui::MenuItem("Open game data folder"))
        {
            OpenFolderInShell(RosePan_PathDataDir());
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

/*
 * Everything the performance overlay shows below the FPS line.  Split out so
 * the overlay can be run in its FPS-only mode, which is the same window with
 * this skipped -- one line, for when the frame rate is the only question.
 */
void DrawPerformanceDetail(const RosePanProfilerStats &profiler)
{
    ImGui::Text("CPU timing avg / 1s max (ms)");
    ImGui::Text("Frame paced %.2f / %.2f | workload %.2f / %.2f",
                profiler.frame_ms, profiler.frame_max_ms,
                profiler.workload_ms, profiler.workload_max_ms);
    ImGui::Text("Game CPU %.2f / %.2f | VBlank/gate %.2f / %.2f",
                profiler.game_cpu_ms, profiler.game_cpu_max_ms,
                profiler.vblank_wait_ms, profiler.vblank_wait_max_ms);
    ImGui::Text("SGD %.2f / %.2f | mesh bridge %.2f / %.2f",
                profiler.sgd_cpu_ms, profiler.sgd_cpu_max_ms,
                profiler.mesh_cpu_ms, profiler.mesh_cpu_max_ms);
    ImGui::Text("Colour/light %.2f / %.2f | VUVN/skin %.2f / %.2f | tex miss %.2f / %.2f",
                profiler.mesh_lighting_ms,
                profiler.mesh_lighting_max_ms,
                profiler.mesh_skinning_ms,
                profiler.mesh_skinning_max_ms,
                profiler.texture_miss_ms,
                profiler.texture_miss_max_ms);
    ImGui::Text("World phases inclusive avg / 1s max (ms)");
    ImGui::Text("Billboard packet %.2f / %.2f | host bridge %.2f / %.2f | sky %.2f / %.2f",
                profiler.billboard_cpu_ms,
                profiler.billboard_cpu_max_ms,
                profiler.billboard_host_bridge_ms,
                profiler.billboard_host_bridge_max_ms,
                profiler.sky_cpu_ms,
                profiler.sky_cpu_max_ms);
    ImGui::Text("MapPut sort %.2f / %.2f | callbacks %.2f / %.2f",
                profiler.mapput_sort_ms,
                profiler.mapput_sort_max_ms,
                profiler.mapput_callback_ms,
                profiler.mapput_callback_max_ms);
    ImGui::Text("Renderer %.2f / %.2f | upload %.2f / %.2f",
                profiler.renderer_ms, profiler.renderer_max_ms,
                profiler.renderer_upload_ms,
                profiler.renderer_upload_max_ms);
    ImGui::Text("Record %.2f / %.2f | acquire %.2f / %.2f | submit %.2f / %.2f",
                profiler.renderer_record_ms,
                profiler.renderer_record_max_ms,
                profiler.renderer_acquire_ms,
                profiler.renderer_acquire_max_ms,
                profiler.renderer_submit_ms,
                profiler.renderer_submit_max_ms);
    ImGui::Text("GPU idle %.2f / %.2f (%d) | swapchain miss %d/%d",
                profiler.gpu_idle_wait_ms,
                profiler.gpu_idle_wait_max_ms,
                profiler.gpu_idle_waits,
                profiler.swapchain_unavailable,
                profiler.swapchain_acquire_attempts);
    ImGui::Text("Pacing target %d VBlanks (~%.1f ms) | drained %d | waits %d | deadline miss %d",
                profiler.vblank_target,
                (double)profiler.vblank_target * (1000.0 / 60.0),
                profiler.vblank_drained,
                profiler.vblank_wait_calls,
                profiler.vblank_deadline_misses);
    ImGui::Text("World counters avg/frame / frame max");
    ImGui::Text("Billboard request %.1f/%llu | queued %.1f/%llu | reject %.1f/%llu | suppressed %.1f/%llu",
                profiler.billboard_requests.per_frame,
                (unsigned long long)profiler.billboard_requests.max,
                profiler.billboard_queued.per_frame,
                (unsigned long long)profiler.billboard_queued.max,
                profiler.billboard_rejected.per_frame,
                (unsigned long long)profiler.billboard_rejected.max,
                profiler.billboard_suppressed.per_frame,
                (unsigned long long)profiler.billboard_suppressed.max);
    ImGui::Text("Billboard cmd %.1f/%llu | verts %.1f/%llu | queued %.1f/%.1f KiB",
                profiler.billboard_commands.per_frame,
                (unsigned long long)profiler.billboard_commands.max,
                profiler.billboard_vertices.per_frame,
                (unsigned long long)profiler.billboard_vertices.max,
                profiler.billboard_stream_bytes.per_frame / 1024.0,
                (double)profiler.billboard_stream_bytes.max / 1024.0);
    ImGui::Text("Billboard joins %.1f/%llu | texture %.1f/%llu | L1 miss %.1f/%llu | legacy skip %.1f/%llu",
                profiler.billboard_compatible_joins.per_frame,
                (unsigned long long)profiler.billboard_compatible_joins.max,
                profiler.billboard_texture_lookups.per_frame,
                (unsigned long long)profiler.billboard_texture_lookups.max,
                profiler.billboard_texture_misses.per_frame,
                (unsigned long long)profiler.billboard_texture_misses.max,
                profiler.billboard_legacy_skips.per_frame,
                (unsigned long long)profiler.billboard_legacy_skips.max);
    ImGui::Text("Sky cmd %.1f/%llu | verts %.1f/%llu | queued %.1f/%.1f KiB | joins %.1f/%llu",
                profiler.sky_commands.per_frame,
                (unsigned long long)profiler.sky_commands.max,
                profiler.sky_vertices.per_frame,
                (unsigned long long)profiler.sky_vertices.max,
                profiler.sky_stream_bytes.per_frame / 1024.0,
                (double)profiler.sky_stream_bytes.max / 1024.0,
                profiler.sky_compatible_joins.per_frame,
                (unsigned long long)profiler.sky_compatible_joins.max);
    ImGui::Text("Sky points %.1f/%llu | cells %.1f/%llu | visible %.1f/%llu | horizon %.1f/%llu",
                profiler.sky_points_transformed.per_frame,
                (unsigned long long)profiler.sky_points_transformed.max,
                profiler.sky_cells_tested.per_frame,
                (unsigned long long)profiler.sky_cells_tested.max,
                profiler.sky_visible_cells.per_frame,
                (unsigned long long)profiler.sky_visible_cells.max,
                profiler.sky_horizon_quads.per_frame,
                (unsigned long long)profiler.sky_horizon_quads.max);
    ImGui::Text("Sky suppressed dome %.1f/%llu | horizon %.1f/%llu",
                profiler.sky_dome_suppressed.per_frame,
                (unsigned long long)profiler.sky_dome_suppressed.max,
                profiler.sky_horizon_suppressed.per_frame,
                (unsigned long long)profiler.sky_horizon_suppressed.max);
    ImGui::Text("MapPut candidate %.1f/%llu | drawable %.1f/%llu | compare %.1f/%llu",
                profiler.mapput_candidates.per_frame,
                (unsigned long long)profiler.mapput_candidates.max,
                profiler.mapput_drawable.per_frame,
                (unsigned long long)profiler.mapput_drawable.max,
                profiler.mapput_comparisons.per_frame,
                (unsigned long long)profiler.mapput_comparisons.max);
    ImGui::Text("MapPut callbacks %.1f/%llu | object draws %.1f/%llu",
                profiler.mapput_callbacks.per_frame,
                (unsigned long long)profiler.mapput_callbacks.max,
                profiler.mapput_object_draws.per_frame,
                (unsigned long long)profiler.mapput_object_draws.max);
    ImGui::Text("%zu draws | %zu verts",
                g_performance_stats.draw_count,
                g_performance_stats.vertex_count);
    ImGui::Text("Mesh tri %d | clip %d",
                g_performance_stats.mesh_triangles_submitted,
                g_performance_stats.mesh_triangles_clipped);
    {
        /* Per logical frame: walker units served from resident geometry and
         * the draws they were folded into. */
        const double frames = g_performance_stats.window_frames > 0
            ? (double)g_performance_stats.window_frames : 1.0;
        ImGui::Text("Resident %zu meshes %.1f MiB | %.0f units -> %.0f draws/frame | built %d",
                    g_performance_stats.resident_meshes,
                    (double)g_performance_stats.resident_bytes /
                        (1024.0 * 1024.0),
                    (double)g_performance_stats.resident_units / frames,
                    (double)g_performance_stats.resident_draws / frames,
                    g_performance_stats.resident_creates);
        /* What still reaches the emulated GS, and the model uploads that no
         * longer do. */
        ImGui::Text("GS uploads %.1f/frame %.0f KB/frame | model TRI2 skipped %.1f/frame replayed %.1f/frame | bound %d",
                    (double)g_performance_stats.gs_uploads / frames,
                    (double)g_performance_stats.gs_upload_bytes / frames /
                        1024.0,
                    (double)g_performance_stats.resident_texture_skips /
                        frames,
                    (double)g_performance_stats.resident_texture_replays /
                        frames,
                    g_performance_stats.resident_texture_captures);
    }
    ImGui::Text("Mesh cache %d/%d | new %d | evict %d | inv %d | fail %d",
                g_performance_stats.mesh_cache_hits,
                g_performance_stats.mesh_cache_hits +
                    g_performance_stats.mesh_cache_misses,
                g_performance_stats.mesh_cache_creates,
                g_performance_stats.mesh_cache_evictions,
                g_performance_stats.mesh_cache_invalidations,
                g_performance_stats.mesh_cache_upload_failures);
    ImGui::Text("Mesh resident %zu | %.1f MiB | avoided %zu verts",
                g_performance_stats.mesh_cache_entries,
                (double)g_performance_stats.mesh_cache_bytes /
                    (1024.0 * 1024.0),
                g_performance_stats.mesh_expanded_vertices_avoided);
    ImGui::Text("Mesh warm pending %zu | promoted %d | build defer %d | upload defer %d",
                g_performance_stats.mesh_cache_pending_entries,
                g_performance_stats.mesh_cache_promotions,
                g_performance_stats.mesh_cache_build_deferred,
                g_performance_stats.mesh_cache_upload_deferred);
    ImGui::Text("Mesh arenas %zu pages | %.1f MiB allocated",
                g_performance_stats.mesh_arena_pages,
                (double)g_performance_stats.mesh_arena_bytes /
                    (1024.0 * 1024.0));
    ImGui::Text("Mesh upload cache %.1f MiB | colour %.1f MiB | animated %.1f MiB",
                (double)g_performance_stats.mesh_cache_upload_bytes /
                    (1024.0 * 1024.0),
                (double)g_performance_stats.mesh_colour_upload_bytes /
                    (1024.0 * 1024.0),
                (double)g_performance_stats.animated_mesh_upload_bytes /
                    (1024.0 * 1024.0));
    const size_t animated_unique =
        g_performance_stats.animated_mesh_vertices;
    const size_t animated_avoided =
        g_performance_stats.animated_mesh_expanded_vertices_avoided;
    const double animated_stream_reduction = animated_unique != 0
            ? (double)(animated_unique + animated_avoided) * 48.0 /
                  ((double)animated_unique * 32.0)
            : 0.0;
    ImGui::Text("Animated unique %zu | expansion avoided %zu | %.2fx smaller",
                animated_unique, animated_avoided,
                animated_stream_reduction);
    ImGui::Text("Mesh direct %zu verts | %.1f MiB staging avoided",
                g_performance_stats.mesh_direct_stream_vertices,
                (double)g_performance_stats.mesh_direct_stream_vertices *
                    84.0 / (1024.0 * 1024.0));
    ImGui::Text("Texture L1 %d/%d | L2 %d",
                g_performance_stats.texture_l1_hits,
                g_performance_stats.texture_l1_lookups,
                g_performance_stats.texture_l2_hits);
    ImGui::Text("Texture new %d | dl %d | inv %d",
                g_performance_stats.texture_creates,
                g_performance_stats.texture_downloads,
                g_performance_stats.texture_invalidations);
    ImGui::Text("Font %d/%d | new %d | inv %d",
                g_performance_stats.font_texture_hits,
                g_performance_stats.font_texture_selects,
                g_performance_stats.font_texture_creates,
                g_performance_stats.font_texture_invalidations);
}

void DrawProfilerGraphs()
{
    if (!g_show_performance_graphs)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(860.0f, 520.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("RosePan Profiler Graphs", &g_show_performance_graphs))
    {
        if (ImGui::Checkbox("Pause history", &g_pause_performance_history) &&
            g_pause_performance_history)
        {
            g_follow_performance_history = false;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Follow latest", &g_follow_performance_history);
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            ClearProfilerHistory();
        }

        if (g_profiler_history_count == 0)
        {
            ImGui::TextDisabled("Waiting for the first profiler sample...");
        }
        else
        {
            const double span =
                NewestProfilerHistorySample().time_seconds -
                OldestProfilerHistorySample().time_seconds;
            ImGui::TextDisabled(
                "%d samples / %.1f s. Average lines are per logical frame; "
                "max lines are the worst frame in each ~1 s sample.",
                g_profiler_history_count, span);

            if (ImGui::BeginTabBar("##RosePanProfilerTabs"))
            {
                if (ImGui::BeginTabItem("Frame"))
                {
                    DrawFrameTimingGraph();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Game CPU"))
                {
                    DrawGameCpuGraph();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Renderer"))
                {
                    DrawRendererGraph();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Hitches"))
                {
                    DrawHitchGraphs();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World objects"))
                {
                    DrawWorldObjectsGraphs();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
    }
    ImGui::End();
}

void RemapGamepadWindowingTrigger()
{
    bool lb = false;
    bool rb = false;
    int gamepad_count = 0;
    SDL_JoystickID *gamepad_ids = SDL_GetGamepads(&gamepad_count);

    if (gamepad_ids != nullptr)
    {
        for (int i = 0; i < gamepad_count; i++)
        {
            SDL_Gamepad *gamepad = SDL_GetGamepadFromID(gamepad_ids[i]);
            if (gamepad == nullptr)
            {
                continue;
            }

            lb |= SDL_GetGamepadButton(
                gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
            rb |= SDL_GetGamepadButton(
                gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
        }
        SDL_free(gamepad_ids);
    }

    const bool chord_was_latched = g_windowing_chord_latched;
    const bool rb_pressed = rb && !g_previous_rb;
    if (lb && rb)
    {
        g_windowing_chord_latched = true;
    }
    else if (!lb && !rb)
    {
        g_windowing_chord_latched = false;
    }

    const bool cycle_forward = g_windowing_chord_latched &&
                               chord_was_latched && rb_pressed;
    g_previous_rb = rb;

    const ImGuiKey override_keys[] = {
        ImGuiKey_GamepadFaceLeft,
        ImGuiKey_GamepadL1,
        ImGuiKey_GamepadR1,
    };
    const bool override_down[] = {
        g_windowing_chord_latched,
        false,
        cycle_forward,
    };

    ImGuiContext *context = ImGui::GetCurrentContext();
    ImGuiIO &io = ImGui::GetIO();
    for (int slot = 0; slot < IM_ARRAYSIZE(override_keys); slot++)
    {
        const ImGuiKey key = override_keys[slot];
        const bool want_down = override_down[slot];
        bool found = false;

        for (ImGuiInputEvent &event : context->InputEventsQueue)
        {
            if (event.Type == ImGuiInputEventType_Key && event.Key.Key == key)
            {
                event.Key.Down = want_down;
                event.Key.AnalogValue = want_down ? 1.0f : 0.0f;
                found = true;
            }
        }

        if (!found && ImGui::IsKeyDown(key) != want_down)
        {
            io.AddKeyEvent(key, want_down);
        }
    }
}

// ---------------------------------------------------------------------------
// Fatal Frame III: The Tormented theme.
//
// The game's palette is a snowbound night in the Manor of Sleep: blue-black
// rooms, snow-white light, slate and shadow on the woodwork, and the tattoo -
// sumi ink with a blue cast that spreads over Rei's skin and glows when the
// dead are near - as the one thing allowed to be bright. The style keeps that
// hierarchy. Chrome at rest is slate and shadow, the tattoo's blues are spent
// only on what is hovered, active or focused, its pale glow on the live
// accent, and lantern gold (the save points' flame, Rei's flashlight) is held
// back for links and highlights. Text contrast (WCAG): snow on night 14.8:1,
// frost (disabled) 4.5:1, snow on the hovered / pressed blues 6.5 / 5.1:1.
// ---------------------------------------------------------------------------
constexpr ImVec4 ThemeColour(int rgb, float alpha = 1.0f)
{
    return ImVec4((float)((rgb >> 16) & 0xFF) / 255.0f,
                  (float)((rgb >> 8) & 0xFF) / 255.0f,
                  (float)(rgb & 0xFF) / 255.0f, alpha);
}

constexpr int kThemeSnow = 0xDCE3EA;       // snow light: body text
constexpr int kThemeFrost = 0x6E7B8C;      // frost: disabled text
constexpr int kThemeVoid = 0x05070B;       // the deepest night, behind all
constexpr int kThemeNight = 0x0A0F17;      // window interiors
constexpr int kThemeShadow = 0x101925;     // title and menu bars at rest
constexpr int kThemeSlot = 0x152131;       // input wells and frames
constexpr int kThemeSlate = 0x1D2B3D;      // buttons and grabs at rest
constexpr int kThemeInk = 0x263A55;        // borders, separators, table rules: the tattoo's ink
constexpr int kThemeTattoo = 0x24507A;     // hovered
constexpr int kThemeSpirit = 0x295F96;     // pressed / active
constexpr int kThemeGlow = 0x8CCBEA;       // the live accent: ticks, tabs, nav - the tattoo's glow
constexpr int kThemeLantern = 0xD8B062;    // lantern gold: links, highlights

void ApplyTormentedStyle()
{
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&style);

    ImVec4 *colours = style.Colors;
    colours[ImGuiCol_Text] = ThemeColour(kThemeSnow);
    colours[ImGuiCol_TextDisabled] = ThemeColour(kThemeFrost);
    colours[ImGuiCol_WindowBg] = ThemeColour(kThemeNight, 0.97f);
    colours[ImGuiCol_ChildBg] = ThemeColour(kThemeVoid, 0.35f);
    colours[ImGuiCol_PopupBg] = ThemeColour(kThemeVoid, 0.98f);
    colours[ImGuiCol_Border] = ThemeColour(kThemeInk, 0.90f);
    colours[ImGuiCol_BorderShadow] = ThemeColour(kThemeVoid, 0.00f);
    colours[ImGuiCol_FrameBg] = ThemeColour(kThemeSlot, 0.95f);
    colours[ImGuiCol_FrameBgHovered] = ThemeColour(kThemeTattoo, 0.65f);
    colours[ImGuiCol_FrameBgActive] = ThemeColour(kThemeSpirit, 0.70f);
    colours[ImGuiCol_TitleBg] = ThemeColour(kThemeShadow);
    colours[ImGuiCol_TitleBgActive] = ThemeColour(kThemeTattoo);
    colours[ImGuiCol_TitleBgCollapsed] = ThemeColour(kThemeShadow, 0.75f);
    colours[ImGuiCol_MenuBarBg] = ThemeColour(kThemeShadow);
    colours[ImGuiCol_ScrollbarBg] = ThemeColour(kThemeVoid, 0.60f);
    colours[ImGuiCol_ScrollbarGrab] = ThemeColour(kThemeSlate);
    colours[ImGuiCol_ScrollbarGrabHovered] = ThemeColour(kThemeTattoo);
    colours[ImGuiCol_ScrollbarGrabActive] = ThemeColour(kThemeSpirit);
    colours[ImGuiCol_CheckMark] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_SliderGrab] = ThemeColour(kThemeTattoo);
    colours[ImGuiCol_SliderGrabActive] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_Button] = ThemeColour(kThemeSlate, 0.90f);
    colours[ImGuiCol_ButtonHovered] = ThemeColour(kThemeTattoo);
    colours[ImGuiCol_ButtonActive] = ThemeColour(kThemeSpirit);
    colours[ImGuiCol_Header] = ThemeColour(kThemeInk, 0.80f);
    colours[ImGuiCol_HeaderHovered] = ThemeColour(kThemeTattoo, 0.90f);
    colours[ImGuiCol_HeaderActive] = ThemeColour(kThemeSpirit);
    colours[ImGuiCol_Separator] = ThemeColour(kThemeInk);
    colours[ImGuiCol_SeparatorHovered] = ThemeColour(kThemeTattoo);
    colours[ImGuiCol_SeparatorActive] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_ResizeGrip] = ThemeColour(kThemeInk, 0.70f);
    colours[ImGuiCol_ResizeGripHovered] = ThemeColour(kThemeTattoo, 0.90f);
    colours[ImGuiCol_ResizeGripActive] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_InputTextCursor] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_Tab] = ThemeColour(kThemeShadow);
    colours[ImGuiCol_TabHovered] = ThemeColour(kThemeTattoo, 0.90f);
    colours[ImGuiCol_TabSelected] = ThemeColour(kThemeInk);
    colours[ImGuiCol_TabSelectedOverline] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_TabDimmed] = ThemeColour(kThemeNight);
    colours[ImGuiCol_TabDimmedSelected] = ThemeColour(kThemeSlot);
    colours[ImGuiCol_TabDimmedSelectedOverline] = ThemeColour(kThemeTattoo);
    colours[ImGuiCol_PlotLines] = ThemeColour(kThemeLantern);
    colours[ImGuiCol_PlotLinesHovered] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_PlotHistogram] = ThemeColour(kThemeTattoo);
    colours[ImGuiCol_PlotHistogramHovered] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_TableHeaderBg] = ThemeColour(kThemeSlot);
    colours[ImGuiCol_TableBorderStrong] = ThemeColour(kThemeInk);
    colours[ImGuiCol_TableBorderLight] = ThemeColour(kThemeSlate, 0.70f);
    colours[ImGuiCol_TableRowBg] = ThemeColour(kThemeVoid, 0.00f);
    colours[ImGuiCol_TableRowBgAlt] = ThemeColour(kThemeSnow, 0.03f);
    colours[ImGuiCol_TextLink] = ThemeColour(kThemeLantern);
    colours[ImGuiCol_TextSelectedBg] = ThemeColour(kThemeSpirit, 0.55f);
    colours[ImGuiCol_TreeLines] = ThemeColour(kThemeInk);
    colours[ImGuiCol_DragDropTarget] = ThemeColour(kThemeLantern);
    colours[ImGuiCol_UnsavedMarker] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_NavCursor] = ThemeColour(kThemeGlow);
    colours[ImGuiCol_NavWindowingHighlight] = ThemeColour(kThemeSnow, 0.70f);
    colours[ImGuiCol_NavWindowingDimBg] = ThemeColour(kThemeVoid, 0.60f);
    colours[ImGuiCol_ModalWindowDimBg] = ThemeColour(kThemeVoid, 0.70f);

    // The border colours above only read once there is a border to paint, and
    // the game frames its own menus as squared-off paper rather than rounded.
    style.FrameBorderSize = 1.0f;
    style.TabRounding = 0.0f;
    style.ScrollbarRounding = 2.0f;
}

void ApplyTormentedPlotStyle()
{
    ImPlotStyle &style = ImPlot::GetStyle();
    ImPlot::StyleColorsAuto(&style);

    ImVec4 *colours = style.Colors;
    colours[ImPlotCol_PlotBg] = ThemeColour(kThemeVoid, 0.85f);
    colours[ImPlotCol_PlotBorder] = ThemeColour(kThemeInk);
    colours[ImPlotCol_LegendBg] = ThemeColour(kThemeVoid, 0.92f);
    colours[ImPlotCol_LegendBorder] = ThemeColour(kThemeInk);
    colours[ImPlotCol_AxisGrid] = ThemeColour(kThemeInk, 0.55f);
    colours[ImPlotCol_AxisTick] = ThemeColour(kThemeInk, 0.80f);
    colours[ImPlotCol_Selection] = ThemeColour(kThemeLantern, 0.80f);
    colours[ImPlotCol_Crosshairs] = ThemeColour(kThemeGlow, 0.80f);

    // A profiler graph carries up to ten lines at once, so the series colours
    // have to stay tellable apart while still belonging to the palette above:
    // the tattoo's blue loudest, then cold and lantern-lit hues in turn.
    // Ordered and checked for colour-blind separation against the plot ground
    // (the dataviz validator, dark mode, surface #070a0f): every one inside the
    // lightness band, the worst neighbouring pair at OKLab dE 8.2 under
    // deuteranopia against a floor of 8, 16.9 under normal vision.
    static const ImVec4 kSeries[] = {
        ThemeColour(0x3B8DD6), // tattoo blue
        ThemeColour(0xB58A2E), // lantern gold
        ThemeColour(0x9F7BD0), // dusk lilac
        ThemeColour(0x26A08A), // spirit teal
        ThemeColour(0xCC5650), // camellia red
        ThemeColour(0x0B9DB8), // snowmelt cyan
        ThemeColour(0x6A9A3A), // courtyard moss
        ThemeColour(0xC46E9A), // plum blossom
        ThemeColour(0x5F6FD8), // night iris
        ThemeColour(0xBE6224), // brazier rust
    };

    ImPlotColormap colormap = ImPlot::GetColormapIndex("Tormented");
    if (colormap == -1)
    {
        colormap = ImPlot::AddColormap("Tormented", kSeries,
                                       IM_ARRAYSIZE(kSeries));
    }
    style.Colormap = colormap;
}

}

namespace RosePanUi
{
bool Init(SDL_Window *window, SDL_GPUDevice *device,
          SDL_GPUTextureFormat swapchain_format)
{
    if (IsInitialized())
    {
        return true;
    }
    if (window == nullptr || device == nullptr ||
        swapchain_format == SDL_GPU_TEXTUREFORMAT_INVALID)
    {
        SDL_Log("RosePan ImGui: invalid renderer state during initialization");
        return false;
    }

    /* Kept so the folder picker can be modal to the game's window. */
    g_window = window;

    IMGUI_CHECKVERSION();
    g_context = ImGui::CreateContext();
    if (g_context == nullptr)
    {
        SDL_Log("RosePan ImGui: ImGui::CreateContext failed");
        return false;
    }

    ImGui::SetCurrentContext(g_context);
    g_plot_context = ImPlot::CreateContext();
    if (g_plot_context == nullptr)
    {
        SDL_Log("RosePan ImPlot: ImPlot::CreateContext failed");
        Shutdown();
        return false;
    }

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
                      ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;
    ApplyTormentedStyle();
    ApplyTormentedPlotStyle();

    g_platform_initialized = ImGui_ImplSDL3_InitForSDLGPU(window);
    if (!g_platform_initialized)
    {
        SDL_Log("RosePan ImGui: SDL3 platform backend initialization failed: %s",
                SDL_GetError());
        Shutdown();
        return false;
    }

    ImGui_ImplSDLGPU3_InitInfo init_info{};
    init_info.Device = device;
    init_info.ColorTargetFormat = swapchain_format;
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    g_renderer_initialized = ImGui_ImplSDLGPU3_Init(&init_info);
    if (!g_renderer_initialized)
    {
        SDL_Log("RosePan ImGui: SDL_GPU renderer backend initialization failed: %s",
                SDL_GetError());
        Shutdown();
        return false;
    }

    return true;
}

void Shutdown()
{
    if (g_context == nullptr)
    {
        return;
    }

    ImGui::SetCurrentContext(g_context);
    if (g_frame_started)
    {
        ImGui::EndFrame();
    }
    if (g_renderer_initialized)
    {
        ImGui_ImplSDLGPU3_Shutdown();
    }
    if (g_platform_initialized)
    {
        ImGui_ImplSDL3_Shutdown();
    }

    if (g_plot_context != nullptr)
    {
        ImPlot::DestroyContext(g_plot_context);
        g_plot_context = nullptr;
    }

    ImGui::DestroyContext(g_context);
    g_context = nullptr;
    g_platform_initialized = false;
    g_renderer_initialized = false;
    g_frame_started = false;
    g_frame_rendered = false;
    g_previous_rb = false;
    g_windowing_chord_latched = false;
    g_show_main_menu_bar = true;
    g_show_performance_summary = false;
    g_summary_fps_only = false;
    g_show_performance_graphs = false;
    g_show_controls = false;
    g_show_data_folder = false;
    CancelCapture();
    g_pause_performance_history = false;
    g_follow_performance_history = true;
    g_performance_stats = {};
    ClearProfilerHistory();
}

void ProcessEvent(const SDL_Event *event)
{
    if (IsInitialized() && event != nullptr)
    {
        ImGui::SetCurrentContext(g_context);

        /* Capture first, and swallow what it takes.  Keyboard and gamepad
         * nav are both on, so a Space or an A reaching ImGui would
         * re-activate the very button that armed the capture. */
        if (HandleCaptureEvent(event))
        {
            return;
        }

        ImGui_ImplSDL3_ProcessEvent(event);
        if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
            event->key.scancode == SDL_SCANCODE_F1)
        {
            g_show_main_menu_bar = !g_show_main_menu_bar;
        }
    }
}

void SetPerformanceStats(const PerformanceStats &stats)
{
    g_performance_stats = stats;
    AppendProfilerHistory(stats);
}

void BeginFrame()
{
    if (!IsInitialized() || g_frame_started)
    {
        return;
    }

    ImGui::SetCurrentContext(g_context);
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    RemapGamepadWindowingTrigger();
    ImGui::NewFrame();
    g_frame_started = true;
    g_frame_rendered = false;
}

void Draw()
{
    if (!g_frame_started)
    {
        return;
    }

    /* Ahead of the menu, so a change made this frame is compared against the
     * state the menu is about to redraw rather than against itself. */
    PersistSettingsIfChanged();

    DrawMainMenuBar();
    if (!g_show_performance_summary)
    {
        DrawProfilerGraphs();
        DrawControlsWindow();
        DrawDataFolderWindow();
        return;
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);

    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("RosePan Performance", nullptr, window_flags))
    {
        const RosePanProfilerStats &profiler = g_performance_stats.profiler;
        const float camera_motion = RosePan_RendererGetCameraMotion();
        char camera_note[32];
        if (camera_motion < 0.0f)
        {
            /* Smoothing off, or no camera pair yet. */
            SDL_strlcpy(camera_note, "--", sizeof(camera_note));
        }
        else
        {
            SDL_snprintf(camera_note, sizeof(camera_note), "%.1f",
                         (double)camera_motion);
        }
        const float geometry_coverage =
            RosePan_RendererGetGeometryCoverage();
        char geometry_note[32];
        if (geometry_coverage < 0.0f)
        {
            /* Motion interpolation off, or no corresponding previous frame. */
            SDL_strlcpy(geometry_note, "--", sizeof(geometry_note));
        }
        else
        {
            SDL_snprintf(geometry_note, sizeof(geometry_note), "%.0f%%",
                         (double)geometry_coverage * 100.0);
        }
        /* `cam` is what says whether interpolation has anything to do: 0.0 in
         * a fixed-angle room is correct, not broken.  `geo` is the same
         * question for the world -- the share of 3D draws that corresponded to
         * one last frame, so a low figure means the draw list keeps changing
         * shape rather than that anything is wrong. */
        ImGui::Text("Game %.1f FPS | Present %.1f FPS | present/frame %.2f | "
                    "cam %s/tick | geo %s",
                    g_performance_stats.game_fps,
                    g_performance_stats.present_fps,
                    g_performance_stats.game_fps > 0.0
                        ? g_performance_stats.present_fps /
                              g_performance_stats.game_fps
                        : 0.0,
                    camera_note, geometry_note);
        if (!g_summary_fps_only)
        {
            DrawPerformanceDetail(profiler);
        }
    }
    ImGui::End();
    DrawProfilerGraphs();
    DrawControlsWindow();
    DrawDataFolderWindow();
}

void EndFrame()
{
    if (!g_frame_started)
    {
        return;
    }

    ImGui::Render();
    g_frame_started = false;
    g_frame_rendered = true;
}

bool PrepareDrawData(SDL_GPUCommandBuffer *command_buffer)
{
    if (!IsInitialized() || !g_frame_rendered || command_buffer == nullptr)
    {
        return false;
    }

    ImDrawData *draw_data = ImGui::GetDrawData();
    if (draw_data == nullptr || draw_data->CmdListsCount == 0 ||
        draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    {
        return false;
    }

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
    return true;
}

void RenderDrawData(SDL_GPUCommandBuffer *command_buffer,
                    SDL_GPURenderPass *render_pass)
{
    if (!IsInitialized() || !g_frame_rendered || command_buffer == nullptr ||
        render_pass == nullptr)
    {
        return;
    }

    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), command_buffer,
                                     render_pass);
    /* The latch is deliberately left set.  ImGui::Render() froze this frame's
     * draw data and it stays valid until the next NewFrame(), and the backend
     * re-uploads it from scratch on every PrepareDrawData(), so a logical frame
     * presented more than once -- a reprojected in-between -- carries the
     * overlay on each present instead of flashing it at the logical rate.
     * BeginFrame() is what clears the latch, once per logical frame. */
}
}

/* The Debug menu's toggles (rosepan_debug.h): plain flags the game code
 * polls once a frame. */
static std::atomic<int> g_debug_light_cone{0};

int RosePan_DebugLightConeEnabled(void)
{
    return g_debug_light_cone.load(std::memory_order_relaxed);
}

void RosePan_DebugSetLightCone(int enabled)
{
    g_debug_light_cone.store(enabled != 0 ? 1 : 0, std::memory_order_relaxed);
}
