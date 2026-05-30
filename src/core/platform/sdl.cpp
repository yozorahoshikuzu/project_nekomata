#include "sdl.hpp"
#include "../log/log.hpp"
#include "../platform/int_def.hpp"
#include "vulkan/vulkan.hpp"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <cstddef>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace nekomata2 {

void sdlPlatformInit() {
    log::info("Initializing SDL...");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log::crit("Failed to initialize SDL: {}", SDL_GetError());
        throw std::runtime_error("failed to initialize SDL");
    }

    auto sdlVideodriver = SDL_GetCurrentVideoDriver();
    log::info("Current SDL video driver: {}", sdlVideodriver);
}
void sdlPlatformDeinit() { SDL_QuitSubSystem(SDL_INIT_VIDEO); SDL_Quit(); }

SdlWindow::SdlWindow(std::nullptr_t) {}

SdlWindow::SdlWindow(const std::string& title, const uint32_t width, const uint32_t height) {
    m_window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (m_window == nullptr) {
        log::crit("Failed to create SDL window: {}", SDL_GetError());
        throw std::runtime_error("failed to create SDL window");

    }
}

SdlWindow::~SdlWindow() { log::info("destroying window..."); SDL_DestroyWindow(m_window); }

auto SdlWindow::vulkanInstanceExtensions() -> std::vector<std::string> {
    uint32_t extensionCount;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    std::vector<std::string> result;
    result.reserve(extensionCount);
    for (uint32_t i = 0; i < extensionCount; i++) {
        result.emplace_back(extensions[i]);
    }

    return result;
}

auto SdlWindow::vulkanCreateRawSurface(const vk::Instance& vkInstance) const -> vk::SurfaceKHR {
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(m_window, vkInstance, nullptr, &surface)) {
        auto msg = SDL_GetError();
        log::crit("SDL Error when creating vk::SurfaceKHR: {}", msg);
        throw std::logic_error(msg);
    }

    return {surface};
}

auto SdlWindow::vulkanGetDrawableSize() const -> vk::Extent2D {
    vk::Extent2D extents;
    SDL_GetWindowSizeInPixels(m_window, reinterpret_cast<i32*>(&extents.width), reinterpret_cast<i32*>(&extents.height));
    return extents;
}

auto SdlWindow::getLogicalSize() const -> math::Vector2i {
    math::Vector2i size;
    SDL_GetWindowSize(m_window, &size.x(), &size.y());
    return size;
}

auto SdlWindow::getDisplayScale() const -> float {
    return SDL_GetWindowDisplayScale(m_window);
}

} // namespace nekomata2

