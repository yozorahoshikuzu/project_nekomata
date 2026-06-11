export module nekomata2:graphics.rendering.shared_rendering_resources;
import :core.platform.int_def;
import :graphics.meshsystem.mesh_asset_storage;
import :graphics.vulkan.vk_pipeline_layout;
import :graphics.vulkan.vk_pipeline_graphics;
import :graphics.fontsystem.font_face;
import :graphics.fontsystem.dynamic_font_atlas;

export namespace nekomata2::graphics {

struct MeshHysteresisState {
    u32 currentLod = meshsystem::kMaxLodCount - 1;
};

/// Shared rendering resources house data used across all render steps.
///
/// # Access Contingency
/// | CPU Read/Write | GPU Access Scope across Queue | GPU Visibility Scope across Queue |
/// |----------------|-------------------------------|-----------------------------------|
/// | Yes            | Shared                        | Shared                            |
///
class SharedRenderingResources {
public:
    SharedRenderingResources(std::nullptr_t);
    SharedRenderingResources();

    [[nodiscard]] VulkanPipelineLayout& simpleLayout() { return m_simpleLayout; }
    [[nodiscard]] VulkanGraphicsPipeline& simplePipeline() { return m_simplePipeline; }

    // Temporary, remove later
    std::chrono::steady_clock::time_point m_tmStart;

    auto refitHysteresisStates(usize renderableSparseCount) -> void;
    auto getHysteresisState(usize renderableSparseIndex) -> MeshHysteresisState& { return m_meshHysteresisStates[renderableSparseIndex]; }


    // do the funny top corner text
    std::string m_textToDisplay = "Unregistered HyperCam 2";

    bool glyphsRendered = false;
    fonts::FontFace m_fontFace;
    rendering::DynamicBitmapFontAtlas m_fontAtlas;

    float displayMs = 0.0f;

    VulkanPipelineLayout m_bitmapFontRendererLayout = nullptr;
    VulkanGraphicsPipeline m_bitmapFontRendererPipeline = nullptr;

    VulkanPipelineLayout m_uiRectRendererLayout = nullptr;
    VulkanGraphicsPipeline m_uiRectRendererPipeline = nullptr;

    VulkanPipelineLayout m_uiTextureRendererLayout = nullptr;
    VulkanGraphicsPipeline m_uiTextureRendererPipeline = nullptr;
private:
    VulkanPipelineLayout m_simpleLayout = nullptr;
    VulkanGraphicsPipeline m_simplePipeline = nullptr;


    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Hysteresis State
    std::vector<MeshHysteresisState> m_meshHysteresisStates;
};

} // namespace nekomata2::graphics