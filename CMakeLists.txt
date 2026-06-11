cmake_minimum_required(VERSION 4.3)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

include(FetchContent)
project(nekomata2 VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------------------------------------------------------
#     Dependencies

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)
find_package(Freetype REQUIRED)
find_package(utf8cpp REQUIRED)

FetchContent_Declare(
        vmahpp
        URL      https://github.com/YaaZ/VulkanMemoryAllocator-Hpp/releases/download/v3.3.0+3/VulkanMemoryAllocator-Hpp-3.3.0.tar.gz
        URL_HASH SHA256=8c2a0573babe1f86f3241d12b4c9df40bb944649b7f4239c1c611797f8af6cd5
        SYSTEM
)
set(VMA_STATIC_VULKAN_FUNCTIONS OFF CACHE BOOL "" FORCE)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(vmahpp)

FetchContent_Declare(xxhash
        GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
        GIT_TAG v0.8.3
        SYSTEM
)
FetchContent_MakeAvailable(xxhash)


FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG        v4.6.1
        SYSTEM
)
FetchContent_MakeAvailable(simdjson)

FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG        v0.9.0
        SYSTEM
)

# Work around fastgltf transiently including simdjson.
set(CMAKE_SKIP_INSTALL_RULES ON)
FetchContent_MakeAvailable(fastgltf)
set(CMAKE_SKIP_INSTALL_RULES OFF)

FetchContent_Declare(
        ktx
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG        v4.4.2
        SYSTEM
)

set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ktx)

# Find tcmalloc, prefer tcmalloc_minimal over tcmalloc.

find_library(TCMALLOC_MINIMAL tcmalloc_minimal)
find_library(TCMALLOC tcmalloc)

if(TCMALLOC_MINIMAL)
    set(TCMALLOC_LIB ${TCMALLOC_MINIMAL})
elseif(TCMALLOC)
    set(TCMALLOC_LIB ${TCMALLOC})
else()
    message(FATAL_ERROR "couldn't find tcmalloc")
endif()

# ----------------------------------------------------------------------------------------------------------------------
#     Vulkan and VMA Modules

add_library(VulkanModule)
target_compile_definitions(
        VulkanModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)
target_link_libraries(VulkanModule PUBLIC Vulkan::Headers)
target_sources(VulkanModule
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${Vulkan_INCLUDE_DIRS}
        FILES
        ${Vulkan_INCLUDE_DIRS}/vulkan/vulkan.cppm
)

add_library(VmaModule)
target_compile_definitions(
        VmaModule PRIVATE
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=0
)
target_link_libraries(VmaModule PRIVATE VulkanModule)
target_include_directories(VmaModule PUBLIC ${vmahpp_SOURCE_DIR})
target_sources(VmaModule PUBLIC
        FILE_SET CXX_MODULES
        FILES ${vmahpp_SOURCE_DIR}/vk_mem_alloc.cppm)

# ----------------------------------------------------------------------------------------------------------------------
#     Slang Setup

find_program(SLANGC_EXECUTABLE slangc
        HINTS $ENV{VULKAN_SDK}/bin
        REQUIRED
)

message(STATUS "Found slangc: ${SLANGC_EXECUTABLE}")

function(add_slang_shaders TARGET)
        cmake_parse_arguments(SLANG "" "TARGET_DIR" "SOURCES;ENTRIES;STAGES" ${ARGN})

        if(NOT SLANG_TARGET_DIR)
                set(SLANG_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
        endif()

        file(MAKE_DIRECTORY ${SLANG_TARGET_DIR})
        set(OUTPUT_FILES "")

        set(SLANG_FLAGS
                -target spirv
                -profile spirv_1_6
                # -O3 # adding this breaks a current waterfall workaround - keep off for now
                -fvk-use-scalar-layout
        )

        foreach(SHADER ${SLANG_SOURCES})
                get_filename_component(NAME_WE ${SHADER} NAME_WE)
                set(OUTPUT_BINARY ${SLANG_TARGET_DIR}/${NAME_WE}.spv)
                set(OUTPUT_DEPFILE ${OUTPUT_BINARY}.d)
                set(CMD
                        ${SLANGC_EXECUTABLE}
                        ${SHADER}
                        ${SLANG_FLAGS}
                        -o
                        ${OUTPUT_BINARY}
                        -depfile
                        ${OUTPUT_DEPFILE}
                )

                add_custom_command(
                        OUTPUT ${OUTPUT_BINARY}
                        DEPFILE ${OUTPUT_DEPFILE}
                        COMMAND ${CMD}
                        DEPENDS ${SHADER}
                        COMMENT "Compiling Slang shader ${SHADER} --> ${OUTPUT_BINARY}"
                        VERBATIM
                )

                list(APPEND OUTPUT_FILES ${OUTPUT_BINARY})
        endforeach()

        add_custom_target(${TARGET} DEPENDS ${OUTPUT_FILES})
endfunction()

# ----------------------------------------------------------------------------------------------------------------------
#     Sources

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/src/*.cpp
)
file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/src/*.ixx
)

file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/src/graphics/shaders/*.slang
)

add_executable(nekomata2)
target_sources(nekomata2
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET CXX_MODULES FILES
        ${MODULE_SOURCES}
)
add_slang_shaders(nekomata2_spv TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/spirv SOURCES ${SHADER_SOURCES})
add_dependencies(nekomata2 nekomata2_spv)

target_include_directories(nekomata2 PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
        nekomata2
        PRIVATE
        ${TCMALLOC_LIB}
        VulkanModule
        VmaModule
        Freetype::Freetype
        SDL3::SDL3
        ktx
        fastgltf::fastgltf
        xxhash
        utf8::cpp
)

target_compile_options(
        nekomata2 PRIVATE

        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wdouble-promotion
        -Wimplicit-fallthrough
        -Wmisleading-indentation
        -Werror=return-type
        -Werror=non-virtual-dtor
        -Werror=uninitialized
        -Werror=conditional-uninitialized
        -Werror=init-self
        -Werror=array-bounds
        -Werror=sequence-point
        -Werror=delete-non-virtual-dtor
        -Werror=infinite-recursion
        -Werror=overloaded-virtual

        -Werror=tautological-compare
)

target_link_options(
        nekomata2 PRIVATE

        -fuse-ld=lld
        -flto=thin

        -Wl,--gc-sections
        -Wl,-z,now
        -Wl,-z,relro
)