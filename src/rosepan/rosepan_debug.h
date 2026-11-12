/* ==========================================================================
 *  rosepan/rosepan_debug.h
 *
 *  PC debug toggles the game code reads and the ImGui "Debug" menu sets
 *  (rosepan_ui.cpp owns the state). None of these exist in FF3.
 * ======================================================================== */
#ifndef ROSEPAN_DEBUG_H
#define ROSEPAN_DEBUG_H

// Draw the player's light cone, its reach, a box and the facing with FF3's
// uncalled debug primitives (effect_sub.c RosePan_EffDebugOverlay).
int  RosePan_DebugLightConeEnabled(void);
void RosePan_DebugSetLightCone(int enabled);

#endif /* ROSEPAN_DEBUG_H */
