#pragma once
#include "cpp_kw_shim.h"

SH_EXPORT namespace projnekomata {

enum class MaterialPropType : uint {
    Color = 0,
    Texture = 1,
};

struct MaterialTextureProperty {
    uint samplerSrtID;
    uint textureSrtID;
};

struct MaterialColorProperty {
    float4 color;
};

}