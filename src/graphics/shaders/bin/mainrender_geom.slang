import "../globallayout";

import "../normalpack/normalpack";

struct ObjectUniforms {
    float4x4 model;
    float3x3 normalMatrix;
};

struct GlobalData {
    float4x4 projview;
    float4x4 projviewInverse;
    float3 cameraPos;
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
    GlobalData* globalData;
    uint textureSrtID;
    uint samplerSrtID;
    float roughness;
    float metallic;
};

[vk::push_constant] PushConstants s_pushConstants;

struct VSOut {
    float3 position : SV_Position;
    float3 color : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
}

[shader("vertex")]
VSOut main(uint vertexID: SV_VulkanVertexID) {
    float3 position = s_pushConstants.vertexbuffer[vertexID].position;
    float3 normal = s_pushConstants.vertexbuffer[vertexID].normal;
    float2 uv = float2(s_pushConstants.vertexbuffer[vertexID].uvX, s_pushConstants.vertexbuffer[vertexID].uvY);

    float4 hposition = mul(s_pushConstants.data.model, float4(position, 1.0));

    VSOut output;
    output.position = hposition.xyz / hposition.w;
    output.color = float3(1.0);
    output.uv = uv;
    output.normal = mul(s_pushConstants.data.normalMatrix, normal);

    return output;
}

struct TCSPatchConst {
    float edges[3] : SV_TessFactor;
    float inner : SV_InsideTessFactor;
}

TCSPatchConst PatchConstantFunc(InputPatch<VSOut, 3> patch) {
    TCSPatchConst patchConst;
    patchConst.edges[0] = 2;
    patchConst.edges[1] = 2;
    patchConst.edges[2] = 2;
    patchConst.inner = 1;
    return patchConst;
}

struct TCSOut {
    float3 position : SV_Position;
    float3 color : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

[shader("tesscontrol")]
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunc")]
TCSOut main(InputPatch<VSOut, 3> patch, uint i : SV_OutputControlPointID) {
    TCSOut output;
    output.position = patch[i].position;
    output.color = patch[i].color;
    output.uv = patch[i].uv;
    output.normal = patch[i].normal;
    return output;
}

struct TESOut {
    float4 positionCS : SV_Position;
    float3 positionWS : POSITION_WS;
    float3 color : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

float3 hsv_to_rgb(float3 c)
{
    const float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

[shader("tesseval")]
[domain("tri")]
TESOut main(TCSPatchConst patchConst, float3 barycoord : SV_DomainLocation, OutputPatch<TCSOut, 3> patch, uint h : SV_PrimitiveID) {
    float3 position = barycoord.x * patch[0].position + barycoord.y * patch[1].position + barycoord.z * patch[2].position;
    float3 color    = barycoord.x * patch[0].color    + barycoord.y * patch[1].color    + barycoord.z * patch[2].color;
    float2 uv       = barycoord.x * patch[0].uv       + barycoord.y * patch[1].uv       + barycoord.z * patch[2].uv;
    float3 normal   = barycoord.x * patch[0].normal   + barycoord.y * patch[1].normal   + barycoord.z * patch[2].normal;


    h = (h ^ 61u) ^ (h >> 16u);
    h *= 9u;
    h = h ^ (h >> 4u);
    h *= 0x27d4eb2du;
    h = h ^ (h >> 15u);

    h ^= (h >> 13u);
    h *= 0x85ebca6bu;
    h ^= (h >> 16u);

    float hash = float(h) / 4294967295.0;

    float hsv_h = hash;
    float hsv_s = 0.85;
    float hsv_v = 0.85;

    TESOut output;
    output.positionCS = mul(s_pushConstants.globalData.projview, float4(position, 1.0));
    output.positionWS = position;
    output.color = hsv_to_rgb(float3(hsv_h, hsv_s, hsv_v));
    output.uv = uv;
    output.normal = normal;
    return output;
}

// ---------------------------------------------------------------------------------------------------------------------

struct FSOut {
    float4 albedoAndRoughness : SV_Target0;
    float2 normal : SV_Target1;
    float2 metallicAndAo : SV_Target2;
}

[shader("fragment")]
FSOut main(TESOut input) {
    //float3 albedo = s_textures2D[push_consts.textureSrtID].Sample(s_samplers[push_consts.samplerSrtID], input.uv).rgb;
    float3 albedo = float3(0.8);
    float metallic = s_pushConstants.metallic;
    float roughness = 1.0 - s_pushConstants.roughness;
    float ao = 1.0f;

    float3 n = normalize(input.normal);
    float2 pn = octahedronNormalPack(n);

    FSOut output;
    output.albedoAndRoughness = float4(albedo, roughness);
    output.normal = pn;
    output.metallicAndAo = float2(metallic, ao);

    return output;
}