export module projnekomata:core.runtime.mainthread;
import std;
import projnekomata.cs;
import :core.runtime.shared_data;
import :graphics.vulkan.context;
import :core.platform.sdl;
import :core.ecs;
import :core.input.inputmanager;
import :graphics.meshsystem.mesh_asset_storage;
import :graphics.texturesystem.texture_manager;
import :graphics.fontsystem.font_manager;
import :core.ui.ui_node;
import :core.ui.ui_system;

export namespace projnekomata {

class MainThread {
public:
    MainThread(std::shared_ptr<MRThreadsSharedData> mrSharedData, Unique<VulkanContext>&& vkContext, SdlWindow&& sdlWindow);

    auto runMainLoop(const std::function<void(Unique<ecs::World>&)>&) -> void;
    auto getCurrentWorld() -> ecs::World*;

private:
    auto loop(float dt) -> void;
    SdlWindow m_sdlWindow = nullptr;

    Unique<ecs::World> m_currentWorld = nullptr;

    std::shared_ptr<MRThreadsSharedData> m_mrSharedData = nullptr;
    Unique<core::input::Input> m_inputManager = nullptr;
    Unique<VulkanContext> m_vkContext = nullptr;
    Unique<meshsystem::MeshAssetStorage> m_meshAssetStorage = nullptr;
    Unique<graphics::texturesystem::TextureManager> m_textureManager = nullptr;
    Unique<graphics::fonts::FontManager> m_fontManager = nullptr;

    Unique<ui::UiSystem> m_uiSystem = nullptr;

    u64 m_frameIndex = 0;
    bool injectOverlay = false;
};

}
