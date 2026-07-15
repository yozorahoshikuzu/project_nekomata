module;
#include <SDL3/SDL_messagebox.h>
module projnekomata;
import std;
import projnekomata.cs;
import :core.platform.sdl;
import :graphics.vulkan.context;
import :core.runtime.shared_data;
import :core.runtime.mainthread;
import :core.runtime.graphicsthread;

namespace projnekomata {

auto entryAfterSdlInit(const std::function<void(Unique<ecs::World>&)>& initFn) -> void {
    Thread::setThreadName("MainThread");
    // TODO: A system to remember player preferences to use here instead.

    auto window = projnekomata::SdlWindow("Project Nekomata", 1920, 1080);

    // NOTE: This creates the vk::SurfaceKHR so this MUST be called here to respect SDL thread safety rules. Creating the renderer on the graphics thread is
    // NOTE: fine though.
    Unique<VulkanContext> vulkanContext = VulkanContext::create(window);
    auto windowCurrentRes = window.vulkanGetDrawableSize();

    auto threadSharedData = std::make_shared<MRThreadsSharedData>(windowCurrentRes);
    threadSharedData->m_sdlVideoDriverName = SDL_GetCurrentVideoDriver();

    MainThread mainthread(threadSharedData, std::move(vulkanContext), std::move(window));

    auto renderThreadHandle = std::thread([&]() {
        RenderThread renderThread(threadSharedData);
        Thread::setThreadName("RenderThread");
        renderThread.runMainLoop();
    });



    mainthread.runMainLoop(initFn);
    renderThreadHandle.join();
}

auto entry(const std::function<void(Unique<ecs::World>&)>& initFn) -> void {
    setupBacktrace();
    sdlPlatformInit();
    entryAfterSdlInit(initFn);
    sdlPlatformDeinit();
}

}