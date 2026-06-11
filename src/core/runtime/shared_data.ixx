export module nekomata2.core.runtime.shared_data;
import std;
import vulkan;
import nekomata2.core.platform.int_def;
import nekomata2.core.ecs.component_pool;
import nekomata2.core.ecs.world.camera;
import nekomata2.core.ecs.world.transform;
import nekomata2.core.ecs.world.renderable;
import nekomata2.core.containers.double_buffer;

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
