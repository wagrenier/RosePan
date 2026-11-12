#define ROSEPAN_VERTEX_STAGE
#include "mikupan_common.hlsli"

/*
 * Receiver pass.  This is the host stand-in for the ROM's SRT_MAPSHADOW draw:
 * gra3dShadow.c walks every registered receiver block a second time and the
 * VU1 program SgSuShadow_dma_main (vu1/ff2_03.vsm) generates projective
 * texture coordinates from matLIP = s_matIP * matLocalWorld.  Here matLocalWorld
 * is the per-draw `model` and s_matIP's role is played by uShadowMatrix, the
 * light camera's world->clip that _RenderShadow() rendered the map with.
 *
 * The world position goes on to the fragment stage for the microcode's
 * receiver culling (shadow_receiver.frag).
 *
 * Vertex layout is RosePan's streamed mesh layout -- position at TEXCOORD2.  The
 * MikuPan original bound aPos to TEXCOORD0, which is the UV stream here.
 */
struct VSInput
{
    float4 aUV : TEXCOORD0;
    float4 inColor : TEXCOORD1;
    float4 aPos : TEXCOORD2;
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 vShadowClip : TEXCOORD0;
    float4 vWorld : TEXCOORD1;
    float vAlpha : TEXCOORD2;
};

/*
 * RoR's receivers are their own model (the map pack's file 2) lying on the
 * floor, depth tested GEQUAL against the floor the map drew.  The GS's Z
 * buffer is 16 bit (PSMZ16S, 65530 near .. 1 far), so a receiver within a
 * step or two of the floor passed there; in the host's float depth the two
 * meshes' rounding decides pixel by pixel and the shadow sank into the
 * ground in patches.  The receiver is pulled toward the eye by a few of
 * those steps -- the reversed depth z / w spans the same near .. far range,
 * so one step is 1 / 65529 of it.
 */
static const float kReceiverDepthSteps = 4.0 / 65529.0;

VSOutput main(VSInput input)
{
    float4 localPos = float4(input.aPos.xyz, 1.0);
    float4 worldPos = mul(model, localPos);

    VSOutput output;
    output.position = mul(mvp, localPos);
    output.position.z = min(output.position.z + kReceiverDepthSteps * output.position.w,
                            output.position.w);
    output.vShadowClip = mul(uShadowMatrix, worldPos);
    output.vWorld = worldPos;
    output.vAlpha = input.inColor.a;
    return output;
}
