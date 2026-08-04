module;
#include <SDL3/SDL_events.h>
module projnekomata;
import vulkan;
import fmt;
import projnekomata.cs;
import :graphics.cmd_alloc;
import :core.ui.components.ui_rect;
import :core.ui.components.ui_texture;
import :core.ecs.world.renderable;
import :core.ecs.world.transform;
import :core.ecs.world.camera;
import :core.runtime.mainthread;

namespace projnekomata {

MainThread::MainThread(std::shared_ptr<MRThreadsSharedData> mrSharedData, Unique<VulkanContext>&& vkContext, SdlWindow&& sdlWindow)
    : m_sdlWindow(std::move(sdlWindow)), m_mrSharedData(std::move(mrSharedData)), m_vkContext(std::move(vkContext)) {

    cmdalloc::VulkanCommandPoolsList::initThreadLocalCommandPools();

    auto windowLogicalSize = m_sdlWindow.getLogicalSize();
    auto windowLogicalSizef = math::Vector2f(m_sdlWindow.getLogicalSize().x(), m_sdlWindow.getLogicalSize().y());
    auto windowDisplayScale = m_sdlWindow.getDisplayScale();
    log::info("Window logical size: {}x{}", windowLogicalSize.x(), windowLogicalSize.y());
    log::info("Window display scale: {}", windowDisplayScale);

    m_currentWorld = Unique<ecs::World>::create();
    m_inputManager = core::input::Input::create();
    m_meshAssetStorage = meshsystem::MeshAssetStorage::create();
    m_textureManager = graphics::texturesystem::TextureManager::create();
    m_materialManager = MaterialManager::create();
    m_fontManager = graphics::fonts::FontManager::create();
    m_uiSystem = ui::UiSystem::create();

    m_overlayFont = graphics::fonts::FontManager::get().loadFont("../../Assets/IosevkaTerm-Light.ttf");
}

auto MainThread::runMainLoop(const std::function<void(Unique<ecs::World>&)>& initFn) -> void {
    initFn(m_currentWorld);

    using clock = std::chrono::steady_clock;

    auto previousClk = clock::now();
    while (true) {
        m_mrSharedData->m_syncpointBarrier.arrive_and_wait();

        if (m_mrSharedData->m_shouldQuit.load(std::memory_order_acquire)) {
            break;
        }
        m_mrSharedData->m_leafs.swap();

        m_mrSharedData->m_syncpointBarrier.arrive_and_wait();

        auto currentClk = clock::now();
        std::chrono::duration<float> delta = currentClk - previousClk;
        previousClk = currentClk;

        loop(delta.count());
    }
    log::info("Main Thread exiting...");

    cmdalloc::VulkanCommandPoolsList::destroyThreadLocalCommandPools();
}

auto MainThread::loop(float dt) -> void {
    m_inputManager->handleNewFrame(m_sdlWindow);
    VulkanContext::get().antiLagPaceInput(m_frameIndex, 0);

    auto logicalSize = m_sdlWindow.getLogicalSize();
    auto logicalSizeFloat = math::Vector2f(logicalSize.x(), logicalSize.y());
    SDL_Event event;
    auto totalMouseDelta = math::Vector2f(0.0f);
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT: {
            m_mrSharedData->m_shouldQuit.store(true, std::memory_order_release);
            return;
        }
        case SDL_EVENT_KEY_DOWN: {
            auto code = core::input::mapSdlKeyToKey(event.key.key);
            auto mod = core::input::mapSdlKeyModToKeyMod(event.key.mod);
            m_inputManager->setKeyState(code, true);
            m_inputManager->insertInputKeyEvent(core::input::InputKeyEvent{code, mod, true, event.key.repeat});
            break;
        }
        case SDL_EVENT_KEY_UP: {
            auto code = core::input::mapSdlKeyToKey(event.key.key);
            auto mod = core::input::mapSdlKeyModToKeyMod(event.key.mod);
            m_inputManager->setKeyState(code, false);
            m_inputManager->insertInputKeyEvent(core::input::InputKeyEvent{code, mod, false, event.key.repeat});
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            auto pos = math::Vector2f(event.button.x, event.button.y);
            if (m_inputManager->getMouseMode() == core::input::MouseMode::Normal) {
                m_uiSystem->testMouseDownHit(pos);
            }

            auto code = core::input::mapSdlMouseButtonToKey(event.button.button);
            m_inputManager->setKeyState(code, true);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            auto pos = math::Vector2f(event.button.x, event.button.y);
            if (m_inputManager->getMouseMode() == core::input::MouseMode::Normal) {
                m_uiSystem->testMouseUpHit(pos);
            }

            auto code = core::input::mapSdlMouseButtonToKey(event.button.button);
            m_inputManager->setKeyState(code, false);
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            auto position = math::Vector2f(event.motion.x, event.motion.y);
            totalMouseDelta += math::Vector2f(event.motion.xrel, event.motion.yrel);
            m_inputManager->setMousePosition(position);

            if (m_inputManager->getMouseMode() == core::input::MouseMode::Normal) {
                m_uiSystem->testMouseHover(position);
            }

            break;
        }
        }
    }
    m_inputManager->setMouseDelta(totalMouseDelta);

    // ---- Renderer overlay control ---------------------------------------------------------------------------------------------------------------------------

    auto specialKeyPressed = m_inputManager->keyEventsThisFrame().iter()
        .find([](const auto& x) {
            bool hasAltMod = (x.modifiers & core::input::KeyModifierFlags::LAlt) != core::input::KeyModifierFlags::None;
            return x.key == core::input::Key::F12 && x.state && (!x.isRepeat) && hasAltMod;
        })
        .isSome();


    m_currentWorld->scriptsUpdate(dt);

    m_mrSharedData->m_leafs.getPrimary().m_currentWindowExtent = m_sdlWindow.vulkanGetDrawableSize();
    m_mrSharedData->m_leafs.getPrimary().m_frameIndex = m_frameIndex;
    if (!m_currentWorld.isNull()) {
        m_currentWorld->components<ecs::components::Renderable>().copyTo(m_mrSharedData->m_leafs.getPrimary().m_renderables);
        m_currentWorld->components<ecs::components::PointLight>().copyTo(m_mrSharedData->m_leafs.getPrimary().m_pointlights);
        m_currentWorld->components<ecs::components::Transform>().copyTo(m_mrSharedData->m_leafs.getPrimary().m_transforms);
        m_currentWorld->components<ecs::components::Camera>().copyTo(m_mrSharedData->m_leafs.getPrimary().m_cameras);
    }
    m_textureManager->textureToShaderIndexTable().snapshotTables(
        m_mrSharedData->m_leafs.getPrimary().m_textureToImageShaderIndexSnapshot,
        m_mrSharedData->m_leafs.getPrimary().m_textureToSamplerShaderIndexSnapshot
    );
    m_mrSharedData->m_leafs.getPrimary().m_materialHeapSnapshotsBySize.clear();

    for (auto [size, heap] : m_materialManager->materialParamHeapsMap().iter()) {
        auto data = heap->data();
        auto vec = Vec<u8>::withCapacity(data.len());
        vec.extend(data);
        m_mrSharedData->m_leafs.getPrimary().m_materialHeapSnapshotsBySize.emplace(size, std::move(vec));
    }

    m_mrSharedData->m_leafs.getPrimary().m_uiDrawCmds.clear();
    auto fontRasterBatches = Vec<graphics::fonts::FontRasterBatch>::create();
    ui::UiSystem::get().buildUi(m_mrSharedData->m_leafs.getPrimary().m_uiDrawCmds, fontRasterBatches, m_mrSharedData->m_fontAtlas, logicalSizeFloat);

    if (m_waitForFrameStats) {
        m_mrSharedData->m_statsReady.wait(false, std::memory_order_acquire);
        auto& physicalDeviceProps = VulkanContext::get().vkPhysicalDeviceProps();
        auto supportsPipelineStatisticsQuery = physicalDeviceProps.m_hasPipelineStatisticsQuery;
        f64 deviceTimestampPeriod = physicalDeviceProps.m_timestampPeriod;
        auto [blockBytes, allocBytes] = VulkanContext::get().currentVramUsage();

        std::string queryStats = "\n [statistics not available]";

        if (m_mrSharedData->m_queryPoolStatsAreValid) {
            auto& queryTimestamps = m_mrSharedData->m_queryTimestamps;
            auto& pipelineStats = m_mrSharedData->m_deferredGeometryPipelineStats;
            auto geomPassTime = (queryTimestamps.geomPassBottomOfPipe - queryTimestamps.geomPassTopOfPipe) * deviceTimestampPeriod / 1000000.0_f64;
            auto lightingPassTime = (queryTimestamps.lightingPassAfterDoneBottomOfPipe - queryTimestamps.lightingPassTopOfPipe) * deviceTimestampPeriod / 1000000.0_f64;
            auto smaaTime = (queryTimestamps.smaaBottomOfPipe - queryTimestamps.smaaTopOfPipe) * deviceTimestampPeriod / 1000000.0_f64;

            if (supportsPipelineStatisticsQuery) {
                queryStats = fmt::format("\n GeomPass: {:.3f} ms #VS: {} #TCS: {} #TES: {} #FS: {}\n LightingPass: {:.3f} ms\n SMAA: {:.3f} ms", geomPassTime, pipelineStats[0], pipelineStats[2], pipelineStats[3], pipelineStats[1], lightingPassTime, smaaTime);
            } else {
                queryStats = fmt::format("\n GeomPass: {:.3f} ms\n LightingPass: {:.3f} ms\n SMAA: {:.3f} ms", geomPassTime, lightingPassTime, smaaTime);
            }
        }

        std::string vramStr;
        if (physicalDeviceProps.m_hasExtMemoryBudget) {
            f64 vramBudget = VulkanContext::get().extMemoryBudgetGetVramBudget();
            vramStr = fmt::format("total {:.2f} MB used/allocd block bytes: {:.2f}/{:.2f} MB budget: {:.2f} MB",
                physicalDeviceProps.m_vramSize / 1024.0_f64 / 1024.0_f64,
                allocBytes / 1024.0_f64 / 1024.0_f64,
                blockBytes / 1024.0_f64 / 1024.0_f64,
                vramBudget / 1024.0_f64 / 1024.0_f64
            );
        } else {
            vramStr = fmt::format("total {:.2f} MB used/allocd block bytes: {:.2f}/{:.2f} MB",
                physicalDeviceProps.m_vramSize / 1024.0_f64 / 1024.0_f64,
                allocBytes / 1024.0_f64 / 1024.0_f64,
                blockBytes / 1024.0_f64 / 1024.0_f64
            );
        }


        std::string text = fmt::format(
            "--- Project Nekomata ---\n"
                " FPS: {:.2f} ({:.3f}ms)\n\n"
                " -SDL-\n Video Driver: {}\n\n"
                " -Vulkan-\n Device: {}\n Driver: {} {}.{}.{}.{} API Version {}.{}.{}.{}\n VRAM: {}\n Shader Cache: {}\n Descriptor Binding Model: {}\n Anti-Lag: {}\n\n"
                " -Stats-\n Drawcalls: {}{}",
            1000.0f / m_mrSharedData->m_deltaTime, m_mrSharedData->m_deltaTime,
            m_mrSharedData->m_sdlVideoDriverName,
            physicalDeviceProps.m_deviceName,
            physicalDeviceProps.m_driverName, physicalDeviceProps.getDriverVersionVariant(), physicalDeviceProps.getDriverVersionMajor(), physicalDeviceProps.getDriverVersionMinor(), physicalDeviceProps.getDriverVersionPatch(),
            physicalDeviceProps.getApiVersionVariant(), physicalDeviceProps.getApiVersionMajor(), physicalDeviceProps.getApiVersionMinor(), physicalDeviceProps.getApiVersionPatch(),
            vramStr,
            VulkanContext::get().shaderCache()->usesPipelineBinaries() ? "Yes" : "No",
            graphics::texturesystem::TextureManager::get().shaderResourceTable().modelName(),
            antiLagMethodToString(VulkanContext::get().antiLagMethod()),
            m_mrSharedData->m_numDrawcalls,
            queryStats
        );

        auto fontSize = 14.0_f32;
        auto rasterbatch = graphics::fonts::FontManager::get().findAndBatchMissingGlyphs(m_overlayFont, m_mrSharedData->m_fontAtlas, text, fontSize);

        if (rasterbatch.isSome()) fontRasterBatches.emplace(std::move(rasterbatch.unwrap()));

        m_mrSharedData->m_leafs.getPrimary().m_uiDrawCmds.emplace(ui::UiTextDrawCmd {
            .ssPosition = Vector2f(4.0f, 18.0f),
            .text = text,
            .face = m_overlayFont,
            .size = fontSize,
            .color = Color::fromRgba32Float(1.0f, 1.0f, 1.0f, 1.0f)
        });
    }

    m_mrSharedData->m_leafs.getPrimary().m_fontsCopyRegions.clear();
    m_mrSharedData->m_leafs.getPrimary().m_fontsUploadPixelBuffer.clear();
    m_mrSharedData->m_leafs.getPrimary().m_fontsNewImageIndices.clear();
    if (fontRasterBatches.len() > 0) {
        graphics::fonts::FontRasterInfo rasterInfo = {
            .batches         = fontRasterBatches.asSlice(),
            .atlas           = m_mrSharedData->m_fontAtlas,
            .copyRegions     = m_mrSharedData->m_leafs.getPrimary().m_fontsCopyRegions,
            .resultBuffer    = m_mrSharedData->m_leafs.getPrimary().m_fontsUploadPixelBuffer,
            .newImageIndices = m_mrSharedData->m_leafs.getPrimary().m_fontsNewImageIndices
        };
        graphics::fonts::FontManager::get().rasterizeGlyphs(rasterInfo);
    }

    if (specialKeyPressed) {
        m_waitForFrameStats = !m_waitForFrameStats;
    }

    m_mrSharedData->m_leafs.getPrimary().m_hasValidFrame = true;
    m_mrSharedData->m_leafs.getPrimary().m_captureStats = m_waitForFrameStats;

    m_frameIndex++;
}

}