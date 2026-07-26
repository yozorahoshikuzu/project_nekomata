module projnekomata;
import projnekomata.cs;
import :graphics.srt.bindless_descriptor_set_srt;

namespace projnekomata::graphics::srt {

BindlessDescriptorSetShaderResourceTable::BindlessDescriptorSetShaderResourceTable(std::nullptr_t) {}
BindlessDescriptorSetShaderResourceTable::BindlessDescriptorSetShaderResourceTable(VulkanDescriptorPool&& descriptorPool,
    VulkanDescriptorSetLayout&& descriptorSetLayout, VulkanDescriptorSet&& descriptorSet, u32 maxSampledImageCount, u32 maxStorageImageCount, u32 maxSamplerCount)
        : m_descriptorPool(std::move(descriptorPool)), m_descriptorSetLayout(std::move(descriptorSetLayout)), m_descriptorSet(std::move(descriptorSet)),
            m_sampledImageIndexAllocator(maxSampledImageCount), m_storageImageIndexAllocator(maxStorageImageCount), m_maxSamplerCount(maxSamplerCount) {}

auto BindlessDescriptorSetShaderResourceTable::create(u32 maxSampledImageCount, u32 maxStorageImageCount, u32 maxSamplerCount) -> Unique<BindlessDescriptorSetShaderResourceTable> {
    auto descriptorSetLayout = VulkanDescriptorSetLayout::builder()
        .addBindingWithFlags(0, maxSampledImageCount, vk::DescriptorType::eSampledImage,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending
        )
        .addBindingWithFlags(1, maxStorageImageCount, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending
        )
        .addBindingWithFlags(2, maxSamplerCount, vk::DescriptorType::eSampler,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending
        )
        .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
        .build();

    auto descriptorPool = VulkanDescriptorPool::builder()
        .setMaxSets(1)
        .setUpdateAfterBindPool(true)
        .setFreeDescriptorSetPool(true)
        .addPoolSize(vk::DescriptorType::eSampledImage, maxSampledImageCount)
        .addPoolSize(vk::DescriptorType::eStorageImage, maxStorageImageCount)
        .addPoolSize(vk::DescriptorType::eSampler, maxSamplerCount)
        .build();

    auto descriptorSet = descriptorPool.allocateDescriptorSet(descriptorSetLayout);

    return Unique<BindlessDescriptorSetShaderResourceTable>::create(std::move(descriptorPool), std::move(descriptorSetLayout), std::move(descriptorSet),
        maxSampledImageCount, maxStorageImageCount, maxSamplerCount);
}

auto BindlessDescriptorSetShaderResourceTable::allocateSampledImageIndex() -> SRTResourceIndex {
    auto index = m_sampledImageIndexAllocator.allocate();
    if (index.isNone()) {
        panic("bindless descriptor set sampled image index allocator ran out of indices");
    }
    return SRTResourceIndex(index.unwrap());
}

auto BindlessDescriptorSetShaderResourceTable::allocateSampledImageIndices(u32 count, Slice<SRTResourceIndex> dstIndices) -> void {
    for (u32 i = 0; i < count; i++) {
        dstIndices[i] = allocateSampledImageIndex();
    }
}

auto BindlessDescriptorSetShaderResourceTable::freeSampledImageIndex(SRTResourceIndex index) -> void {
    m_sampledImageIndexAllocator.release(index.imageIndex);
}

auto BindlessDescriptorSetShaderResourceTable::freeSampledImageIndices(Slice<const SRTResourceIndex> indices) -> void {
    for (auto index : indices) {
        freeSampledImageIndex(index);
    }
}
auto BindlessDescriptorSetShaderResourceTable::allocateStorageImageIndex() -> SRTResourceIndex {
    auto index = m_storageImageIndexAllocator.allocate();
    if (index.isNone()) {
        panic("bindless descriptor set storage image index allocator ran out of indices");
    }
    return SRTResourceIndex(index.unwrap());
}
auto BindlessDescriptorSetShaderResourceTable::allocateStorageImageIndices(u32 count, Slice<SRTResourceIndex> dstIndices) -> void {
    for (u32 i = 0; i < count; i++) {
        dstIndices[i] = allocateStorageImageIndex();
    }
}
auto BindlessDescriptorSetShaderResourceTable::freeStorageImageIndex(SRTResourceIndex index) -> void {
    m_storageImageIndexAllocator.release(index.imageIndex);
}
auto BindlessDescriptorSetShaderResourceTable::freeStorageImageIndices(Slice<const SRTResourceIndex> indices) -> void {
    for (auto index : indices) {
        freeStorageImageIndex(index);
    }
}

auto BindlessDescriptorSetShaderResourceTable::allocateSamplerIndex() -> SRTResourceIndex {
    auto index = m_nextSamplerIndex.fetch_add(1, std::memory_order_relaxed);
    if (index >= m_maxSamplerCount) {
        panic("bindless descriptor set sampler index allocator ran out of indices");
    }
    return SRTResourceIndex(index);
}

auto BindlessDescriptorSetShaderResourceTable::allocateSamplerIndices(u32 count, Slice<SRTResourceIndex> dstIndices) -> void {
    for (u32 i = 0; i < count; i++) {
        dstIndices[i] = allocateSamplerIndex();
    }
}

auto BindlessDescriptorSetShaderResourceTable::bindSampledImage(const VulkanImage& image, SRTResourceIndex index) -> void {
    VulkanDescriptorSetWriter(m_descriptorSet)
        .bindSampledImage(0, index.imageIndex, image)
        .commit();
}
auto BindlessDescriptorSetShaderResourceTable::bindSampledImageView(const VulkanImageView& imageView, SRTResourceIndex index) -> void {
    VulkanDescriptorSetWriter(m_descriptorSet)
        .bindSampledImage(0, index.imageIndex, imageView)
        .commit();
}
auto BindlessDescriptorSetShaderResourceTable::bindStorageImage(const VulkanImage& image, SRTResourceIndex index) -> void {
    VulkanDescriptorSetWriter(m_descriptorSet)
        .bindStorageImage(1, index.imageIndex, image)
        .commit();
}
auto BindlessDescriptorSetShaderResourceTable::bindStorageImageView(const VulkanImageView& imageView, SRTResourceIndex index) -> void {
    VulkanDescriptorSetWriter(m_descriptorSet)
        .bindStorageImage(1, index.imageIndex, imageView)
        .commit();
}

auto BindlessDescriptorSetShaderResourceTable::bindSampler(const VulkanSampler& sampler, SRTResourceIndex index) -> void {
    VulkanDescriptorSetWriter(m_descriptorSet)
        .bindSampler(2, index.imageIndex, sampler)
        .commit();
}

auto BindlessDescriptorSetShaderResourceTable::bindToCommandBuffer(const VulkanCommandBuffer& cmd, const VulkanPipelineLayout& pipelineLayout, vk::PipelineBindPoint pipelineBindPoint) -> void {
    cmd.vkCommandBuffer().bindDescriptorSets(pipelineBindPoint, pipelineLayout.vkPipelineLayout(), 0, *m_descriptorSet.vkDescriptorSet(), nullptr);
}
auto BindlessDescriptorSetShaderResourceTable::descriptorSetLayout() const -> const VulkanDescriptorSetLayout& {
    return m_descriptorSetLayout;
}

} // namespace projnekomata::graphics::srt