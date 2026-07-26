#include "globallayout_structs.h"

// ---- Sampled Images, float4 -----------------------------------------------------------------------------------------
[vk::binding(0, 0)] Texture1D s_textures1D[];
[vk::binding(0, 0)] Texture2D s_textures2D[];
[vk::binding(0, 0)] Texture3D s_textures3D[];
[vk::binding(0, 0)] Texture1DArray s_textures1DArray[];
[vk::binding(0, 0)] Texture2DArray s_textures2DArray[];
[vk::binding(0, 0)] TextureCube s_texturesCube[];

// ---- Sampled Images, uint4 ------------------------------------------------------------------------------------------
[vk::binding(0, 0)] Texture1D<uint4> s_textures1D_u32[];
[vk::binding(0, 0)] Texture2D<uint4> s_textures2D_u32[];
[vk::binding(0, 0)] Texture3D<uint4> s_textures3D_u32[];
[vk::binding(0, 0)] Texture1DArray<uint4> s_textures1DArray_u32[];
[vk::binding(0, 0)] Texture2DArray<uint4> s_textures2DArray_u32[];
[vk::binding(0, 0)] TextureCube<uint4> s_texturesCube_u32[];

// ---- Storage Images, float -----------------------------------------------------------------------------------------
[vk::binding(1, 0)] RWTexture1D s_images1D_f32[];
[vk::binding(1, 0)] RWTexture2D s_images2D_f32[];
[vk::binding(1, 0)] RWTexture3D s_images3D_f32[];
[vk::binding(1, 0)] RWTexture1DArray s_images1DArray_f32[];
[vk::binding(1, 0)] RWTexture2DArray s_images2DArray_f32[];

// ---- Storage Images, float2 -----------------------------------------------------------------------------------------
[vk::binding(1, 0)] RWTexture1D<float2> s_images1D_f32x2[];
[vk::binding(1, 0)] RWTexture2D<float2> s_images2D_f32x2[];
[vk::binding(1, 0)] RWTexture3D<float2> s_images3D_f32x2[];
[vk::binding(1, 0)] RWTexture1DArray<float2> s_images1DArray_f32x2[];
[vk::binding(1, 0)] RWTexture2DArray<float2> s_images2DArray_f32x2[];

// ---- Storage Images, float4 -----------------------------------------------------------------------------------------
[vk::binding(1, 0)] RWTexture1D<float4> s_images1D_f32x4[];
[vk::binding(1, 0)] RWTexture2D<float4> s_images2D_f32x4[];
[vk::binding(1, 0)] RWTexture3D<float4> s_images3D_f32x4[];
[vk::binding(1, 0)] RWTexture1DArray<float4> s_images1DArray_f32x4[];
[vk::binding(1, 0)] RWTexture2DArray<float4> s_images2DArray_f32x4[];

// ---- Storage Images, uint -------------------------------------------------------------------------------------------
[vk::binding(1, 0)] RWTexture1D<uint> s_images1D_u32[];
[vk::binding(1, 0)] RWTexture2D<uint> s_images2D_u32[];
[vk::binding(1, 0)] RWTexture3D<uint> s_images3D_u32[];
[vk::binding(1, 0)] RWTexture1DArray<uint> s_images1DArray_u32[];
[vk::binding(1, 0)] RWTexture2DArray<uint> s_images2DArray_u32[];

// ---- Storage Images, uint2 ------------------------------------------------------------------------------------------

[vk::binding(1, 0)] RWTexture1D<uint2> s_images1D_u32x2[];
[vk::binding(1, 0)] RWTexture2D<uint2> s_images2D_u32x2[];
[vk::binding(1, 0)] RWTexture3D<uint2> s_images3D_u32x2[];
[vk::binding(1, 0)] RWTexture1DArray<uint2> s_images1DArray_u32x2[];
[vk::binding(1, 0)] RWTexture2DArray<uint2> s_images2DArray_u32x2[];

// ---- Storage Images, uint4 ------------------------------------------------------------------------------------------
[vk::binding(1, 0)] RWTexture1D<uint4> s_images1D_u32x4[];
[vk::binding(1, 0)] RWTexture2D<uint4> s_images2D_u32x4[];
[vk::binding(1, 0)] RWTexture3D<uint4> s_images3D_u32x4[];
[vk::binding(1, 0)] RWTexture1DArray<uint4> s_images1DArray_u32x4[];
[vk::binding(1, 0)] RWTexture2DArray<uint4> s_images2DArray_u32x4[];

// ---- Samplers -------------------------------------------------------------------------------------------------------
[vk::binding(2, 0)] SamplerState s_samplers[];

namespace pipeline {

Texture1D getTexture1DBySrtId(uint srtId) {
    return s_textures1D[srtId];
}

Texture1DArray getTexture1DArrayBySrtId(uint srtId) {
    return s_textures1DArray[srtId];
}

Texture2D getTexture2DBySrtId(uint srtId) {
    return s_textures2D[srtId];
}

Texture2DArray getTexture2DArrayBySrtId(uint srtId) {
    return s_textures2DArray[srtId];
}

Texture3D getTexture3DBySrtId(uint srtId) {
    return s_textures3D[srtId];
}

TextureCube getTextureCubeBySrtId(uint srtId) {
    return s_texturesCube[srtId];
}

SamplerState getSamplerBySrtId(uint srtId) {
    return s_samplers[srtId];
}

}

// ---- Rendering Outputs ----------------------------------------------------------------------------------------------

struct DeferredRendererFSOut {
    float4 albedoAndRoughness : SV_Target0;
    float4 normalAndGeometricNormal : SV_Target1;
    float2 metallicAndAo : SV_Target2;
    float2 velocity : SV_Target3;
}