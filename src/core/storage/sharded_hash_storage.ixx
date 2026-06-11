export module nekomata2:core.storage.sharded_hash_storage;
import std;
import :core.platform.int_def;

export namespace nekomata2::storage {

enum class HashStorageWriteError {
    FileOpenError,
    FileWriteError,
};

enum class HashStorageLoadError {
    ObjectMissing,
    FileOpenError,
    FileReadError,
};

class ShardedHashStorage {
public:
    explicit ShardedHashStorage(const std::filesystem::path& directory, u32 nibblesPerLevel, u32 levelCount);

    auto store(const std::span<const u8>& hash, const std::span<const u8>& data) -> std::expected<std::monostate, HashStorageWriteError>;
    auto load(const std::span<const u8>& hash, std::vector<u8>& dst)             -> std::expected<std::monostate, HashStorageLoadError>;

    auto invalidate() -> void;

private:
    std::filesystem::path m_directory;
    u32 m_nibblesPerLevel;
    u32 m_levelCount;

    auto buildPathFromHash(const std::span<const u8>& hash) const -> std::filesystem::path;
};

}