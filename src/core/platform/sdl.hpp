#pragma once
#include "core/math/matrix_types.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace nekomata2 {

// TODO: Document
void sdlPlatformInit();
void sdlPlatformDeinit();

// TODO: Document
class SdlWindow {
public:
    SdlWindow(std::nullptr_t);
    SdlWindow(const std::string& title, uint32_t width, uint32_t height);
    ~SdlWindow();
    
    SdlWindow(const SdlWindow&) = delete;
    SdlWindow(SdlWindow&&) = default;
    SdlWindow& operator=(const SdlWindow&) = delete;
    SdlWindow& operator=(SdlWindow&&) = default;

    // TODO: Document
    [[nodiscard]] static auto vulkanInstanceExtensions() -> std::vector<std::string>;
    // TODO: Document
    [[nodiscard]] auto vulkanCreateRawSurface(const vk::Instance& vkInstance) const -> vk::SurfaceKHR;
    // TODO: Document
    [[nodiscard]] auto vulkanGetDrawableSize() const -> vk::Extent2D;

    auto getLogicalSize() const -> math::Vector2i;
    auto getDisplayScale() const -> float;

private:
    SDL_Window* m_window = nullptr;
};

} // namespace nekomata2
