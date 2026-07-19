import "../globallayout";

struct PushConstants {
    float4 rtMetrics;
    uint edgesTextureSrtID;
    uint areaTextureSrtID;
    uint searchTextureSrtID;
    uint smaaLinearSamplerSrtID;
    uint smaaNearestSamplerSrtID;
    uint frameIndex;
};

[vk::push_constant] PushConstants s_pushConstants;

#define LinearSampler pipeline::getSamplerBySrtId(s_pushConstants.smaaLinearSamplerSrtID)
#define PointSampler pipeline::getSamplerBySrtId(s_pushConstants.smaaNearestSamplerSrtID)
#define SMAA_RT_METRICS s_pushConstants.rtMetrics
#include "../smaa/smaa_incl.h"

struct VSOut {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float4 offsets[3] : TEXCOORD1;
    float2 pixcoord : TEXCOORD2;
}

[shader("vertex")]
VSOut main(uint vertexID : SV_VulkanVertexID) {
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    float2 pos = uv * 2.0f - 1.0f;

    VSOut output;
    output.pos = float4(pos, 0.0f, 1.0f);
    output.uv = uv;

    SMAABlendingWeightCalculationVS(uv, output.pixcoord, output.offsets);

    return output;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    let subsampleIndices = ((s_pushConstants.frameIndex & 1) == 0) ? float4(1.0f, 1.0f, 1.0f, 0.0f) : float4(2.0f, 2.0f, 2.0f, 0.0f);
    return SMAABlendingWeightCalculationPS(
        input.uv, input.pixcoord, input.offsets,
        pipeline::getTexture2DBySrtId(s_pushConstants.edgesTextureSrtID),
        pipeline::getTexture2DBySrtId(s_pushConstants.areaTextureSrtID),
        pipeline::getTexture2DBySrtId(s_pushConstants.searchTextureSrtID),
        subsampleIndices
    );
}