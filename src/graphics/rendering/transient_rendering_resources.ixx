export module projnekomata:graphics.rendering.transient_rendering_resources;
import vulkan;
import :graphics.vulkan.vk_image;
import :graphics.srt.shader_resource_table;

export namespace projnekomata::graphics {

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
    [[nodiscard]] VulkanImage& albedoAndRoughnessBuffer() { return m_albedoAndRoughnessBuffer; }
    [[nodiscard]] VulkanImage& normalBuffer() { return m_normalBuffer; }
    [[nodiscard]] VulkanImage& metallicAndAoBuffer() { return m_metallicAndAoBuffer; }
    [[nodiscard]] VulkanImage& colorBuffer() { return m_colorBuffer; }
    [[nodiscard]] VulkanImageView& colorBufferUnormView() { return m_colorBufferUnormView; }

    [[nodiscard]] VulkanImage& smaaEdgesImage() { return m_smaaEdgesImage; }
    [[nodiscard]] VulkanImage& smaaWeightsImage() { return m_smaaWeightsImage; }

    [[nodiscard]] VulkanImage& finalDrawBuffer() { return m_finalDrawBuffer; }
    [[nodiscard]] VulkanImageView& finalDrawBufferUnormView() { return m_finalDrawBufferUnormView; }

    [[nodiscard]] auto depthBufferIndex() const -> srt::SRTResourceIndex { return m_depthBufferIndex; }
    [[nodiscard]] auto albedoAndRoughnessBufferIndex() const -> srt::SRTResourceIndex { return m_albedoAndRoughnessBufferIndex; }
    [[nodiscard]] auto normalBufferIndex() const -> srt::SRTResourceIndex { return m_normalBufferIndex; }
    [[nodiscard]] auto metallicAndAoBufferIndex() const -> srt::SRTResourceIndex { return m_metallicAndAoBufferIndex; }
    [[nodiscard]] auto colorBufferIndex() const -> srt::SRTResourceIndex { return m_colorBufferIndex; }
    [[nodiscard]] auto colorBufferUnormViewIndex() const -> srt::SRTResourceIndex { return m_colorBufferUnormViewIndex; }
    [[nodiscard]] auto smaaEdgesImageIndex() const -> srt::SRTResourceIndex { return m_smaaEdgesImageIndex; }
    [[nodiscard]] auto smaaWeightsImageIndex() const -> srt::SRTResourceIndex { return m_smaaWeightsImageIndex; }


    auto handleWindowSizeChange(vk::Extent2D newWindowSize) -> void;

private:
    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Render Targets
    VulkanImage m_depthBuffer = nullptr;
    VulkanImage m_albedoAndRoughnessBuffer = nullptr;
    VulkanImage m_normalBuffer = nullptr;
    VulkanImage m_metallicAndAoBuffer = nullptr;
    VulkanImage m_colorBuffer = nullptr;
    VulkanImageView m_colorBufferUnormView = nullptr;

    VulkanImage m_smaaEdgesImage = nullptr;
    VulkanImage m_smaaWeightsImage = nullptr;

    srt::SRTResourceIndex m_depthBufferIndex              = {};
    srt::SRTResourceIndex m_albedoAndRoughnessBufferIndex = {};
    srt::SRTResourceIndex m_normalBufferIndex             = {};
    srt::SRTResourceIndex m_metallicAndAoBufferIndex      = {};
    srt::SRTResourceIndex m_colorBufferIndex              = {};
    srt::SRTResourceIndex m_colorBufferUnormViewIndex     = {};
    srt::SRTResourceIndex m_smaaEdgesImageIndex           = {};
    srt::SRTResourceIndex m_smaaWeightsImageIndex         = {};


    VulkanImage m_finalDrawBuffer = nullptr;
    VulkanImageView m_finalDrawBufferUnormView = nullptr;

    auto setupRenderingAttachments(vk::Extent2D renderImageExtent) -> void;

};

}