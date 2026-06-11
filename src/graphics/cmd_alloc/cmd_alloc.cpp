module nekomata2;
import :graphics.cmd_alloc;

namespace nekomata2::cmdalloc {

auto VulkanCommandPoolsList::initThreadLocalCommandPools() -> void {
    tl_graphicsCommandPool = VulkanCommandPool::createForGraphics(true);
    tl_asyncComputeCommandPool = VulkanCommandPool::createForAsyncCompute(true);
}

auto VulkanCommandPoolsList::destroyThreadLocalCommandPools() -> void {
    tl_graphicsCommandPool = nullptr;
    tl_asyncComputeCommandPool = nullptr;
}

} // namespace nekomata2::cmdalloc