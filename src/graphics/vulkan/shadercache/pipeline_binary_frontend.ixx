module;
#include <string.h>
export module nekomata2:graphics.vulkan.shadercache.pipeline_binary_frontend;
import std;
import vulkan;
import :core.log;
import :core.platform.int_def;
import :core.storage.sharded_hash_storage;
import :graphics.vulkan.context;

export namespace nekomata2 {

constexpr u32 uint32Le(u32 x) {
    if constexpr (std::endian::native == std::endian::little) {
        return x;
    }
    return __builtin_bswap32(x);
}

class PipelineKeysToBinaryKeyObjectsSerde {
public:
    static auto serialize(const std::span<const vk::PipelineBinaryKeyKHR>& pipelineKeys, std::vector<u8>& buffer) -> void {
        u32 keyCount = uint32Le(pipelineKeys.size());
        buffer.insert(buffer.end(), reinterpret_cast<const u8*>(&keyCount), reinterpret_cast<const u8*>(&keyCount) + sizeof(keyCount));

        for (const auto& pipelineKey : pipelineKeys) {
            u32 keySize = uint32Le(pipelineKey.keySize);
            buffer.insert(buffer.end(), reinterpret_cast<const u8*>(&keySize), reinterpret_cast<const u8*>(&keySize) + sizeof(keySize));
            buffer.insert(buffer.end(), pipelineKey.key.begin(), pipelineKey.key.end());
        }
    }

    static auto deserialize(std::span<const u8> buffer, std::vector<vk::PipelineBinaryKeyKHR>& pipelineKeys) -> bool {
        if (buffer.size() < 4) return false;
        u32 keyCount;
        memcpy(&keyCount, buffer.data(), sizeof(keyCount));
        keyCount = uint32Le(keyCount);

        usize cursor = 4;

        for (u32 i = 0; i < keyCount; i++) {
            if (buffer.size() < cursor + 4) return false;
            u32 keySize;
            memcpy(&keySize, buffer.data() + cursor, sizeof(keySize));
            keySize = uint32Le(keySize);
            cursor += 4;
            if (buffer.size() < cursor + keySize) return false;

            auto pipelineKey = vk::PipelineBinaryKeyKHR{};
            pipelineKey.keySize = keySize;
            memcpy(pipelineKey.key.data(), buffer.data() + cursor, keySize);
            cursor += keySize;
            pipelineKeys.push_back(pipelineKey);
        }

        return true;
    }
};

class ShaderCachePipelineBinaryFrontend {
public:
    ShaderCachePipelineBinaryFrontend(const std::filesystem::path& pipelineCacheDirectory, u32 storesNibblesPerLevel, u32 storesLevelCount)
        : m_pipelineCacheDirectory(pipelineCacheDirectory),
            m_pipelineKeysToBinaryKeyObjects(pipelineCacheDirectory / "pk", storesNibblesPerLevel, storesLevelCount),
            m_binaryKeysToBinaryObjects(pipelineCacheDirectory / "bin", storesNibblesPerLevel, storesLevelCount) {}

    void checkGlobalKeyAndInvalidateStale() {
        auto globalKey = VulkanContext::get().vkDevice().getPipelineKeyKHR();
        std::span<const u8> gpkKey = { globalKey.key.data(), globalKey.keySize };
        std::vector<u8> byteBuffer;
        if (!getGlobalKeyFromStorage(byteBuffer)) {
            log::warn("Failed to load global pipeline key, will try to invalidate pipeline cache");
            m_pipelineKeysToBinaryKeyObjects.invalidate();
            m_binaryKeysToBinaryObjects.invalidate();
        } else {
            bool globalKeyMatches = std::ranges::equal(byteBuffer, gpkKey);
            if (!globalKeyMatches) {
                log::warn("Global pipeline key mismatch, invalidating pipeline cache.");
                m_pipelineKeysToBinaryKeyObjects.invalidate();
                m_binaryKeysToBinaryObjects.invalidate();
            }
        }
        if (!storeGlobalKeyToStorage(gpkKey)) {
            log::error("Failed to store global pipeline key!");
        }
    }

    template <typename... ScElements>
        requires (std::is_same_v<vk::GraphicsPipelineCreateInfo, ScElements> || ...)
        && (std::is_same_v<vk::PipelineCreateFlags2CreateInfo, ScElements> || ...)
        && (std::is_same_v<vk::PipelineBinaryInfoKHR, ScElements> || ...)
    auto handleCreateGraphicsPipeline(vk::StructureChain<ScElements...>& chain) -> vk::raii::Pipeline {
        auto pipelineCreateInfo = vk::PipelineCreateInfoKHR{}
            .setPNext(&chain.template get<vk::GraphicsPipelineCreateInfo>());

        auto pipelineKey = VulkanContext::get().vkDevice().getPipelineKeyKHR(pipelineCreateInfo);
        std::vector<vk::PipelineBinaryKeyKHR> binaryKeys;
        if (!getBinaryKeysForPipelineKey(pipelineKey, binaryKeys)) {
            return handleCreateGraphicsPipelineUncached(chain, pipelineKey);
        }
        std::vector<vk::raii::PipelineBinaryKHR> binaries;
        if (!getBinariesForBinaryKeys(binaryKeys, binaries)) {
            return handleCreateGraphicsPipelineUncached(chain, pipelineKey);
        }

        std::vector<vk::PipelineBinaryKHR> binaryHandles(binaries.size());
        std::transform(binaries.begin(), binaries.end(), binaryHandles.begin(),
                       [](const auto& b) { return *b; });

        chain.template get<vk::PipelineBinaryInfoKHR>()
            .setPipelineBinaries(binaryHandles);

        return VulkanContext::get().vkDevice().createGraphicsPipeline(nullptr, chain.template get<vk::GraphicsPipelineCreateInfo>());
    }

private:
    std::filesystem::path m_pipelineCacheDirectory;
    storage::ShardedHashStorage m_pipelineKeysToBinaryKeyObjects;
    storage::ShardedHashStorage m_binaryKeysToBinaryObjects;

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------
    // Storage Helpers

    auto getBinaryKeysForPipelineKey(vk::PipelineBinaryKeyKHR pipelineKey, std::vector<vk::PipelineBinaryKeyKHR>& buffer) -> bool {
        std::span<const u8> key = { pipelineKey.key.data(), pipelineKey.keySize };
        std::vector<u8> byteBuffer;

        auto result = m_pipelineKeysToBinaryKeyObjects.load(key, byteBuffer);
        if (!result.has_value()) {
            auto err = result.error();
            switch (err) {
            case storage::HashStorageLoadError::FileOpenError: log::error("Failed to load pipeline key to binary key object: file open error"); break;
            case storage::HashStorageLoadError::FileReadError: log::error("Failed to load pipeline key to binary key object: file read error"); break;
            case storage::HashStorageLoadError::ObjectMissing: log::warn("Missed pipeline binary key cache"); break;
            }
            return false;
        }

        if (!PipelineKeysToBinaryKeyObjectsSerde::deserialize(byteBuffer, buffer)) {
            log::error("Failed to deserialize pipeline binary key objects");
            return false;
        }
        return true;
    }

    auto getBinariesForBinaryKeys(const std::span<const vk::PipelineBinaryKeyKHR> binaryKeys, std::vector<vk::raii::PipelineBinaryKHR>& buffer) -> bool {
        std::vector<std::vector<u8>> byteBuffers;
        for (const auto& binaryKey : binaryKeys) {
            std::span<const u8> key = { binaryKey.key.data(), binaryKey.keySize };
            std::vector<u8> byte_buffer;
            auto result = m_binaryKeysToBinaryObjects.load(key, byte_buffer);
            if (!result.has_value()) {
                auto err = result.error();
                switch (err) {
                case storage::HashStorageLoadError::FileOpenError: log::error("Failed to load binary key to binary object: file open error"); break;
                case storage::HashStorageLoadError::FileReadError: log::error("Failed to load binary key to binary object: file read error"); break;
                case storage::HashStorageLoadError::ObjectMissing: log::warn("Missed binary cache"); break;
                }
                return false;
            }
            byteBuffers.emplace_back(std::move(byte_buffer));
        }

        std::vector<vk::PipelineBinaryDataKHR> binaryData(byteBuffers.size());
        for (usize i = 0; i < byteBuffers.size(); i++) {
            binaryData[i] = vk::PipelineBinaryDataKHR{}
                .setData<u8>(byteBuffers[i]);
        }

        auto binaryAndDataInfo = vk::PipelineBinaryKeysAndDataKHR{}
            .setPipelineBinaryKeys(binaryKeys)
            .setPipelineBinaryData(binaryData);

        auto binaryCreateInfo = vk::PipelineBinaryCreateInfoKHR{}
            .setPKeysAndDataInfo(&binaryAndDataInfo);

        buffer = VulkanContext::get().vkDevice().createPipelineBinariesKHR(binaryCreateInfo);

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------
    // GPK Key Utils

    auto getGlobalKeyFromStorage(std::vector<u8>& buffer) -> bool {
        std::filesystem::path keyPath = m_pipelineCacheDirectory / "gpk";

        if (!std::filesystem::exists(keyPath)) {
            return false;
        }
        std::ifstream keyFile(keyPath, std::ios::binary | std::ios::ate);
        if (!keyFile) {
            log::error("Failed to open global pipeline key file!");
            return false;
        }
        buffer.resize(keyFile.tellg());
        keyFile.seekg(0);
        keyFile.read(reinterpret_cast<std::istream::char_type*>(buffer.data()), buffer.size());
        if (!keyFile) {
            log::error("Failed to read global pipeline key!");
            return false;
        }
        return true;
    }

    auto storeGlobalKeyToStorage(const std::span<const u8>& data) -> bool {
        std::filesystem::create_directories(m_pipelineCacheDirectory);
        std::filesystem::path keyPath = m_pipelineCacheDirectory / "gpk";
        std::ofstream keyFile(keyPath, std::ios::binary);
        if (!keyFile) {
            log::error("Failed to open global pipeline key file!");
            return false;
        }
        keyFile.write(reinterpret_cast<const std::ostream::char_type*>(data.data()), data.size());
        if (!keyFile) {
            log::error("Failed to write global pipeline key!");
            return false;
        }
        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------
    // Cache Miss Paths

    template <typename... ScElements>
        requires (std::is_same_v<vk::GraphicsPipelineCreateInfo, ScElements> || ...)
        && (std::is_same_v<vk::PipelineCreateFlags2CreateInfo, ScElements> || ...)
        && (std::is_same_v<vk::PipelineBinaryInfoKHR, ScElements> || ...)
    auto handleCreateGraphicsPipelineUncached(vk::StructureChain<ScElements...>& chain, vk::PipelineBinaryKeyKHR pipelineKey) -> vk::raii::Pipeline {
        chain.template get<vk::PipelineCreateFlags2CreateInfo>().flags |= vk::PipelineCreateFlagBits2::eCaptureDataKHR;

        auto pipeline = VulkanContext::get().vkDevice().createGraphicsPipeline(nullptr, chain.template get<vk::GraphicsPipelineCreateInfo>());

        // -----------------------------------------------------------------------------------------------------------------------------------------------------
        // Binary Extraction

        auto binaryCreateInfo = vk::PipelineBinaryCreateInfoKHR{}
            .setPipeline(pipeline);
        auto binaries = VulkanContext::get().vkDevice().createPipelineBinariesKHR(binaryCreateInfo);

        bool silentlyFailPipelineKeyWrite = false;
        std::vector<vk::PipelineBinaryKeyKHR> binaryKeys;
        for (const auto& binary : binaries) {
            auto binaryDataInfo = vk::PipelineBinaryDataInfoKHR{}
                .setPipelineBinary(binary);

            auto [binaryDataKey, binaryData] = VulkanContext::get().vkDevice().getPipelineBinaryDataKHR(binaryDataInfo);
            binaryKeys.emplace_back(binaryDataKey);

            std::span<const u8> binaryDataKeySpan = { binaryDataKey.key.data(), binaryDataKey.keySize };
            auto storeResult = m_binaryKeysToBinaryObjects.store(binaryDataKeySpan, binaryData);

            if (!storeResult.has_value()) {
                log::error("Failed to store pipeline binary object!");
                silentlyFailPipelineKeyWrite = true;
            }
        }

        auto release_info = vk::ReleaseCapturedPipelineDataInfoKHR{}
            .setPipeline(pipeline);

        if (silentlyFailPipelineKeyWrite) {
            log::warn("One of the pipeline binary objects failed to store, will not store the pipeline binary keys list.");
            VulkanContext::get().vkDevice().releaseCapturedPipelineDataKHR(release_info);
            return pipeline;
        }

        std::vector<u8> byteBuffer;
        PipelineKeysToBinaryKeyObjectsSerde::serialize(binaryKeys, byteBuffer);
        std::span<const u8> pipelineKeySpan = { pipelineKey.key.data(), pipelineKey.keySize };
        auto storeResult = m_pipelineKeysToBinaryKeyObjects.store(pipelineKeySpan, byteBuffer);

        if (!storeResult.has_value()) {
            log::error("Failed to store pipeline binary keys object!");
        }
        VulkanContext::get().vkDevice().releaseCapturedPipelineDataKHR(release_info);
        return pipeline;
    }
};

}