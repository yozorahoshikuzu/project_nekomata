export module projnekomata:graphics.vulkan.vk_physical_device_props;
import std;
import vulkan;
import vk_mem_alloc;
import :graphics.vulkan.vk_queue_family_swizzling;
import :core.platform.int_def;
import :core.cs.vec;
import :core.cs.result;

export namespace projnekomata {

enum class PhysicalDevicePropertyQueryErrorKind {
    MissingKhrSwapchain,
    MissingVk14Maintenance5,
    MissingVk13Synchronization2,
    MissingVk13DynamicRendering,
    MissingVk12BufferDeviceAddress,
    MissingVk12DescriptorIndexing,
    MissingVk12ShaderSampledImageArrayNonUniformIndexing,
    MissingVk12DescriptorBindingPartiallyBound,
    MissingVk12DescriptorBindingSampledImageUpdateAfterBind,
    MissingVk12DescriptorBindingUpdateUnusedWhilePending,
    MissingVk12RuntimeDescriptorArray,
    MissingVk12ScalarBlockLayout,
    MissingVk12TimelineSemaphore,
    MissingVk10SamplerAnisotropy,
    MissingExtImageViewMinLod,
};

struct PhysicalDevicePropertyQueryError {
    PhysicalDevicePropertyQueryErrorKind m_kind;

    std::string toString() const {
        std::string cause = "";
        switch (m_kind) {
            case PhysicalDevicePropertyQueryErrorKind::MissingKhrSwapchain:
                cause = "Missing Vulkan extension VK_KHR_swapchain";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk14Maintenance5:
                cause = "Missing Vulkan 1.4 feature maintenance5";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk13Synchronization2:
                cause = "Missing Vulkan 1.3 feature synchronization2";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk13DynamicRendering:
                cause = "Missing Vulkan 1.3 feature dynamicRendering";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12BufferDeviceAddress:
                cause = "Missing Vulkan 1.2 feature bufferDeviceAddress";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorIndexing:
                cause = "Missing Vulkan 1.2 feature descriptorIndexing";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12ShaderSampledImageArrayNonUniformIndexing:
                cause = "Missing Vulkan 1.2 feature shaderSampledImageArrayNonUniformIndexing";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorBindingPartiallyBound:
                cause = "Missing Vulkan 1.2 feature descriptorBindingPartiallyBound";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorBindingSampledImageUpdateAfterBind:
                cause = "Missing Vulkan 1.2 feature descriptorBindingSampledImageUpdateAfterBind";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorBindingUpdateUnusedWhilePending:
                cause = "Missing Vulkan 1.2 feature descriptorBindingUpdateUnusedWhilePending";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12RuntimeDescriptorArray:
                cause = "Missing Vulkan 1.2 feature runtimeDescriptorArray";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12ScalarBlockLayout:
                cause = "Missing Vulkan 1.2 feature scalarBlockLayout";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk12TimelineSemaphore:
                cause = "Missing Vulkan 1.2 feature timelineSemaphore";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingVk10SamplerAnisotropy:
                cause = "Missing Vulkan 1.0 feature samplerAnisotropy";
                break;
            case PhysicalDevicePropertyQueryErrorKind::MissingExtImageViewMinLod:
                cause = "Missing Vulkan extension VK_EXT_image_view_min_lod";
                break;
        }
        return cause;
    }
};

consteval auto defaultEnabledVk10Features() -> vk::PhysicalDeviceFeatures;
consteval auto defaultEnabledVk11Features() -> vk::PhysicalDeviceVulkan11Features;
consteval auto defaultEnabledVk12Features() -> vk::PhysicalDeviceVulkan12Features;
consteval auto defaultEnabledVk13Features() -> vk::PhysicalDeviceVulkan13Features;
consteval auto defaultEnabledVk14Features() -> vk::PhysicalDeviceVulkan14Features;

class VulkanPhysicalDeviceProperties {
public:
    static auto query(const vk::raii::PhysicalDevice& vkPhysicalDevice, const vk::raii::SurfaceKHR& vkSurface) -> Result<VulkanPhysicalDeviceProperties, PhysicalDevicePropertyQueryError>;
    [[nodiscard]] auto autoselectPriorityScore() const -> u64;
    [[nodiscard]] auto queueCreateInfos() const -> std::vector<vk::DeviceQueueCreateInfo>;
    [[nodiscard]] auto vmaAllocatorCreateFlags() const -> vma::AllocatorCreateFlags;

    std::string m_deviceName            = "(unknown)";
    std::string m_driverName            = "(unknown driver name)";
    vk::PhysicalDeviceType m_deviceType = vk::PhysicalDeviceType::eOther;
    vk::DriverId m_driverId             = {};
    u32 m_driverVersion                 = 0_u32;
    u64 m_vramSize                      = 0_u64;
    u32 m_apiVersion                    = 0_u32;

    u32 m_vramMemoryHeapIndex = 0_u32;

    bool m_hasExtMemoryBudget   = false;
    bool m_hasExtMemoryPriority = false;
    bool m_hasKhrMaintenance4   = false;
    bool m_hasRayTracing        = false;
    bool m_hasExtDescriptorHeap = false;
    bool m_hasKhrPipelineBinary = false;
    bool m_hasAMDAntiLag2       = false;

    vk::PhysicalDeviceAccelerationStructurePropertiesKHR m_accelerationStructureProperties = {};
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR    m_rayTracingPipelineProperties    = {};

    u32 m_graphicsQueueIndex     = 0_u32;
    u32 m_presentQueueIndex      = 0_u32;
    u32 m_asyncComputeQueueIndex = 0_u32;

    VulkanQueueFamilySwizzling m_queueFamilies = nullptr;

    // Passed to Vulkan device creation

    Vec<std::string> m_enabledExtensions                     = Vec<std::string>::create();
    vk::PhysicalDeviceFeatures m_enabledVk10Features         = {};
    vk::PhysicalDeviceVulkan11Features m_enabledVk11Features = {};
    vk::PhysicalDeviceVulkan12Features m_enabledVk12Features = {};
    vk::PhysicalDeviceVulkan13Features m_enabledVk13Features = {};
    vk::PhysicalDeviceVulkan14Features m_enabledVk14Features = {};

    auto printInfo() const -> void;

    auto getApiVersionVariant() const -> u32 { return m_apiVersion >> 29; }
    auto getApiVersionMajor() const -> u32 { return (m_apiVersion >> 22) & 0x7f_u32; }
    auto getApiVersionMinor() const -> u32 { return (m_apiVersion >> 12) & 0x3ff_u32; }
    auto getApiVersionPatch() const -> u32 { return m_apiVersion & 0xfff_u32; }

    auto getDriverVersionVariant() const -> u32 {
        if (m_driverId == vk::DriverId::eNvidiaProprietary) return m_driverVersion >> 22;
        return m_driverVersion >> 29;
    }
    auto getDriverVersionMajor() const -> u32 {
        if (m_driverId == vk::DriverId::eNvidiaProprietary) return (m_driverVersion >> 14) & 0xff_u32;
        return (m_driverVersion >> 22) & 0x7f_u32;
    }
    auto getDriverVersionMinor() const -> u32 {
        if (m_driverId == vk::DriverId::eNvidiaProprietary) return (m_driverVersion >> 6) & 0xff_u32;
        return (m_driverVersion >> 12) & 0x3ff_u32;
    }
    auto getDriverVersionPatch() const -> u32 {
        if (m_driverId == vk::DriverId::eNvidiaProprietary) return m_driverVersion & 0x3f_u32;
        return m_driverVersion & 0xfff_u32;
    }

};

class VulkanPhysicalDeviceSurfaceProperties {
public:
    static auto query(const vk::raii::PhysicalDevice& vkPhysicalDevice, const vk::raii::SurfaceKHR& vkSurface) -> VulkanPhysicalDeviceSurfaceProperties;

    vk::SurfaceCapabilitiesKHR m_capabilities;
    Vec<vk::SurfaceFormatKHR> m_surfaceFormats = Vec<vk::SurfaceFormatKHR>::create();
    Vec<vk::PresentModeKHR> m_presentModes     = Vec<vk::PresentModeKHR>::create();
};

} // namespace projnekomata