import "../globallayout";

struct PushConstants {
    float4 rtMetrics;
    uint colorTextureSrtID;
    uint blendTextureSrtID;
    uint smaaLinearSamplerSrtID;
    uint smaaNearestSamplerSrtID;
};

[vk::push_constant] PushConstants s_pushConstants;

#define LinearSampler pipeline::getSamplerBySrtId(s_pushConstants.smaaLinearSamplerSrtID)
#define PointSampler pipeline::getSamplerBySrtId(s_pushConstants.smaaNearestSamplerSrtID)
#define SMAA_RT_METRICS s_pushConstants.rtMetrics
#include "../smaa/smaa_incl.h"

struct VSOut {
    float4 pos    : SV_Position;
    float2 uv     : TEXCOORD0;
    float4 offset : TEXCOORD1;
}

[shader("vertex")]
VSOut main(uint vertexID : SV_VulkanVertexID) {
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    float2 pos = uv * 2.0f - 1.0f;

    VSOut output;
    output.pos = float4(pos, 0.0f, 1.0f);
    output.uv = uv;

    SMAANeighborhoodBlendingVS(uv, output.offset);

    return output;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    return SMAANeighborhoodBlendingPS(
        input.uv, input.offset,
        pipeline::getTexture2DBySrtId(s_pushConstants.colorTextureSrtID),
        pipeline::getTexture2DBySrtId(s_pushConstants.blendTextureSrtID)
    );
}