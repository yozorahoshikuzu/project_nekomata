import "../globallayout";

struct PushConstants {
    float2 ndcBegin;
    float2 ndcEnd;
    float2 texcoordBegin;
    float2 texcoordEnd;
    uint textureSrtID;
    uint samplerSrtID;
};

[vk::push_constant] PushConstants s_pushConstants;

struct VSOut {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

[shader("vertex")]
VSOut main(uint vertexId : SV_VulkanVertexID) {
    float2 baseTexcoord = float2(vertexId & 1, vertexId >> 1);

    VSOut output;
    output.position = float4(lerp(s_pushConstants.ndcBegin, s_pushConstants.ndcEnd, baseTexcoord), 0.0f, 1.0f);
    output.texcoord = lerp(s_pushConstants.texcoordBegin, s_pushConstants.texcoordEnd, baseTexcoord);
    return output;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    return s_textures2D[s_pushConstants.textureSrtID].Sample(s_samplers[s_pushConstants.samplerSrtID], input.texcoord);
}