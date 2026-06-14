module nekomata2;
import std;
import vulkan;
import vk_mem_alloc;
import :graphics.vulkan.vk_physical_device_props;

using namespace std::literals;

namespace nekomata2 {

struct RequiredFeatureRule {
    std::string_view m_name;
    vk::Bool32 vk::PhysicalDeviceFeatures::* m_vk10 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan11Features::* m_vk11 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan12Features::* m_vk12 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan13Features::* m_vk13 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan14Features::* m_vk14 = nullptr;
    PhysicalDevicePropertyQueryErrorKind m_errorKindIfMissing;
};

// clang-format off
static constexpr auto REQUIRED_FEATURES = std::array<RequiredFeatureRule, 13>{{
    { "maintenance5"sv,                                      {}, {}, {}, {}, &vk::PhysicalDeviceVulkan14Features::maintenance5, PhysicalDevicePropertyQueryErrorKind::MissingVk14Maintenance5 },
    { "synchronization2"sv,                                  {}, {}, {}, &vk::PhysicalDeviceVulkan13Features::synchronization2, {},   PhysicalDevicePropertyQueryErrorKind::MissingVk13Synchronization2 },
    { "dynamicRendering"sv,                                  {}, {}, {}, &vk::PhysicalDeviceVulkan13Features::dynamicRendering, {},   PhysicalDevicePropertyQueryErrorKind::MissingVk13DynamicRendering },
    { "bufferDeviceAddress"sv,                               {}, {}, &vk::PhysicalDeviceVulkan12Features::bufferDeviceAddress, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12BufferDeviceAddress },
    { "descriptorIndexing"sv,                                {}, {}, &vk::PhysicalDeviceVulkan12Features::descriptorIndexing,  {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorIndexing },
    { "shaderSampledImageArrayNonUniformIndexing"sv,         {}, {}, &vk::PhysicalDeviceVulkan12Features::shaderSampledImageArrayNonUniformIndexing, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12ShaderSampledImageArrayNonUniformIndexing },
    { "descriptorBindingPartiallyBound"sv,                   {}, {}, &vk::PhysicalDeviceVulkan12Features::descriptorBindingPartiallyBound, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorBindingPartiallyBound },
    { "descriptorBindingSampledImageUpdateAfterBind"sv,      {}, {}, &vk::PhysicalDeviceVulkan12Features::descriptorBindingSampledImageUpdateAfterBind, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorBindingSampledImageUpdateAfterBind },
    { "descriptorBindingUpdateUnusedWhilePending"sv,         {}, {}, &vk::PhysicalDeviceVulkan12Features::descriptorBindingUpdateUnusedWhilePending, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12DescriptorBindingUpdateUnusedWhilePending },
    { "runtimeDescriptorArray"sv,                            {}, {}, &vk::PhysicalDeviceVulkan12Features::runtimeDescriptorArray, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12RuntimeDescriptorArray },
    { "scalarBlockLayout"sv,                                 {}, {}, &vk::PhysicalDeviceVulkan12Features::scalarBlockLayout, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12ScalarBlockLayout },
    { "timelineSemaphore"sv,                                 {}, {}, &vk::PhysicalDeviceVulkan12Features::timelineSemaphore, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk12TimelineSemaphore },
    { "samplerAnisotropy"sv,                                 &vk::PhysicalDeviceFeatures::samplerAnisotropy, {}, {}, {}, {}, PhysicalDevicePropertyQueryErrorKind::MissingVk10SamplerAnisotropy },
}};
// clang-format on

consteval auto defaultEnabledVk10Features() -> vk::PhysicalDeviceFeatures {
    auto features = vk::PhysicalDeviceFeatures{};
    for (auto& rule : REQUIRED_FEATURES) {
        if (rule.m_vk10)
            features.*(rule.m_vk10) = true;
    }
    return features;
}

consteval auto defaultEnabledVk11Features() -> vk::PhysicalDeviceVulkan11Features {
    auto features = vk::PhysicalDeviceVulkan11Features{};
    for (auto& rule : REQUIRED_FEATURES) {
        if (rule.m_vk11)
            features.*(rule.m_vk11) = true;
    }
    return features;
}

consteval auto defaultEnabledVk12Features() -> vk::PhysicalDeviceVulkan12Features {
    auto features = vk::PhysicalDeviceVulkan12Features{};
    for (auto& rule : REQUIRED_FEATURES) {
        if (rule.m_vk12)
            features.*(rule.m_vk12) = true;
    }
    return features;
}

consteval auto defaultEnabledVk13Features() -> vk::PhysicalDeviceVulkan13Features {
    auto features = vk::PhysicalDeviceVulkan13Features{};
    for (auto& rule : REQUIRED_FEATURES) {
        if (rule.m_vk13)
            features.*(rule.m_vk13) = true;
    }
    return features;
}

consteval auto defaultEnabledVk14Features() -> vk::PhysicalDeviceVulkan14Features {
    auto features = vk::PhysicalDeviceVulkan14Features{};
    for (auto& rule : REQUIRED_FEATURES) {
        if (rule.m_vk14)
            features.*(rule.m_vk14) = true;
    }
    return features;
}

auto findBestQueue(const std::vector<u32>& queueIndices, std::unordered_set<u32>& hashset) -> u32 {
    auto it = std::ranges::find_if(queueIndices, [&](u32 x) -> bool {
        return !hashset.contains(x);
    });

    return it != queueIndices.end() ? *it : queueIndices.front();
}

auto dedupQueueIndices(std::vector<u32>& queueIndices) {
    std::unordered_set<u32> queueIndicesSet(queueIndices.begin(), queueIndices.end());
    queueIndices.assign(queueIndicesSet.begin(), queueIndicesSet.end());
}

auto VulkanPhysicalDeviceProperties::query(const vk::raii::PhysicalDevice& vkPhysicalDevice, const vk::raii::SurfaceKHR& vkSurface)
    -> std::expected<VulkanPhysicalDeviceProperties, PhysicalDevicePropertyQueryError> {
    auto supportedExtensionProperties = vkPhysicalDevice.enumerateDeviceExtensionProperties();
    auto supportedExtensionNames = supportedExtensionProperties |
                                     std::views::transform([](const vk::ExtensionProperties& ext) -> std::string { return std::string(ext.extensionName); }) |
                                     std::ranges::to<std::vector>();

    bool hasKhrSwapchain = std::ranges::contains(supportedExtensionNames, vk::KHRSwapchainExtensionName);
    if (!hasKhrSwapchain) {
        return std::unexpected(PhysicalDevicePropertyQueryError{.m_kind = PhysicalDevicePropertyQueryErrorKind::MissingKhrSwapchain});
    }


    auto featuresQuery = vkPhysicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceVulkan14Features, vk::PhysicalDeviceImageViewMinLodFeaturesEXT, vk::PhysicalDeviceAntiLagFeaturesAMD>();
    auto supportedVk10Features = featuresQuery.get<vk::PhysicalDeviceFeatures2>();
    auto supportedVk11Features = featuresQuery.get<vk::PhysicalDeviceVulkan11Features>();
    auto supportedVk12Features = featuresQuery.get<vk::PhysicalDeviceVulkan12Features>();
    auto supportedVk13Features = featuresQuery.get<vk::PhysicalDeviceVulkan13Features>();
    auto supportedVk14Features = featuresQuery.get<vk::PhysicalDeviceVulkan14Features>();
    auto supportedExtImageViewMinLodFeatures = featuresQuery.get<vk::PhysicalDeviceImageViewMinLodFeaturesEXT>();

    for (auto& rule : REQUIRED_FEATURES) {
        bool satisfied = true;
        if (rule.m_vk10)
            satisfied &= supportedVk10Features.features.*(rule.m_vk10);
        if (rule.m_vk11)
            satisfied &= supportedVk11Features.*(rule.m_vk11);
        if (rule.m_vk12)
            satisfied &= supportedVk12Features.*(rule.m_vk12);
        if (rule.m_vk13)
            satisfied &= supportedVk13Features.*(rule.m_vk13);
        if (rule.m_vk14)
            satisfied &= supportedVk14Features.*(rule.m_vk14);

        if (!satisfied)
            return std::unexpected(PhysicalDevicePropertyQueryError{.m_kind = rule.m_errorKindIfMissing});
    }

    if (!supportedExtImageViewMinLodFeatures.minLod) {
        return std::unexpected(PhysicalDevicePropertyQueryError{.m_kind = PhysicalDevicePropertyQueryErrorKind::MissingExtImageViewMinLod});
    }

    auto enabledVk10Features = defaultEnabledVk10Features();
    auto enabledVk11Features = defaultEnabledVk11Features();
    auto enabledVk12Features = defaultEnabledVk12Features();
    auto enabledVk13Features = defaultEnabledVk13Features().setMaintenance4(supportedVk13Features.maintenance4);
    auto enabledVk14Features = defaultEnabledVk14Features();

    bool hasExtMemoryBudget = std::ranges::contains(supportedExtensionNames, vk::EXTMemoryBudgetExtensionName);
    bool hasExtMemoryPriority = std::ranges::contains(supportedExtensionNames, vk::EXTMemoryPriorityExtensionName);
    bool hasKhrRayTracing = std::ranges::contains(supportedExtensionNames, vk::KHRAccelerationStructureExtensionName) &&
                               std::ranges::contains(supportedExtensionNames, vk::KHRRayTracingPipelineExtensionName) &&
                               std::ranges::contains(supportedExtensionNames, vk::KHRDeferredHostOperationsExtensionName);
    bool hasKhrMaintenance4 = supportedVk13Features.maintenance4;
    bool hasKhrMaintenance5 = supportedVk14Features.maintenance5;
    bool hasExtDescriptorHeap = std::ranges::contains(supportedExtensionNames, vk::EXTDescriptorHeapExtensionName);
    bool hasKhrPipelineBinary = std::ranges::contains(supportedExtensionNames, vk::KHRPipelineBinaryExtensionName);

    auto antiLagMethod = PhysicalDeviceAntiLagMethod::None;

    if (std::ranges::contains(supportedExtensionNames, vk::AMDAntiLagExtensionName) && featuresQuery.get<vk::PhysicalDeviceAntiLagFeaturesAMD>().antiLag) {
        antiLagMethod = PhysicalDeviceAntiLagMethod::AMDAntiLag2;
    }

    std::vector<std::string> enabledExtensions;

    enabledExtensions.emplace_back(vk::KHRSwapchainExtensionName);
    enabledExtensions.emplace_back(vk::EXTImageViewMinLodExtensionName);

    if (hasExtMemoryBudget) {
        enabledExtensions.emplace_back(vk::EXTMemoryBudgetExtensionName);
    }

    if (hasKhrRayTracing) {
        enabledExtensions.emplace_back(vk::KHRAccelerationStructureExtensionName);
        enabledExtensions.emplace_back(vk::KHRRayTracingPipelineExtensionName);
        enabledExtensions.emplace_back(vk::KHRDeferredHostOperationsExtensionName);
    }

    if (hasExtDescriptorHeap) {
        enabledExtensions.emplace_back(vk::EXTDescriptorHeapExtensionName);
    }

    if (hasKhrPipelineBinary) {
        enabledExtensions.emplace_back(vk::KHRPipelineBinaryExtensionName);
    }

    switch (antiLagMethod) {
    case PhysicalDeviceAntiLagMethod::AMDAntiLag2:
        enabledExtensions.emplace_back(vk::AMDAntiLagExtensionName);
        break;
    default: break;
    }

    auto propertiesQuery = vkPhysicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceVulkan12Properties,
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    auto coreProperties = propertiesQuery.get<vk::PhysicalDeviceProperties2>().properties;
    auto accelerationStructureProperties = propertiesQuery.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    auto rayTracingPipelineProperties = propertiesQuery.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    auto deviceName = std::string(coreProperties.deviceName);
    auto deviceType = coreProperties.deviceType;

    auto memoryProperties = vkPhysicalDevice.getMemoryProperties();
    u64 vramSize = 0;
    for (usize i = 0; i < memoryProperties.memoryHeapCount; i++) {
        auto& heap = memoryProperties.memoryHeaps[i];
        if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
            vramSize += heap.size;
        }
    }

    auto queueProps = vkPhysicalDevice.getQueueFamilyProperties();
    std::vector<u32> graphicsIndices, presentIndices, asyncComputeIndices;
    
    for (u32 queueIndex = 0; queueIndex < queueProps.size(); queueIndex++) {
        vk::QueueFamilyProperties& family = queueProps[queueIndex];

        if (family.queueFlags & vk::QueueFlagBits::eGraphics) graphicsIndices.emplace_back(queueIndex);
        if (family.queueFlags & vk::QueueFlagBits::eCompute && !(family.queueFlags & vk::QueueFlagBits::eGraphics)) asyncComputeIndices.emplace_back(queueIndex);
        if (vkPhysicalDevice.getSurfaceSupportKHR(queueIndex, vkSurface)) presentIndices.emplace_back(queueIndex);
    }

    std::unordered_set<u32> usedQueueIndices;
    u32 graphicsQueueIndex = findBestQueue(graphicsIndices, usedQueueIndices);
    u32 presentQueueIndex = findBestQueue(presentIndices, usedQueueIndices);
    u32 asyncComputeQueueIndex = findBestQueue(asyncComputeIndices, usedQueueIndices);
    std::vector<u32> allQueueIndices = { graphicsQueueIndex, presentQueueIndex, asyncComputeQueueIndex };
    dedupQueueIndices(allQueueIndices);
    std::vector<u32> swapchainImageQueueIndices = { graphicsQueueIndex, presentQueueIndex };
    dedupQueueIndices(swapchainImageQueueIndices);

    VulkanPhysicalDeviceProperties props{};
    props.m_deviceName = deviceName;
    props.m_deviceType = deviceType;
    props.m_driverId = propertiesQuery.get<vk::PhysicalDeviceVulkan12Properties>().driverID;
    props.m_vramSize = vramSize;
    props.m_hasExtMemoryBudget = hasExtMemoryBudget;
    props.m_hasExtMemoryPriority = hasExtMemoryPriority;
    props.m_hasKhrMaintenance4 = hasKhrMaintenance4;
    props.m_hasKhrMaintenance5 = hasKhrMaintenance5;
    props.m_hasRayTracing = hasKhrRayTracing;
    props.m_hasExtDescriptorHeap = hasExtDescriptorHeap;
    props.m_hasKhrPipelineBinary = hasKhrPipelineBinary;
    props.m_accelerationStructureProperties = accelerationStructureProperties;
    props.m_rayTracingPipelineProperties = rayTracingPipelineProperties;
    props.m_enabledVk10Features = enabledVk10Features;
    props.m_enabledVk11Features = enabledVk11Features;
    props.m_enabledVk12Features = enabledVk12Features;
    props.m_enabledVk13Features = enabledVk13Features;
    props.m_enabledVk14Features = enabledVk14Features;
    props.m_enabledExtensions = enabledExtensions;
    props.m_graphicsQueueIndex = graphicsQueueIndex;
    props.m_presentQueueIndex = presentQueueIndex;
    props.m_asyncComputeQueueIndex = asyncComputeQueueIndex;
    props.m_queueFamilies = VulkanQueueFamilySwizzling(graphicsQueueIndex, presentQueueIndex, asyncComputeQueueIndex);
    props.m_antiLagMethod = antiLagMethod;

    return props;
}

auto VulkanPhysicalDeviceProperties::autoselectPriorityScore() const -> u64 {
    u64 priority = 0;
    switch (m_deviceType) {
        // clang-format off
        case vk::PhysicalDeviceType::eDiscreteGpu:   priority = 0b111; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: priority = 0b110; break;
        case vk::PhysicalDeviceType::eVirtualGpu:    priority = 0b101; break;
        case vk::PhysicalDeviceType::eCpu:           priority = 0b100; break;
        case vk::PhysicalDeviceType::eOther:         priority = 0b011; break;
        // clang-format on
    }

    u64 score = (priority << 61) | (m_vramSize >> 3);
    return score;
}

auto VulkanPhysicalDeviceProperties::vmaAllocatorCreateFlags() const -> vma::AllocatorCreateFlags {
    vma::AllocatorCreateFlags flags = vma::AllocatorCreateFlagBits::eBufferDeviceAddress;
    if (m_hasExtMemoryBudget) flags |= vma::AllocatorCreateFlagBits::eExtMemoryBudget;
    if (m_hasExtMemoryPriority) flags |= vma::AllocatorCreateFlagBits::eExtMemoryPriority;
    if (m_hasKhrMaintenance4) flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance4;
    if (m_hasKhrMaintenance5) flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance5;
    return flags;
}

static f32 g_queuePriority = 1.0f;

auto VulkanPhysicalDeviceProperties::queueCreateInfos() const -> std::vector<vk::DeviceQueueCreateInfo> {
    auto infos = m_queueFamilies.allUniqueQueueIndices()
        | std::views::transform([&](u32 index) -> vk::DeviceQueueCreateInfo {
            return vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(index)
                .setQueueCount(1)
                .setQueuePriorities(g_queuePriority);
        })
        | std::ranges::to<std::vector>();
    return infos;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------

auto VulkanPhysicalDeviceSurfaceProperties::query(const vk::raii::PhysicalDevice &vkPhysicalDevice, const vk::raii::SurfaceKHR &vkSurface) -> VulkanPhysicalDeviceSurfaceProperties {
    auto capabilities = vkPhysicalDevice.getSurfaceCapabilitiesKHR(vkSurface);
    auto surfaceFormats = vkPhysicalDevice.getSurfaceFormatsKHR(vkSurface);
    auto presentModes = vkPhysicalDevice.getSurfacePresentModesKHR(vkSurface);

    VulkanPhysicalDeviceSurfaceProperties props;
    props.m_surfaceFormats = surfaceFormats;
    props.m_capabilities = capabilities;
    props.m_presentModes = presentModes;
    return props;
}

} // namespace nekomata2
