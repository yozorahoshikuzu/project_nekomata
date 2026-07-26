export module projnekomata:graphics.srt.shader_resource_table;
import std;
import vulkan;
import projnekomata.cs;
import :graphics.vulkan.vk_image;
import :graphics.vulkan.vk_sampler;
import :graphics.vulkan.vk_pipeline_layout;
import :graphics.vulkan.vk_commands;
import :graphics.vulkan.vk_descriptor_set_layout;

export namespace projnekomata::graphics::srt {

struct SRTResourceIndex {
    u32 imageIndex;
};

class IShaderResourceTable {
public:
    virtual ~IShaderResourceTable() = default;

    virtual auto modelName() const -> std::string_view = 0;

    virtual auto allocateSampledImageIndex() -> SRTResourceIndex = 0;
    virtual auto allocateSampledImageIndices(u32 count, Slice<SRTResourceIndex> dstIndices) -> void = 0;
    virtual auto freeSampledImageIndex(SRTResourceIndex index) -> void = 0;
    virtual auto freeSampledImageIndices(Slice<const SRTResourceIndex> indices) -> void = 0;

    virtual auto allocateStorageImageIndex() -> SRTResourceIndex = 0;
    virtual auto allocateStorageImageIndices(u32 count, Slice<SRTResourceIndex> dstIndices) -> void = 0;
    virtual auto freeStorageImageIndex(SRTResourceIndex index) -> void = 0;
    virtual auto freeStorageImageIndices(Slice<const SRTResourceIndex> indices) -> void = 0;

    virtual auto allocateSamplerIndex() -> SRTResourceIndex = 0;
    virtual auto allocateSamplerIndices(u32 count, Slice<SRTResourceIndex> dstIndices) -> void = 0;

    virtual auto bindSampledImage(const VulkanImage& image, SRTResourceIndex index) -> void = 0;
    virtual auto bindSampledImageView(const VulkanImageView& imageView, SRTResourceIndex index) -> void = 0;
    virtual auto bindStorageImage(const VulkanImage& image, SRTResourceIndex index) -> void = 0;
    virtual auto bindStorageImageView(const VulkanImageView& imageView, SRTResourceIndex index) -> void = 0;
    virtual auto bindSampler(const VulkanSampler& sampler, SRTResourceIndex index) -> void = 0;

    virtual auto bindToCommandBuffer(const VulkanCommandBuffer& cmd, const VulkanPipelineLayout& pipelineLayout, vk::PipelineBindPoint pipelineBindPoint) -> void = 0;

    // temporary
    virtual auto descriptorSetLayout() const -> const VulkanDescriptorSetLayout& = 0;
};

}