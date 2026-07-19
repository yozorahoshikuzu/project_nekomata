#pragma once

#ifdef __cplusplus
#define SH_EXPORT export
#define float4x4 math::Matrix4x4f
#define float2 math::Vector2f
#define float3 math::Vector3f
#define float4 math::Vector4f
#define uint u32
#else
#define SH_EXPORT
#endif

SH_EXPORT namespace projnekomata {

struct ShGlobalData {
    float4x4 jitteredProjview;
    float4x4 projview;
    float4x4 prevProjview;
    float4x4 prevProjviewNoTranslation;
    float4x4 projviewInverse;
    float4x4 projviewNoTranslationInverse;
    float3 cameraPos;
    uint frameIndex;
};

}
