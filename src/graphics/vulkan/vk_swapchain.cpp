module nekomata2;
import :core.log;
import :graphics.vulkan.vk_physical_device_props;
import :graphics.vulkan.context;
import :graphics.vulkan.vk_queue_family_swizzling;

namespace nekomata2 {

SwapchainImage::SwapchainImage() = default;
SwapchainImage::SwapchainImage(vk::Image image, vk::Extent2D extent, vk::Format format, VulkanBinarySemaphore&& imagePresentSemaphore) : m_vkImage(image), m_format(format), m_extent(extent), m_vkSemaphoreImagePresent(std::move(imagePresentSemaphore)) {}

auto SwapchainImage::from(vk::Image vkImage, vk::Extent2D extent, vk::Format format) -> SwapchainImage {
    return SwapchainImage(vkImage, extent, format, VulkanBinarySemaphore::create());
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------

static constexpr std::array<vk::PresentModeKHR, 4> PRESENT_MODE_PRIORITY_VSYNC = { vk::PresentModeKHR::eFifoRelaxed, vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate };
static constexpr std::array<vk::PresentModeKHR, 4> PRESENT_MODE_PRIORITY_NO_VSYNC = { vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eFifoRelaxed, vk::PresentModeKHR::eFifo };

VulkanSwapchain::VulkanSwapchain(std::nullptr_t) {}
VulkanSwapchain::VulkanSwapchain(vk::raii::SwapchainKHR&& swapchain, vk::Extent2D swapchainImageExtent, std::vector<SwapchainImage>&& swapchainImages) : m_vkSwapchain(std::move(swapchain)), m_swapchainImageExtent(swapchainImageExtent), m_vkSwapchainImages(std::move(swapchainImages)) {}

auto VulkanSwapchain::create(vk::Extent2D windowDrawableExtent, std::optional<VulkanSwapchain>&& oldSwapchain, bool vsyncEnable) -> VulkanSwapchain {
    auto surfaceProps = VulkanPhysicalDeviceSurfaceProperties::query(VulkanContext::get().vkPhysicalDevice(), VulkanContext::get().vkSurface());

    auto surfaceFormatIt = std::ranges::find_if(surfaceProps.m_surfaceFormats, [&](const vk::SurfaceFormatKHR& sformat) -> bool {
        return (sformat.format == vk::Format::eR8G8B8A8Srgb || sformat.format == vk::Format::eB8G8R8A8Srgb) && sformat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; 
    });
    auto surfaceFormat = surfaceFormatIt != surfaceProps.m_surfaceFormats.end() ? *surfaceFormatIt : surfaceProps.m_surfaceFormats[0];

    auto& presentModePriority = vsyncEnable ? PRESENT_MODE_PRIORITY_VSYNC : PRESENT_MODE_PRIORITY_NO_VSYNC;
    auto presentMode = *std::ranges::find_if(presentModePriority, [&](const vk::PresentModeKHR& pmode) -> bool {
        return std::ranges::contains(surfaceProps.m_presentModes, pmode);
    });

    vk::Extent2D imageExtent = surfaceProps.m_capabilities.currentExtent.width == std::numeric_limits<u32>::max() ? windowDrawableExtent : surfaceProps.m_capabilities.currentExtent;

    auto maxImageCount = surfaceProps.m_capabilities.maxImageCount == 0 ? std::numeric_limits<u32>::max() : surfaceProps.m_capabilities.maxImageCount;
    auto imageCount = std::min(surfaceProps.m_capabilities.minImageCount + 1, maxImageCount);
    auto imageSharingMode = VulkanContext::get().vkPhysicalDeviceProps().m_graphicsQueueIndex == VulkanContext::get().vkPhysicalDeviceProps().m_presentQueueIndex ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;

    // Reduce noise from window resizing / out-of-date events by checking if this is the first time calling this function is oldSwapchain is nullopt.
    if (!oldSwapchain.has_value()) {
        log::info("Swapchain parameters:");
        log::info("  Image count: {}", imageCount);
        log::info("  Image sharing mode: {}", vk::to_string(imageSharingMode));
        log::info("  Image format: {}", vk::to_string(surfaceFormat.format));
        log::info("  Image color space: {}", vk::to_string(surfaceFormat.colorSpace));
        log::info("  Present mode: {}", vk::to_string(presentMode));
    }

    auto queueFamiliesForSwapchain = VulkanContext::get().vkPhysicalDeviceProps().m_queueFamilies[QueueFamily::Graphics | QueueFamily::Present];
    auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR{}
        .setSurface(VulkanContext::get().vkSurface())
        .setImageFormat(surfaceFormat.format)
        .setImageColorSpace(surfaceFormat.colorSpace)
        .setImageExtent(imageExtent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eTransferDst)
        .setImageSharingMode(imageSharingMode)
        .setMinImageCount(imageCount)
        .setQueueFamilyIndices(queueFamiliesForSwapchain)
        .setPreTransform(surfaceProps.m_capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(true)
        .setPresentMode(presentMode);
    
    if (oldSwapchain.has_value()) {
        swapchainCreateInfo.oldSwapchain = oldSwapchain->vkSwapchain();
    }

    auto swapchain = VulkanContext::get().vkDevice().createSwapchainKHR(swapchainCreateInfo);
    auto preparedSwapchainImages = swapchain.getImages();
    auto swapchainImages = preparedSwapchainImages
        | std::views::transform([&](vk::Image image) -> SwapchainImage {
            return SwapchainImage::from(image, imageExtent, surfaceFormat.format);
        })
        | std::ranges::to<std::vector>();

    return VulkanSwapchain(std::move(swapchain), imageExtent, std::move(swapchainImages));
}

auto VulkanSwapchain::acquireNextImage(u64 timeoutNanos, const VulkanBinarySemaphore& imageAcquireSemaphore) -> std::pair<std::optional<u32>, bool> {
    auto index = m_vkSwapchain.vkHandle().acquireNextImage(timeoutNanos, imageAcquireSemaphore.vkSemaphore());
    if (index.result == vk::Result::eSuccess) {
        return { *index, false };
    }
    if (index.result == vk::Result::eSuboptimalKHR) {
        return { *index, true };
    }

    if (index.result == vk::Result::eErrorOutOfDateKHR) {
        return { std::nullopt, true };
    }

    throw std::logic_error("acquire next image failed");
}

}
