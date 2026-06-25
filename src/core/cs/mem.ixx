module;
#include <cstdlib>
export module nekomata2:core.cs.mem;
import :core.platform.int_def;

export class Mem {
public:
    template <typename T> static auto alloc(usize len) -> T* { return static_cast<T*>(malloc(len * sizeof(T))); }
    template <typename T> static auto free(T* ptr) -> void { ::free(static_cast<void*>(ptr)); }
    template <typename T> static auto allocAligned(usize len, usize alignment) -> T* { return static_cast<T*>(aligned_alloc(alignment, len * sizeof(T))); }
    template <typename T> static auto freeAligned(T* ptr) -> void { ::free(static_cast<void*>(ptr)); }
};