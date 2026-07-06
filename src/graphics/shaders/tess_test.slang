struct ObjectUniforms {
    float4x4 model;
    float3x3 normalMatrix;
};

struct GlobalData {
    float4x4 projview;
    float3 cameraPos;
};

struct Pointlight {
    float3 position;
    float3 lightRadiance;
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
    Pointlight* pointlights;
    uint pointlightCount;
    uint textureId;
    uint samplerId;
    float time;
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
    float3 position = push_consts.vertexbuffer[vertexID].position;
    float3 normal = push_consts.vertexbuffer[vertexID].normal;
    float2 uv = float2(push_consts.vertexbuffer[vertexID].uvX, push_consts.vertexbuffer[vertexID].uvY);

    float4 hposition = mul(push_consts.data.model, float4(position, 1.0));

    VSOut output;
    output.position = hposition.xyz / hposition.w;
    output.color = float3(1.0);
    output.uv = uv;
    output.normal = mul(push_consts.data.normalMatrix, normal);

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
    output.positionCS = mul(push_consts.globalData.projview, float4(position, 1.0));
    output.positionWS = position;
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

// ---- PBR stuff ------------------------------------------------------------------------------------------------------

float3 fresnelSchlickRoughnessApprox(float costheta, float3 f0, float roughness) {
    return f0 + (max(float3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - costheta, 0.0, 1.0), 5.0);
}
func fresnelSchlick(cos_theta: float, f0: float3) -> float3 {
    return f0 + (float3(1.0) - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float ggxDist(float nh, float roughness) {
    float nh2 = nh * nh;

    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

    float denomPrec = nh2 * (alpha2 - 1.0) + 1.0;
    float denom = float::getPi() * denomPrec * denomPrec;

    return alpha2 / denom;
}

float schlickGgxGeometryTerm(float nv, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float denom = nv * (1.0 - k) + k;
    return nv / denom;
}

float smithGeometryTerm(float nv, float nl, float roughness) {
    return schlickGgxGeometryTerm(nv, roughness) * schlickGgxGeometryTerm(nl, roughness);
}

float3 brdf(float3 norm, float3 viewdir, float3 lightdir, float nl, float3 albedo, float metallic, float roughness) {
    float3 halfvec = normalize(viewdir + lightdir);
    float  nv = max(dot(norm, viewdir), 0.0001);

    float3 f0 = lerp(float3(0.04), albedo, metallic);
    float  nh = max(dot(norm, halfvec), 0.0);
    float  vh = max(dot(viewdir, halfvec), 0.0);
    float  lh = max(dot(lightdir, halfvec), 0.0);

    float  ndf = ggxDist(nh, roughness);
    float  g = smithGeometryTerm(nv, nl, roughness);
    float3 f = fresnelSchlick(vh, f0);

    float3 kS = f;
    float3 kD = (float3(1.0) - kS) * (1.0 - metallic);

    float3 specular = (ndf * g * f) / (4.0 * nv * nl + 0.0001);
    float  fd90 = 0.5 + 2.0 * lh * lh * roughness;

    float lightScatter = 1.0 + (fd90 - 1.0) * pow(1.0 - nl, 5);
    float viewScatter = 1.0 + (fd90 - 1.0) * pow(1.0 - nv, 5);

    float3 diffuse = kD * albedo * lightScatter * viewScatter / float::getPi();

    return diffuse + specular;
}

float3 computeLightContribution(float3 norm, float3 viewdir, float3 lightdir, float3 radiance, float3 albedo, float metallic, float roughness) {
    float nl = max(dot(norm, lightdir), 0.0001);
    float3 brdfterm = brdf(norm, viewdir, lightdir, nl, albedo, metallic, roughness);
    return brdfterm * radiance * nl;
}

float3 computePixelColor(float3 fragmentPosWS, float3 norm, float3 albedo, float metallic, float roughness, float ao) {
    float3 viewdir = normalize(push_consts.globalData.cameraPos - fragmentPosWS);
    float3 accum = float3(0.0);

    // ---- The static pointlights
    for (uint i = 0; i < push_consts.pointlightCount; i++) {
        let light = push_consts.pointlights[i];
        float3 lz = light.position - fragmentPosWS;
        float3 l = normalize(lz);
        float3 ld = length(lz);
        float3 radiance = light.lightRadiance / (ld * ld);
        accum += computeLightContribution(norm, viewdir, l, radiance, albedo, metallic, roughness);
    }

    float3 ambient = float3(0.03) * albedo * ao;
    return accum + ambient;
}

[shader("fragment")]
float4 main(TESOut input) : SV_Target {
    float3 albedo = textures[push_consts.textureId].Sample(samplers[push_consts.samplerId], input.uv).rgb;
    float metallic = albedo.r;
    float roughness = albedo.g;
    float ao = albedo.b;

    float3 norm = normalize(input.normal);

    float3 col = computePixelColor(input.positionWS, norm, albedo, metallic, roughness, ao);

    float3 finalCol = AcesTonemap::map(col);

    return float4(finalCol, 1.0);
}