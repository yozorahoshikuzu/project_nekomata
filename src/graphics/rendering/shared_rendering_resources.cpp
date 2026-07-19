module projnekomata;
import vulkan;
import :graphics.texturesystem.texture_manager;
import :graphics.vulkan.spv_shader_code;
import :graphics.fontsystem.font_manager;
import :graphics.rendering.shared_rendering_resources;
import :graphics.cmd_alloc;
import :graphics.vulkan.vk_commands_barriers;

namespace projnekomata::graphics {

constexpr u32 prefilterImageSize = 512;
constexpr u32 prefilterImageMips = std::bit_width(prefilterImageSize) - 3;

SharedRenderingResources::SharedRenderingResources(std::nullptr_t) {}
SharedRenderingResources::SharedRenderingResources() {

    auto samplerParams = texturesystem::SamplerParams::defaultValues()
        .setMinFilter(vk::Filter::eLinear)
        .setMagFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear);

    m_skyCubemap = texturesystem::TextureManager::get().loadKtx2TextureBlocking(
        "../../Assets/sky.ktx2",
        samplerParams
    );

    m_skyIrradianceCubemap = texturesystem::TextureManager::get().createTexture(
        32, 32, 1, 1, 1, true,
        vk::Format::eB10G11R11UfloatPack32,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        samplerParams
    );

    m_skyPrefilterCubemap = texturesystem::TextureManager::get().createTexture(
        prefilterImageSize, prefilterImageSize, 1, 1, prefilterImageMips, true,
        vk::Format::eB10G11R11UfloatPack32,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        samplerParams
    );

    m_brdfLUT = texturesystem::TextureManager::get().loadKtx2TextureBlocking(
        "../../Assets/ibllut.ktx2",
        samplerParams
    );

    m_smaaAreaTexture = texturesystem::TextureManager::get().loadKtx2TextureBlocking(
        "../../Assets/smaa_areatex.ktx2",
        samplerParams
    );

    m_smaaSearchTexture = texturesystem::TextureManager::get().loadKtx2TextureBlocking(
        "../../Assets/smaa_searchtex.ktx2",
        samplerParams
    );

    auto iblIrradianceGenShader = SpirvShaderCode::loadFromFile("../spirv/ibl_irradiance_cube_gen.spv").unwrap();
    auto iblPrefilterGenShader = SpirvShaderCode::loadFromFile("../spirv/ibl_prefiltered_spec_mip_gen.spv").unwrap();

    m_iblIrradianceCubeGeneratorLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 12, vk::ShaderStageFlagBits::eFragment)
        .build();
    m_iblIrradianceCubeGeneratorPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_iblIrradianceCubeGeneratorLayout)
        .addShader(iblIrradianceGenShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(iblIrradianceGenShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
            vk::PipelineColorBlendAttachmentState{}
                .setBlendEnable(false)
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eB10G11R11UfloatPack32
        )
        .setMultiviewViewsMask(0b111111)
        .build();

    m_iblPrefilterCubeGeneratorLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 16, vk::ShaderStageFlagBits::eFragment)
        .build();
    m_iblPrefilterCubeGeneratorPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_iblPrefilterCubeGeneratorLayout)
        .addShader(iblPrefilterGenShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(iblPrefilterGenShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
            vk::PipelineColorBlendAttachmentState{}
                .setBlendEnable(false)
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eB10G11R11UfloatPack32
        )
        .setMultiviewViewsMask(0b111111)
        .build();

    m_mainGeometryRenderLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(
            0, 40,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation
            | vk::ShaderStageFlagBits::eFragment
        )
        .build();
    auto geometryRenderShader = SpirvShaderCode::loadFromFile("../spirv/mainrender_geom.spv").unwrap();
    m_mainGeometryRenderPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_mainGeometryRenderLayout)
        .addShader(geometryRenderShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(geometryRenderShader, vk::ShaderStageFlagBits::eTessellationControl)
        .addShader(geometryRenderShader, vk::ShaderStageFlagBits::eTessellationEvaluation)
        .addShader(geometryRenderShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::ePatchList)
        .setTessellationPatchControlPoints(3)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0)
        .disableMultisampling()
        .enableDepthTest()
        .enableDepthWrite()
        .setDepthAttachmentFormat(vk::Format::eD32Sfloat)
        .pushRenderingAttachment(
            vk::PipelineColorBlendAttachmentState{}
                 .setBlendEnable(false)
                 .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8B8A8Unorm
        )
        .pushRenderingAttachment(
            vk::PipelineColorBlendAttachmentState{}
                 .setBlendEnable(false)
                 .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR16G16Snorm
        )
        .pushRenderingAttachment(
            vk::PipelineColorBlendAttachmentState{}
                 .setBlendEnable(false)
                 .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8Unorm
        )
        .pushRenderingAttachment(
            vk::PipelineColorBlendAttachmentState{}
                .setBlendEnable(false)
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR16G16Sfloat
        )
        .build();

    m_mainLightingPassLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(
            0, 64,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        )
        .build();
    auto lightingPassShader = SpirvShaderCode::loadFromFile("../spirv/mainrender_lighting.spv").unwrap();
    m_mainLightingPassPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_mainLightingPassLayout)
        .addShader(lightingPassShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(lightingPassShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
             .setBlendEnable(false)
             .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8B8A8Srgb
        )
        .build();

    m_bitmapFontRendererLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 28, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto bitmapFontRendererShader = SpirvShaderCode::loadFromFile("../spirv/bitmap_font.spv").unwrap();
    m_bitmapFontRendererPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_bitmapFontRendererLayout)
        .addShader(bitmapFontRendererShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(bitmapFontRendererShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleStrip)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
                    .setBlendEnable(true)
                    .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                    .setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                    .setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                    .setAlphaBlendOp(vk::BlendOp::eAdd),
                vk::Format::eR8G8B8A8Srgb
        )
        .build();

    m_uiRectRendererLayout = VulkanPipelineLayout::builder()
        .addPushConstantRange(0, 32, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto uiRectRendererShader = SpirvShaderCode::loadFromFile("../spirv/ui_rect.spv").unwrap();
    m_uiRectRendererPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_uiRectRendererLayout)
        .addShader(uiRectRendererShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(uiRectRendererShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleStrip)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
                    .setBlendEnable(true)
                    .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                    .setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                    .setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                    .setAlphaBlendOp(vk::BlendOp::eAdd),
            vk::Format::eR8G8B8A8Srgb
        )
    .build();

    m_uiTextureRendererLayout = VulkanPipelineLayout::builder()
        .addPushConstantRange(0, 40, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .build();
    auto uiTextureRendererShader = SpirvShaderCode::loadFromFile("../spirv/ui_texture.spv").unwrap();
    m_uiTextureRendererPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_uiTextureRendererLayout)
        .addShader(uiTextureRendererShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(uiTextureRendererShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleStrip)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
                    .setBlendEnable(true)
                    .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                    .setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                    .setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                    .setAlphaBlendOp(vk::BlendOp::eAdd),
            vk::Format::eR8G8B8A8Srgb
        )
        .build();

    // ---- SMAA -----------------------------------------------------------------------------------------------------------------------------------------------

    m_smaaEdgeDetectLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 32, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto smaaEdgeDetectShader = SpirvShaderCode::loadFromFile("../spirv/smaa_edgedetect.spv").unwrap();
    m_smaaEdgeDetectPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_smaaEdgeDetectLayout)
        .addShader(smaaEdgeDetectShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(smaaEdgeDetectShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
             .setBlendEnable(false)
             .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8Unorm
        )
        .build();

    m_smaaBlendWeightLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 40, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto smaaBlendWeightShader = SpirvShaderCode::loadFromFile("../spirv/smaa_blendweight.spv").unwrap();
    m_smaaBlendWeightPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_smaaBlendWeightLayout)
        .addShader(smaaBlendWeightShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(smaaBlendWeightShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
             .setBlendEnable(false)
             .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8B8A8Unorm
        )
        .build();

    m_smaaNeighborhoodBlendLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 36, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto smaaNeighborhoodBlendShader = SpirvShaderCode::loadFromFile("../spirv/smaa_neighborhoodblend.spv").unwrap();
    m_smaaNeighborhoodBlendPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_smaaNeighborhoodBlendLayout)
        .addShader(smaaNeighborhoodBlendShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(smaaNeighborhoodBlendShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
             .setBlendEnable(false)
             .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8B8A8Srgb
        )
        .build();

    m_smaaTemporalResolveLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 36, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto smaaTemporalResolveShader = SpirvShaderCode::loadFromFile("../spirv/smaa_temporalresolve.spv").unwrap();
    m_smaaTemporalResolvePipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_smaaTemporalResolveLayout)
        .addShader(smaaTemporalResolveShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(smaaTemporalResolveShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
             .setBlendEnable(false)
             .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8B8A8Unorm
        )
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
             .setBlendEnable(false)
             .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR8G8B8A8Unorm
        )
        .build();

    m_velbufferBgLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 8, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto velbufferBgShader = SpirvShaderCode::loadFromFile("../spirv/velbuffer_bg.spv").unwrap();
    m_velbufferBgPipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_velbufferBgLayout)
        .addShader(velbufferBgShader, vk::ShaderStageFlagBits::eVertex)
        .addShader(velbufferBgShader, vk::ShaderStageFlagBits::eFragment)
        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
        .setRastPolygonMode(vk::PolygonMode::eFill)
        .setRastCulling(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .setRastLineWidth(1.0f)
        .disableMultisampling()
        .disableDepthTest()
        .pushRenderingAttachment(
        vk::PipelineColorBlendAttachmentState{}
             .setBlendEnable(false)
             .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
            vk::Format::eR16G16Sfloat
        )
        .build();

    buildIblSecondaryCubemaps();
}
auto SharedRenderingResources::refitHysteresisStates(usize renderableSparseCount) -> void {
    if (m_meshHysteresisStates.len() < renderableSparseCount) {
        m_meshHysteresisStates.resize(renderableSparseCount, MeshHysteresisState());
    }
    if (m_lastRenderableModelMatrices.len() < renderableSparseCount) {
        m_lastRenderableModelMatrices.resize(renderableSparseCount, Matrix4x4f::identity());
    }
}
auto SharedRenderingResources::buildIblSecondaryCubemaps() -> void {
    auto& irradianceImage = texturesystem::TextureManager::get().getTextureResources(m_skyIrradianceCubemap).image();
    auto& prefilterImage = texturesystem::TextureManager::get().getTextureResources(m_skyPrefilterCubemap).image();
    f32 cubeResolution = static_cast<f32>(texturesystem::TextureManager::get().getTextureResources(m_skyCubemap).image().extent().width);

    auto irradianceImageArrayView = irradianceImage.createImageView(
        0, 1, 0, 6, false
    );

    u32 sampler = texturesystem::TextureManager::get().samplerCache().acquireSampler(
        texturesystem::SamplerParams::defaultValues()
    );

    auto prefilterImageArrayViews = Vec<VulkanImageView>::withCapacity(prefilterImageMips);
    for (u32 i = 0; i < prefilterImageMips; i++) {
        auto view = prefilterImage.createImageView(
            i, 1, 0, 6, false
        );
        prefilterImageArrayViews.emplace(std::move(view));
    }

    auto cb = cmdalloc::VulkanCommandPoolsList::getAssignedGraphicsCommandPool().allocateCommandBuffer(vk::CommandBufferLevel::ePrimary);
    auto& cmd = cb.vkCommandBuffer();

    auto beginInfo = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    vkCheckResult(cmd.begin(beginInfo));

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(irradianceImage,
            vk::ImageLayout::eUndefined, {}, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .insertImageMemoryBarrier(prefilterImage,
            vk::ImageLayout::eUndefined, {}, {},
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        )
        .flush(cb);

    // ---- Irradiance Image -----------------------------------------------------------------------------------------------------------------------------------

    auto imageAttachmentInfo = vk::RenderingAttachmentInfo{}
        .setImageView(irradianceImageArrayView.vkImageView())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore);

    auto renderingArea = vk::Extent2D{irradianceImage.extent().width, irradianceImage.extent().height};

    auto viewport = vk::Viewport{}
        .setWidth(static_cast<f32>(renderingArea.width))
        .setHeight(static_cast<f32>(renderingArea.height))
        .setMinDepth(0.0)
        .setMaxDepth(1.0);
    auto scissor = vk::Rect2D{}.setExtent(renderingArea);

    auto renderingInfo = vk::RenderingInfo{}
        .setColorAttachments(imageAttachmentInfo)
        .setViewMask(0b111111)
        .setRenderArea(vk::Rect2D{}.setExtent(renderingArea));

    cmd.beginRendering(renderingInfo);
    cmd.setViewport(0, viewport);
    cmd.setScissor(0, scissor);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_iblIrradianceCubeGeneratorPipeline.vkPipeline());
    texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(cb, m_iblIrradianceCubeGeneratorLayout, vk::PipelineBindPoint::eGraphics);

    struct PushConstants {
        u32 envmapTextureIndex;
        u32 envmapSamplerIndex;
        float cubeResolution;
    };

    auto pc = PushConstants {
        .envmapTextureIndex = texturesystem::TextureManager::get().textureToShaderIndexTable().textureToShaderImageIndex(m_skyCubemap.index),
        .envmapSamplerIndex = sampler,
        .cubeResolution = cubeResolution
    };

    cmd.pushConstants<PushConstants>(m_iblIrradianceCubeGeneratorLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, pc);
    cmd.draw(3, 1, 0, 0);

    cmd.endRendering();

    // ---- Prefiltered Spec Maps ------------------------------------------------------------------------------------------------------------------------------

    for (u32 mip = 0; mip < prefilterImageMips; mip++) {
        auto imageAttachmentInfo = vk::RenderingAttachmentInfo{}
            .setImageView(prefilterImageArrayViews[mip].vkImageView())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStoreOp(vk::AttachmentStoreOp::eStore);

        auto renderingArea = vk::Extent2D{std::max(1u, prefilterImage.extent().width >> mip), std::max(1u, prefilterImage.extent().height >> mip)};

        auto viewport = vk::Viewport{}
            .setWidth(static_cast<f32>(renderingArea.width))
            .setHeight(static_cast<f32>(renderingArea.height))
            .setMinDepth(0.0)
            .setMaxDepth(1.0);
        auto scissor = vk::Rect2D{}.setExtent(renderingArea);

        auto renderingInfo = vk::RenderingInfo{}
            .setColorAttachments(imageAttachmentInfo)
            .setViewMask(0b111111)
            .setRenderArea(vk::Rect2D{}.setExtent(renderingArea));

        cmd.beginRendering(renderingInfo);
        cmd.setViewport(0, viewport);
        cmd.setScissor(0, scissor);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_iblPrefilterCubeGeneratorPipeline.vkPipeline());
        texturesystem::TextureManager::get().shaderResourceTable().bindToCommandBuffer(cb, m_iblPrefilterCubeGeneratorLayout, vk::PipelineBindPoint::eGraphics);

        struct PushConstants {
            u32 envmapTextureIndex;
            u32 envmapSamplerIndex;
            float roughness;
            float cubeResolution;
        };

        auto pc = PushConstants {
            .envmapTextureIndex = texturesystem::TextureManager::get().textureToShaderIndexTable().textureToShaderImageIndex(m_skyCubemap.index),
            .envmapSamplerIndex = sampler,
            .roughness = prefilterImageMips > 1 ? static_cast<f32>(mip) / (static_cast<f32>(prefilterImageMips) - 1.0f) : 0.0f,
            .cubeResolution = cubeResolution
        };

        log::trace("Prefilter Spec Map Mip {} Roughness: {:.2f}", mip, pc.roughness);

        cmd.pushConstants<PushConstants>(m_iblPrefilterCubeGeneratorLayout.vkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, pc);
        cmd.draw(3, 1, 0, 0);

        cmd.endRendering();
    }

    VulkanPipelineBarriers::builder()
        .insertImageMemoryBarrier(irradianceImage,
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .insertImageMemoryBarrier(prefilterImage,
            vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        )
        .flush(cb);

    vkCheckResult(cmd.end());

    auto timeBefore = std::chrono::steady_clock::now();
    VulkanContext::get().vkQueueGraphics().submitOneCommandBuffer(cmd, {}, {}, None)
        .await();
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timeBefore);
    log::trace("Irradiance and Prefiltered Spec Maps built in {:.2f}ms", time.count() / 1000.0f);
}

} // namespace projnekomata::graphics