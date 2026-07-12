export module projnekomata:graphics.srt.bindless_descriptor_set_srt;
import std;
import vulkan;
import :core.platform.int_def;
import :graphics.srt.shader_resource_table;
import :graphics.vulkan.vk_descriptor_pool;
import :graphics.vulkan.vk_descriptor_set_layout;
import :graphics.vulkan.vk_descriptor_set;
import :graphics.vulkan.vk_commands;
import :graphics.vulkan.vk_image;
import :graphics.vulkan.vk_sampler;
import :graphics.vulkan.vk_pipeline_layout;
import :core.containers.abia;

export namespace projnekomata::graphics::srt {

class BindlessDescriptorSetShaderResourceTable : public IShaderResourceTable {
public:
    BindlessDescriptorSetShaderResourceTable(std::nullptr_t);
    BindlessDescriptorSetShaderResourceTable(VulkanDescriptorPool&& descriptorPool, VulkanDescriptorSetLayout&& descriptorSetLayout,
        VulkanDescriptorSet&& descriptorSet, u32 maxImageCount, u32 maxSamplerCount);

    static auto create(u32 maxImageCount, u32 maxSamplerCount) -> std::unique_ptr<BindlessDescriptorSetShaderResourceTable>;

    auto modelName() const -> std::string_view override { return "Bindless"; }

    auto allocateImageIndex() -> SRTResourceIndex override;
    auto allocateImageIndices(u32 count, std::span<SRTResourceIndex> dstIndices) -> void override;
    auto freeImageIndex(SRTResourceIndex index) -> void override;
    auto freeImageIndices(std::span<SRTResourceIndex> indices) -> void override;

    auto allocateSamplerIndex() -> SRTResourceIndex override;
    auto allocateSamplerIndices(u32 count, std::span<SRTResourceIndex> dstIndices) -> void override;

    auto bindImage(const VulkanImage& image, SRTResourceIndex index) -> void override;
    auto bindImageView(const VulkanImageView& imageView, SRTResourceIndex index) -> void override;
    auto bindSampler(const VulkanSampler& sampler, SRTResourceIndex index) -> void override;

    auto bindToCommandBuffer(const VulkanCommandBuffer& cmd, const VulkanPipelineLayout& pipelineLayout, vk::PipelineBindPoint pipelineBindPoint) -> void override;

    auto descriptorSetLayout() const -> const VulkanDescriptorSetLayout& override;

private:
    VulkanDescriptorPool      m_descriptorPool = nullptr;
    VulkanDescriptorSetLayout m_descriptorSetLayout = nullptr;
    VulkanDescriptorSet       m_descriptorSet = nullptr;

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------
    // Allocators

    // Image array index allocator - Because those are dynamic, we need an index allocator that is fully threadsafe and supports reclamation.
    AtomicBitmapIndexAllocator m_imageIndexAllocator = nullptr;

    // Sampler array index allocator - The number of samplers only ever grows, and in a predictable manner, so a plain linear allocator can be used.
    std::atomic<u32> m_nextSamplerIndex = 0;
    u32 m_maxSamplerCount;
};

}