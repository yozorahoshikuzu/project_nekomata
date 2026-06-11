module nekomata2.graphics.vulkan.spv_shader_code;
import nekomata2.core.log;

namespace nekomata2 {

SpirvShaderCode::SpirvShaderCode(std::nullptr_t) {}
SpirvShaderCode::SpirvShaderCode(std::vector<u32>&& spvCode) : m_spvCode(std::move(spvCode)) {}

auto SpirvShaderCode::loadFromFile(const std::filesystem::path& path) -> std::expected<SpirvShaderCode, ShaderLoadError> {
    log::info("loading file: {}", path.string());
    std::ifstream shaderCodeFile(path, std::ios::binary | std::ios::ate);
    if (!shaderCodeFile) {
        log::crit("Failed to read shader SPIR-V code at {}: cannot access file", path.string());
        return std::unexpected(ShaderLoadError::FileLoadingError);
    }
    
    auto fileSize = shaderCodeFile.tellg();
    
    if ((fileSize & 0b11) != 0) {
        log::crit("Failed to read shader SPIR-V code at {}: file size is not a multiple of 4", path.string());
        return std::unexpected(ShaderLoadError::MisalignedFileSize);
    }

    auto shaderCodeDwordCount = fileSize / 4;
    std::vector<u32> spvCode(shaderCodeDwordCount);

    shaderCodeFile.seekg(0);
    shaderCodeFile.read(reinterpret_cast<char*>(spvCode.data()), fileSize);
    if (!shaderCodeFile) {
        log::crit("Failed to read shader SPIR-V code at {}: could not read all dwords", path.string());
        return std::unexpected(ShaderLoadError::FileReadingError);
    }

    return SpirvShaderCode(std::move(spvCode));
}

auto SpirvShaderCode::shaderModuleCreateInfo() const -> vk::ShaderModuleCreateInfo {
    return vk::ShaderModuleCreateInfo{}
        .setCode(m_spvCode);
}

}