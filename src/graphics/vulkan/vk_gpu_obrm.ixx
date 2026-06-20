export module nekomata2:graphics.vulkan.vk_gpu_obrm;
import std;
import vulkan;
import :graphics.vulkan.deletion_queue;
import :graphics.vulkan.vk_gpu_obrm_structs;

export namespace nekomata2 {

template <typename T>
concept AnyVulkanRaiiHandle = requires {
    { vk::raii::isVulkanRAIIHandleType<T>::value } -> std::same_as<const bool&>;
    requires vk::raii::isVulkanRAIIHandleType<T>::value;
};

template <typename T>
class VulkanAsyncRaiiWrapper {
public:
    VulkanAsyncRaiiWrapper(std::nullptr_t) : m_vkHandle(nullptr) {}
    VulkanAsyncRaiiWrapper(T&& vkHandle) : m_vkHandle(std::move(vkHandle)) {}
    ~VulkanAsyncRaiiWrapper() {
        if (m_vkHandle != nullptr) {
            auto raiiUseMarker = GpuResourceRetireTimelineValues::latestSubmitValues();
            VulkanResourceDeletionQueue::get().pushObject(raiiUseMarker, std::move(m_vkHandle));
        }
    }

    VulkanAsyncRaiiWrapper(const VulkanAsyncRaiiWrapper&) = delete;
    VulkanAsyncRaiiWrapper(VulkanAsyncRaiiWrapper&&) = default;
    VulkanAsyncRaiiWrapper& operator=(const VulkanAsyncRaiiWrapper&) = delete;
    VulkanAsyncRaiiWrapper& operator=(VulkanAsyncRaiiWrapper&&) = default;

    [[nodiscard]] auto vkHandle() const -> const T& {
        return m_vkHandle;
    }

private:
    T m_vkHandle = nullptr;
};

} // namespace nekomata2