import "../globallayout";

import "../normalpack/normalpack";

struct ObjectUniforms {
    float4x4 model;
    float4x4 prevModel;
    float3x3 normalMatrix;
};

struct Vertex {
    float3 position;
    float uvX;
    float3 normal;
    float uvY;
    float4 color;
};

struct PushConstants {
    ObjectUniforms* data;
    Vertex* vertexbuffer;
    projnekomata::ShGlobalData* globalData;
    uint textureSrtID;
    uint samplerSrtID;
    float roughness;
    float metallic;
};

[vk::push_constant] PushConstants s_pushConstants;

struct VSOut {
    float4 position : SV_Position;
    float4 positionCS : TEXCOORD0;
    float4 prevPositionCS : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 normal : TEXCOORD3;
}

[shader("vertex")]
VSOut main(uint vertexID: SV_VulkanVertexID) {
    float3 position = s_pushConstants.vertexbuffer[vertexID].position;
    float3 normal = s_pushConstants.vertexbuffer[vertexID].normal;
    float2 uv = float2(s_pushConstants.vertexbuffer[vertexID].uvX, s_pushConstants.vertexbuffer[vertexID].uvY);

    float4 positionh = float4(position, 1.0f);

    float4 ndcPos = mul(s_pushConstants.globalData.projview, mul(s_pushConstants.data.model, positionh));
    float4 ndcPosPrev = mul(s_pushConstants.globalData.prevProjview, mul(s_pushConstants.data.prevModel, positionh));

    VSOut output;
    output.position = ndcPos;
    output.positionCS = ndcPos;
    output.prevPositionCS = ndcPosPrev;
    output.uv = uv;
    output.normal = mul(s_pushConstants.data.normalMatrix, normal);

    return output;
}

// ---------------------------------------------------------------------------------------------------------------------

struct FSOut {
    float4 albedoAndRoughness : SV_Target0;
    float2 normal : SV_Target1;
    float2 metallicAndAo : SV_Target2;
    float2 velocity : SV_Target3;
}

[shader("fragment")]
FSOut main(VSOut input) {
    //float3 albedo = s_textures2D[s_pushConstants.textureSrtID].Sample(s_samplers[s_pushConstants.samplerSrtID], input.uv).rgb;
    float3 albedo = float3(0.8);
    float metallic = s_pushConstants.metallic;
    float roughness = 1.0 - s_pushConstants.roughness;
    float ao = 1.0f;

    float3 n = normalize(input.normal);
    float2 pn = octahedronNormalPack(n);

    float2 ndcCurrent = input.positionCS.xy / input.positionCS.w;
    float2 ndcPrev    = input.prevPositionCS.xy / input.prevPositionCS.w;
    float2 velocity = (ndcCurrent - ndcPrev) * 0.5f;

    FSOut output;
    output.albedoAndRoughness = float4(albedo, roughness);
    output.normal = pn;
    output.metallicAndAo = float2(metallic, ao);
    output.velocity = velocity;

    return output;
}