export module nekomata2.graphics.vulkan.vk_descriptor_pool;
import std;
import vulkan;
import nekomata2.core.platform.int_def;
import nekomata2.graphics.vulkan.vk_gpu_obrm;
import nekomata2.graphics.vulkan.vk_descriptor_set_layout;
import nekomata2.graphics.vulkan.vk_descriptor_set;

export namespace nekomata2 {
class VulkanDescriptorPoolBuilder;

class VulkanDescriptorPool {
public:
    VulkanDescriptorPool(std::nullptr_t);
    VulkanDescriptorPool(vk::raii::DescriptorPool&& vkDescriptorPool);

    static auto create(u32 maxSets, std::span<const vk::DescriptorPoolSize> sizes, bool updateAfterBindPool, bool freeDescriptorSetPool) -> VulkanDescriptorPool;
    static auto builder() -> VulkanDescriptorPoolBuilder;

    VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool(VulkanDescriptorPool&&) = default;
    VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool& operator=(VulkanDescriptorPool&&) = default;

    auto reset() -> void;
    auto allocateDescriptorSet(const VulkanDescriptorSetLayout& descriptorSetLayout) -> VulkanDescriptorSet;
    [[nodiscard]] auto vkDescriptorPool() const -> const vk::raii::DescriptorPool& { return m_vkDescriptorPool.vkHandle(); }

private:
    VulkanAsyncRaiiWrapper<vk::raii::DescriptorPool> m_vkDescriptorPool = nullptr;
};

class VulkanDescriptorPoolBuilder {
public:
    constexpr VulkanDescriptorPoolBuilder() = default;
    constexpr VulkanDescriptorPoolBuilder(std::nullptr_t) {}

    VulkanDescriptorPoolBuilder(const VulkanDescriptorPoolBuilder&) = delete;
    VulkanDescriptorPoolBuilder(VulkanDescriptorPoolBuilder&&) = default;
    VulkanDescriptorPoolBuilder& operator=(const VulkanDescriptorPoolBuilder&) = delete;
    VulkanDescriptorPoolBuilder& operator=(VulkanDescriptorPoolBuilder&&) = default;

    [[nodiscard]] constexpr auto addPoolSize(const vk::DescriptorType type, const u32 count) noexcept -> VulkanDescriptorPoolBuilder& {
        auto poolSize = vk::DescriptorPoolSize{}
            .setType(type)
            .setDescriptorCount(count);
        m_poolSizes.emplace_back(poolSize);
        return *this;
    }

    [[nodiscard]] constexpr auto setMaxSets(const u32 maxSets) noexcept -> VulkanDescriptorPoolBuilder& {
        m_maxSets = maxSets;
        return *this;
    }

    [[nodiscard]] constexpr auto setUpdateAfterBindPool(const bool updateAfterBindPool) noexcept -> VulkanDescriptorPoolBuilder& {
        m_updateAfterBindPool = updateAfterBindPool;
        return *this;
    }

    [[nodiscard]] constexpr auto setFreeDescriptorSetPool(const bool freeDescriptorSetPool) noexcept -> VulkanDescriptorPoolBuilder& {
        m_freeDescriptorSetPool = freeDescriptorSetPool;
        return *this;
    }

    [[nodiscard]] constexpr auto build() const -> VulkanDescriptorPool {
        return VulkanDescriptorPool::create(m_maxSets, m_poolSizes, m_updateAfterBindPool, m_freeDescriptorSetPool);
    }

private:
    std::vector<vk::DescriptorPoolSize> m_poolSizes;
    u32 m_maxSets = 0;
    bool m_updateAfterBindPool = false;
    bool m_freeDescriptorSetPool = false;
};

}