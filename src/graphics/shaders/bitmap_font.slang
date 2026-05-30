struct GlyphInstance {
    float2 positionStart;
    float2 positionEnd;
    float2 texcoordStart;
    float2 texcoordEnd;
    uint textureIndex;
};

struct PushConstants {
    GlyphInstance* glyphs;
    uint samplerIndex;
};

[vk::push_constant] PushConstants pushConstants;
[vk::binding(0, 0)] Texture2D textures[];
[vk::binding(1, 0)] SamplerState samplers[];

struct VSOut {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
    nointerpolation uint textureIndex;
};

[shader("vertex")]
VSOut main(uint vertexId : SV_VulkanVertexID, uint instanceId : SV_VulkanInstanceID) {
    GlyphInstance glyph = pushConstants.glyphs[instanceId];

    float2 baseTexcoord = float2(vertexId & 1, vertexId >> 1);

    VSOut output;
    output.position = float4(lerp(glyph.positionStart, glyph.positionEnd, baseTexcoord), 0.0, 1.0);
    output.texcoord = lerp(glyph.texcoordStart, glyph.texcoordEnd, baseTexcoord);
    output.textureIndex = glyph.textureIndex;
    return output;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    float alpha = 0.0;
    uint thisInvocationIndex = input.textureIndex;
    while (true) {
        uint uniformIndex = WaveReadLaneFirst(thisInvocationIndex);
        if (uniformIndex == thisInvocationIndex) {
            alpha = textures[uniformIndex].SampleLevel(samplers[pushConstants.samplerIndex], input.texcoord, 0.0).r;
            break;
        }
    }
    return float4(1.0, 1.0, 1.0, alpha);
}
/*
[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    float alpha = textures[NonUniformResourceIndex(input.textureIndex)].SampleLevel(samplers[pushConstants.samplerIndex], input.texcoord, 0.0).r;
    return float4(1.0, 1.0, 1.0, alpha);
}
*/