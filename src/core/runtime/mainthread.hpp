#pragma once

#include "core/ecs/ecs.hpp"
#include "core/platform/sdl.hpp"
#include "core/runtime/shared_data.hpp"
#include "core/ui/ui_node.hpp"
#include "graphics/fontsystem/font_manager.hpp"
#include "graphics/meshsystem/mesh_asset_storage.hpp"
#include "graphics/vulkan/context.hpp"

#include <memory>
namespace nekomata2 {

class MainThread {
public:
    MainThread(std::shared_ptr<MRThreadsSharedData> mrSharedData, std::unique_ptr<VulkanContext>&& vkContext, SdlWindow&& sdlWindow);

    auto runMainLoop(const std::function<void(std::unique_ptr<ecs::World>&)>&) -> void;
    auto getCurrentWorld() -> ecs::World*;

private:
    auto loop(float dt) -> void;
    SdlWindow m_sdlWindow = nullptr;

    std::unique_ptr<ecs::World> m_currentWorld = nullptr;

    std::shared_ptr<MRThreadsSharedData> m_mrSharedData = nullptr;
    std::unique_ptr<VulkanContext> m_vkContext = nullptr;
    std::unique_ptr<meshsystem::MeshAssetStorage> m_meshAssetStorage = nullptr;
    std::unique_ptr<graphics::texturesystem::TextureManager> m_textureManager = nullptr;
    std::unique_ptr<graphics::fonts::FontManager> m_fontManager = nullptr;

    std::unique_ptr<ui::UiNode> m_uiRoot = nullptr;
};

}
