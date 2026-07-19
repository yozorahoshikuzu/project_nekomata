#include "globallayout_structs.h"

[vk::binding(0, 0)] Texture1D s_textures1D[];
[vk::binding(0, 0)] Texture1DArray s_textures1DArray[];
[vk::binding(0, 0)] Texture2D s_textures2D[];
[vk::binding(0, 0)] Texture2DArray s_textures2DArray[];
[vk::binding(0, 0)] Texture3D s_textures3D[];
[vk::binding(0, 0)] TextureCube s_texturesCube[];
[vk::binding(1, 0)] SamplerState s_samplers[];

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