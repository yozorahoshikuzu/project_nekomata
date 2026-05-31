#pragma once
#include "core/platform/int_def.hpp"
#include "vk_queue_family_swizzling.hpp"
#include "vulkan_prelude.hpp"
#include <expected>
#include <string>
#include <vector>

namespace nekomata2 {

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
};

enum class PhysicalDeviceAntiLagMethod {
    None,
    AMDAntiLag2,
};

consteval auto defaultEnabledVk10Features() -> vk::PhysicalDeviceFeatures;
consteval auto defaultEnabledVk11Features() -> vk::PhysicalDeviceVulkan11Features;
consteval auto defaultEnabledVk12Features() -> vk::PhysicalDeviceVulkan12Features;
consteval auto defaultEnabledVk13Features() -> vk::PhysicalDeviceVulkan13Features;
consteval auto defaultEnabledVk14Features() -> vk::PhysicalDeviceVulkan14Features;

class VulkanPhysicalDeviceProperties {
public:
    static auto query(const vk::raii::PhysicalDevice& vkPhysicalDevice, const vk::raii::SurfaceKHR& vkSurface) -> std::expected<VulkanPhysicalDeviceProperties, PhysicalDevicePropertyQueryError>;
    [[nodiscard]] auto autoselectPriorityScore() const -> u64;
    [[nodiscard]] auto queueCreateInfos() const -> std::vector<vk::DeviceQueueCreateInfo>;
    [[nodiscard]] auto vmaAllocatorCreateFlags() const -> vma::AllocatorCreateFlags;

    std::string m_deviceName;
    vk::PhysicalDeviceType m_deviceType;
    u64 m_vramSize{};

    bool m_hasExtMemoryBudget{};
    bool m_hasExtMemoryPriority{};
    bool m_hasKhrMaintenance4{};
    bool m_hasKhrMaintenance5{};
    bool m_hasRayTracing{};
    bool m_hasExtDescriptorHeap{};
    bool m_hasKhrPipelineBinary{};

    vk::PhysicalDeviceAccelerationStructurePropertiesKHR m_accelerationStructureProperties;
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rayTracingPipelineProperties;

    u32 m_graphicsQueueIndex{};
    u32 m_presentQueueIndex{};
    u32 m_asyncComputeQueueIndex{};

    VulkanQueueFamilySwizzling m_queueFamilies = nullptr;

    PhysicalDeviceAntiLagMethod m_antiLagMethod = PhysicalDeviceAntiLagMethod::None;

    // Passed to Vulkan device creation

    std::vector<std::string> m_enabledExtensions;
    vk::PhysicalDeviceFeatures m_enabledVk10Features;
    vk::PhysicalDeviceVulkan11Features m_enabledVk11Features;
    vk::PhysicalDeviceVulkan12Features m_enabledVk12Features;
    vk::PhysicalDeviceVulkan13Features m_enabledVk13Features;
    vk::PhysicalDeviceVulkan14Features m_enabledVk14Features;
};

class VulkanPhysicalDeviceSurfaceProperties {
public:
    static auto query(const vk::raii::PhysicalDevice& vkPhysicalDevice, const vk::raii::SurfaceKHR& vkSurface) -> VulkanPhysicalDeviceSurfaceProperties;

    vk::SurfaceCapabilitiesKHR m_capabilities;
    std::vector<vk::SurfaceFormatKHR> m_surfaceFormats;
    std::vector<vk::PresentModeKHR> m_presentModes;
};

} // namespace nekomata2