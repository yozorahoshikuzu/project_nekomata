module nekomata2;
import std;
import vulkan;
import vk_mem_alloc;
import :graphics.vulkan.vk_physical_device_props;
import :core.overloaded;

using namespace std::literals;

namespace nekomata2 {

using VulkanFeaturePtr = std::variant<
    vk::Bool32 vk::PhysicalDeviceFeatures::*,
    vk::Bool32 vk::PhysicalDeviceVulkan11Features::*,
    vk::Bool32 vk::PhysicalDeviceVulkan12Features::*,
    vk::Bool32 vk::PhysicalDeviceVulkan13Features::*,
    vk::Bool32 vk::PhysicalDeviceVulkan14Features::*,
    vk::Bool32 vk::PhysicalDeviceDescriptorHeapFeaturesEXT::*,
    vk::Bool32 vk::PhysicalDevicePipelineBinaryFeaturesKHR::*,
    vk::Bool32 vk::PhysicalDeviceAntiLagFeaturesAMD::*,
    vk::Bool32 vk::PhysicalDeviceRayTracingPipelineFeaturesKHR::*,
    vk::Bool32 vk::PhysicalDeviceAccelerationStructureFeaturesKHR::*
>;

struct RequiredFeatureRule {
    std::string_view m_name;
    vk::Bool32 vk::PhysicalDeviceFeatures::* m_vk10 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan11Features::* m_vk11 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan12Features::* m_vk12 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan13Features::* m_vk13 = nullptr;
    vk::Bool32 vk::PhysicalDeviceVulkan14Features::* m_vk14 = nullptr;
    PhysicalDevicePropertyQueryErrorKind m_errorKindIfMissing;
};

struct RequiredExtensionRule {
    std::string_view m_extensionName;
    PhysicalDevicePropertyQueryErrorKind m_errorKindIfMissing;
};

struct OptFeatureRule {
    std::string_view m_name;
    std::span<const std::string_view> m_requiredExtensionNames;
    std::span<const VulkanFeaturePtr> m_requiredFeatures;
    bool VulkanPhysicalDeviceProperties::* m_setsIfSupported;
};

// TODO: move somewhere else
template <typename T> constexpr auto emptyArray() { return std::array<T, 0>{}; }

// clang-format off

// ---- Required Device Extensions and Features ----------------------------------------------------------------------------------------------------------------

static constexpr auto kRequiredPhysicalDeviceExtensions = std::to_array<RequiredExtensionRule>({
    { vk::KHRSwapchainExtensionName, PhysicalDevicePropertyQueryErrorKind::MissingKhrSwapchain },
    { vk::EXTImageViewMinLodExtensionName, PhysicalDevicePropertyQueryErrorKind::MissingExtImageViewMinLod },
});

static constexpr auto kRequiredPhysicalDeviceFeatures = std::to_array<RequiredFeatureRule>({
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
});

// ---- Optional Features --------------------------------------------------------------------------------------------------------------------------------------

// for ExtMemoryBudget
static constexpr auto kOptExtensionsExtMemoryBudget = std::to_array<std::string_view>({ vk::EXTMemoryBudgetExtensionName });
static constexpr auto kOptFeaturesExtMemoryBudget = emptyArray<VulkanFeaturePtr>();

// for ExtMemoryPriority
static constexpr auto kOptExtensionsExtMemoryPriority = std::to_array<std::string_view>({ vk::EXTMemoryPriorityExtensionName });
static constexpr auto kOptFeaturesExtMemoryPriority = emptyArray<VulkanFeaturePtr>();

// for RayTracing
static constexpr auto kOptExtensionsRayTracing = std::to_array<std::string_view>({ vk::KHRAccelerationStructureExtensionName, vk::KHRRayTracingPipelineExtensionName, vk::KHRDeferredHostOperationsExtensionName });
static constexpr auto kOptFeaturesRayTracing = std::to_array<VulkanFeaturePtr>({ &vk::PhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure, &vk::PhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline });

// for KhrMaintenance4
static constexpr auto kOptExtensionsKhrMaintenance4 = emptyArray<std::string_view>();
static constexpr auto kOptFeaturesKhrMaintenance4 = std::to_array<VulkanFeaturePtr>({ &vk::PhysicalDeviceVulkan13Features::maintenance4 });

// for ExtDescriptorHeap
static constexpr auto kOptExtensionsExtDescriptorHeap = std::to_array<std::string_view>({ vk::EXTDescriptorHeapExtensionName });
static constexpr auto kOptFeaturesExtDescriptorHeap = std::to_array<VulkanFeaturePtr>({ &vk::PhysicalDeviceDescriptorHeapFeaturesEXT::descriptorHeap });

// for KhrPipelineBinary
static constexpr auto kOptExtensionsKhrPipelineBinary = std::to_array<std::string_view>({ vk::KHRPipelineBinaryExtensionName });
static constexpr auto kOptFeaturesKhrPipelineBinary = std::to_array<VulkanFeaturePtr>({ &vk::PhysicalDevicePipelineBinaryFeaturesKHR::pipelineBinaries });

// for AMDAntiLag2
static constexpr auto kOptExtensionsAMDAntiLag2 = std::to_array<std::string_view>({ vk::AMDAntiLagExtensionName });
static constexpr auto kOptFeaturesAMDAntiLag2 = std::to_array<VulkanFeaturePtr>({ &vk::PhysicalDeviceAntiLagFeaturesAMD::antiLag });

// Table
static constexpr auto kOptionalPhysicalDeviceFeatures = std::to_array<OptFeatureRule>({
    { "Memory Budget Tracking"sv,                            kOptExtensionsExtMemoryBudget, kOptFeaturesExtMemoryBudget, &VulkanPhysicalDeviceProperties::m_hasExtMemoryBudget },
    { "Memory Priority Tagging"sv,                           kOptExtensionsExtMemoryPriority, kOptFeaturesExtMemoryPriority, &VulkanPhysicalDeviceProperties::m_hasExtMemoryPriority },
    { "Ray Tracing"sv,                                       kOptExtensionsRayTracing, kOptFeaturesRayTracing, &VulkanPhysicalDeviceProperties::m_hasRayTracing },
    { "maintenance4"sv,                                      kOptExtensionsKhrMaintenance4, kOptFeaturesKhrMaintenance4, &VulkanPhysicalDeviceProperties::m_hasKhrMaintenance4 },
    { "Descriptor Heap"sv,                                   kOptExtensionsExtDescriptorHeap, kOptFeaturesExtDescriptorHeap, &VulkanPhysicalDeviceProperties::m_hasExtDescriptorHeap },
    { "Pipeline Binaries"sv,                                 kOptExtensionsKhrPipelineBinary, kOptFeaturesKhrPipelineBinary, &VulkanPhysicalDeviceProperties::m_hasKhrPipelineBinary },
    { "AMD Anti-Lag 2"sv,                                    kOptExtensionsAMDAntiLag2, kOptFeaturesAMDAntiLag2, &VulkanPhysicalDeviceProperties::m_hasAMDAntiLag2 },
});
// clang-format on

consteval auto defaultEnabledVk10Features() -> vk::PhysicalDeviceFeatures {
    auto features = vk::PhysicalDeviceFeatures{};
    for (auto& rule : kRequiredPhysicalDeviceFeatures) {
        if (rule.m_vk10)
            features.*(rule.m_vk10) = true;
    }
    return features;
}

consteval auto defaultEnabledVk11Features() -> vk::PhysicalDeviceVulkan11Features {
    auto features = vk::PhysicalDeviceVulkan11Features{};
    for (auto& rule : kRequiredPhysicalDeviceFeatures) {
        if (rule.m_vk11)
            features.*(rule.m_vk11) = true;
    }
    return features;
}

consteval auto defaultEnabledVk12Features() -> vk::PhysicalDeviceVulkan12Features {
    auto features = vk::PhysicalDeviceVulkan12Features{};
    for (auto& rule : kRequiredPhysicalDeviceFeatures) {
        if (rule.m_vk12)
            features.*(rule.m_vk12) = true;
    }
    return features;
}

consteval auto defaultEnabledVk13Features() -> vk::PhysicalDeviceVulkan13Features {
    auto features = vk::PhysicalDeviceVulkan13Features{};
    for (auto& rule : kRequiredPhysicalDeviceFeatures) {
        if (rule.m_vk13)
            features.*(rule.m_vk13) = true;
    }
    return features;
}

consteval auto defaultEnabledVk14Features() -> vk::PhysicalDeviceVulkan14Features {
    auto features = vk::PhysicalDeviceVulkan14Features{};
    for (auto& rule : kRequiredPhysicalDeviceFeatures) {
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
    auto supportedExtensionProperties = Vec<vk::ExtensionProperties>::fromStdVector(vkCheckResult(vkPhysicalDevice.enumerateDeviceExtensionProperties()));
    auto supportedExtensionNames = supportedExtensionProperties.iter()
        .map([](auto&& ext) { return std::string(ext.extensionName); })
        .inspect([](const auto& x) { log::info("Supported extension: {}", x); })
        .collect<Vec>();

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------
    // Check the physical device for support of required extensions

    for (auto& rule : kRequiredPhysicalDeviceExtensions) {
        if (!supportedExtensionNames.contains(rule.m_extensionName)) {
            return std::unexpected(PhysicalDevicePropertyQueryError{.m_kind = rule.m_errorKindIfMissing});
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------
    // Check the physical device for support of required features

    auto featuresQuery = vkPhysicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceVulkan14Features,
        vk::PhysicalDeviceDescriptorHeapFeaturesEXT, vk::PhysicalDevicePipelineBinaryFeaturesKHR,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR, vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        vk::PhysicalDeviceImageViewMinLodFeaturesEXT, vk::PhysicalDeviceAntiLagFeaturesAMD
    >();

    for (auto& rule : kRequiredPhysicalDeviceFeatures) {
        bool satisfied = true;
        if (rule.m_vk10)
            satisfied &= featuresQuery.get<vk::PhysicalDeviceFeatures2>().features.*(rule.m_vk10);
        if (rule.m_vk11)
            satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan11Features>().*(rule.m_vk11);
        if (rule.m_vk12)
            satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan12Features>().*(rule.m_vk12);
        if (rule.m_vk13)
            satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan13Features>().*(rule.m_vk13);
        if (rule.m_vk14)
            satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan14Features>().*(rule.m_vk14);

        if (!satisfied)
            return std::unexpected(PhysicalDevicePropertyQueryError{.m_kind = rule.m_errorKindIfMissing});
    }

    // TODO: refactor
    if (!featuresQuery.get<vk::PhysicalDeviceImageViewMinLodFeaturesEXT>().minLod) {
        return std::unexpected(PhysicalDevicePropertyQueryError{.m_kind = PhysicalDevicePropertyQueryErrorKind::MissingExtImageViewMinLod});
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------
    // Check for availability of optional features

    auto props = VulkanPhysicalDeviceProperties{};
    auto enabledExtensions = Vec<std::string>::create();
    for (auto& rule : kRequiredPhysicalDeviceExtensions) { enabledExtensions.emplace(rule.m_extensionName); }
    auto enabledVk10Features = defaultEnabledVk10Features();
    auto enabledVk11Features = defaultEnabledVk11Features();
    auto enabledVk12Features = defaultEnabledVk12Features();
    auto enabledVk13Features = defaultEnabledVk13Features();
    auto enabledVk14Features = defaultEnabledVk14Features();

    for (auto& rule : kOptionalPhysicalDeviceFeatures) {
        bool satisfied = true;

        for (auto& reqdExtensionName : rule.m_requiredExtensionNames) {
            satisfied &= supportedExtensionNames.contains(reqdExtensionName);
        }

        for (auto& reqdFeature : rule.m_requiredFeatures) {
            match(reqdFeature,
                [&](vk::Bool32 vk::PhysicalDeviceFeatures::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceFeatures2>().features.*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceVulkan11Features::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan11Features>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceVulkan12Features::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan12Features>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceVulkan13Features::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan13Features>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceVulkan14Features::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceVulkan14Features>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceDescriptorHeapFeaturesEXT::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceDescriptorHeapFeaturesEXT>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDevicePipelineBinaryFeaturesKHR::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDevicePipelineBinaryFeaturesKHR>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceAntiLagFeaturesAMD::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceAntiLagFeaturesAMD>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceRayTracingPipelineFeaturesKHR::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>().*(ptr); },
                [&](vk::Bool32 vk::PhysicalDeviceAccelerationStructureFeaturesKHR::* ptr) { satisfied &= featuresQuery.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>().*(ptr); }
            );
        }

        props.*(rule.m_setsIfSupported) = satisfied;
        if (satisfied) {
            for (auto& extName : rule.m_requiredExtensionNames) {
                enabledExtensions.emplace(extName);
            }
        }
    }

    auto propertiesQuery = vkPhysicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceVulkan12Properties,
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    auto coreProperties = propertiesQuery.get<vk::PhysicalDeviceProperties2>().properties;
    auto accelerationStructureProperties = propertiesQuery.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    auto rayTracingPipelineProperties = propertiesQuery.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    auto deviceName = std::string(coreProperties.deviceName);
    auto driverName = std::string(propertiesQuery.get<vk::PhysicalDeviceVulkan12Properties>().driverName);
    auto deviceType = coreProperties.deviceType;

    auto memoryProperties = vkPhysicalDevice.getMemoryProperties();
    u64 vramSize = 0;
    u32 vramHeapIndex = 0;
    for (usize i = 0; i < memoryProperties.memoryHeapCount; i++) {
        auto& heap = memoryProperties.memoryHeaps[i];
        if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
            vramSize += heap.size;
            vramHeapIndex = i;
        }
    }

    auto queueProps = vkPhysicalDevice.getQueueFamilyProperties();
    std::vector<u32> graphicsIndices, presentIndices, asyncComputeIndices;
    
    for (u32 queueIndex = 0; queueIndex < queueProps.size(); queueIndex++) {
        vk::QueueFamilyProperties& family = queueProps[queueIndex];

        if (family.queueFlags & vk::QueueFlagBits::eGraphics) graphicsIndices.emplace_back(queueIndex);
        if (family.queueFlags & vk::QueueFlagBits::eCompute && !(family.queueFlags & vk::QueueFlagBits::eGraphics)) asyncComputeIndices.emplace_back(queueIndex);
        if (vkCheckResult(vkPhysicalDevice.getSurfaceSupportKHR(queueIndex, vkSurface))) presentIndices.emplace_back(queueIndex);
    }

    std::unordered_set<u32> usedQueueIndices;
    u32 graphicsQueueIndex = findBestQueue(graphicsIndices, usedQueueIndices);
    u32 presentQueueIndex = findBestQueue(presentIndices, usedQueueIndices);
    u32 asyncComputeQueueIndex = findBestQueue(asyncComputeIndices, usedQueueIndices);
    std::vector<u32> allQueueIndices = { graphicsQueueIndex, presentQueueIndex, asyncComputeQueueIndex };
    dedupQueueIndices(allQueueIndices);
    std::vector<u32> swapchainImageQueueIndices = { graphicsQueueIndex, presentQueueIndex };
    dedupQueueIndices(swapchainImageQueueIndices);

    props.m_deviceName = deviceName;
    props.m_driverName = driverName;
    props.m_deviceType = deviceType;
    props.m_driverId = propertiesQuery.get<vk::PhysicalDeviceVulkan12Properties>().driverID;
    props.m_driverVersion = coreProperties.driverVersion;
    props.m_apiVersion = coreProperties.apiVersion;
    props.m_vramSize = vramSize;
    props.m_vramMemoryHeapIndex = vramHeapIndex;
    props.m_accelerationStructureProperties = accelerationStructureProperties;
    props.m_rayTracingPipelineProperties = rayTracingPipelineProperties;
    props.m_enabledVk10Features = enabledVk10Features;
    props.m_enabledVk11Features = enabledVk11Features;
    props.m_enabledVk12Features = enabledVk12Features;
    props.m_enabledVk13Features = enabledVk13Features;
    props.m_enabledVk14Features = enabledVk14Features;
    props.m_enabledExtensions = std::move(enabledExtensions);
    props.m_graphicsQueueIndex = graphicsQueueIndex;
    props.m_presentQueueIndex = presentQueueIndex;
    props.m_asyncComputeQueueIndex = asyncComputeQueueIndex;
    props.m_queueFamilies = VulkanQueueFamilySwizzling(graphicsQueueIndex, presentQueueIndex, asyncComputeQueueIndex);

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
    flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance5;
    if (m_hasExtMemoryBudget) flags |= vma::AllocatorCreateFlagBits::eExtMemoryBudget;
    if (m_hasExtMemoryPriority) flags |= vma::AllocatorCreateFlagBits::eExtMemoryPriority;
    if (m_hasKhrMaintenance4) flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance4;

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


auto VulkanPhysicalDeviceProperties::printInfo() const -> void {
    log::info("  Device Information:");
    log::info("    Name: {}", m_deviceName);
    log::info("    Type: {}", vk::to_string(m_deviceType));
    log::info("    Driver ID: {}", vk::to_string(m_driverId));
    log::info("    VRAM heap size: {} MiB", m_vramSize / 1024 / 1024);
    log::info("  Device Features:");
    for (auto& rule : kOptionalPhysicalDeviceFeatures) {
        log::info("    {}: {}", rule.m_name, this->*rule.m_setsIfSupported ? "Yes" : "No");
    }

    auto asyncComputeQueueContingency = m_asyncComputeQueueIndex == m_graphicsQueueIndex ? "aliases graphics queue"sv : "dedicated"sv;
    auto presentQueueContingency = m_presentQueueIndex == m_graphicsQueueIndex ? "aliases graphics queue"sv
        : m_presentQueueIndex == m_asyncComputeQueueIndex ? "aliases async compute queue"sv : "dedicated"sv;

    log::info("  Device Queue Family Indices:");
    log::info("    Graphics queue: {}", m_graphicsQueueIndex);
    log::info("    Async compute queue: {} ({})", m_asyncComputeQueueIndex, asyncComputeQueueContingency);
    log::info("    Present queue: {} ({})", m_presentQueueIndex, presentQueueContingency);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------

auto VulkanPhysicalDeviceSurfaceProperties::query(const vk::raii::PhysicalDevice &vkPhysicalDevice, const vk::raii::SurfaceKHR &vkSurface) -> VulkanPhysicalDeviceSurfaceProperties {
    auto capabilities = vkCheckResult(vkPhysicalDevice.getSurfaceCapabilitiesKHR(vkSurface));
    auto surfaceFormats = vkCheckResult(vkPhysicalDevice.getSurfaceFormatsKHR(vkSurface));
    auto presentModes = vkCheckResult(vkPhysicalDevice.getSurfacePresentModesKHR(vkSurface));

    VulkanPhysicalDeviceSurfaceProperties props;
    props.m_surfaceFormats = Vec<vk::SurfaceFormatKHR>::fromStdVector(std::move(surfaceFormats));
    props.m_capabilities = capabilities;
    props.m_presentModes = Vec<vk::PresentModeKHR>::fromStdVector(std::move(presentModes));
    return props;
}

} // namespace nekomata2
