module;
#include <cstdlib>
export module nekomata2:core.cs.vec;
import std;
import :core.platform.int_def;
import :core.cs.iterators;

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

export template <typename T> struct TTriviallyRelocatable : std::bool_constant<std::is_trivially_copyable_v<T>> {};

template <typename T> inline constexpr bool TTriviallyRelocatableValue = TTriviallyRelocatable<T>::value;

template <typename T> class VecSliceIter : public IteratorBase<VecSliceIter<T>> {
public:
    using Item = T*;

    constexpr VecSliceIter(T* begin, T* end) : m_begin(begin), m_end(end) {}
    constexpr auto next() -> std::optional<Item> {
        if (m_begin == m_end) return std::nullopt;
        return m_begin++;
    }

private:
    T* m_begin;
    T* m_end;
};

export template <typename T> class Vec {
public:
    static constexpr bool kUsesTriviallyRelocatableFastpath = TTriviallyRelocatableValue<T>;
    static constexpr bool kNeedsFinalizer = !std::is_trivially_destructible_v<T>;

    // TODO: remove later
    Vec() = default;
    constexpr ~Vec() { destroyFinalize(); free(m_data); }

    constexpr Vec(Vec&& other) noexcept : m_data(other.m_data), m_len(other.m_len), m_capacity(other.m_capacity) {
        other.m_data = nullptr;
        other.m_len = 0;
        other.m_capacity = 0;
    }
    constexpr Vec& operator=(Vec&& other) noexcept {
        if (this == &other) return *this;
        destroyFinalize();
        free(m_data);

        m_data = other.m_data;
        m_len = other.m_len;
        m_capacity = other.m_capacity;
        other.m_data = nullptr;
        other.m_len = 0;
        other.m_capacity = 0;
        return *this;
    }

    constexpr static auto create() noexcept -> Vec { return Vec(nullptr, 0, 0); }
    constexpr static auto create(std::initializer_list<T> list) -> Vec {
        auto vec = Vec::withCapacity(list.size());
        for (auto& elem : list) vec.emplace(elem);
        return vec;
    }
    constexpr static auto fromValue(usize len, const T& val) -> Vec {
        auto vec = Vec::create();
        vec.resize(len, val);
        return vec;
    }
    constexpr static auto fromValue(usize len, T&& val) -> Vec {
        auto vec = Vec::create();
        vec.resize(len, std::move(val));
        return vec;
    }
    constexpr static auto withCapacity(usize capacity) -> Vec {
        auto vec = Vec::create();
        vec.reserveExact(capacity);

        return vec;
    }
    constexpr static auto fromStdVector(std::vector<T>&& vec) -> Vec {
        Vec dst = Vec::withCapacity(vec.size());
        if constexpr (kUsesTriviallyRelocatableFastpath) {
            if (!vec.empty()) std::memcpy(dst.m_data, vec.data(), vec.size() * sizeof(T));
            dst.m_len = vec.size();
        } else {
            for (auto& elem : vec) dst.emplace(std::move(elem));
        }
        return dst;
    }

    // ---- Access ---------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto len() const noexcept -> usize { return m_len; }
    constexpr auto size() const noexcept -> usize { return m_len; }
    constexpr auto capacity() const noexcept -> usize { return m_capacity; }

    constexpr auto data() noexcept -> T* { return m_data; }
    constexpr auto begin() noexcept -> T* { return m_data; }
    constexpr auto end() noexcept -> T* { return m_data + m_len; }

    constexpr auto data() const noexcept -> const T* { return m_data; }
    constexpr auto begin() const noexcept -> const T* { return m_data; }
    constexpr auto end() const noexcept -> const T* { return m_data + m_len; }

    constexpr T& operator[](usize index) noexcept { return m_data[index]; }
    constexpr const T& operator[](usize index) const noexcept { return m_data[index]; }

    constexpr T& first() noexcept { return m_data[0]; }
    constexpr const T& first() const noexcept { return m_data[0]; }
    constexpr T& last() noexcept { return m_data[m_len - 1]; }
    constexpr const T& last() const noexcept { return m_data[m_len - 1]; }

    constexpr auto isEmpty() const noexcept -> bool { return m_len == 0; }
    template <typename U> constexpr auto contains(const U& val) const noexcept -> bool { return std::find(m_data, m_data + m_len, val) != m_data + m_len; }

    // ---- Reserve / Resize -----------------------------------------------------------------------------------------------------------------------------------

    constexpr auto reserve(usize newCapacity) {
        if (newCapacity <= m_capacity) return;
        usize autoNewCap = m_capacity + m_capacity / 2;
        reallocate(std::max(newCapacity, autoNewCap));
    }

    constexpr auto reserveExact(usize newCapacity) {
        if (newCapacity <= m_capacity) return;
        reallocate(newCapacity);
    }

    constexpr auto shrinkToFit() {
        if (m_capacity == m_len) return;
        reallocate(m_len);
    }

    constexpr auto resize(usize newLen, const T& val = T()) {
        if (newLen > m_capacity) reserve(newLen);
        resizeCommon(newLen, val);
    }

    constexpr auto resizeExact(usize newLen, const T& val = T()) {
        if (newLen > m_capacity) reserveExact(newLen);
        resizeCommon(newLen, val);
    }

    constexpr auto truncate(usize newLen) {
        if (newLen >= m_len) return;
        if constexpr (kNeedsFinalizer) {
            for (usize i = newLen; i < m_len; i++) m_data[i].~T();
        }
        m_len = newLen;
    }

    // ---- Mutation -------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto push(const T& val) { emplace(val); }
    constexpr auto push(T&& val) { emplace(std::move(val)); }

    template <typename... Args>
    constexpr auto emplace(Args&&... args) -> T& {
        if (m_len == m_capacity) reserve(m_capacity + 1);
        auto index = m_data + m_len;
        new (index) T(std::forward<Args>(args)...);
        m_len++;
        return *index;
    }

    constexpr auto pop() {
        if (m_len == 0) return;
        m_len--;
        if constexpr (kNeedsFinalizer) m_data[m_len].~T();
    }

    constexpr auto clear() {
        if (m_len == 0) return;
        destroyFinalize();
        m_len = 0;
    }

    template <typename P> constexpr auto retain(P pred) requires requires(T& d) { { pred(d) } -> std::convertible_to<bool>; }
    {
        usize newLen = 0;
        for (usize src = 0; src < m_len; src++) {
            if (pred(m_data[src])) {
                if (newLen != src) {
                    if constexpr (kUsesTriviallyRelocatableFastpath) {
                        m_data[newLen] = std::move(m_data[src]);
                    } else {
                        new (m_data + newLen) T(std::move(m_data[src]));
                        if constexpr (kNeedsFinalizer) m_data[src].~T();
                    }
                }
                newLen++;
            } else {
                if constexpr (kNeedsFinalizer) m_data[src].~T();
            }
        }

        m_len = newLen;
    }

    // ---- Iterators ------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto iter() -> VecSliceIter<T> { return VecSliceIter<T>(m_data, m_data + m_len); }
    constexpr auto iter() const -> VecSliceIter<const T> { return VecSliceIter<const T>(m_data, m_data + m_len); }

private:
    constexpr Vec(T* data, usize len, usize capacity) noexcept : m_data(data), m_len(len), m_capacity(capacity) {}

    constexpr auto destroyFinalize() {
        if constexpr (kNeedsFinalizer) {
            for (usize i = 0; i < m_len; i++) m_data[i].~T();
        }
    }

    constexpr auto resizeCommon(usize newLen, const T& val) {
        if (newLen > m_len) {
            for (usize i = m_len; i < newLen; i++) new (m_data + i) T(val);
        } else {
            if constexpr (kNeedsFinalizer) {
                for (usize i = newLen; i < m_len; i++) m_data[i].~T();
            }
        }
        m_len = newLen;
    }

    constexpr auto reallocate(usize newCapacity) {
        if constexpr (kUsesTriviallyRelocatableFastpath) {
            auto ptr = static_cast<T*>(realloc(m_data, newCapacity * sizeof(T)));
            if (!ptr) throw std::bad_alloc();
            m_data = ptr;
        } else {
            auto ptr = static_cast<T*>(malloc(newCapacity * sizeof(T)));
            if (!ptr) throw std::bad_alloc();
            for (usize i = 0; i < m_len; i++) {
                new (ptr + i) T(std::move(m_data[i]));
                if constexpr (kNeedsFinalizer) m_data[i].~T();
            }
            free(m_data);
            m_data = ptr;
        }
        m_capacity = newCapacity;
    }

    T* m_data        = nullptr;
    usize m_len      = 0_usize;
    usize m_capacity = 0_usize;
};