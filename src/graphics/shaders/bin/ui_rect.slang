import "../globallayout";

struct PushConstants {
    float2 ssBegin;
    float2 ssEnd;
    float2 screenSize;
    float4 color;
};

[vk::push_constant] PushConstants s_pushConstants;

struct VSOut {
    float4 position : SV_Position;
};

[shader("vertex")]
VSOut main(uint vertexId : SV_VulkanVertexID) {
    float2 baseTexcoord = float2(vertexId & 1, vertexId >> 1);

    float2 ndcBegin = (s_pushConstants.ssBegin / s_pushConstants.screenSize) * 2.0f - 1.0f;
    float2 ndcEnd = (s_pushConstants.ssEnd / s_pushConstants.screenSize) * 2.0f - 1.0f;


    VSOut output;
    output.position = float4(lerp(ndcBegin, ndcEnd, baseTexcoord), 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    return s_pushConstants.color;
}