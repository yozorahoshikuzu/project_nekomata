export module nekomata2:graphics.vulkan.vk_pipeline_layout;
import std;
import vulkan;
import :core.platform.int_def;
import :graphics.vulkan.context;
import :graphics.vulkan.vk_gpu_obrm;

export namespace nekomata2 {

class VulkanPipelineLayoutBuilder;
class VulkanPipelineLayout {
public:
    VulkanPipelineLayout(std::nullptr_t);
    VulkanPipelineLayout(vk::raii::PipelineLayout&& vkPipelineLayout);

    VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout(VulkanPipelineLayout&&) = default;
    VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout& operator=(VulkanPipelineLayout&&) = default;

    // TODO: refactor to return directly in the header
    static auto builder() -> VulkanPipelineLayoutBuilder;

    [[nodiscard]] auto vkPipelineLayout() const -> const vk::raii::PipelineLayout& { return m_vkPipelineLayout.vkHandle(); }

private:
    VulkanAsyncRaiiWrapper<vk::raii::PipelineLayout> m_vkPipelineLayout = nullptr;
};

class VulkanPipelineLayoutBuilder {
public:
    constexpr VulkanPipelineLayoutBuilder() = default;
    constexpr VulkanPipelineLayoutBuilder(std::nullptr_t) {}

    VulkanPipelineLayoutBuilder(const VulkanPipelineLayoutBuilder&) = delete;
    VulkanPipelineLayoutBuilder(VulkanPipelineLayoutBuilder&&) = default;
    VulkanPipelineLayoutBuilder& operator=(const VulkanPipelineLayoutBuilder&) = delete;
    VulkanPipelineLayoutBuilder& operator=(VulkanPipelineLayoutBuilder&&) = default;

    // TODO: change to VulkanDescriptorSetLayout type
    [[nodiscard]] constexpr auto addDescriptorSetLayout(vk::DescriptorSetLayout layout) noexcept -> VulkanPipelineLayoutBuilder& {
        m_descriptorSetLayouts.emplace_back(layout);
        return *this;
    }
    [[nodiscard]] constexpr auto addPushConstantRange(u32 offset, u32 size, vk::ShaderStageFlags stageFlags) noexcept -> VulkanPipelineLayoutBuilder& {
        auto range = vk::PushConstantRange{}.setStageFlags(stageFlags).setOffset(offset).setSize(size);

        m_pushConstantRanges.emplace_back(range);
        return *this;
    }
    [[nodiscard]] constexpr auto buildCreateInfo() const noexcept -> vk::PipelineLayoutCreateInfo {
        return vk::PipelineLayoutCreateInfo{}.setSetLayouts(m_descriptorSetLayouts).setPushConstantRanges(m_pushConstantRanges);
    }
    [[nodiscard]] constexpr auto build() const -> VulkanPipelineLayout {
        auto pipelineLayout = VulkanContext::get().vkDevice().createPipelineLayout(buildCreateInfo());

        return VulkanPipelineLayout(std::move(pipelineLayout));
    }

private:
    std::vector<vk::PushConstantRange> m_pushConstantRanges;
    std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
};

} // namespace nekomata2