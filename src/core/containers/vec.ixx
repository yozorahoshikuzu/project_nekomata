module;
#include <cstdlib>
export module nekomata2:core.containers.vec;
import std;
import :core.platform.int_def;

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

template <typename Iter> concept CIterator = requires(Iter&& iter) {
    { iter.hasNext() } -> std::convertible_to<bool>;
    { iter.next() } -> std::convertible_to<decltype(iter.next())>;
    { iter.current() } -> std::convertible_to<decltype(iter.current())>;
};

template <typename A, typename B, typename Output> concept Add = requires(A a, B b) { { a + b } -> std::convertible_to<Output>; };
template <typename A, typename B, typename Output> concept Mul = requires(A a, B b) { { a * b } -> std::convertible_to<Output>; };

template <typename A, typename B> concept Ord = requires(A a, B b) { { a < b } -> std::convertible_to<bool>; };

template <typename Iter, typename Output> concept IterSummable =
    CIterator<Iter>
     && Add<Output, std::decay_t<decltype(std::declval<Iter&>().next())>, Output>;

template <typename Iter, typename Output> concept IterProducts =
    CIterator<Iter>
     && Mul<Output, std::decay_t<decltype(std::declval<Iter&>().next())>, Output>;

template <typename Inner, typename F> class MapIter;
template <typename Inner>             class EnumerateIter;
template <typename Inner, typename P> class FilterIter;
template <typename Inner, typename I> class InspectIter;

class IteratorCppEndProxy {};

template <typename Derived>
class IteratorMixin {
public:
    // ---- Iterator Builders ----------------------------------------------------------------------------------------------------------------------------------

    template <typename F> requires CIterator<Derived> constexpr auto map(F f) { return MapIter<Derived, F>(std::move(self()), std::move(f)); }
    template <typename P> requires CIterator<Derived> constexpr auto filter(P pred) { return FilterIter<Derived, P>(std::move(self()), std::move(pred)); }
    constexpr auto enumerate() requires CIterator<Derived> { return EnumerateIter<Derived>(std::move(self())); }

    template <typename I> requires CIterator<Derived> constexpr auto inspect(I i) { return InspectIter<Derived, I>(std::move(self()), std::move(i)); }

    // ---- Horizontal Reduction -------------------------------------------------------------------------------------------------------------------------------

    template <typename P> requires CIterator<Derived> constexpr auto all(P pred) -> bool {
        while (self().hasNext()) {
            if (!pred(self().next())) return false;
        }
        return true;
    }

    template <typename P> requires CIterator<Derived> constexpr auto any(P pred) -> bool {
        while (self().hasNext()) {
            if (pred(self().next())) return true;
        }
        return false;
    }

    constexpr auto sum() requires IterSummable<Derived, std::decay_t<decltype(std::declval<Derived&>().next())>> {
        using T = std::decay_t<decltype(self().next())>;
        T sum = T(0);
        while (self().hasNext()) {
            sum += self().next();
        }
        return sum;
    }

    constexpr auto prod() requires IterProducts<Derived, std::decay_t<decltype(std::declval<Derived&>().next())>> {
        using T = std::decay_t<decltype(self().next())>;
        T prod = T(1);
        while (self().hasNext()) {
            prod *= self().next();
        }
        return prod;
    }

    constexpr auto min() requires Ord<std::decay_t<decltype(std::declval<Derived&>().next())>, std::decay_t<decltype(std::declval<Derived&>().next())>> {
        using T = std::decay_t<decltype(self().next())>;
        if (!self().hasNext()) return T();
        T min = std::move(self().next());
        while (self().hasNext()) {
            auto val = std::move(self().next());
            if (val < min) min = std::move(val);
        }
        return min;
    }

    template <typename KeyFn> constexpr auto minByKey(KeyFn keyFn)
        requires CIterator<Derived>
         && requires(Derived& d, KeyFn& f) { { f(d.next()) } -> Ord<decltype(f(std::declval<Derived&>()))>; }
    {
        using T = std::decay_t<decltype(self().next())>;
        using K = std::decay_t<decltype(keyFn(self().next()))>;

        auto minElem = std::optional<T>{};
        auto minKey = std::optional<K>{};

        while (self().hasNext()) {
            auto&& val = self().next();
            K key = keyFn(val);
            if (!minElem.has_value() || key < *minKey) {
                minElem = std::move(val);
                minKey = std::move(key);
            }
        }
        return minElem;
    }

    constexpr auto max() requires Ord<std::decay_t<decltype(std::declval<Derived&>().next())>, std::decay_t<decltype(std::declval<Derived&>().next())>> {
        using T = std::decay_t<decltype(self().next())>;
        if (!self().hasNext()) return T();
        T max = std::move(self().next());
        while (self().hasNext()) {
            auto val = std::move(self().next());
            if (val > max) max = std::move(val);
        }
        return max;
    }

    template <typename KeyFn> constexpr auto maxByKey(KeyFn keyFn)
        requires CIterator<Derived>
    && requires(Derived& d, KeyFn& f) { { f(d.next()) } -> Ord<decltype(f(std::declval<Derived&>()))>; }
    {
        using T = std::decay_t<decltype(self().next())>;
        using K = std::decay_t<decltype(keyFn(self().next()))>;

        auto minElem = std::optional<T>{};
        auto minKey = std::optional<K>{};

        while (self().hasNext()) {
            auto&& val = self().next();
            K key = keyFn(val);
            if (!minElem.has_value() || key > *minKey) {
                minElem = std::move(val);
                minKey = std::move(key);
            }
        }
        return minElem;
    }

    // ---- Collector ------------------------------------------------------------------------------------------------------------------------------------------

    template <template <typename> class Container> constexpr auto collect() {
        using T = std::decay_t<decltype(self().next())>;
        auto out = Container<T>::create();
        while (self().hasNext()) out.push(self().next());
        return out;
    }

    // ---- Compatibility with for-loops -----------------------------------------------------------------------------------------------------------------------

    constexpr auto begin() { return self(); }
    constexpr auto end() { return IteratorCppEndProxy{}; }
    constexpr auto begin() const { return self(); }
    constexpr auto end() const { return IteratorCppEndProxy{}; }

    constexpr auto operator++() { self().next(); }
    constexpr auto operator*() { return self().current(); }

    constexpr auto operator==(const IteratorCppEndProxy&) const -> bool { return !self().hasNext(); }

private:
    constexpr Derived& self() { return static_cast<Derived&>(*this); }
    constexpr const Derived& self() const { return static_cast<const Derived&>(*this); }
};

template <typename TPtr> class Iter : public IteratorMixin<Iter<TPtr>> {
public:
    constexpr Iter(TPtr begin, TPtr end) : m_begin(begin), m_end(end) {}

    constexpr auto hasNext() const -> bool { return m_begin != m_end; }
    constexpr auto next() -> decltype(auto) { return *m_begin++; }
    constexpr auto current() const -> decltype(auto) { return *m_begin; }

private:
    TPtr m_begin, m_end;
};

template <typename Inner, typename F> class MapIter : public IteratorMixin<MapIter<Inner, F>> {
public:
    constexpr MapIter(Inner inner, F f) : m_inner(std::move(inner)), m_f(std::move(f)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() -> decltype(auto) { return m_f(m_inner.next()); }
    constexpr auto current() const -> decltype(auto) { return m_f(m_inner.current()); }

private:
    Inner m_inner;
    F m_f;
};

template <typename Inner> class EnumerateIter : public IteratorMixin<EnumerateIter<Inner>> {
public:
    constexpr EnumerateIter(Inner inner) : m_inner(std::move(inner)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() { return std::pair{m_index++, m_inner.next()}; }
    constexpr auto current() const { return std::pair{m_index, m_inner.current()}; }

private:
    Inner m_inner;
    usize m_index = 0;
};

template <typename Inner, typename P> class FilterIter : public IteratorMixin<FilterIter<Inner, P>> {
public:
    constexpr FilterIter(Inner inner, P p) : m_inner(std::move(inner)), m_p(std::move(p)) { advance(); }

    constexpr auto hasNext() const -> bool { return m_currentElem.has_value(); }
    constexpr auto next() -> decltype(auto) { auto val = std::move(*m_currentElem); advance(); return val; }
    constexpr auto current() const -> decltype(auto) { return *m_currentElem; }

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

template <typename Inner, typename I> class InspectIter : public IteratorMixin<InspectIter<Inner, I>> {
public:
    constexpr InspectIter(Inner inner, I i) : m_inner(std::move(inner)), m_i(std::move(i)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() -> decltype(auto) { auto& next = m_inner.next(); m_i(next); return next; }
    constexpr auto current() const -> decltype(auto) { return m_inner.current(); }

private:
    Inner m_inner;
    I m_i;
};

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

export template <typename T> struct TTriviallyRelocatable : std::bool_constant<std::is_trivially_copyable_v<T>> {};

template <typename T> inline constexpr bool TTriviallyRelocatableValue = TTriviallyRelocatable<T>::value;

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

    T* m_data        = nullptr;
    usize m_len      = 0_usize;
    usize m_capacity = 0_usize;
};