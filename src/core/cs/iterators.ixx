export module nekomata2:core.cs.iterators;
import std;
import :core.platform.int_def;
import :core.cs.nonzero_ptr;
import :core.cs.option;
import :core.log;

// ---- Iterator Trait  ----------------------------------------------------------------------------------------------------------------------------------------

export template <typename T> concept Iterator = requires(T t) {
    typename T::Item;
    { t.next() } -> std::same_as<Option<typename T::Item>>;
};

// ---- References ---------------------------------------------------------------------------------------------------------------------------------------------

template <typename T> struct Enumerand {
    usize index; T value;
};

template <typename T> requires HasNiche<T> struct NicheValue<Enumerand<T>> {
    static Enumerand<T> niche() { return Enumerand<T>(0, NicheValue<T>::niche()); }
    static bool isNiche(const Enumerand<T>& ptr) { return NicheValue<T>::isNiche(ptr.value); }
};

export template <typename K, typename V> struct KeyValue {
    K key; V value;
};

template <typename K, typename V> requires HasNiche<K> struct NicheValue<KeyValue<K, V>> {
    alignas(V) inline static u8 m_invalidV[sizeof(V)] = {};
    static KeyValue<K,V> niche() { return KeyValue<K,V>(NicheValue<K>::niche(), *reinterpret_cast<V*>(m_invalidV)); }
    static bool isNiche(const KeyValue<K, V>& ptr) { return NicheValue<K>::isNiche(ptr.key); }
};

template <typename K, typename V> requires (!HasNiche<K> && HasNiche<V>) struct NicheValue<KeyValue<K, V>> {
    alignas(K) inline static u8 m_invalidK[sizeof(K)] = {};
    static KeyValue<K,V> niche() { return KeyValue<K,V>(*reinterpret_cast<K*>(m_invalidK), NicheValue<V>::niche()); }
    static bool isNiche(const KeyValue<K, V>& ptr) { return NicheValue<V>::isNiche(ptr.value); }
};

// todo: automatically deduce this shit

// Maps T -> T& and U* -> U&.
export template <typename T> struct AsLvalueRef { using type = T&; };
template <typename T> struct AsLvalueRef<T*> { using type = T&; };
template <typename T> struct AsLvalueRef<NonZeroPtr<T>> { using type = T&; };
template <typename T> struct AsLvalueRef<Enumerand<T*>> { using type = Enumerand<T&>; };
template <typename T> struct AsLvalueRef<Enumerand<NonZeroPtr<T>>> { using type = Enumerand<T&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<K*, V>> { using type = KeyValue<K&, V&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<K, V*>> { using type = KeyValue<K&, V&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<K*, V*>> { using type = KeyValue<K&, V&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<NonZeroPtr<K>, V>> { using type = KeyValue<K&, V&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<K, NonZeroPtr<V>>> { using type = KeyValue<K&, V&>; };
template <typename K, typename V> struct AsLvalueRef<KeyValue<NonZeroPtr<K>, NonZeroPtr<V>>> { using type = KeyValue<K&, V&>; };

template <typename T> using AsLvalueRefT = typename AsLvalueRef<T>::type;

export template <typename T> struct Deref { using type = std::remove_pointer_t<std::remove_reference_t<T>>; };
template<typename T> struct Deref<NonZeroPtr<T>> { using type = T; };
template <typename T> using DerefT = typename Deref<T>::type;

template <typename T> struct UnwrapOptional;
template <typename T> struct UnwrapOptional<std::optional<T>> { using type = T; };
template <typename T> struct UnwrapOptional<Option<T>> { using type = T; };
template <typename T> using UnwrapOptionalT = typename UnwrapOptional<T>::type;

template <typename T> constexpr auto asLvalueRef(Enumerand<T*> x) -> AsLvalueRefT<Enumerand<T*>> { return {x.index, *x.value}; }
template <typename T> constexpr auto asLvalueRef(Enumerand<NonZeroPtr<T>> x) -> AsLvalueRefT<Enumerand<NonZeroPtr<T>>> { return {x.index, *x.value}; }
template <typename T> constexpr auto asLvalueRef(T&& x) -> AsLvalueRefT<T> { return x; }
template <typename T> constexpr auto asLvalueRef(T* x) -> AsLvalueRefT<T*> { return *x; }
template <typename T> constexpr auto asLvalueRef(NonZeroPtr<T> x) -> AsLvalueRefT<NonZeroPtr<T>> { return *x; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<K, V*> x) -> AsLvalueRefT<KeyValue<K, V*>> { return {x.key, *x.value}; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<K*, V> x) -> AsLvalueRefT<KeyValue<K*, V>> { return {*x.key, x.value}; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<K*, V*> x) -> AsLvalueRefT<KeyValue<K*, V*>> { return {*x.key, *x.value}; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<K, NonZeroPtr<V>> x) -> AsLvalueRefT<KeyValue<K, NonZeroPtr<V>>> { return {x.key, *x.value}; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<NonZeroPtr<K>, V> x) -> AsLvalueRefT<KeyValue<NonZeroPtr<K>, V>> { return {*x.key, x.value}; }
template <typename K, typename V> constexpr auto asLvalueRef(KeyValue<NonZeroPtr<K>, NonZeroPtr<V>> x) -> AsLvalueRefT<KeyValue<NonZeroPtr<K>, NonZeroPtr<V>>> { return {*x.key, *x.value}; }


template <typename T> constexpr auto mapArgs(T& x) -> T& { return x; }
template <typename T> constexpr auto mapArgs(T&& x) -> T&& { return std::forward<T>(x); }
template <typename T> constexpr auto mapArgs(T* x) -> T& { return *x; }
template <typename T> constexpr auto mapArgs(NonZeroPtr<T> x) -> T& { return *x; }
template <typename T> constexpr auto mapArgs(Enumerand<T*> x) -> Enumerand<T&> { return {x.index, *x.value}; }
template <typename K, typename V> constexpr auto mapArgs(KeyValue<K, V> x) -> KeyValue<K&, V&> { return x; }
template <typename K, typename V> constexpr auto mapArgs(KeyValue<K*, V> x) -> KeyValue<K&, V&> { return {*x.key, x.value}; }
template <typename K, typename V> constexpr auto mapArgs(KeyValue<K, V*> x) -> KeyValue<K&, V&> { return {x.key, *x.value}; }
template <typename K, typename V> constexpr auto mapArgs(KeyValue<K*, V*> x) -> KeyValue<K&, V&> { return {*x.key, *x.value}; }

template <typename T> constexpr auto deref(T&& x) -> DerefT<T> { return x; }
template <typename T> constexpr auto deref(T* x) -> DerefT<T*> { return *x; }


// ---- Basic Iterators ----------------------------------------------------------------------------------------------------------------------------------------

export template <typename Derived> class IteratorBase;

template <Iterator Inner, typename F> class MapIter : public IteratorBase<MapIter<Inner, F>> {
public:
    using Item = std::invoke_result_t<F, AsLvalueRefT<typename Inner::Item>>;

    constexpr MapIter(Inner inner, F f) : m_inner(std::move(inner)), m_f(std::move(f)) {}
    constexpr auto next() -> Option<Item> {
        if (auto next = m_inner.next()) return Option<Item>::some(m_f(mapArgs(std::move(next.unwrap()))));
        return Option<Item>::none();
    }

private:
    Inner m_inner;
    F     m_f;
};

template <Iterator Inner, typename P> class FilterIter : public IteratorBase<FilterIter<Inner, P>> {
public:
    using Item = typename Inner::Item;

    constexpr FilterIter(Inner inner, P p) : m_inner(std::move(inner)), m_p(std::move(p)) {}
    constexpr auto next() -> Option<Item> {
        while (auto next = m_inner.next()) { if (m_p(asLvalueRef(next.unwrap()))) return Option<Item>::some(std::move(next.unwrap())); }
        return Option<Item>::none();
    }

private:
    Inner m_inner;
    P     m_p;
};

template <Iterator Inner, typename P> class FilterMapIter : public IteratorBase<FilterMapIter<Inner, P>> {
public:
    using Item = UnwrapOptionalT<std::invoke_result_t<P, AsLvalueRefT<typename Inner::Item>>>;

    constexpr FilterMapIter(Inner inner, P p) : m_inner(std::move(inner)), m_p(std::move(p)) {}
    constexpr auto next() -> Option<Item> {
        while (auto next = m_inner.next()) { if (auto mapped = m_p(mapArgs(std::move(next.unwrap())))) return Option<Item>::some(std::move(mapped.unwrap())); }
        return Option<Item>::none();
    }

private:
    Inner m_inner;
    P     m_p;
};

template <Iterator Inner> class EnumerateIter : public IteratorBase<EnumerateIter<Inner>> {
public:
    using Item = Enumerand<typename Inner::Item>;

    constexpr EnumerateIter(Inner inner) : m_inner(std::move(inner)) {}
    constexpr auto next() -> Option<Item> {
        if (auto next = m_inner.next()) return Option<Item>::some(Enumerand{m_index++, std::forward<typename Inner::Item>(next.unwrap())});
        return Option<Item>::none();
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
    constexpr auto next() -> Option<Item> {
        if (auto next = m_inner.next()) {
            m_f(asLvalueRef(next.unwrap()));
            return Option<Item>::some(std::move(next.unwrap()));
        }
        return Option<Item>::none();
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
    constexpr auto operator*() -> reference { return asLvalueRef(m_current.unwrap()); }
    constexpr auto operator->() -> pointer { return &m_current.unwrap(); }
    constexpr auto operator->() const -> pointer { return &m_current.unwrap(); }
    constexpr auto operator!=(const EndSentinel&) const -> bool { return m_current.isSome(); }

private:
    Iter m_inner;
    Option<typename Iter::Item> m_current = Option<typename Iter::Item>::none();
};

// ---- Common CRTP iterator mixin -----------------------------------------------------------------------------------------------------------------------------

export template <typename Derived> class IteratorBase {
public:

    // ---- Iterator Extensions --------------------------------------------------------------------------------------------------------------------------------

    template <typename F> constexpr auto map(F&& f) && { return MapIter<Derived, std::decay_t<F>>(std::move(self()), std::forward<F>(f)); }
    template <typename P> constexpr auto filter(P&& p) && { return FilterIter<Derived, std::decay_t<P>>(std::move(self()), std::forward<P>(p)); }
    template <typename P> constexpr auto filterMap(P&& p) && { return FilterMapIter<Derived, std::decay_t<P>>(std::move(self()), std::forward<P>(p)); }
    constexpr auto enumerate() && { return EnumerateIter<Derived>(std::move(self())); }
    template <typename F> constexpr auto inspect(F&& f) && { return InspectIter<Derived, std::decay_t<F>>(std::move(self()), std::forward<F>(f)); }

    // ---- Iterator Reductions --------------------------------------------------------------------------------------------------------------------------------

    constexpr auto sum() {
        using Item = typename Derived::Item;
        using ValueType = DerefT<Item>;
        ValueType sum = ValueType(0);
        while (auto v = self().next()) sum = std::move(sum) + deref(std::move(v.unwrap()));
        return sum;
    }

    constexpr auto prod() {
        using Item = typename Derived::Item;
        using ValueType = DerefT<Item>;
        ValueType prod = ValueType(1);
        while (auto v = self().next()) prod = std::move(prod) * deref(std::move(v.unwrap()));
        return prod;
    }

    template <typename P> constexpr auto all(P&& pred) -> bool {
        while (auto v = self().next()) {
            if (!pred(asLvalueRef(std::move(v.unwrap())))) return false;
        }
        return true;
    }

    template <typename P> constexpr auto any(P&& pred) -> bool {
        while (auto v = self().next()) {
            if (pred(asLvalueRef(std::move(v.unwrap())))) return true;
        }
        return false;
    }

    constexpr auto min() {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return Option<KeyValueType>::none();

        KeyValueType best = std::move(firstOpt.unwrap());

        while (auto v = self().next()) {
            if (asLvalueRef(v.unwrap()) < asLvalueRef(best)) best = std::move(v.unwrap());
        }

        return Option<KeyValueType>::some(std::move(best));
    }


    constexpr auto max() {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return Option<KeyValueType>::none();

        KeyValueType best = std::move(firstOpt.unwrap());

        while (auto v = self().next()) {
            if (asLvalueRef(v.unwrap()) > asLvalueRef(best)) best = std::move(v.unwrap());
        }

        return Option<KeyValueType>::some(std::move(best));
    }

    template <typename F> constexpr auto minByKey(F&& f) {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return Option<ValueType>::none();

        ValueType best = std::move(firstOpt.unwrap());

        while (auto v = self().next()) {
            if (f(asLvalueRef(v.unwrap())) < f(asLvalueRef(best))) best = std::move(v.unwrap());
        }
        return Option<ValueType>::some(std::move(best));
    }

    template <typename F> constexpr auto maxByKey(F&& f) {
        using Item = typename Derived::Item;
        using ValueType = Item;
        using KeyValueType = DerefT<ValueType>;

        auto firstOpt = std::move(self().next());
        if (!firstOpt) return Option<ValueType>::none();

        ValueType best = std::move(firstOpt.unwrap());

        while (auto v = self().next()) {
            if (f(asLvalueRef(v.unwrap())) > f(asLvalueRef(best))) best = std::move(v.unwrap());
        }
        return Option<ValueType>::some(std::move(best));
    }


    // ---- Collector ------------------------------------------------------------------------------------------------------------------------------------------

    template <template <typename> class Container> constexpr auto collect() {
        using Item = typename Derived::Item;
        using T = std::remove_const_t<Item>;
        auto out = Container<T>::create();
        while (auto v = self().next()) out.emplace(std::move(v.unwrap()));
        return out;
    }

    // ---- CXX iterator shi -----------------------------------------------------------------------------------------------------------------------------------

    constexpr auto begin() const { return CxxIterInner<Derived>(std::move(self())); }
    constexpr auto end() const { return typename CxxIterInner<Derived>::EndSentinel{}; }

private:
    constexpr       Derived& self()       { return static_cast<Derived&>(*this); }
    constexpr const Derived& self() const { return static_cast<const Derived&>(*this); }
};