use ddsfile::DxgiFormat;
use vulkanite::vk;

pub fn map_dxgi_format_to_vk(format: DxgiFormat) -> vk::Format {
    match format {
        DxgiFormat::R8_UNorm => vk::Format::R8Unorm,
        DxgiFormat::R8_SNorm => vk::Format::R8Snorm,
        DxgiFormat::R8_UInt => vk::Format::R8Uint,
        DxgiFormat::R8_SInt => vk::Format::R8Sint,
        DxgiFormat::R8G8_UNorm => vk::Format::R8G8Unorm,
        DxgiFormat::R8G8_SNorm => vk::Format::R8G8Snorm,
        DxgiFormat::R8G8_UInt => vk::Format::R8G8Uint,
        DxgiFormat::R8G8_SInt => vk::Format::R8G8Sint,
        DxgiFormat::R8G8B8A8_UNorm => vk::Format::R8G8B8A8Unorm,
        DxgiFormat::R8G8B8A8_UNorm_sRGB => vk::Format::R8G8B8A8Srgb,
        DxgiFormat::R8G8B8A8_SNorm => vk::Format::R8G8B8A8Snorm,
        DxgiFormat::R8G8B8A8_UInt => vk::Format::R8G8B8A8Uint,
        DxgiFormat::R8G8B8A8_SInt => vk::Format::R8G8B8A8Sint,


        DxgiFormat::R16_UNorm => vk::Format::R16Unorm,
        DxgiFormat::R16_SNorm => vk::Format::R16Snorm,
        DxgiFormat::R16_UInt => vk::Format::R16Uint,
        DxgiFormat::R16_SInt => vk::Format::R16Sint,
        DxgiFormat::R16_Float => vk::Format::R16Sfloat,
        DxgiFormat::R16G16_UNorm => vk::Format::R16G16Unorm,
        DxgiFormat::R16G16_SNorm => vk::Format::R16G16Snorm,
        DxgiFormat::R16G16_UInt => vk::Format::R16G16Uint,
        DxgiFormat::R16G16_SInt => vk::Format::R16G16Sint,
        DxgiFormat::R16G16_Float => vk::Format::R16G16Sfloat,
        DxgiFormat::R16G16B16A16_UNorm => vk::Format::R16G16B16A16Unorm,
        DxgiFormat::R16G16B16A16_SNorm => vk::Format::R16G16B16A16Snorm,
        DxgiFormat::R16G16B16A16_UInt => vk::Format::R16G16B16A16Uint,
        DxgiFormat::R16G16B16A16_SInt => vk::Format::R16G16B16A16Sint,
        DxgiFormat::R16G16B16A16_Float => vk::Format::R16G16B16A16Sfloat,

        DxgiFormat::R32_UInt => vk::Format::R32Uint,
        DxgiFormat::R32_SInt => vk::Format::R32Sint,
        DxgiFormat::R32_Float => vk::Format::R32Sfloat,
        DxgiFormat::R32G32_UInt => vk::Format::R32G32Uint,
        DxgiFormat::R32G32_SInt => vk::Format::R32G32Sint,
        DxgiFormat::R32G32_Float => vk::Format::R32G32Sfloat,
        DxgiFormat::R32G32B32_UInt => vk::Format::R32G32B32Uint,
        DxgiFormat::R32G32B32_SInt => vk::Format::R32G32B32Sint,
        DxgiFormat::R32G32B32_Float => vk::Format::R32G32B32Sfloat,
        DxgiFormat::R32G32B32A32_UInt => vk::Format::R32G32B32A32Uint,
        DxgiFormat::R32G32B32A32_SInt => vk::Format::R32G32B32A32Sint,
        DxgiFormat::R32G32B32A32_Float => vk::Format::R32G32B32A32Sfloat,

        DxgiFormat::BC1_UNorm => vk::Format::Bc1RgbaUnormBlock,
        DxgiFormat::BC1_UNorm_sRGB => vk::Format::Bc1RgbaSrgbBlock,

        DxgiFormat::BC2_UNorm => vk::Format::Bc2UnormBlock,
        DxgiFormat::BC2_UNorm_sRGB => vk::Format::Bc2SrgbBlock,

        DxgiFormat::BC3_UNorm => vk::Format::Bc3UnormBlock,
        DxgiFormat::BC3_UNorm_sRGB => vk::Format::Bc3SrgbBlock,

        DxgiFormat::BC4_UNorm => vk::Format::Bc4UnormBlock,
        DxgiFormat::BC4_SNorm => vk::Format::Bc4SnormBlock,

        DxgiFormat::BC5_UNorm => vk::Format::Bc5UnormBlock,
        DxgiFormat::BC5_SNorm => vk::Format::Bc5SnormBlock,

        DxgiFormat::BC6H_UF16 => vk::Format::Bc6HUfloatBlock,
        DxgiFormat::BC6H_SF16 => vk::Format::Bc6HSfloatBlock,

        DxgiFormat::BC7_UNorm => vk::Format::Bc7UnormBlock,
        DxgiFormat::BC7_UNorm_sRGB => vk::Format::Bc7SrgbBlock,
        
        _ => vk::Format::Undefined,
    }
}