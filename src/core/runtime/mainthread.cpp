module;
#include <SDL3/SDL_events.h>
module projnekomata;
import vulkan;
import :core.log;
import :graphics.cmd_alloc;
import :core.ui.components.ui_rect;
import :core.ui.components.ui_texture;
import :core.ecs.world.renderable;
import :core.ecs.world.transform;
import :core.ecs.world.camera;
import :core.runtime.mainthread;

namespace projnekomata {

MainThread::MainThread(std::shared_ptr<MRThreadsSharedData> mrSharedData, std::unique_ptr<VulkanContext>&& vkContext, SdlWindow&& sdlWindow)
    : m_sdlWindow(std::move(sdlWindow)), m_mrSharedData(std::move(std::move(mrSharedData))), m_vkContext(std::move(vkContext)) {

    cmdalloc::VulkanCommandPoolsList::initThreadLocalCommandPools();

    auto windowLogicalSize = m_sdlWindow.getLogicalSize();
    auto windowLogicalSizef = math::Vector2f(m_sdlWindow.getLogicalSize().x(), m_sdlWindow.getLogicalSize().y());
    auto windowDisplayScale = m_sdlWindow.getDisplayScale();
    log::info("Window logical size: {}x{}", windowLogicalSize.x(), windowLogicalSize.y());
    log::info("Window display scale: {}", windowDisplayScale);

    m_currentWorld = std::make_unique<ecs::World>();
    m_inputManager = core::input::Input::create();
    m_meshAssetStorage = meshsystem::MeshAssetStorage::create();
    m_textureManager = graphics::texturesystem::TextureManager::create();
    m_fontManager = graphics::fonts::FontManager::create();
    m_uiSystem = ui::UiSystem::create();
}

auto MainThread::runMainLoop(const std::function<void(std::unique_ptr<ecs::World>&)>& initFn) -> void {
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
            m_uiSystem->testMouseDownHit(pos);

            auto code = core::input::mapSdlMouseButtonToKey(event.button.button);
            m_inputManager->setKeyState(code, true);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            auto pos = math::Vector2f(event.button.x, event.button.y);
            m_uiSystem->testMouseUpHit(pos);

            auto code = core::input::mapSdlMouseButtonToKey(event.button.button);
            m_inputManager->setKeyState(code, false);
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            auto position = math::Vector2f(event.motion.x, event.motion.y);
            totalMouseDelta += math::Vector2f(event.motion.xrel, event.motion.yrel);
            m_inputManager->setMousePosition(position);
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
    if (specialKeyPressed) {
        injectOverlay = !injectOverlay;
    }

    m_currentWorld->scriptsUpdate(dt);

    m_mrSharedData->m_leafs.getPrimary().m_currentWindowExtent = m_sdlWindow.vulkanGetDrawableSize();
    m_mrSharedData->m_leafs.getPrimary().m_frameIndex = m_frameIndex;
    if (m_currentWorld) {
        m_currentWorld->components<ecs::components::Renderable>().copyTo(m_mrSharedData->m_leafs.getPrimary().m_renderables);
        m_currentWorld->components<ecs::components::Transform>().copyTo(m_mrSharedData->m_leafs.getPrimary().m_transforms);
        m_currentWorld->components<ecs::components::Camera>().copyTo(m_mrSharedData->m_leafs.getPrimary().m_cameras);
    }
    m_textureManager.get()->textureToShaderIndexTable().snapshotTables(
        m_mrSharedData->m_leafs.getPrimary().m_textureToImageShaderIndexSnapshot,
        m_mrSharedData->m_leafs.getPrimary().m_textureToSamplerShaderIndexSnapshot
    );
    m_mrSharedData->m_leafs.getPrimary().m_uiDrawCmds.clear();
    ui::UiSystem::get().buildUi(m_mrSharedData->m_leafs.getPrimary().m_uiDrawCmds, logicalSizeFloat);

    m_mrSharedData->m_leafs.getPrimary().m_hasValidFrame = true;
    m_mrSharedData->m_leafs.getPrimary().injectOverlay = injectOverlay;

    m_frameIndex++;
}

}