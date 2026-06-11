module nekomata2;
import :core.runtime.shared_data;

namespace nekomata2 {

MRThreadsSharedData::MRThreadsSharedData(vk::Extent2D windowCurrentRes) : m_syncpointBarrier(std::barrier(2)), m_shouldQuit(std::atomic(false)) {
    m_leafs.getPrimary().m_currentWindowExtent = windowCurrentRes;
    m_leafs.getSecondary().m_currentWindowExtent = windowCurrentRes;

    m_leafs.getPrimary().m_textureToImageShaderIndexSnapshot.resize(4096);
    m_leafs.getSecondary().m_textureToImageShaderIndexSnapshot.resize(4096);
    m_leafs.getSecondary().m_textureToSamplerShaderIndexSnapshot.resize(4096);
    m_leafs.getPrimary().m_textureToSamplerShaderIndexSnapshot.resize(4096);
}

}