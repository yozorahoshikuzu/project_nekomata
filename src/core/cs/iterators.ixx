export module nekomata2:core.cs.iterators;
import std;
import :core.platform.int_def;

// ---- Iterator Trait  ----------------------------------------------------------------------------------------------------------------------------------------

export template <typename T> concept Iterator = requires(T t) {
    typename T::Item;
    { t.next() } -> std::same_as<std::optional<typename T::Item>>;
};

// ---- References ---------------------------------------------------------------------------------------------------------------------------------------------

template <typename T> struct Enumerand {
    usize index; T value;
};

export template <typename K, typename V> struct KeyValue {
    K key; V value;
};

// Maps T -> T& and U* -> U&.
export template <typename T> struct AsLvalueRef { using type = T&; };
template <typename T> struct AsLvalueRef<T*> { using type = T&; };
template <typename T> struct AsLvalueRef<Enumerand<T*>> { using type = Enumerand<T&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<K*, V>> { using type = KeyValue<K&, V&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<K, V*>> { using type = KeyValue<K&, V&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<K*, V*>> { using type = KeyValue<K&, V&>; };
template <typename T> using AsLvalueRefT = typename AsLvalueRef<T>::type;
template <typename T> using DerefT = std::remove_pointer_t<std::remove_reference_t<T>>;

template <typename T> constexpr auto asLvalueRef(Enumerand<T*> x) -> AsLvalueRefT<Enumerand<T*>> { return {x.index, *x.value}; }
template <typename T> constexpr auto asLvalueRef(T&& x) -> AsLvalueRefT<T> { return x; }
template <typename T> constexpr auto asLvalueRef(T* x) -> AsLvalueRefT<T*> { return *x; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<K, V*> x) -> AsLvalueRefT<KeyValue<K, V*>> { return {x.key, *x.value}; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<K*, V> x) -> AsLvalueRefT<KeyValue<K*, V>> { return {*x.key, x.value}; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<K*, V*> x) -> AsLvalueRefT<KeyValue<K*, V*>> { return {*x.key, *x.value}; }

template <typename T> constexpr auto deref(T&& x) -> DerefT<T> { return x; }
template <typename T> constexpr auto deref(T* x) -> DerefT<T*> { return *x; }


// ---- Basic Iterators ----------------------------------------------------------------------------------------------------------------------------------------

export template <typename Derived> class IteratorBase;

template <Iterator Inner, typename F> class MapIter : public IteratorBase<MapIter<Inner, F>> {
public:
    using Item = std::invoke_result_t<F, AsLvalueRefT<typename Inner::Item>>;

    constexpr MapIter(Inner inner, F f) : m_inner(std::move(inner)), m_f(std::move(f)) {}
    constexpr auto next() -> std::optional<Item> {
        if (auto next = m_inner.next()) return m_f(asLvalueRef(std::move(*next)));
        return std::nullopt;
    }

private:
    Inner m_inner;
    F     m_f;
};

template <Iterator Inner, typename P> class FilterIter : public IteratorBase<FilterIter<Inner, P>> {
public:
    using Item = typename Inner::Item;

    constexpr FilterIter(Inner inner, P p) : m_inner(std::move(inner)), m_p(std::move(p)) {}
    constexpr auto next() -> std::optional<Item> {
        while (auto next = m_inner.next()) { if (asLvalueRef(m_p(*next))) return next; }
        return std::nullopt;
    }

private:
    Inner m_inner;
    P     m_p;
};

template <Iterator Inner> class EnumerateIter : public IteratorBase<EnumerateIter<Inner>> {
public:
    using Item = Enumerand<typename Inner::Item>;

    constexpr EnumerateIter(Inner inner) : m_inner(std::move(inner)) {}
    constexpr auto next() -> std::optional<Item> {
        if (auto next = m_inner.next()) return Enumerand{m_index++, std::forward<typename Inner::Item>(*next)};
        return std::nullopt;
    }

private:
    Inner m_inner;
    usize m_index = 0;
};

// ---- Inspector Iterators ------------------------------------------------------------------------------------------------------------------------------------

template <Iterator Inner, typename F> class InspectIter : public IteratorBase<InspectIter<Inner, F>> {
public:
    using Item = typename Inner::Item;

    constexpr InspectIter(Inner inner, F f) : m_inner(std::move(inner)), m_f(std::move(f)) {}
    constexpr auto next() -> std::optional<Item> {
        if (auto next = m_inner.next()) {
            m_f(asLvalueRef(*next));
            return next;
        }
        return std::nullopt;
    }

private:
    Inner m_inner;
    F     m_f;
};

// ---- CXX iterator shi ---------------------------------------------------------------------------------------------------------------------------------------

template <Iterator Iter> class CxxIterInner {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = std::remove_reference_t<typename Iter::Item>;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = AsLvalueRefT<value_type>;

    class EndSentinel {};

    explicit constexpr CxxIterInner(Iter inner) : m_inner(std::move(inner)) { ++*this; }

    constexpr auto operator++() -> CxxIterInner& { m_current = m_inner.next(); return *this; }
    constexpr auto operator*() -> reference { return asLvalueRef(*m_current); }
    constexpr auto operator->() -> pointer { return &*m_current; }
    constexpr auto operator->() const -> pointer { return &*m_current; }
    constexpr auto operator!=(const EndSentinel&) const -> bool { return m_current.has_value(); }

private:
    Iter m_inner;
    std::optional<typename Iter::Item> m_current;
};

// ---- Common CRTP iterator mixin -----------------------------------------------------------------------------------------------------------------------------

export template <typename Derived> class IteratorBase {
public:

    // ---- Iterator Extensions --------------------------------------------------------------------------------------------------------------------------------

    template <typename F> constexpr auto map(F&& f) && { return MapIter<Derived, std::decay_t<F>>(std::move(self()), std::forward<F>(f)); }
    template <typename P> constexpr auto filter(P&& p) && { return FilterIter<Derived, std::decay_t<P>>(std::move(self()), std::forward<P>(p)); }
    constexpr auto enumerate() && { return EnumerateIter<Derived>(std::move(self())); }
    template <typename F> constexpr auto inspect(F&& f) && { return InspectIter<Derived, std::decay_t<F>>(std::move(self()), std::forward<F>(f)); }

    // ---- Iterator Reductions --------------------------------------------------------------------------------------------------------------------------------

    constexpr auto sum() {
        using Item = typename Derived::Item;
        using ValueType = DerefT<Item>;
        ValueType sum = ValueType(0);
        while (auto v = self().next()) sum = std::move(sum) + deref(std::move(*v));
        return sum;
    }

    constexpr auto prod() {
        using Item = typename Derived::Item;
        using ValueType = DerefT<Item>;
        ValueType prod = ValueType(1);
        while (auto v = self().next()) prod = std::move(prod) * deref(std::move(*v));
        return prod;
    }

    template <typename P> constexpr auto all(P&& pred) -> bool {
        while (auto v = self().next()) {
            if (!pred(asLvalueRef(std::move(*v)))) return false;
        }
        return true;
    }

    template <typename P> constexpr auto any(P&& pred) -> bool {
        while (auto v = self().next()) {
            if (pred(asLvalueRef(std::move(*v)))) return true;
        }
        return false;
    }

    constexpr auto min() {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return std::nullopt;

        KeyValueType best = std::move(*firstOpt);

        while (auto v = self().next()) {
            if (asLvalueRef(*v) < asLvalueRef(best)) best = std::move(*v);
        }

        return best;
    }


    constexpr auto max() {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return std::nullopt;

        KeyValueType best = std::move(*firstOpt);

        while (auto v = self().next()) {
            if (asLvalueRef(*v) > asLvalueRef(best)) best = std::move(*v);
        }

        return best;
    }

    template <typename F> constexpr auto minByKey(F&& f) {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return std::optional<ValueType>(std::nullopt);

        ValueType best = std::move(*firstOpt);

        while (auto v = self().next()) {
            if (f(asLvalueRef(*v)) < f(asLvalueRef(best))) best = std::move(*v);
        }
        return std::optional<ValueType>{best};
    }

    template <typename F> constexpr auto maxByKey(F&& f) {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return std::optional<ValueType>{std::nullopt};

        ValueType best = std::move(*firstOpt);

        while (auto v = self().next()) {
            if (f(asLvalueRef(*v)) > f(asLvalueRef(best))) best = std::move(*v);
        }
        return std::optional<ValueType>{best};
    }


    // ---- Collector ------------------------------------------------------------------------------------------------------------------------------------------

    template <template <typename> class Container> constexpr auto collect() {
        using Item = typename Derived::Item;
        using T = std::remove_const_t<Item>;
        auto out = Container<T>::create();
        while (auto v = self().next()) out.emplace(std::move(*v));
        return out;
    }

    // ---- CXX iterator shi -----------------------------------------------------------------------------------------------------------------------------------

    constexpr auto begin() const { return CxxIterInner<Derived>(std::move(self())); }
    constexpr auto end() const { return typename CxxIterInner<Derived>::EndSentinel{}; }

private:
    constexpr       Derived& self()       { return static_cast<Derived&>(*this); }
    constexpr const Derived& self() const { return static_cast<const Derived&>(*this); }
};