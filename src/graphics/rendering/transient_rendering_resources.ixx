export module nekomata2.graphics.rendering.transient_rendering_resources;
import vulkan;
import nekomata2.graphics.vulkan.vk_image;

export namespace nekomata2::graphics {

/// Transient rendering resources house all the data used in a single render step.
///
/// # Access Contingency
/// | CPU Read/Write | GPU Access Scope across Queue | GPU Visibility Scope across Queue |
/// |----------------|-------------------------------|-----------------------------------|
/// | No             | Exclusive                     | Shared                            |
///
class TransientRenderingResources {
public:
    TransientRenderingResources(std::nullptr_t);

    TransientRenderingResources(vk::Extent2D renderImageExtent);

    [[nodiscard]] VulkanImage& depthBuffer() { return m_depthBuffer; }
    [[nodiscard]] VulkanImage& finalDrawBuffer() { return m_finalDrawBuffer; }

    auto handleWindowSizeChange(vk::Extent2D newWindowSize) -> void;

private:
    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Render Targets
    VulkanImage m_depthBuffer = nullptr;
    VulkanImage m_finalDrawBuffer = nullptr;

    auto setupRenderingAttachments(vk::Extent2D renderImageExtent) -> void;

};

}