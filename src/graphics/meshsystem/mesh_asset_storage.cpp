module nekomata2.graphics.meshsystem.mesh_asset_storage;
import nekomata2.graphics.vulkan.context;
import nekomata2.graphics.vulkan.vk_queue_family_swizzling;
import nekomata2.core.platform.assert;

namespace nekomata2::meshsystem {

auto MeshAssetStorage::makeMeshPoolConfig() -> MeshPoolConfig {
    MeshPoolConfig poolConfig{};
    poolConfig.dedicatedAllocationThreshold = 8 * 1024 * 1024;
    poolConfig.defaultDeferFrames = 4;
    poolConfig.maxSlabCount = 16;
    poolConfig.slabSize = 16 * 1024 * 1024;
    poolConfig.queueFamilyIndices = VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics | QueueFamily::AsyncCompute];
    return poolConfig;
}

MeshAssetStorage::MeshAssetStorage() : m_meshPool(makeMeshPoolConfig()) {}

auto MeshAssetStorage::create() -> std::unique_ptr<MeshAssetStorage> {
    debug_assert(g_meshAssetStorage == nullptr, "only one MeshAssetStorage may live at any given time");
    auto meshAssetStorage = std::make_unique<MeshAssetStorage>();
    g_meshAssetStorage = meshAssetStorage.get();
    return meshAssetStorage;
}

auto MeshAssetStorage::allocateMeshAsset() -> MeshAsset {
    auto storageIndex = m_lodLists.emplace();
    return MeshAsset{storageIndex};
}

auto MeshAssetStorage::freeMeshAsset(MeshAsset asset) -> void {
    m_lodLists.free(asset.storageIndex);
}

auto MeshAssetStorage::perpareLodSpace(MeshAsset asset, u32 lodIndex, usize vertexBufferSize, usize indexBufferSize, usize vertexBufferAlignment, usize indexBufferAlignment) -> void {
    getLodList(asset).lods[lodIndex].meshSuballocation = m_meshPool.allocateMesh(vertexBufferSize, indexBufferSize, vertexBufferAlignment, indexBufferAlignment);
}
auto MeshAssetStorage::tickGC(u64 currentFrameIndex) -> void {
    m_meshPool.tickGC(currentFrameIndex);
}

} // namespace nekomata2::meshsystem