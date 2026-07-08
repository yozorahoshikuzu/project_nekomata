import "../normalpack/normalpack";

struct GlobalData {
    float4x4 projview;
    float4x4 projviewInverse;
    float3 cameraPos;
};

struct Pointlight {
    float3 position;
    float3 lightRadiance;
};

struct PushConstants {
    GlobalData* globalData;
    Pointlight* pointlights;
    uint pointlightCount;
    uint depthTextureIndex;
    uint albedoAndRoughnessTextureIndex;
    uint normalTextureIndex;
    uint metallicAoTextureIndex;
    uint skyboxTextureId;
    uint irradianceTextureId;
    uint prefiltTextureId;
    uint iblLutTextureId;
    uint linearSamplerIndex;
    uint nearestSamplerIndex;
};

[vk::push_constant] PushConstants pushConstants;
[vk::binding(0, 0)] Texture2D s_textures2D[];
[vk::binding(0, 0)] TextureCube s_texturesCube[];
[vk::binding(1, 0)] SamplerState s_samplers[];

struct VSOut {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float2 positionCS : TEXCOORD1;
}

[shader("vertex")]
VSOut main(uint vertexId : SV_VulkanVertexID) {
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 pos = uv * 2.0f - 1.0f;

    VSOut output;
    output.position = float4(pos, 0.0f, 1.0f);
    output.texcoord = uv;
    output.positionCS = pos;
    return output;
}

// ---------------------------------------------------------------------------------------------------------------------

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

float3 reconstructPositionWS(float2 ndc, float depth) {
    float4 positionSS = float4(ndc, depth, 1.0);
    float4 positionWS = mul(pushConstants.globalData.projviewInverse, positionSS);
    return positionWS.xyz / positionWS.w;
}

float3 reconstructViewdir(float2 ndc) {
    const float near = 0.0001;

    float4 nearClip = float4(ndc, near, 1.0);
    float4 worldNear = mul(pushConstants.globalData.projviewInverse, nearClip);

    worldNear.xyz /= worldNear.w;

    float3 viewDir = normalize(worldNear.xyz - pushConstants.globalData.cameraPos);
    viewDir.y *= -1.0;

    return viewDir;
}

float3 fresnelSchlickRoughnessApprox(float costheta, float3 f0, float roughness) {
    return f0 + (max(float3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - costheta, 0.0, 1.0), 5.0);
}
float3 fresnelSchlick(float cos_theta, float3 f0) {
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

constexpr static float roughnessMaxLod = 7.0f;
float3 computePixelColor(float3 fragmentPosWS, float3 norm, float3 albedo, float metallic, float roughness, float ao) {
    float3 viewdir = normalize(pushConstants.globalData.cameraPos - fragmentPosWS);
    float3 accum = float3(0.0);

    // ---- The static pointlights
    for (uint i = 0; i < pushConstants.pointlightCount; i++) {
        let light = pushConstants.pointlights[i];
        float3 lz = light.position - fragmentPosWS;
        float3 l = normalize(lz);
        float3 ld = length(lz);
        float3 radiance = light.lightRadiance / (ld * ld);
        accum += computeLightContribution(norm, viewdir, l, radiance, albedo, metallic, roughness);
    }

    // ---- Diffuse IBL ------------------------------------------------------------------------------------------------
    float3 irradiance = s_texturesCube[pushConstants.irradianceTextureId].Sample(s_samplers[pushConstants.linearSamplerIndex], norm).rgb;
    float3 diffuseibl = irradiance * albedo;

    // ---- Specular IBL -----------------------------------------------------------------------------------------------
    float nv = max(dot(norm, viewdir), 0.0001);
    float3 r = reflect(-viewdir, norm);

    float3 prefiltered = s_texturesCube[pushConstants.prefiltTextureId].SampleLevel(s_samplers[pushConstants.linearSamplerIndex], r, roughness * roughnessMaxLod).rgb;
    float2 envbrdf = s_textures2D[pushConstants.iblLutTextureId].Sample(s_samplers[pushConstants.linearSamplerIndex], float2(nv, roughness)).rg;

    float3 f0 = lerp(float3(0.04), albedo, metallic);
    float3 fibl = fresnelSchlickRoughnessApprox(max(dot(norm, viewdir), 0.0001), f0, roughness);
    float3 kdibl = (float3(1.0) - fibl) * (1.0 - metallic);

    float3 specularibl = prefiltered * (fibl * envbrdf.x + envbrdf.y);
    float3 ambient = (kdibl * diffuseibl + specularibl) * ao;

    return ambient + accum;
}

[shader("fragment")]
float4 main(VSOut input) : SV_Target {
    let depth = s_textures2D[pushConstants.depthTextureIndex].SampleLevel(s_samplers[pushConstants.nearestSamplerIndex], input.texcoord, 0).r;
    let albedoAndRoughness = s_textures2D[pushConstants.albedoAndRoughnessTextureIndex].SampleLevel(s_samplers[pushConstants.nearestSamplerIndex], input.texcoord, 0);
    let pn = s_textures2D[pushConstants.normalTextureIndex].SampleLevel(s_samplers[pushConstants.nearestSamplerIndex], input.texcoord, 0).xy;
    let metallicAo = s_textures2D[pushConstants.metallicAoTextureIndex].SampleLevel(s_samplers[pushConstants.nearestSamplerIndex], input.texcoord, 0).xy;

    float3 viewdir = reconstructViewdir(input.positionCS);
    float3 vddx = ddx(viewdir);
    float3 vddy = ddy(viewdir);

    if (depth == 0.0) {
        return s_texturesCube[pushConstants.skyboxTextureId].SampleGrad(s_samplers[pushConstants.linearSamplerIndex], viewdir, vddx, vddy);
    }

    float3 position = reconstructPositionWS(input.positionCS, depth);
    float3 albedo = albedoAndRoughness.rgb;
    float3 n = octahedronNormalUnpack(pn);
    float roughness = albedoAndRoughness.a;
    float metallic = metallicAo.r;
    float ao = metallicAo.g;

    float3 color = computePixelColor(position, n, albedo, metallic, roughness, ao);
    return float4(AcesTonemap::map(color), 1.0);
}