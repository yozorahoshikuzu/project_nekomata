export module nekomata2:core.runtime.shared_data;
import std;
import vulkan;
import :core.platform.int_def;
import :core.ecs.component_pool;
import :core.ecs.world.camera;
import :core.ecs.world.transform;
import :core.ecs.world.renderable;
import :core.containers.double_buffer;
import :core.ui.ui_drawcmds;

export namespace nekomata2 {

struct MRThreadsSharedDataLeaf {
    u64 m_frameIndex;
    vk::Extent2D m_currentWindowExtent;
    ecs::ComponentSetSnapshot<ecs::components::Renderable> m_renderables;
    ecs::ComponentSetSnapshot<ecs::components::Transform> m_transforms;
    ecs::ComponentSetSnapshot<ecs::components::Camera> m_cameras;
    std::vector<u32> m_textureToImageShaderIndexSnapshot;
    std::vector<u32> m_textureToSamplerShaderIndexSnapshot;
    std::vector<ui::UiDrawCmd> m_uiDrawCmds;
    bool m_hasValidFrame = false;
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
};

}
