export module nekomata2:core.containers.iter;
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

export template <typename Inner, typename F> class MapIter;
export template <typename Inner>             class EnumerateIter;
export template <typename Inner, typename P> class FilterIter;
export template <typename Inner, typename I> class InspectIter;

class IteratorCppEndProxy {};

export template <typename Derived>
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

export template <typename TPtr> class Iter : public IteratorMixin<Iter<TPtr>> {
public:
    constexpr Iter(TPtr begin, TPtr end) : m_begin(begin), m_end(end) {}

    constexpr auto hasNext() const -> bool { return m_begin != m_end; }
    constexpr auto next() -> decltype(auto) { return *m_begin++; }
    constexpr auto current() const -> decltype(auto) { return *m_begin; }

private:
    TPtr m_begin, m_end;
};

export template <typename Inner, typename F> class MapIter : public IteratorMixin<MapIter<Inner, F>> {
public:
    constexpr MapIter(Inner inner, F f) : m_inner(std::move(inner)), m_f(std::move(f)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() -> decltype(auto) { return m_f(m_inner.next()); }
    constexpr auto current() const -> decltype(auto) { return m_f(m_inner.current()); }

private:
    Inner m_inner;
    F m_f;
};

export template <typename Inner> class EnumerateIter : public IteratorMixin<EnumerateIter<Inner>> {
public:
    constexpr EnumerateIter(Inner inner) : m_inner(std::move(inner)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() { return std::pair{m_index++, m_inner.next()}; }
    constexpr auto current() const { return std::pair{m_index, m_inner.current()}; }

private:
    Inner m_inner;
    usize m_index = 0;
};

export template <typename Inner, typename P> class FilterIter : public IteratorMixin<FilterIter<Inner, P>> {
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

export template <typename Inner, typename I> class InspectIter : public IteratorMixin<InspectIter<Inner, I>> {
public:
    constexpr InspectIter(Inner inner, I i) : m_inner(std::move(inner)), m_i(std::move(i)) {}

    constexpr auto hasNext() const -> bool { return m_inner.hasNext(); }
    constexpr auto next() -> decltype(auto) { auto& next = m_inner.next(); m_i(next); return next; }
    constexpr auto current() const -> decltype(auto) { return m_inner.current(); }

private:
    Inner m_inner;
    I m_i;
};
