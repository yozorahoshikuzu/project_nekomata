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
constexpr u32 prefilterImageMips = std::bit_width(prefilterImageSize);

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

    auto iblIrradianceGenShader = SpirvShaderCode::loadFromFile("../spirv/ibl_irradiance_cube_gen.spv").unwrap();
    auto iblPrefilterGenShader = SpirvShaderCode::loadFromFile("../spirv/ibl_prefiltered_spec_mip_gen.spv").unwrap();

    m_iblIrradianceCubeGeneratorLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 8, vk::ShaderStageFlagBits::eFragment)
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

    m_simpleLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(
            0, 56,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation
            | vk::ShaderStageFlagBits::eFragment
        )
        .build();
    auto shader = SpirvShaderCode::loadFromFile("../spirv/tess_test.spv").unwrap();
    m_simplePipeline = VulkanGraphicsPipeline::builder()
        .setPipelineLayout(m_simpleLayout)
        .addShader(shader, vk::ShaderStageFlagBits::eVertex)
        .addShader(shader, vk::ShaderStageFlagBits::eTessellationControl)
        .addShader(shader, vk::ShaderStageFlagBits::eTessellationEvaluation)
        .addShader(shader, vk::ShaderStageFlagBits::eFragment)
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
            vk::Format::eA2R10G10B10UnormPack32
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
        .setDepthAttachmentFormat(vk::Format::eD32Sfloat)
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
                vk::Format::eA2R10G10B10UnormPack32
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
        .setDepthAttachmentFormat(vk::Format::eD32Sfloat)
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
            vk::Format::eA2R10G10B10UnormPack32
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
        .setDepthAttachmentFormat(vk::Format::eD32Sfloat)
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
            vk::Format::eA2R10G10B10UnormPack32
        )
        .build();

    m_tmStart = std::chrono::steady_clock::now();

    m_fontFace = fonts::FontManager::get().loadFont("/usr/share/fonts/noto/NotoSans-Regular.ttf");

    buildIblSecondaryCubemaps();
}
auto SharedRenderingResources::refitHysteresisStates(usize renderableSparseCount) -> void {
    if (m_meshHysteresisStates.size() < renderableSparseCount) {
        m_meshHysteresisStates.resize(renderableSparseCount);
    }
}
auto SharedRenderingResources::buildIblSecondaryCubemaps() -> void {
    auto& irradianceImage = texturesystem::TextureManager::get().getTextureResources(m_skyIrradianceCubemap).image();
    auto& prefilterImage = texturesystem::TextureManager::get().getTextureResources(m_skyPrefilterCubemap).image();

    auto irradianceImageArrayView = irradianceImage.createImageView(
        0, 1, 0, 6, false
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
    };

    auto pc = PushConstants {
        .envmapTextureIndex = texturesystem::TextureManager::get().textureToShaderIndexTable().textureToShaderImageIndex(m_skyCubemap.index),
        .envmapSamplerIndex = texturesystem::TextureManager::get().textureToShaderIndexTable().textureToShaderSamplerIndex(m_skyCubemap.index)
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
            .envmapSamplerIndex = texturesystem::TextureManager::get().textureToShaderIndexTable().textureToShaderSamplerIndex(m_skyCubemap.index),
            .roughness = prefilterImageMips > 1 ? static_cast<f32>(mip) / (static_cast<f32>(prefilterImageMips) - 1.0f) : 0.0f,
            .cubeResolution = static_cast<f32>(prefilterImage.extent().width) // in cubemaps, width == height
        };

        log::info("mip {} roughness: {:.2f}", mip, pc.roughness);

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

    VulkanContext::get().vkQueueGraphics().submitOneCommandBuffer(cmd, {}, {}, None)
        .await();
}

} // namespace projnekomata::graphics