export module projnekomata:graphics.rendering.shared_rendering_resources;
import projnekomata.cs;
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

    auto refitHysteresisStates(usize renderableSparseCount) -> void;
    auto getHysteresisState(usize renderableSparseIndex) -> MeshHysteresisState& { return m_meshHysteresisStates[renderableSparseIndex]; }
    auto getLastRenderableModelMatrix(usize renderableSparseIndex) -> math::Matrix4x4f& { return m_lastRenderableModelMatrices[renderableSparseIndex]; }


    rendering::DynamicBitmapFontAtlas m_fontAtlas;

    float displayMs = 0.0f;

    texturesystem::Texture m_skyCubemap = {};
    texturesystem::Texture m_skyIrradianceCubemap = {};
    texturesystem::Texture m_skyPrefilterCubemap = {};
    texturesystem::Texture m_brdfLUT = {};

    texturesystem::Texture m_smaaAreaTexture = {};
    texturesystem::Texture m_smaaSearchTexture = {};

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

    VulkanPipelineLayout m_smaaBlendWeightLayout = nullptr;
    VulkanGraphicsPipeline m_smaaBlendWeightPipeline = nullptr;

    VulkanPipelineLayout m_smaaEdgeDetectLayout = nullptr;
    VulkanGraphicsPipeline m_smaaEdgeDetectPipeline = nullptr;

    VulkanPipelineLayout m_smaaNeighborhoodBlendLayout = nullptr;
    VulkanGraphicsPipeline m_smaaNeighborhoodBlendPipeline = nullptr;

    VulkanPipelineLayout m_smaaTemporalResolveLayout = nullptr;
    VulkanGraphicsPipeline m_smaaTemporalResolvePipeline = nullptr;

    VulkanPipelineLayout m_velbufferBgLayout = nullptr;
    VulkanGraphicsPipeline m_velbufferBgPipeline = nullptr;

    math::Matrix4x4f m_lastProjview = math::Matrix4x4f::identity();
    math::Matrix4x4f m_lastProjviewNoTranslation = math::Matrix4x4f::identity();

private:

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Hysteresis State
    Vec<MeshHysteresisState> m_meshHysteresisStates = Vec<MeshHysteresisState>::create();
    Vec<math::Matrix4x4f> m_lastRenderableModelMatrices = Vec<math::Matrix4x4f>::create();

    auto buildIblSecondaryCubemaps() -> void;
};

} // namespace projnekomata::graphics