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
    [[nodiscard]] VulkanImage& velocityBuffer() { return m_velocityBuffer; }
    [[nodiscard]] VulkanImage& colorBuffer() { return m_colorBuffer; }
    [[nodiscard]] VulkanImageView& colorBufferUnormView() { return m_colorBufferUnormView; }
    [[nodiscard]] VulkanImage& smaaColorResolvedBuffer0() { return m_smaaColorResolvedBuffer0; }
    [[nodiscard]] VulkanImage& smaaColorResolvedBuffer1() { return m_smaaColorResolvedBuffer1; }
    [[nodiscard]] VulkanImageView& smaaColorResolvedBuffer0UnormView() { return m_smaaColorResolvedBuffer0UnormView; }
    [[nodiscard]] VulkanImageView& smaaColorResolvedBuffer1UnormView() { return m_smaaColorResolvedBuffer1UnormView; }
    [[nodiscard]] VulkanImage& finalImage() { return m_finalImage; }
    [[nodiscard]] VulkanImageView& finalImageUnormView() { return m_finalImageUnormView; }

    [[nodiscard]] VulkanImage& smaaEdgesImage() { return m_smaaEdgesImage; }
    [[nodiscard]] VulkanImage& smaaWeightsImage() { return m_smaaWeightsImage; }

    [[nodiscard]] VulkanImage& postSmaaImage() { return m_postSmaaImage; }
    [[nodiscard]] VulkanImageView& postSmaaImageUnormView() { return m_postSmaaImageUnormView; }

    [[nodiscard]] auto depthBufferIndex() const -> srt::SRTResourceIndex { return m_depthBufferIndex; }
    [[nodiscard]] auto albedoAndRoughnessBufferIndex() const -> srt::SRTResourceIndex { return m_albedoAndRoughnessBufferIndex; }
    [[nodiscard]] auto normalBufferIndex() const -> srt::SRTResourceIndex { return m_normalBufferIndex; }
    [[nodiscard]] auto metallicAndAoBufferIndex() const -> srt::SRTResourceIndex { return m_metallicAndAoBufferIndex; }
    [[nodiscard]] auto velocityBufferIndex() const -> srt::SRTResourceIndex { return m_velocityBufferIndex; }
    [[nodiscard]] auto colorBufferIndex() const -> srt::SRTResourceIndex { return m_colorBufferIndex; }
    [[nodiscard]] auto colorBufferUnormViewIndex() const -> srt::SRTResourceIndex { return m_colorBufferUnormViewIndex; }
    [[nodiscard]] auto smaaColorResolvedBuffer0UnormViewIndex() const -> srt::SRTResourceIndex { return m_smaaColorResolvedBuffer0UnormViewIndex; }
    [[nodiscard]] auto smaaColorResolvedBuffer1UnormViewIndex() const -> srt::SRTResourceIndex { return m_smaaColorResolvedBuffer1UnormViewIndex; }
    [[nodiscard]] auto smaaEdgesImageIndex() const -> srt::SRTResourceIndex { return m_smaaEdgesImageIndex; }
    [[nodiscard]] auto smaaWeightsImageIndex() const -> srt::SRTResourceIndex { return m_smaaWeightsImageIndex; }
    [[nodiscard]] auto postSmaaImageIndex() const -> srt::SRTResourceIndex { return m_postSmaaImageIndex; }
    [[nodiscard]] auto postSmaaImageUnormViewIndex() const -> srt::SRTResourceIndex { return m_postSmmaImageUnormViewIndex; }


    auto handleWindowSizeChange(vk::Extent2D newWindowSize) -> void;

private:
    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Render Targets
    VulkanImage m_depthBuffer = nullptr;
    VulkanImage m_albedoAndRoughnessBuffer = nullptr;
    VulkanImage m_normalBuffer = nullptr;
    VulkanImage m_metallicAndAoBuffer = nullptr;
    VulkanImage m_velocityBuffer = nullptr;
    VulkanImage m_colorBuffer = nullptr;
    VulkanImageView m_colorBufferUnormView = nullptr;

    VulkanImage m_smaaColorResolvedBuffer0 = nullptr;
    VulkanImage m_smaaColorResolvedBuffer1 = nullptr;
    VulkanImageView m_smaaColorResolvedBuffer0UnormView = nullptr;
    VulkanImageView m_smaaColorResolvedBuffer1UnormView = nullptr;

    VulkanImage m_smaaEdgesImage = nullptr;
    VulkanImage m_smaaWeightsImage = nullptr;

    VulkanImage m_finalImage = nullptr;
    VulkanImageView m_finalImageUnormView = nullptr;

    srt::SRTResourceIndex m_depthBufferIndex              = {};
    srt::SRTResourceIndex m_albedoAndRoughnessBufferIndex = {};
    srt::SRTResourceIndex m_normalBufferIndex             = {};
    srt::SRTResourceIndex m_metallicAndAoBufferIndex      = {};
    srt::SRTResourceIndex m_velocityBufferIndex           = {};
    srt::SRTResourceIndex m_colorBufferIndex              = {};
    srt::SRTResourceIndex m_colorBufferUnormViewIndex     = {};
    srt::SRTResourceIndex m_smaaColorResolvedBuffer0UnormViewIndex = {};
    srt::SRTResourceIndex m_smaaColorResolvedBuffer1UnormViewIndex = {};
    srt::SRTResourceIndex m_smaaEdgesImageIndex           = {};
    srt::SRTResourceIndex m_smaaWeightsImageIndex         = {};
    srt::SRTResourceIndex m_postSmaaImageIndex          = {};
    srt::SRTResourceIndex m_postSmmaImageUnormViewIndex = {};


    VulkanImage m_postSmaaImage = nullptr;
    VulkanImageView m_postSmaaImageUnormView = nullptr;

    auto setupRenderingAttachments(vk::Extent2D renderImageExtent) -> void;
    auto zeroinitColorBuffers() -> void;

};

}