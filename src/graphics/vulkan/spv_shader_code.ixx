export module projnekomata:graphics.vulkan.spv_shader_code;
import std;
import vulkan;
import :core.platform.int_def;
import :core.cs.result;
import :core.cs.vec;

export namespace projnekomata {

enum class ShaderLoadError {
    FileLoadingError,
    FileReadingError,
    MisalignedFileSize,
};

class SpirvShaderCode {
public:
    SpirvShaderCode(std::nullptr_t);
    SpirvShaderCode(Vec<u32>&& spvCode);

    SpirvShaderCode(const SpirvShaderCode&) = delete;
    SpirvShaderCode(SpirvShaderCode&&) = default;
    SpirvShaderCode& operator=(const SpirvShaderCode&) = delete;
    SpirvShaderCode& operator=(SpirvShaderCode&&) = default;

    static auto loadFromFile(const std::filesystem::path& path) -> Result<SpirvShaderCode, ShaderLoadError>;
    [[nodiscard]] auto shaderModuleCreateInfo() const           -> vk::ShaderModuleCreateInfo;

    [[nodiscard]] auto spvCode() const -> Slice<const u32> { return m_spvCode.asSlice(); }
private:
    Vec<u32> m_spvCode;
};

}
