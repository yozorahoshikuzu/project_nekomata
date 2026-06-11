export module nekomata2.graphics.vulkan.spv_shader_code;
import std;
import vulkan;
import nekomata2.core.platform.int_def;

export namespace nekomata2 {

enum class ShaderLoadError {
    FileLoadingError,
    FileReadingError,
    MisalignedFileSize,
};

class SpirvShaderCode {
public:
    SpirvShaderCode(std::nullptr_t);
    SpirvShaderCode(std::vector<u32>&& spvCode);

    SpirvShaderCode(const SpirvShaderCode&) = delete;
    SpirvShaderCode(SpirvShaderCode&&) = default;
    SpirvShaderCode& operator=(const SpirvShaderCode&) = delete;
    SpirvShaderCode& operator=(SpirvShaderCode&&) = default;

    static auto loadFromFile(const std::filesystem::path& path) -> std::expected<SpirvShaderCode, ShaderLoadError>;
    [[nodiscard]] auto shaderModuleCreateInfo() const           -> vk::ShaderModuleCreateInfo;

    [[nodiscard]] auto spvCode() const -> std::span<const u32> { return m_spvCode; }
private:
    std::vector<u32> m_spvCode;
};

}
