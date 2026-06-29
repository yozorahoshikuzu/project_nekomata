export module nekomata2:graphics.vulkan.vk_descriptor_set_layout;
import std;
import vulkan;
import :core.platform.int_def;
import :graphics.vulkan.context;
import :graphics.vulkan.vk_gpu_obrm;

export namespace nekomata2 {

class VulkanDescriptorSetLayoutBuilder;
class VulkanDescriptorSetLayout {
public:
    VulkanDescriptorSetLayout(std::nullptr_t);
    VulkanDescriptorSetLayout(vk::raii::DescriptorSetLayout&& vkDescriptorSetLayout);

    [[nodiscard]] static auto builder() -> VulkanDescriptorSetLayoutBuilder;

    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&&) = default;
    VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&&) = default;

    [[nodiscard]] auto vkDescriptorSetLayout() const -> const vk::raii::DescriptorSetLayout& { return m_vkDescriptorSetLayout.vkHandle(); }

private:
    VulkanAsyncRaiiWrapper<vk::raii::DescriptorSetLayout> m_vkDescriptorSetLayout = nullptr;
};

class VulkanDescriptorSetLayoutBuilder {
public:
    constexpr VulkanDescriptorSetLayoutBuilder() = default;
    constexpr VulkanDescriptorSetLayoutBuilder(std::nullptr_t) {}

    VulkanDescriptorSetLayoutBuilder(const VulkanDescriptorSetLayoutBuilder&) = delete;
    VulkanDescriptorSetLayoutBuilder(VulkanDescriptorSetLayoutBuilder&&) = default;
    VulkanDescriptorSetLayoutBuilder& operator=(const VulkanDescriptorSetLayoutBuilder&) = delete;
    VulkanDescriptorSetLayoutBuilder& operator=(VulkanDescriptorSetLayoutBuilder&&) = default;

    [[nodiscard]] constexpr auto addBinding(const u32 bindingIndex, const u32 descriptorCount, const vk::DescriptorType descriptorType, const vk::ShaderStageFlags shaderStages) noexcept -> VulkanDescriptorSetLayoutBuilder& {
        return addBindingWithFlags(bindingIndex, descriptorCount, descriptorType, shaderStages, vk::DescriptorBindingFlags{});
    }

    [[nodiscard]] constexpr auto addBindingWithFlags(const u32 bindingIndex, const u32 descriptorCount, const vk::DescriptorType descriptorType, const vk::ShaderStageFlags shaderStages, const vk::DescriptorBindingFlags flags) noexcept -> VulkanDescriptorSetLayoutBuilder& {
        auto binding = vk::DescriptorSetLayoutBinding{}
            .setBinding(bindingIndex)
            .setDescriptorType(descriptorType)
            .setDescriptorCount(descriptorCount)
            .setStageFlags(shaderStages);

        m_bindings.emplace_back(binding);
        m_bindingFlags.emplace_back(flags);
        return *this;
    }

    [[nodiscard]] constexpr auto setFlags(const vk::DescriptorSetLayoutCreateFlags flags) noexcept -> VulkanDescriptorSetLayoutBuilder& {
        m_flags = flags;
        return *this;
    }

    [[nodiscard]] constexpr auto build() const -> VulkanDescriptorSetLayout {
        auto descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo{}
            .setBindings(m_bindings)
            .setFlags(m_flags);

        auto descriptorBindingFlagsInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{}
            .setBindingFlags(m_bindingFlags);

        auto sc = vk::StructureChain{
            descriptorSetLayoutCreateInfo,
            descriptorBindingFlagsInfo
        };

        return VulkanDescriptorSetLayout(vkCheckResult(VulkanContext::get().vkDevice().createDescriptorSetLayout(sc.get<vk::DescriptorSetLayoutCreateInfo>())));
    }

private:
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
    std::vector<vk::DescriptorBindingFlags> m_bindingFlags;
    vk::DescriptorSetLayoutCreateFlags m_flags = {};
};

}