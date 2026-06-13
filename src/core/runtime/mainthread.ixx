export module nekomata2:core.runtime.mainthread;
import std;
import :core.platform.int_def;
import :core.runtime.shared_data;
import :graphics.vulkan.context;
import :core.platform.sdl;
import :core.ecs;
import :core.input.inputmanager;
import :graphics.meshsystem.mesh_asset_storage;
import :graphics.texturesystem.texture_manager;
import :graphics.fontsystem.font_manager;
import :core.ui.ui_node;

export namespace nekomata2 {

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
    std::unique_ptr<core::input::Input> m_inputManager = nullptr;
    std::unique_ptr<VulkanContext> m_vkContext = nullptr;
    std::unique_ptr<meshsystem::MeshAssetStorage> m_meshAssetStorage = nullptr;
    std::unique_ptr<graphics::texturesystem::TextureManager> m_textureManager = nullptr;
    std::unique_ptr<graphics::fonts::FontManager> m_fontManager = nullptr;

    std::unique_ptr<ui::UiNode> m_uiRoot = nullptr;

    u64 m_frameIndex = 0;
};

}
