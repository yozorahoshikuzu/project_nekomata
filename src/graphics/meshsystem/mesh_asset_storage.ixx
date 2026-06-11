export module nekomata2.graphics.meshsystem.mesh_asset_storage;
import std;
import nekomata2.core.platform.int_def;
import nekomata2.graphics.meshsystem.pool.mesh_pool;
import nekomata2.core.containers.freelist_pool;
import nekomata2.core.math;

export namespace nekomata2::meshsystem {
class MeshAssetStorage;

inline MeshAssetStorage* g_meshAssetStorage = nullptr;

constexpr u32 kMaxLodCount = 4;

struct MeshAsset {
    usize storageIndex;
};

// todo: RAII semantics (refcounting)
struct Lod {
    MeshSuballocation meshSuballocation = {};
    float screenSizeThreshold = 0.0f;
};

struct LodList {
    std::array<Lod, kMaxLodCount> lods = {};

    /// Specifies the highest LOD level for the mesh.
    u32 maxLodIndex = 0;

    float boundingSphereRadius = 1.0f;
    float lodHysteresisFactor = 1.1f;

    /// Specifies the current index of the best available LOD. If set to ~0, then no LODs are available.
    std::atomic<u32> bestLodIndex = ~0;

    LodList() = default;
    LodList(const LodList& other) = delete;
    LodList& operator=(const LodList& other) = delete;
    LodList(LodList&& other) noexcept
        : lods(std::move(other.lods)), maxLodIndex(std::exchange(other.maxLodIndex, 0u)), bestLodIndex(other.bestLodIndex.exchange(~0u, std::memory_order_relaxed)) {}
    LodList& operator=(LodList&& other) noexcept {
        if (this == &other) return *this;

        lods = std::move(other.lods);
        maxLodIndex = std::exchange(other.maxLodIndex, 0u);
        bestLodIndex = other.bestLodIndex.exchange(~0u, std::memory_order_relaxed);
        return *this;
    }

    float computeScreenSpaceError(math::Vector3f objectPos, math::Vector3f cameraPos, float perspectiveFocalLength, float objectUniformScale) const {
        float distance = (objectPos - cameraPos).length();
        if (distance < math::consts::EPSILON) return std::numeric_limits<float>::infinity();
        return (2.0f * objectUniformScale * boundingSphereRadius / distance) * perspectiveFocalLength;
    }

};

class MeshAssetStorage {
public:
    static auto get() -> MeshAssetStorage& { return *g_meshAssetStorage; }
    static auto makeMeshPoolConfig() -> MeshPoolConfig;
    static auto create() -> std::unique_ptr<MeshAssetStorage>;

    MeshAssetStorage(std::nullptr_t) {}
    MeshAssetStorage();

    auto allocateMeshAsset() -> MeshAsset;
    auto freeMeshAsset(MeshAsset asset) -> void;

    auto getLodList(MeshAsset asset) -> LodList& { return m_lodLists[asset.storageIndex]; }
    auto getLodList(MeshAsset asset) const -> const LodList& { return m_lodLists[asset.storageIndex]; }

    auto perpareLodSpace(MeshAsset asset, u32 lodIndex, usize vertexBufferSize, usize indexBufferSize, usize vertexBufferAlignment, usize indexBufferAlignment) -> void;

    auto tickGC(u64 currentFrameIndex) -> void;
private:

    MeshPool m_meshPool = nullptr;
    FreelistPool<LodList, 10, 4096> m_lodLists;
};

}