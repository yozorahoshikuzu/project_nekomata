export module nekomata2.graphics.vulkan.vk_buffer;
import std;
import vulkan;
import vk_mem_alloc;
import nekomata2.core.platform.int_def;
import nekomata2.graphics.vulkan.vk_gpu_obrm;

export namespace nekomata2 {

enum class VulkanBufferMemoryMapping {
    DontMap,
    MapForSequentialWrite,
    MapForRandomAccess,
};

class VulkanBuffer {
public:
    VulkanBuffer(std::nullptr_t);
    VulkanBuffer(vk::raii::Buffer&& buffer, vma::raii::Allocation&& allocation, u64 size, u8* mVkBufferMemoryHostPtr, vk::DeviceAddress mVkBufferMemoryDevicePtr);

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&&) = default;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(VulkanBuffer&&)  noexcept = default;

    static auto create(u64 size, vk::BufferUsageFlags usage, VulkanBufferMemoryMapping hostMemoryMapping, vma::MemoryUsage memoryUsage, vk::MemoryPropertyFlags memoryRequiredFlags, const std::span<const u32>& queueFamilyIndices) -> VulkanBuffer;

    [[nodiscard]] auto vkBuffer() const        -> const vk::raii::Buffer& { return m_vkBuffer.vkHandle(); }
    [[nodiscard]] auto size() const            -> u64 { return m_size; }
    [[nodiscard]] auto memoryHostPtr() const   -> u8* { return m_vkBufferMemoryHostPtr; }
    [[nodiscard]] auto memoryDevicePtr() const -> vk::DeviceAddress { return m_vkBufferMemoryDevicePtr; }

private:
    VulkanAsyncRaiiWrapper<vk::raii::Buffer>      m_vkBuffer      = nullptr;
    VulkanAsyncRaiiWrapper<vma::raii::Allocation> m_vmaAllocation = nullptr;


    u8*               m_vkBufferMemoryHostPtr   = nullptr;
    vk::DeviceAddress m_vkBufferMemoryDevicePtr = vk::DeviceAddress(nullptr);

    u64 m_size{};
};

}