export module projnekomata:core.storage.sharded_hash_storage;
import std;
import :core.platform.int_def;
import :core.cs.result;
import :core.cs.slice;
import :core.cs.vec;

export namespace projnekomata::storage {

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

    auto store(Slice<const u8> hash, Slice<const u8> data) -> Result<std::monostate, HashStorageWriteError>;
    auto load(Slice<const u8> hash, Vec<u8>& dst)             -> Result<std::monostate, HashStorageLoadError>;

    auto invalidate() -> void;

private:
    std::filesystem::path m_directory;
    u32 m_nibblesPerLevel;
    u32 m_levelCount;

    auto buildPathFromHash(Slice<const u8> hash) const -> std::filesystem::path;
};

}