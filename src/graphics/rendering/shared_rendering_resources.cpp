module projnekomata;
import vulkan;
import :graphics.texturesystem.texture_manager;
import :graphics.vulkan.spv_shader_code;
import :graphics.fontsystem.font_manager;
import :graphics.rendering.shared_rendering_resources;

namespace projnekomata::graphics {

SharedRenderingResources::SharedRenderingResources(std::nullptr_t) {}
SharedRenderingResources::SharedRenderingResources() {

    m_simpleLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(
            0, 28,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation
            | vk::ShaderStageFlagBits::eFragment
        )
        .build();
    auto shader = SpirvShaderCode::loadFromFile("./spirv/tess_test.spv").unwrap();
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
            vk::Format::eR16G16B16A16Sfloat
        )
        .build();

    m_bitmapFontRendererLayout = VulkanPipelineLayout::builder()
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .addPushConstantRange(0, 12, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto bitmapFontRendererShader = SpirvShaderCode::loadFromFile("./spirv/bitmap_font.spv").unwrap();
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
                vk::Format::eR16G16B16A16Sfloat
        )
        .build();

    m_uiRectRendererLayout = VulkanPipelineLayout::builder()
        .addPushConstantRange(0, 32, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .build();
    auto uiRectRendererShader = SpirvShaderCode::loadFromFile("./spirv/ui_rect.spv").unwrap();
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
            vk::Format::eR16G16B16A16Sfloat
        )
    .build();

    m_uiTextureRendererLayout = VulkanPipelineLayout::builder()
        .addPushConstantRange(0, 40, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .addDescriptorSetLayout(texturesystem::TextureManager::get().shaderResourceTable().descriptorSetLayout())
        .build();
    auto uiTextureRendererShader = SpirvShaderCode::loadFromFile("./spirv/ui_texture.spv").unwrap();
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
            vk::Format::eR16G16B16A16Sfloat
        )
        .build();

    m_tmStart = std::chrono::steady_clock::now();

    m_fontFace = fonts::FontManager::get().loadFont("/usr/share/fonts/noto/NotoSans-Regular.ttf");
}
auto SharedRenderingResources::refitHysteresisStates(usize renderableSparseCount) -> void {
    if (m_meshHysteresisStates.size() < renderableSparseCount) {
        m_meshHysteresisStates.resize(renderableSparseCount);
    }
}

} // namespace projnekomata::graphics