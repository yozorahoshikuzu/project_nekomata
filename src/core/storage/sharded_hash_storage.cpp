#include "sharded_hash_storage.hpp"

#include "core/log/log.hpp"

#include <fstream>

namespace nekomata2::storage {

ShardedHashStorage::ShardedHashStorage(const std::filesystem::path& directory, u32 nibblesPerLevel, u32 levelCount)
    : m_directory(directory), m_nibblesPerLevel(nibblesPerLevel), m_levelCount(levelCount) {}

auto ShardedHashStorage::store(const std::span<const u8>& hash, const std::span<const u8>& data) -> std::expected<std::monostate, HashStorageWriteError> {
    auto fullpath = buildPathFromHash(hash);

    std::error_code ec;
    std::filesystem::create_directories(fullpath.parent_path(), ec);

    if (ec) {
        log::crit("Failed to create directories for hash storage: {}", ec.message());
        return std::unexpected(HashStorageWriteError::FileOpenError);
    }

    std::ofstream file(fullpath, std::ios::binary | std::ios::trunc);
    if (!file) return std::unexpected(HashStorageWriteError::FileOpenError);

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file) return std::unexpected(HashStorageWriteError::FileWriteError);

    return std::monostate{};
}

auto ShardedHashStorage::load(const std::span<const u8>& hash, std::vector<u8>& dst) -> std::expected<std::monostate, HashStorageLoadError> {
    auto fullpath = buildPathFromHash(hash);
    if (!std::filesystem::exists(fullpath))
        return std::unexpected(HashStorageLoadError::ObjectMissing);

    auto file = std::ifstream(fullpath, std::ios::binary | std::ios::ate);
    if (!file) return std::unexpected(HashStorageLoadError::FileOpenError);
    dst.resize(file.tellg());
    file.seekg(0);

    file.read(reinterpret_cast<char*>(dst.data()), dst.size());
    if (!file) return std::unexpected(HashStorageLoadError::FileReadError);

    return std::monostate{};
}
auto ShardedHashStorage::invalidate() -> void {
    std::filesystem::remove_all(m_directory);
}

auto ShardedHashStorage::buildPathFromHash(const std::span<const u8>& hash) const -> std::filesystem::path {
    static constexpr char kHexLut[] = "0123456789abcdef";

    std::string hashHexStr(hash.size() * 2, '\0');
    for (usize i = 0; i < hash.size(); i++) {
        hashHexStr[i * 2    ] = kHexLut[hash[i] >> 4];
        hashHexStr[i * 2 + 1] = kHexLut[hash[i] & 0xf];
    }

    std::filesystem::path fullpath = m_directory;

    for (u32 i = 0; i < m_levelCount; i++) {
        fullpath /= hashHexStr.substr(i * m_nibblesPerLevel, m_nibblesPerLevel);
    }

    return fullpath / hashHexStr;
}

} // namespace nekomata2::storage