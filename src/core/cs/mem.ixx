module;
#include <cstdlib>
#include <malloc.h>
export module projnekomata.cs:mem;
import :panic;
import :primitives;

export class Mem {
public:
    // ---- Allocation -----------------------------------------------------------------------------------------------------------------------------------------
    template <typename T> static auto alloc(usize len) -> T* { return static_cast<T*>(::malloc(len * sizeof(T))); }
    template <typename T> static auto allocAligned(usize len, usize alignment) -> T* { return static_cast<T*>(::aligned_alloc(alignment, len * sizeof(T))); }
    template <typename T> static auto realloc(T* ptr, usize len) -> T* { return static_cast<T*>(::realloc(static_cast<void*>(ptr), len * sizeof(T))); }

    // ---- Ensured Allocation ---------------------------------------------------------------------------------------------------------------------------------
    template <typename T> static auto allocChecked(usize len) -> T* {
        auto ptr = alloc<T>(len);
        if (!ptr) panic("out of host memory");
        return ptr;
    }
    template <typename T> static auto allocAlignedChecked(usize len, usize alignment) -> T* {
        auto ptr = allocAligned<T>(len, alignment);
        if (!ptr) panic("out of host memory");
        return ptr;
    }
    template <typename T> static auto reallocChecked(T* ptr, usize len) -> T* {
        auto ptr2 = realloc<T>(ptr, len);
        if (!ptr2) panic("out of host memory");
        return ptr2;
    }

    // ---- Allocation Metadata --------------------------------------------------------------------------------------------------------------------------------
    template <typename T> static auto allocUsableSize(T* ptr) -> usize { return ::malloc_usable_size(static_cast<void*>(ptr)) / sizeof(T); }

    // ---- Freeing --------------------------------------------------------------------------------------------------------------------------------------------
    template <typename T> static auto free(T* ptr) -> void { ::free(static_cast<void*>(ptr)); }
    template <typename T> static auto freeAligned(T* ptr) -> void { ::free(static_cast<void*>(ptr)); }
};