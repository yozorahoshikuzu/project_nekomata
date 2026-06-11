module nekomata2;
import :core.log;
import :graphics.srt.bindless_descriptor_set_srt;

namespace nekomata2::graphics::srt {

BindlessDescriptorSetShaderResourceTable::BindlessDescriptorSetShaderResourceTable(std::nullptr_t) {}
BindlessDescriptorSetShaderResourceTable::BindlessDescriptorSetShaderResourceTable(VulkanDescriptorPool&& descriptorPool,
    VulkanDescriptorSetLayout&& descriptorSetLayout, VulkanDescriptorSet&& descriptorSet, u32 maxImageCount, u32 maxSamplerCount)
        : m_descriptorPool(std::move(descriptorPool)), m_descriptorSetLayout(std::move(descriptorSetLayout)), m_descriptorSet(std::move(descriptorSet)),
            m_imageIndexAllocator(maxImageCount), m_maxSamplerCount(maxSamplerCount) {}

auto BindlessDescriptorSetShaderResourceTable::create(u32 maxImageCount, u32 maxSamplerCount) -> std::unique_ptr<BindlessDescriptorSetShaderResourceTable> {
    auto descriptorSetLayout = VulkanDescriptorSetLayout::builder()
        .addBindingWithFlags(0, maxImageCount, vk::DescriptorType::eSampledImage,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending)
        .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
        .addBindingWithFlags(1, maxSamplerCount, vk::DescriptorType::eSampler,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending
        )
        .build();

    auto descriptorPool = VulkanDescriptorPool::builder()
        .setMaxSets(1)
        .setUpdateAfterBindPool(true)
        .setFreeDescriptorSetPool(true)
        .addPoolSize(vk::DescriptorType::eSampledImage, maxImageCount)
        .addPoolSize(vk::DescriptorType::eSampler, maxSamplerCount)
        .build();

    auto descriptorSet = descriptorPool.allocateDescriptorSet(descriptorSetLayout);

    return std::make_unique<BindlessDescriptorSetShaderResourceTable>(std::move(descriptorPool), std::move(descriptorSetLayout), std::move(descriptorSet),
        maxImageCount, maxSamplerCount);
}

auto BindlessDescriptorSetShaderResourceTable::allocateImageIndex() -> SRTResourceIndex {
    auto index = m_imageIndexAllocator.allocate();
    if (!index.has_value()) {
        log::crit("Bindless descriptor set image index allocator ran out of indices!");
        throw std::runtime_error("out of image indices");
    }
    return SRTResourceIndex(index.value());
}

auto BindlessDescriptorSetShaderResourceTable::allocateImageIndices(u32 count, std::span<SRTResourceIndex> dstIndices) -> void {
    for (u32 i = 0; i < count; i++) {
        dstIndices[i] = allocateImageIndex();
    }
}

auto BindlessDescriptorSetShaderResourceTable::freeImageIndex(SRTResourceIndex index) -> void {
    m_imageIndexAllocator.release(index.imageIndex);
}

auto BindlessDescriptorSetShaderResourceTable::freeImageIndices(std::span<SRTResourceIndex> indices) -> void {
    for (auto index : indices) {
        freeImageIndex(index);
    }
}

auto BindlessDescriptorSetShaderResourceTable::allocateSamplerIndex() -> SRTResourceIndex {
    auto index = m_nextSamplerIndex.fetch_add(1, std::memory_order_relaxed);
    if (index >= m_maxSamplerCount) {
        log::crit("Bindless descriptor set sampler index allocator ran out of indices!");
        throw std::runtime_error("out of sampler indices");
    }
    return SRTResourceIndex(index);
}

auto BindlessDescriptorSetShaderResourceTable::allocateSamplerIndices(u32 count, std::span<SRTResourceIndex> dstIndices) -> void {
    for (u32 i = 0; i < count; i++) {
        dstIndices[i] = allocateSamplerIndex();
    }
}

auto BindlessDescriptorSetShaderResourceTable::bindImage(const VulkanImage& image, SRTResourceIndex index) -> void {
    VulkanDescriptorSetWriter(m_descriptorSet)
        .bindImage(0, index.imageIndex, image)
        .commit();
}

auto BindlessDescriptorSetShaderResourceTable::bindSampler(const VulkanSampler& sampler, SRTResourceIndex index) -> void {
    VulkanDescriptorSetWriter(m_descriptorSet)
        .bindSampler(1, index.imageIndex, sampler)
        .commit();
}

auto BindlessDescriptorSetShaderResourceTable::bindToCommandBuffer(const VulkanCommandBuffer& cmd, const VulkanPipelineLayout& pipelineLayout, vk::PipelineBindPoint pipelineBindPoint) -> void {
    cmd.vkCommandBuffer().bindDescriptorSets(pipelineBindPoint, pipelineLayout.vkPipelineLayout(), 0, *m_descriptorSet.vkDescriptorSet(), nullptr);
}
auto BindlessDescriptorSetShaderResourceTable::descriptorSetLayout() const -> const VulkanDescriptorSetLayout& {
    return m_descriptorSetLayout;
}

} // namespace nekomata2::graphics::srt