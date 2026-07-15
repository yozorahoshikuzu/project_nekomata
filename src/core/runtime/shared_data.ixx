export module projnekomata:core.runtime.shared_data;
import std;
import projnekomata.cs;
import vulkan;
import :core.ecs.component_pool;
import :core.ecs.world.camera;
import :core.ecs.world.transform;
import :core.ecs.world.renderable;
import :core.containers.double_buffer;
import :core.ui.ui_drawcmds;
import :core.ecs.world.pointlight;

export namespace projnekomata {

struct MRThreadsSharedDataLeaf {
    MRThreadsSharedDataLeaf() = default;
    u64 m_frameIndex;
    vk::Extent2D m_currentWindowExtent;

    ecs::ComponentSetSnapshot<ecs::components::Renderable> m_renderables;
    ecs::ComponentSetSnapshot<ecs::components::PointLight> m_pointlights;
    ecs::ComponentSetSnapshot<ecs::components::Transform> m_transforms;
    ecs::ComponentSetSnapshot<ecs::components::Camera> m_cameras;

    Vec<u32> m_textureToImageShaderIndexSnapshot   = Vec<u32>::fromValue(4096, 0);
    Vec<u32> m_textureToSamplerShaderIndexSnapshot = Vec<u32>::fromValue(4096, 0);
    Vec<ui::UiDrawCmd> m_uiDrawCmds                = Vec<ui::UiDrawCmd>::create();
    bool m_hasValidFrame = false;
    bool injectOverlay = false;
};

class MRThreadsSharedData {
public:
    MRThreadsSharedData(vk::Extent2D windowCurrentRes);

    MRThreadsSharedData(const MRThreadsSharedData&) = delete;
    MRThreadsSharedData(MRThreadsSharedData&&) = delete;
    MRThreadsSharedData& operator=(const MRThreadsSharedData&) = delete;
    MRThreadsSharedData& operator=(MRThreadsSharedData&&) = delete;

    DoubleBuffer<MRThreadsSharedDataLeaf> m_leafs;
    std::barrier<> m_syncpointBarrier;

    std::atomic<bool> m_shouldQuit;
    std::string_view m_sdlVideoDriverName;
};

}
