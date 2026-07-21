import "../globallayout";

struct PushConstants {
    projnekomata::ShGlobalData* globalData;
};

[vk::push_constant] PushConstants s_pushConstants;

struct VSOut {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float2 ndc : TEXCOORD1;
};

[shader("vertex")]
VSOut main(uint vid : SV_VulkanVertexID) {
    float2 uv = float2((vid << 1) & 2, vid & 2);
    float2 pos = uv * 2.0f - 1.0f;

    VSOut output;
    output.position = float4(pos, 0.0f, 1.0f);
    output.texcoord = uv;
    output.ndc = pos;
    return output;
}

[shader("fragment")]
float2 main(VSOut input) : SV_Target {
    float4 farndc = float4(input.ndc, 0.0f, 1.0f);

    float4 worldpos = mul(s_pushConstants.globalData.projviewNoTranslationInverse, farndc);
    worldpos.xyz /= worldpos.w;

    float3 dir = normalize(worldpos.xyz);

    float4 infinitypt = float4(dir, 0.0f);

    float4 prevclip = mul(s_pushConstants.globalData.prevProjviewNoTranslation, infinitypt);
    float2 prevndc = prevclip.xy / prevclip.w;
    float2 prevtexcoord = prevndc * 0.5f + 0.5f;
    float2 velocity = input.texcoord - prevtexcoord;

    return velocity;
}