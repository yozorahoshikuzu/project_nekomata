struct PushConstants {
    float2 ndcBegin;
    float2 ndcEnd;
    float2 texcoordBegin;
    float2 texcoordEnd;
    uint textureIndex;
    uint samplerIndex;
};

[vk::push_constant] PushConstants pushConstants;
[vk::binding(0, 0)] Texture2D textures[];
[vk::binding(1, 0)] SamplerState samplers[];

struct VSOut {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

[shader("vertex")]
VSOut main(uint vertexId : SV_VulkanVertexID) {
    float2 baseTexcoord = float2(vertexId & 1, vertexId >> 1);

    VSOut output;
    output.position = float4(lerp(pushConstants.ndcBegin, pushConstants.ndcEnd, baseTexcoord), 0.0f, 1.0f);
    output.texcoord = lerp(pushConstants.texcoordBegin, pushConstants.texcoordEnd, baseTexcoord);
    return output;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    return textures[pushConstants.textureIndex].Sample(samplers[pushConstants.samplerIndex], input.texcoord);
}