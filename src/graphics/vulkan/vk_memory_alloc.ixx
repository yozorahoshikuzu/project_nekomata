// Adapted from https://github.com/YaaZ/VulkanMemoryAllocator-Hpp/blob/master/include/vk_mem_alloc.cppm

module;
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_HPP_CXX_MODULE
#include <cassert>
#include <vk_mem_alloc.hpp>
#include <vk_mem_alloc_raii.hpp>
export module vk_mem_alloc;
export import vulkan;

export namespace vma {
    using vma::functionsFromDispatchers;
    using vma::functionsFromDispatcher;
    using vma::operator&;
    using vma::operator|;
    using vma::operator^;
    using vma::operator~;
    using vma::operator<;
    using vma::operator<=;
    using vma::operator>;
    using vma::operator>=;
    using vma::operator==;
    using vma::operator!=;
    using vma::operator<=>;
    using vma::AllocatorCreateFlagBits;
    using vma::AllocatorCreateFlags;
    using vma::MemoryUsage;
    using vma::AllocationCreateFlagBits;
    using vma::AllocationCreateFlags;
    using vma::PoolCreateFlagBits;
    using vma::PoolCreateFlags;
    using vma::DefragmentationFlagBits;
    using vma::DefragmentationFlags;
    using vma::DefragmentationMoveOperation;
    using vma::VirtualBlockCreateFlagBits;
    using vma::VirtualBlockCreateFlags;
    using vma::VirtualAllocationCreateFlagBits;
    using vma::VirtualAllocationCreateFlags;
    using vma::DeviceMemoryCallbacks;
    using vma::VulkanFunctions;
    using vma::AllocatorCreateInfo;
    using vma::AllocatorInfo;
    using vma::Statistics;
    using vma::DetailedStatistics;
    using vma::TotalStatistics;
    using vma::Budget;
    using vma::AllocationCreateInfo;
    using vma::PoolCreateInfo;
    using vma::AllocationInfo;
    using vma::AllocationInfo2;
    using vma::DefragmentationInfo;
    using vma::DefragmentationMove;
    using vma::DefragmentationPassMoveInfo;
    using vma::DefragmentationStats;
    using vma::VirtualBlockCreateInfo;
    using vma::VirtualAllocationCreateInfo;
    using vma::VirtualAllocationInfo;
    using vma::Allocator;
    using vma::Pool;
    using vma::Allocation;
    using vma::DefragmentationContext;
    using vma::VirtualAllocation;
    using vma::VirtualBlock;
    using vma::createAllocator;
    using vma::createVirtualBlock;
    namespace raii {
        using vma::raii::operator<=>;
        using vma::raii::operator==;
        using vma::raii::operator!=;
        using vma::raii::Allocator;
        using vma::raii::Pool;
        using vma::raii::Allocation;
        using vma::raii::DefragmentationContext;
        using vma::raii::VirtualAllocation;
        using vma::raii::VirtualBlock;
        using vma::raii::Buffer;
        using vma::raii::Image;
        using vma::raii::StatsString;
        using vma::raii::createAllocator;
        using vma::raii::createVirtualBlock;
    }
}

module : private;

namespace vk {
    template<> struct FlagTraits<vma::AllocatorCreateFlagBits>;
    template<> struct FlagTraits<vma::AllocationCreateFlagBits>;
    template<> struct FlagTraits<vma::PoolCreateFlagBits>;
    template<> struct FlagTraits<vma::DefragmentationFlagBits>;
    template<> struct FlagTraits<vma::VirtualBlockCreateFlagBits>;
    template<> struct FlagTraits<vma::VirtualAllocationCreateFlagBits>;
    template<> struct isVulkanHandleType<vma::Allocator>;
    template<> struct isVulkanHandleType<vma::Pool>;
    template<> struct isVulkanHandleType<vma::Allocation>;
    template<> struct isVulkanHandleType<vma::DefragmentationContext>;
    template<> struct isVulkanHandleType<vma::VirtualAllocation>;
    template<> struct isVulkanHandleType<vma::VirtualBlock>;
    namespace raii {
        template<> struct isVulkanRAIIHandleType<vma::raii::Allocator>;
        template<> struct isVulkanRAIIHandleType<vma::raii::Pool>;
        template<> struct isVulkanRAIIHandleType<vma::raii::Allocation>;
        template<> struct isVulkanRAIIHandleType<vma::raii::DefragmentationContext>;
        template<> struct isVulkanRAIIHandleType<vma::raii::VirtualAllocation>;
        template<> struct isVulkanRAIIHandleType<vma::raii::VirtualBlock>;
        template<> struct isVulkanRAIIHandleType<vma::raii::Buffer>;
        template<> struct isVulkanRAIIHandleType<vma::raii::Image>;
        template<> struct isVulkanRAIIHandleType<vma::raii::StatsString>;
    }
}