export module nekomata2:graphics.vulkan.context;
import std;
import vulkan;
import vk_mem_alloc;
import :core.platform.int_def;
import :core.platform.sdl;
import :graphics.vulkan.vk_physical_device_props;
import :graphics.vulkan.vk_queue;
import :core.cs.panic;

export namespace nekomata2 {

template <typename T> inline auto vkCheckResult(vk::ResultValue<T> x) {
    if (x.result != vk::Result::eSuccess) {
        panic("Vulkan result check failed: returned {}", vk::to_string(x.result));
    }
    return std::move(x.value);
}

inline auto vkCheckResult(vk::Result x) {
    if (x != vk::Result::eSuccess) {
        panic("Vulkan result check failed: returned {}", vk::to_string(x));
    }
}

enum class AntiLagMethod {
    None,
    AMDAntiLag2,
};

class VulkanContext {
public:
    VulkanContext(std::nullptr_t);
    ~VulkanContext();

    static auto get() -> VulkanContext&;

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;
    
    static auto create(SdlWindow& sdlWindow) -> std::unique_ptr<VulkanContext>;

    [[nodiscard]] auto vkPhysicalDevice() const -> const vk::raii::PhysicalDevice& { return m_vkPhysicalDevice; }
    [[nodiscard]] auto vkPhysicalDeviceProps() const -> const VulkanPhysicalDeviceProperties& { return m_vkPhysicalDeviceProperties; }
    [[nodiscard]] auto vkSurface() const -> const vk::raii::SurfaceKHR& { return m_vkSurface; }
    [[nodiscard]] auto vkDevice() const -> const vk::raii::Device& { return m_vkDevice; }
    [[nodiscard]] auto vmaAllocator() const -> const vma::raii::Allocator& { return m_vmaAllocator; }

    [[nodiscard]] auto vkQueueGraphics() const -> VulkanQueue& { return *m_vkGraphicsQueue; }
    [[nodiscard]] auto vkQueueAsyncCompute() const -> VulkanQueue& { return *m_vkAsyncComputeQueue; }
    [[nodiscard]] auto vkQueuePresent() const -> VulkanQueue& { return *m_vkPresentQueue; }

    [[nodiscard]] auto vkDeletionQueue() -> std::unique_ptr<VulkanResourceDeletionQueue>& { return m_vkResourceDeletionQueue; }
    [[nodiscard]] auto shaderCache() -> std::unique_ptr<class ShaderCache>& { return m_shaderCache; }

    auto antiLagPaceInput(u64 frameIndex, u32 targetFps) -> void;
    auto antiLagPacePresent(u64 frameIndex, u32 targetFps) -> void;

    auto extMemoryBudgetGetVramBudget() const -> u64;

private:

    // TODO: Document
    static auto initVkRaiiContext() -> vk::raii::Context;
    // TODO: Document
    static auto createVkInstance(vk::raii::Context& vkRaiiContext, bool debugEnable, nekomata2::SdlWindow& sdlWindow) -> vk::raii::Instance;
    // TODO: Document
    static auto createVkSurface(const vk::raii::Instance& vkInstance, nekomata2::SdlWindow& sdlWindow) -> vk::raii::SurfaceKHR;
    // TODO: Document
    static auto pickVkPhysicalDevice(const vk::raii::Instance& vkInstance, const vk::raii::SurfaceKHR& vkSurface) -> std::tuple<vk::raii::PhysicalDevice, VulkanPhysicalDeviceProperties>;
    // TODO: Document
    static auto createVkDevice(const vk::raii::PhysicalDevice& vkPhysicalDevice, const VulkanPhysicalDeviceProperties& vkPhysicalDeviceProps) -> vk::raii::Device;
    // TODO: Document
    static auto createVmaAllocator(const vk::raii::Instance& vkInstance, const vk::raii::PhysicalDevice& vkPhysicalDevice, const VulkanPhysicalDeviceProperties& vkPhysicalDeviceProps, const vk::raii::Device& vkDevice) -> vma::raii::Allocator;

    vk::detail::DynamicLoader m_vkDynamicLoader;
    vk::raii::Context m_vkRaiiContext;

    vk::raii::Instance m_vkInstance = nullptr;
    vk::raii::SurfaceKHR m_vkSurface = nullptr;
    vk::raii::PhysicalDevice m_vkPhysicalDevice = nullptr;
    VulkanPhysicalDeviceProperties m_vkPhysicalDeviceProperties;
    vk::raii::Device m_vkDevice = nullptr;

    std::vector<std::unique_ptr<VulkanQueue>> m_vkQueues;
    VulkanQueue* m_vkGraphicsQueue{};
    VulkanQueue* m_vkPresentQueue{};
    VulkanQueue* m_vkAsyncComputeQueue{};

    vma::raii::Allocator m_vmaAllocator = nullptr;

    std::unique_ptr<class VulkanResourceDeletionQueue> m_vkResourceDeletionQueue;
    std::unique_ptr<class ShaderCache> m_shaderCache;

    std::atomic<AntiLagMethod> m_antiLagMethod = AntiLagMethod::AMDAntiLag2;
};

inline VulkanContext* g_vkContext = nullptr;

// ------------------------------------------------------------------------------------------------------------------------------------------------------------


} // namespace nekomata2