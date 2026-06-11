export module nekomata2:core.containers.freelist_pool;
import std;
import :core.platform.int_def;
import :core.platform.assert;

export namespace nekomata2 {

template <typename T, usize ChunkSizePwr, usize MaxChunks, std::unsigned_integral Index = usize> class FreelistPool {
public:
    static constexpr Index kInvalidIndex = ~Index(0);
    static constexpr usize kChunkSize = 1 << ChunkSizePwr;
    static constexpr usize kChunkIndexMask = kChunkSize - 1;

    FreelistPool() {
        for (auto& p : m_pages) p.store(nullptr, std::memory_order_relaxed);
    }
    ~FreelistPool() {
        for (auto& ap : m_pages) {
            Page* p = ap.load(std::memory_order_relaxed);
            delete p;
        }
    }

    FreelistPool(const FreelistPool&)            = delete;
    FreelistPool& operator=(const FreelistPool&) = delete;

    template <class... Args>
    Index emplace(Args&&... args) {
        std::scoped_lock lock(m_allocationMutex);
        Index index;
        if (m_freelistHead != kInvalidIndex) {
            // There is an available index via the freelist.
            index = m_freelistHead;
            m_freelistHead = getNodeByIndex(index).nextFree;
        } else {
            // No indices in freelist. Need to get a new node.
            debug_assert(m_allocatedNodesCount < MaxChunks * kChunkSize, "FreelistPool exceeded max capacity");
            index = static_cast<Index>(m_allocatedNodesCount);
            usize pageIdx = index >> ChunkSizePwr;

            // If it crosses a page boundary, we have to allocate a new page.
            if (m_pages[pageIdx].load(std::memory_order_relaxed) == nullptr) {
                m_pages[pageIdx].store(new Page(), std::memory_order_release);
            }

            m_allocatedNodesCount++;
        }
        new (&getNodeByIndex(index).resource) T(std::forward<Args>(args)...);
        return index;
    }


    void free(Index index) {
        std::scoped_lock lock(m_allocationMutex);
        getNodeByIndex(index).resource.~T();
        getNodeByIndex(index).nextFree = m_freelistHead;
        m_freelistHead                 = index;
    }

    T& operator[](Index index) {
        return getNodeByIndex(index).resource;
    }

    const T& operator[](Index index) const {
        return getNodeByIndex(index).resource;
    }

private:
    struct Node {
        union { T resource; Index nextFree; };
        Node() {}
        ~Node() {}
    };

    struct Page {
        std::array<Node, kChunkSize> data;
        Page() {}
        ~Page() {}
    };

    Node& getNodeByIndex(Index index) {
        usize pageIndex = static_cast<usize>(index) >> ChunkSizePwr;
        usize chunkIndex = static_cast<usize>(index) & kChunkIndexMask;
        Page* page = m_pages[pageIndex].load(std::memory_order_acquire);
        debug_assert(page != nullptr, "FreelistPool accessed unallocated page");
        return page->data[chunkIndex];
    }

    const Node& getNodeByIndex(Index index) const {
        usize pageIndex = static_cast<usize>(index) >> ChunkSizePwr;
        usize chunkIndex = static_cast<usize>(index) & kChunkIndexMask;
        Page* page = m_pages[pageIndex].load(std::memory_order_acquire);
        debug_assert(page != nullptr, "FreelistPool accessed unallocated page");
        return page->data[chunkIndex];
    }

    bool allocNeedsNewPage() const {
        return (m_allocatedNodesCount & kChunkIndexMask) == 0;
    }

    std::array<std::atomic<Page*>, MaxChunks> m_pages;
    Index m_freelistHead = kInvalidIndex;
    usize m_allocatedNodesCount = 0;
    std::mutex m_allocationMutex;
};

}