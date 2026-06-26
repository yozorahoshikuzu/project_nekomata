module;
#include <SDL3/SDL_messagebox.h>
module nekomata2;
import std;
import :core.log;
import :core.platform.sdl;
import :core.platform.thread;
import :graphics.vulkan.context;
import :core.runtime.shared_data;
import :core.runtime.mainthread;
import :core.runtime.graphicsthread;

namespace nekomata2 {

auto entryAfterSdlInit(const std::function<void(std::unique_ptr<ecs::World>&)>& initFn) -> void {
    // TODO: A system to remember player preferences to use here instead.

    auto window = nekomata2::SdlWindow("Project Nekomata", 1920, 1080);

    // NOTE: This creates the vk::SurfaceKHR so this MUST be called here to respect SDL thread safety rules. Creating the renderer on the graphics thread is
    // NOTE: fine though.
    std::unique_ptr<VulkanContext> vulkanContext = nullptr;
    try {
        vulkanContext = VulkanContext::create(window);
    } catch (const std::runtime_error& e) {
        auto errorMsg = std::string("Failed to initialize the Vulkan context: ") + std::string(e.what());
        log::crit("{}", errorMsg);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", errorMsg.c_str(), nullptr);
        return;
    }
    auto windowCurrentRes = window.vulkanGetDrawableSize();

    auto threadSharedData = std::make_shared<MRThreadsSharedData>(windowCurrentRes);
    threadSharedData->m_sdlVideoDriverName = SDL_GetCurrentVideoDriver();

    MainThread mainthread(threadSharedData, std::move(vulkanContext), std::move(window));
    setThreadName("MainThread");

    auto renderThreadHandle = std::thread([&]() {
        RenderThread renderThread(threadSharedData);
        setThreadName("RenderThread");
        renderThread.runMainLoop();
    });



    mainthread.runMainLoop(initFn);
    renderThreadHandle.join();
}

auto entry(const std::function<void(std::unique_ptr<ecs::World>&)>& initFn) -> void {
    sdlPlatformInit();
    entryAfterSdlInit(initFn);
    sdlPlatformDeinit();
}

}