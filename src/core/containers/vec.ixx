module;
#include <cstdlib>
export module nekomata2:core.containers.vec;
import std;
import :core.platform.int_def;

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

template <typename Inner, typename F> class MapIter;
template <typename Inner>             class EnumerateIter;
template <typename Inner, typename P> class FilterIter;

template <typename TPtr> class Iter {
public:
    constexpr Iter(TPtr begin, TPtr end) : m_begin(begin), m_end(end) {}

    constexpr auto hasNext() const -> bool { return m_begin != m_end; }
    constexpr auto next() -> decltype(auto) { return *m_begin++; }

    template <typename F> constexpr auto map(F f) -> MapIter<Iter, F> { return MapIter<Iter, F>(*this, std::move(f)); }
    constexpr auto enumerate() -> EnumerateIter<Iter> { return EnumerateIter<Iter>(*this); }
    template <typename P> constexpr auto filter(P p) -> FilterIter<Iter, P> { return FilterIter<Iter, P>(*this, std::move(p)); }

    template <template <typename> class DstContainer> constexpr auto collect() {
        using T = std::decay_t<decltype(*m_begin)>;
        auto dst = DstContainer<T>::create();
        while (hasNext()) dst.push(next());
        return dst;
    }

private:
    TPtr m_begin, m_end;
};

template <typename Inner, typename F> class MapIter {
public:
    constexpr MapIter(Inner inner, F f) : m_inner(std::move(inner)), m_f(std::move(f)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() -> decltype(auto) { return m_f(m_inner.next()); }

    template <typename G> constexpr auto map(G g) { return MapIter<MapIter, G>(*this, std::move(g)); }
    constexpr auto enumerate() { return EnumerateIter<MapIter>(*this); }
    template <typename P> constexpr auto filter(P pred) { return FilterIter<MapIter, P>(*this, std::move(pred)); }

    template <template <typename> class Container> constexpr auto collect() {
        using T = std::decay_t<decltype(m_f(m_inner.next()))>;
        auto out = Container<T>::create();
        while (hasNext()) out.push(next());
        return out;
    }

private:
    Inner m_inner;
    F m_f;
};

template <typename Inner> class EnumerateIter {
public:
    constexpr EnumerateIter(Inner inner) : m_inner(std::move(inner)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() { return std::pair{m_index++, m_inner.next()}; }

    template <typename F> constexpr auto map(F f) { return MapIter<EnumerateIter, F>(*this, std::move(f)); }
    template <typename P> constexpr auto filter(P p) { return FilterIter<EnumerateIter, P>(*this, std::move(p)); }

    template <template <typename> class Container> constexpr auto collect() {
        using T = decltype(std::pair{m_index, m_inner.next()});
        auto out = Container<T>::create();
        while (hasNext()) out.push(next());
        return out;
    }

private:
    Inner m_inner;
    usize m_index = 0;
};

template <typename Inner, typename P> class FilterIter {
public:
    constexpr FilterIter(Inner inner, P p) : m_inner(std::move(inner)), m_p(std::move(p)) { advance(); }

    constexpr auto hasNext() const -> bool { return m_currentElem.has_value(); }
    constexpr auto next() -> decltype(auto) { auto val = std::move(*m_currentElem); advance(); return val; }

    template <typename F> constexpr auto map(F f) { return MapIter<FilterIter, F>(*this, std::move(f)); }
    constexpr auto enumerate() { return EnumerateIter<FilterIter>(*this); }

    template <template <typename> class Container> constexpr auto collect() {
        auto out = Container<ElemT>::create();
        while (hasNext()) out.push(next());
        return out;
    }

private:
    using ElemT = std::decay_t<decltype(std::declval<Inner&>().next())>;

    constexpr auto advance() {
        m_currentElem.reset();
        while (m_inner.hasNext()) {
            auto v = std::move(m_inner.next());
            if (m_p(v)) {
                m_currentElem = std::move(v);
                return;
            }
        }
    }

    Inner m_inner;
    P m_p;
    std::optional<ElemT> m_currentElem;
};

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

export template <typename T> struct TTriviallyRelocatable : std::bool_constant<std::is_trivially_copyable_v<T>> {};

template <typename T> inline constexpr bool TTriviallyRelocatableValue = TTriviallyRelocatable<T>::value;

export template <typename T> class Vec {
public:
    static constexpr bool kUsesTriviallyRelocatableFastpath = TTriviallyRelocatableValue<T>;
    static constexpr bool kNeedsFinalizer = !std::is_trivially_destructible_v<T>;

    Vec() = delete;

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

    // ---- Iterators ------------------------------------------------------------------------------------------------------------------------------------------

    auto iter() -> Iter<T*> { return Iter<T*>(m_data, m_data + m_len); }
    auto iter() const -> Iter<const T*> { return Iter<const T*>(m_data, m_data + m_len); }

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

    T* m_data;
    usize m_len;
    usize m_capacity;
};