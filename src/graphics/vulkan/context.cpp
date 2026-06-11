module nekomata2;
import :core.log;
import :graphics.vulkan.deletion_queue;
import :graphics.vulkan.vk_queue;
import :core.platform.assert;
import :graphics.vulkan.shadercache;

namespace nekomata2 {

VulkanContext::VulkanContext(std::nullptr_t) {}
VulkanContext::~VulkanContext() {
    if (m_vkDevice != nullptr) {
        m_vkDevice.waitIdle();
        m_vkGraphicsQueue->waitIdle();
        m_vkAsyncComputeQueue->waitIdle();
        m_vkPresentQueue->waitIdle();
    }
    log::info("destroying vulkan context..");
}

auto VulkanContext::get() -> VulkanContext& {
    return *g_vkContext;
}

auto VulkanContext::create(nekomata2::SdlWindow& sdlWindow) -> std::unique_ptr<VulkanContext> {
    debug_assert(g_vkContext == nullptr, "only one VulkanContext may live at any given time");
    auto vkContext = std::make_unique<VulkanContext>(nullptr);
    g_vkContext = vkContext.get();

    // We're dynamically loading Vulkan, and this requires extra work with the vk::detail::DynamicLoader and to initialize the vulkan.hpp dispatcher.
    vkContext->m_vkDynamicLoader = vk::detail::DynamicLoader{};
    auto vkGetInstanceProcAddr = vkContext->m_vkDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    vk::detail::defaultDispatchLoaderDynamic.init(vkGetInstanceProcAddr);

    vkContext->m_vkRaiiContext = initVkRaiiContext();
    vkContext->m_vkInstance = createVkInstance(vkContext->m_vkRaiiContext, false, sdlWindow);

    // To access vkInstance* functions, the vulkan.hpp dispatcher must be made aware of the instance.
    vk::detail::defaultDispatchLoaderDynamic.init(*vkContext->m_vkInstance);

    vkContext->m_vkSurface = createVkSurface(vkContext->m_vkInstance, sdlWindow);
    auto [vk_physical_device, vk_physical_device_props] = pickVkPhysicalDevice(vkContext->m_vkInstance, vkContext->m_vkSurface);
    vkContext->m_vkPhysicalDevice = vk_physical_device;
    vkContext->m_vkDevice = createVkDevice(vk_physical_device, vk_physical_device_props);
    vkContext->m_vkPhysicalDeviceProperties = vk_physical_device_props;

    // To access vkDevice* functions, the vulkan.hpp dispatcher must be made aware of the device.
    vk::detail::defaultDispatchLoaderDynamic.init(*vkContext->m_vkDevice);

    if (vkContext->m_vkPhysicalDeviceProperties.m_hasExtDescriptorHeap) {
        log::warn("VK_EXT_descriptor_heap support is very experimental and not implemented everywhere!");
    }

    std::unordered_map<u32, u32> queueFamilyIndexToQueueSlot;

    for (auto [i, queue_index] : vk_physical_device_props.m_queueFamilies.allUniqueQueueIndices() | std::views::enumerate) {
        auto vkQueue = VulkanContext::get().vkDevice().getQueue(queue_index, 0);
        u64 lastTimelineSubmissionValue = 0;

        auto semaphoreTypeInfo = vk::SemaphoreTypeCreateInfo{}
            .setSemaphoreType(vk::SemaphoreType::eTimeline)
            .setInitialValue(lastTimelineSubmissionValue);
        
        auto chain = vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo>{
            vk::SemaphoreCreateInfo{},
            semaphoreTypeInfo
        };

        auto handle = VulkanContext::get().vkDevice().createSemaphore(chain.get<vk::SemaphoreCreateInfo>());
        auto queue = std::make_unique<VulkanQueue>(std::move(vkQueue), std::move(handle), lastTimelineSubmissionValue);

        vkContext->m_vkQueues.emplace_back(std::move(queue));
        queueFamilyIndexToQueueSlot[queue_index] = i;
    }

    vkContext->m_vkGraphicsQueue = vkContext->m_vkQueues[queueFamilyIndexToQueueSlot[vk_physical_device_props.m_graphicsQueueIndex]].get();
    vkContext->m_vkPresentQueue = vkContext->m_vkQueues[queueFamilyIndexToQueueSlot[vk_physical_device_props.m_presentQueueIndex]].get();
    vkContext->m_vkAsyncComputeQueue = vkContext->m_vkQueues[queueFamilyIndexToQueueSlot[vk_physical_device_props.m_asyncComputeQueueIndex]].get();

    vkContext->m_vmaAllocator = createVmaAllocator(vkContext->m_vkInstance, vkContext->m_vkPhysicalDevice, vk_physical_device_props, vkContext->m_vkDevice);

    vkContext->m_vkResourceDeletionQueue = std::make_unique<VulkanResourceDeletionQueue>();
    vkContext->m_vkResourceDeletionQueue->run();
    g_vkResourceDeletionQueue = vkContext->m_vkResourceDeletionQueue.get();
    vkContext->m_shaderCache = std::make_unique<ShaderCache>();

    return vkContext;
}

auto VulkanContext::antiLagPaceInput(u64 frameIndex, u32 targetFps) -> void {
    switch (m_vkPhysicalDeviceProperties.m_antiLagMethod) {
    case PhysicalDeviceAntiLagMethod::None: return;
    case PhysicalDeviceAntiLagMethod::AMDAntiLag2:
        auto antilagPresentInfo = vk::AntiLagPresentationInfoAMD{}
            .setFrameIndex(frameIndex)
            .setStage(vk::AntiLagStageAMD::eInput);

        auto antilagData = vk::AntiLagDataAMD{}
            .setMode(m_antilagEnable ? vk::AntiLagModeAMD::eOn : vk::AntiLagModeAMD::eOff)
            .setMaxFPS(targetFps)
            .setPPresentationInfo(&antilagPresentInfo);

        m_vkDevice.antiLagUpdateAMD(antilagData);
        break;
    }
}

auto VulkanContext::antiLagPacePresent(u64 frameIndex, u32 targetFps) -> void {
    switch (m_vkPhysicalDeviceProperties.m_antiLagMethod) {
    case PhysicalDeviceAntiLagMethod::None: return;
    case PhysicalDeviceAntiLagMethod::AMDAntiLag2:
        auto antilagPresentInfo = vk::AntiLagPresentationInfoAMD{}
            .setFrameIndex(frameIndex)
            .setStage(vk::AntiLagStageAMD::ePresent);

        auto antilagData = vk::AntiLagDataAMD{}
            .setMode(m_antilagEnable ? vk::AntiLagModeAMD::eOn : vk::AntiLagModeAMD::eOff)
            .setMaxFPS(targetFps)
            .setPPresentationInfo(&antilagPresentInfo);

        m_vkDevice.antiLagUpdateAMD(antilagData);
        break;
    }
}

auto VulkanContext::initVkRaiiContext() -> vk::raii::Context {
    return {};
}

auto VulkanContext::createVkInstance(vk::raii::Context& vkRaiiContext, bool debugEnable, nekomata2::SdlWindow& sdlWindow) -> vk::raii::Instance {
    auto appInfo = vk::ApplicationInfo{}
        .setPApplicationName("project_nekomata")
        .setPEngineName("project_nekomata")
        .setApplicationVersion(vk::makeApiVersion(0, 0, 1, 0))
        .setEngineVersion(vk::makeApiVersion(0, 0, 1, 0))
        .setApiVersion(vk::ApiVersion14);
    
    // TODO: Extension and layer availability check
    auto availableInstanceLayers = vk::enumerateInstanceLayerProperties()
        | std::views::transform([](const auto& cstr) { return std::string(cstr.layerName); })
        | std::ranges::to<std::vector>();
    auto instanceExtensions = nekomata2::SdlWindow::vulkanInstanceExtensions();
    
    auto instanceExtensionsC = instanceExtensions
        | std::views::transform(&std::string::c_str)
        | std::ranges::to<std::vector>();
    
    std::vector<const char*> instanceLayersC;

    if (debugEnable)
        instanceLayersC.emplace_back("VK_LAYER_KHRONOS_validation");

    if (std::ranges::contains(availableInstanceLayers, "VK_LAYER_KORTHOS_low_latency"))
        instanceLayersC.emplace_back("VK_LAYER_KORTHOS_low_latency");

    auto instanceInfo = vk::InstanceCreateInfo{}
        .setPApplicationInfo(&appInfo)
        .setPEnabledExtensionNames(instanceExtensionsC)
        .setPEnabledLayerNames(instanceLayersC);
    
    auto instance = vk::raii::Instance(vkRaiiContext, instanceInfo);

    return instance;
}

auto VulkanContext::createVkSurface(const vk::raii::Instance& vkInstance, nekomata2::SdlWindow& sdlWindow) -> vk::raii::SurfaceKHR {
    auto surface = sdlWindow.vulkanCreateRawSurface(*vkInstance);

    return vk::raii::SurfaceKHR(vkInstance, surface);
}

auto VulkanContext::pickVkPhysicalDevice(const vk::raii::Instance& vkInstance, const vk::raii::SurfaceKHR& vkSurface) -> std::tuple<vk::raii::PhysicalDevice, VulkanPhysicalDeviceProperties> {
    auto physicalDevices = vkInstance.enumeratePhysicalDevices();

    if (physicalDevices.empty()) throw std::logic_error("no vulkan physical devices");

    auto props = physicalDevices
        | std::views::transform([&](const vk::raii::PhysicalDevice& vkPhysicalDevice) {
            return VulkanPhysicalDeviceProperties::query(vkPhysicalDevice, vkSurface);
        })
        | std::views::enumerate
        | std::ranges::to<std::vector>();

    if (std::ranges::all_of(props, [](const auto& val) { return !std::get<1>(val).has_value(); })) {
        log::crit("No GPUs shown by loader are supported!");
        throw std::logic_error("no GPUs shown by loader are supported");
    }

    std::vector<u64> scores(props.size(), 0);

    for (auto& [i, prop] : props) {
        if (!prop.has_value()) {
            log::warn("GPU #{} is not supported", i);
            continue;
        }
        scores[i] = prop->autoselectPriorityScore();
        
        // TODO: Move somewhere else
        log::info("GPU #{}: {}", i, prop->m_deviceName);
        log::info("  Driver ID: {}", vk::to_string(prop->m_driverId));
        log::info("  VRAM heap size: {} bytes ({} MiB)", prop->m_vramSize, prop->m_vramSize / 1024 / 1024);
        log::info("  has_ext_memory_budget: {}", prop->m_hasExtMemoryBudget);
        log::info("  has_ext_memory_priority: {}", prop->m_hasExtMemoryPriority);
        log::info("  has_khr_maintenance4: {}", prop->m_hasKhrMaintenance4);
        log::info("  has_khr_maintenance5: {}", prop->m_hasKhrMaintenance5);
        log::info("  has_ray_tracing: {}", prop->m_hasRayTracing);
        log::info("  has_ext_descriptor_heap: {}", prop->m_hasExtDescriptorHeap);
        log::info("  has_khr_pipeline_binary: {}", prop->m_hasKhrPipelineBinary);
        switch (prop->m_antiLagMethod) {
        case PhysicalDeviceAntiLagMethod::AMDAntiLag2:
            log::info("  anti_lag_method: AMD Anti-Lag 2");
            break;
        case PhysicalDeviceAntiLagMethod::None:
            log::info("  anti_lag_method: None");
            break;
        }
        if (prop->m_hasRayTracing) {
            log::info("    Max AS bound per shader stage: {}", prop->m_accelerationStructureProperties.maxPerStageDescriptorAccelerationStructures);
            log::info("    AS min scratch buffer offset alignment: {} bytes", prop->m_accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment);
            log::info("    AS max geometry count: {}", prop->m_accelerationStructureProperties.maxGeometryCount);
            log::info("    AS max instance count: {}", prop->m_accelerationStructureProperties.maxInstanceCount);
            log::info("    AS max primitive count: {}", prop->m_accelerationStructureProperties.maxPrimitiveCount);
            log::info("    RT pipeline shader group handle size: {} bytes", prop->m_rayTracingPipelineProperties.shaderGroupHandleSize);
            log::info("    RT pipeline shader group handle alignment: {} bytes", prop->m_rayTracingPipelineProperties.shaderGroupHandleAlignment);
            log::info("    RT pipeline shader group base alignment: {} bytes", prop->m_rayTracingPipelineProperties.shaderGroupBaseAlignment);
            log::info("    RT pipeline max shader group stride: {} bytes", prop->m_rayTracingPipelineProperties.maxShaderGroupStride);
            log::info("    RT pipeline shader max ray hit attribute size: {} bytes", prop->m_rayTracingPipelineProperties.maxRayHitAttributeSize);
            log::info("    RT pipeline shader max ray recursion depth: {}", prop->m_rayTracingPipelineProperties.maxRayRecursionDepth);
        }
        log::info("  Graphics queue index: {}", prop->m_graphicsQueueIndex);
        log::info("  Present queue index: {}", prop->m_presentQueueIndex);
        log::info("  Async compute queue index: {}", prop->m_asyncComputeQueueIndex);
        log::info("  Autoselect priority score: {:016x}", scores[i]);
    }

    auto scoreMaxIndex = std::distance(scores.begin(), std::ranges::max_element(scores));
    log::info("Picked GPU #{}", scoreMaxIndex);
    auto [_, prop] = props[scoreMaxIndex];
    return { physicalDevices[scoreMaxIndex], *prop };
}

auto VulkanContext::createVkDevice(const vk::raii::PhysicalDevice& vkPhysicalDevice, const VulkanPhysicalDeviceProperties& vkPhysicalDeviceProps) -> vk::raii::Device {
    auto deviceExtensionsC = vkPhysicalDeviceProps.m_enabledExtensions
        | std::views::transform(&std::string::c_str)
        | std::ranges::to<std::vector>();

    log::info("Creating device..");
    for (auto& ext : vkPhysicalDeviceProps.m_enabledExtensions) {
        log::info("  Enabling device extension: {}", ext);
    }
    
    auto queueCreateInfos = vkPhysicalDeviceProps.queueCreateInfos();

    auto deviceCreateInfo = vk::DeviceCreateInfo{}
        .setPEnabledExtensionNames(deviceExtensionsC)
        .setQueueCreateInfos(queueCreateInfos)
        .setPEnabledFeatures(&vkPhysicalDeviceProps.m_enabledVk10Features);

    auto chain = vk::StructureChain{
        deviceCreateInfo,
        vkPhysicalDeviceProps.m_enabledVk11Features,
        vkPhysicalDeviceProps.m_enabledVk12Features,
        vkPhysicalDeviceProps.m_enabledVk13Features,
        vkPhysicalDeviceProps.m_enabledVk14Features,
        vk::PhysicalDeviceImageViewMinLodFeaturesEXT{}.setMinLod(true),
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR{}.setAccelerationStructure(true),
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{}.setRayTracingPipeline(true),
        vk::PhysicalDeviceAntiLagFeaturesAMD{}.setAntiLag(true),
    };

    if (!vkPhysicalDeviceProps.m_hasRayTracing) {
        chain.unlink<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
        chain.unlink<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
    }

    if (vkPhysicalDeviceProps.m_antiLagMethod != PhysicalDeviceAntiLagMethod::AMDAntiLag2) {
        chain.unlink<vk::PhysicalDeviceAntiLagFeaturesAMD>();
    }

    auto device = vkPhysicalDevice.createDevice(chain.get<vk::DeviceCreateInfo>());
    return device;
}

auto VulkanContext::createVmaAllocator(const vk::raii::Instance& vkInstance, const vk::raii::PhysicalDevice& vkPhysicalDevice, const VulkanPhysicalDeviceProperties& vkPhysicalDeviceProps, const vk::raii::Device& vkDevice) -> vma::raii::Allocator {
    // The VMA HPP bindings load the Vulkan functions from the instance and device dispatchers automatically.
    auto allocatorCreateInfo = vma::AllocatorCreateInfo{}
        .setPhysicalDevice(vkPhysicalDevice)
        .setVulkanApiVersion(vk::ApiVersion14)
        .setFlags(vkPhysicalDeviceProps.vmaAllocatorCreateFlags());

    return vma::raii::createAllocator(vkInstance, vkDevice, allocatorCreateInfo);
}

}