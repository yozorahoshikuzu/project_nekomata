module projnekomata;
import :core.log;
import :graphics.vulkan.deletion_queue;
import :graphics.vulkan.vk_queue;
import :core.platform.assert;
import :graphics.vulkan.shadercache;

namespace projnekomata {

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

auto VulkanContext::create(projnekomata::SdlWindow& sdlWindow) -> std::unique_ptr<VulkanContext> {
    debug_assert(g_vkContext == nullptr, "only one VulkanContext may live at any given time");
    auto vkContext = std::make_unique<VulkanContext>(nullptr);
    g_vkContext = vkContext.get();

    // We're dynamically loading Vulkan, and this requires extra work with the vk::detail::DynamicLoader and to initialize the vulkan.hpp dispatcher.
    vkContext->m_vkDynamicLoader = vk::detail::DynamicLoader{};
    auto vkGetInstanceProcAddr = vkContext->m_vkDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    vk::detail::defaultDispatchLoaderDynamic.init(vkGetInstanceProcAddr);

    vkContext->m_vkRaiiContext = initVkRaiiContext();
    vkContext->m_vkInstance = createVkInstance(vkContext->m_vkRaiiContext, false);

    // To access vkInstance* functions, the vulkan.hpp dispatcher must be made aware of the instance.
    vk::detail::defaultDispatchLoaderDynamic.init(*vkContext->m_vkInstance);

    vkContext->m_vkSurface = createVkSurface(vkContext->m_vkInstance, sdlWindow);
    auto [vk_physical_device, vk_physical_device_props] = pickVkPhysicalDevice(vkContext->m_vkInstance, vkContext->m_vkSurface);
    vkContext->m_vkPhysicalDevice = vk_physical_device;
    vkContext->m_vkPhysicalDeviceProperties = std::move(vk_physical_device_props);
    vkContext->m_vkDevice = createVkDevice(vk_physical_device, vkContext->m_vkPhysicalDeviceProperties);

    // To access vkDevice* functions, the vulkan.hpp dispatcher must be made aware of the device.
    vk::detail::defaultDispatchLoaderDynamic.init(*vkContext->m_vkDevice);

    if (vkContext->m_vkPhysicalDeviceProperties.m_hasExtDescriptorHeap) {
        log::warn("VK_EXT_descriptor_heap support is very experimental and not implemented everywhere!");
    }

    auto queueFamilyIndexToQueueSlot = HashMap<u32, usize>::create();

    for (auto [i, queue_index] : vkContext->m_vkPhysicalDeviceProperties.m_queueFamilies.allUniqueQueueIndices() | std::views::enumerate) {
        auto vkQueue = VulkanContext::get().vkDevice().getQueue(queue_index, 0);
        u64 lastTimelineSubmissionValue = 0;

        auto semaphoreTypeInfo = vk::SemaphoreTypeCreateInfo{}
            .setSemaphoreType(vk::SemaphoreType::eTimeline)
            .setInitialValue(lastTimelineSubmissionValue);
        
        auto chain = vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo>{
            vk::SemaphoreCreateInfo{},
            semaphoreTypeInfo
        };

        auto handle = vkCheckResult(VulkanContext::get().vkDevice().createSemaphore(chain.get<vk::SemaphoreCreateInfo>()));
        auto queue = std::make_unique<VulkanQueue>(std::move(vkQueue), std::move(handle), lastTimelineSubmissionValue);

        vkContext->m_vkQueues.emplace_back(std::move(queue));
        queueFamilyIndexToQueueSlot.insert(queue_index, i);
    }

    vkContext->m_vkGraphicsQueue = vkContext->m_vkQueues[queueFamilyIndexToQueueSlot[vkContext->m_vkPhysicalDeviceProperties.m_graphicsQueueIndex]].get();
    vkContext->m_vkPresentQueue = vkContext->m_vkQueues[queueFamilyIndexToQueueSlot[vkContext->m_vkPhysicalDeviceProperties.m_presentQueueIndex]].get();
    vkContext->m_vkAsyncComputeQueue = vkContext->m_vkQueues[queueFamilyIndexToQueueSlot[vkContext->m_vkPhysicalDeviceProperties.m_asyncComputeQueueIndex]].get();

    vkContext->m_vmaAllocator = createVmaAllocator(vkContext->m_vkInstance, vkContext->m_vkPhysicalDevice, vkContext->m_vkPhysicalDeviceProperties, vkContext->m_vkDevice);

    vkContext->m_vkResourceDeletionQueue = std::make_unique<VulkanResourceDeletionQueue>();
    vkContext->m_vkResourceDeletionQueue->run();
    g_vkResourceDeletionQueue = vkContext->m_vkResourceDeletionQueue.get();
    vkContext->m_shaderCache = std::make_unique<ShaderCache>(vkContext->m_vkPhysicalDeviceProperties.m_hasKhrPipelineBinary);

    if (vkContext->m_vkPhysicalDeviceProperties.m_hasAMDAntiLag2) {
        vkContext->m_antiLagMethod.store(AntiLagMethod::AMDAntiLag2);
    }

    return vkContext;
}

auto VulkanContext::antiLagPaceInput(u64 frameIndex, u32 targetFps) -> void {
    auto antilagMethod = m_antiLagMethod.load(std::memory_order_relaxed);

    if (m_vkPhysicalDeviceProperties.m_hasAMDAntiLag2) {
        auto mode = (antilagMethod == AntiLagMethod::AMDAntiLag2) ? vk::AntiLagModeAMD::eOn : vk::AntiLagModeAMD::eOff;
        auto antilagPresentInfo = vk::AntiLagPresentationInfoAMD{}
            .setFrameIndex(frameIndex)
            .setStage(vk::AntiLagStageAMD::eInput);

        auto antilagData = vk::AntiLagDataAMD{}
            .setMode(mode)
            .setMaxFPS(targetFps)
            .setPPresentationInfo(&antilagPresentInfo);

        m_vkDevice.antiLagUpdateAMD(antilagData);
    }
}

auto VulkanContext::antiLagPacePresent(u64 frameIndex, u32 targetFps) -> void {
    auto antilagMethod = m_antiLagMethod.load(std::memory_order_relaxed);

    if (m_vkPhysicalDeviceProperties.m_hasAMDAntiLag2) {
        auto mode = (antilagMethod == AntiLagMethod::AMDAntiLag2) ? vk::AntiLagModeAMD::eOn : vk::AntiLagModeAMD::eOff;
        auto antilagPresentInfo = vk::AntiLagPresentationInfoAMD{}
            .setFrameIndex(frameIndex)
            .setStage(vk::AntiLagStageAMD::ePresent);

        auto antilagData = vk::AntiLagDataAMD{}
            .setMode(mode)
            .setMaxFPS(targetFps)
            .setPPresentationInfo(&antilagPresentInfo);

        m_vkDevice.antiLagUpdateAMD(antilagData);
    }
}
auto VulkanContext::currentVramUsage() const -> std::tuple<u64, u64> {
    auto budget = Vec<vma::Budget>::fromStdVector(m_vmaAllocator.getHeapBudgets());

    auto& vramStat = budget[m_vkPhysicalDeviceProperties.m_vramMemoryHeapIndex].statistics;
    return { vramStat.blockBytes, vramStat.allocationBytes };
}

auto VulkanContext::extMemoryBudgetGetVramBudget() const -> u64 {
    debug_assert(m_vkPhysicalDeviceProperties.m_hasExtMemoryBudget, "extMemoryBudget not supported");

    auto query = m_vkPhysicalDevice.getMemoryProperties2<vk::PhysicalDeviceMemoryProperties2, vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();
    return query.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>().heapBudget[m_vkPhysicalDeviceProperties.m_vramMemoryHeapIndex];
}

auto VulkanContext::initVkRaiiContext() -> vk::raii::Context {
    return {};
}

auto VulkanContext::createVkInstance(vk::raii::Context& vkRaiiContext, bool debugEnable) -> vk::raii::Instance {
    auto appInfo = vk::ApplicationInfo{}
        .setPApplicationName("project_nekomata")
        .setPEngineName("project_nekomata")
        .setApplicationVersion(vk::makeApiVersion(0, 0, 1, 0))
        .setEngineVersion(vk::makeApiVersion(0, 0, 1, 0))
        .setApiVersion(vk::ApiVersion14);

    auto availableInstanceLayerProps = Vec<vk::LayerProperties>::fromStdVector(vkCheckResult(vk::enumerateInstanceLayerProperties()));
    auto availableInstanceLayerNames = availableInstanceLayerProps.iter()
        .map([](auto&& layer) { return std::string(layer.layerName); })
        .collect<Vec>();

    auto instanceExtensions = projnekomata::SdlWindow::vulkanInstanceExtensions();
    
    auto instanceExtensionsC = instanceExtensions.iter()
        .map([](auto&& ext) { return ext.c_str(); })
        .collect<Vec>();

    auto instanceLayersC = Vec<const char*>::create();

    if (debugEnable && availableInstanceLayerNames.contains("VK_LAYER_KHRONOS_validation"))
        instanceLayersC.emplace("VK_LAYER_KHRONOS_validation");

    auto instanceInfo = vk::InstanceCreateInfo{}
        .setPApplicationInfo(&appInfo)
        .setPEnabledExtensionNames(instanceExtensionsC)
        .setPEnabledLayerNames(instanceLayersC);
    
    auto instance = vkCheckResult(vkRaiiContext.createInstance(instanceInfo));

    return instance;
}

auto VulkanContext::createVkSurface(const vk::raii::Instance& vkInstance, projnekomata::SdlWindow& sdlWindow) -> vk::raii::SurfaceKHR {
    auto surface = sdlWindow.vulkanCreateRawSurface(*vkInstance);

    return vk::raii::SurfaceKHR(vkInstance, surface);
}

auto VulkanContext::pickVkPhysicalDevice(const vk::raii::Instance& vkInstance, const vk::raii::SurfaceKHR& vkSurface) -> std::tuple<vk::raii::PhysicalDevice, VulkanPhysicalDeviceProperties> {
    auto physicalDevices = Vec<vk::raii::PhysicalDevice>::fromStdVector(vkCheckResult(vkInstance.enumeratePhysicalDevices()));

    if (physicalDevices.isEmpty()) panic("no GPUs found");

    auto physicalDeviceOpt = physicalDevices.iter()
        .enumerate()
        .filterMap([&](auto&& pdKey) {
            auto query = VulkanPhysicalDeviceProperties::query(pdKey.value, vkSurface);
            if (query.isOk()) {
                log::info("GPU #{}:", pdKey.index);
                query.unwrap().printInfo();
            } else {
                log::warn("GPU #{} is not supported, reason: {}", pdKey.index, query.unwrapErr().toString());
            }
            return query.ok().map([&](auto&& props) { return std::pair(pdKey.index, std::move(props)); });
        })
        .maxByKey([](const auto& exp) {
            return exp.second.autoselectPriorityScore();
        });

    if (physicalDeviceOpt.isNone()) panic("no GPUs supported");
    auto [physicalDeviceIndex, props] = std::move(physicalDeviceOpt.unwrap());

    log::info("Picked GPU #{}", physicalDeviceIndex);
    return { physicalDevices[physicalDeviceIndex], std::move(props) };
}

auto VulkanContext::createVkDevice(const vk::raii::PhysicalDevice& vkPhysicalDevice, const VulkanPhysicalDeviceProperties& vkPhysicalDeviceProps) -> vk::raii::Device {
    log::info("Creating device..");

    auto deviceExtensionsC = vkPhysicalDeviceProps.m_enabledExtensions.iter()
        .inspect([](const auto& x) { log::info("  Enabling device extension: {}", x); })
        .map([](auto&& x) { return x.c_str(); })
        .collect<Vec>();

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

    if (!vkPhysicalDeviceProps.m_hasAMDAntiLag2) {
        chain.unlink<vk::PhysicalDeviceAntiLagFeaturesAMD>();
    }

    auto device = vkCheckResult(vkPhysicalDevice.createDevice(chain.get<vk::DeviceCreateInfo>()));
    return device;
}

auto VulkanContext::createVmaAllocator(const vk::raii::Instance& vkInstance, const vk::raii::PhysicalDevice& vkPhysicalDevice, const VulkanPhysicalDeviceProperties& vkPhysicalDeviceProps, const vk::raii::Device& vkDevice) -> vma::raii::Allocator {
    // The VMA HPP bindings load the Vulkan functions from the instance and device dispatchers automatically.
    auto allocatorCreateInfo = vma::AllocatorCreateInfo{}
        .setPhysicalDevice(vkPhysicalDevice)
        .setVulkanApiVersion(vk::ApiVersion14)
        .setFlags(vkPhysicalDeviceProps.vmaAllocatorCreateFlags());

    return vkCheckResult(vma::raii::createAllocator(vkInstance, vkDevice, allocatorCreateInfo));
}

}