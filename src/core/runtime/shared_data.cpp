module projnekomata;
import :core.runtime.shared_data;

namespace projnekomata {

MRThreadsSharedData::MRThreadsSharedData(vk::Extent2D windowCurrentRes)
    : m_syncpointBarrier(std::barrier(2)), m_shouldQuit(std::atomic(false)) {
    m_leafs.getPrimary().m_currentWindowExtent = windowCurrentRes;
    m_leafs.getSecondary().m_currentWindowExtent = windowCurrentRes;
}

}