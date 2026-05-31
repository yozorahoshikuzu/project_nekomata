struct ObjectUniforms {
    float4x4 model;
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
    float time;
    uint textureId;
    uint samplerId;
    uint shouldInvertColor;
};

[vk::push_constant] PushConstants push_consts;
[vk::binding(0, 0)] Texture2D textures[];
[vk::binding(1, 0)] SamplerState samplers[];

struct VSOut {
    float3 position : SV_Position;
    float3 color : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
}

[shader("vertex")]
VSOut main(uint vertexID: SV_VulkanVertexID) {
    /// Pull the vertices from the vertex buffer based on SV_VulkanVertexID.
    float3 position = push_consts.vertexbuffer[vertexID].position;
    float2 uv = float2(push_consts.vertexbuffer[vertexID].uvX, push_consts.vertexbuffer[vertexID].uvY);


    VSOut output;
    output.position = position;
    output.color = float3(1.0);
    output.uv = uv;
    output.normal = push_consts.vertexbuffer[vertexID].normal;

    return output;
}

struct TCSPatchConst {
    float edges[3] : SV_TessFactor;
    float inner : SV_InsideTessFactor;
}

TCSPatchConst PatchConstantFunc(InputPatch<VSOut, 3> patch) {
    TCSPatchConst patchConst;
    patchConst.edges[0] = 10;
    patchConst.edges[1] = 10;
    patchConst.edges[2] = 10;
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
    output.positionCS = mul(push_consts.data.model, float4(position, 1.0));
    output.color = hsv_to_rgb(float3(hsv_h, hsv_s, hsv_v));
    output.uv = uv;
    output.normal = normal;
    return output;
}

namespace AcesTonemap {
    func map(x: float3) -> float3 {
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;
        return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
    }
}

[shader("fragment")]
float4 main(TESOut input, uint h : SV_PrimitiveID) : SV_Target {
    float3 col = textures[push_consts.textureId].Sample(samplers[push_consts.samplerId], input.uv).rgb;
    float3 finalCol = AcesTonemap::map(col);
    float3 xcol = lerp(finalCol, input.color, 0.1);



    return float4(xcol, 1.0);
}