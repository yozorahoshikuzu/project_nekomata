import "../globallayout";

struct GlyphInstance {
    float2 sspStart;
    float2 sspEnd;
    float2 texcoordStart;
    float2 texcoordEnd;
    uint textureSrtID;
};

struct PushConstants {
    GlyphInstance* glyphs;
    float2 ssGlobalOffset;
    float2 screenSize;
    uint samplerSrtID;
    float4 color;
};

[vk::push_constant] PushConstants s_pushConstants;

struct VSOut {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
    nointerpolation uint textureSrtID;
};

[shader("vertex")]
VSOut main(uint vertexId : SV_VulkanVertexID, uint instanceId : SV_VulkanInstanceID) {
    GlyphInstance glyph = s_pushConstants.glyphs[instanceId];

    float2 baseTexcoord = float2(vertexId & 1, vertexId >> 1);

    float2 sspStart = glyph.sspStart + s_pushConstants.ssGlobalOffset;
    float2 sspEnd = glyph.sspEnd + s_pushConstants.ssGlobalOffset;
    float2 ndcStart = (sspStart / s_pushConstants.screenSize) * 2.0 - 1.0;
    float2 ndcEnd = (sspEnd / s_pushConstants.screenSize) * 2.0 - 1.0;

    VSOut output;
    output.position = float4(lerp(ndcStart, ndcEnd, baseTexcoord), 0.0, 1.0);
    output.texcoord = lerp(glyph.texcoordStart, glyph.texcoordEnd, baseTexcoord);
    output.textureSrtID = glyph.textureSrtID;
    return output;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    float alpha = s_textures2D[NonUniformResourceIndex(input.textureSrtID)].SampleLevel(s_samplers[s_pushConstants.samplerSrtID], input.texcoord, 0.0).r;
    return float4(s_pushConstants.color.rgb, s_pushConstants.color.a * alpha);
}