export module projnekomata:graphics.rendering.shared_rendering_resources;
import :core.platform.int_def;
import :graphics.meshsystem.mesh_asset_storage;
import :graphics.vulkan.vk_pipeline_layout;
import :graphics.vulkan.vk_pipeline_graphics;
import :graphics.fontsystem.font_face;
import :graphics.fontsystem.dynamic_font_atlas;

export namespace projnekomata::graphics {

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

    texturesystem::Texture m_skyCubemap = {};
    texturesystem::Texture m_skyIrradianceCubemap = {};
    texturesystem::Texture m_skyPrefilterCubemap = {};
    texturesystem::Texture m_brdfLUT = {};

    VulkanPipelineLayout m_iblIrradianceCubeGeneratorLayout = nullptr;
    VulkanGraphicsPipeline m_iblIrradianceCubeGeneratorPipeline = nullptr;

    VulkanPipelineLayout m_iblPrefilterCubeGeneratorLayout = nullptr;
    VulkanGraphicsPipeline m_iblPrefilterCubeGeneratorPipeline = nullptr;

    VulkanPipelineLayout m_bitmapFontRendererLayout = nullptr;
    VulkanGraphicsPipeline m_bitmapFontRendererPipeline = nullptr;

    VulkanPipelineLayout m_uiRectRendererLayout = nullptr;
    VulkanGraphicsPipeline m_uiRectRendererPipeline = nullptr;

    VulkanPipelineLayout m_uiTextureRendererLayout = nullptr;
    VulkanGraphicsPipeline m_uiTextureRendererPipeline = nullptr;

    VulkanPipelineLayout m_mainGeometryRenderLayout = nullptr;
    VulkanGraphicsPipeline m_mainGeometryRenderPipeline = nullptr;

    VulkanPipelineLayout m_mainLightingPassLayout = nullptr;
    VulkanGraphicsPipeline m_mainLightingPassPipeline = nullptr;

private:

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Hysteresis State
    Vec<MeshHysteresisState> m_meshHysteresisStates = Vec<MeshHysteresisState>::create();

    auto buildIblSecondaryCubemaps() -> void;
};

} // namespace projnekomata::graphics