export module projnekomata:core.cs.option;
import std;
import :core.platform.int_def;
import :core.cs.niche;

export template <typename T> class Option {
public:
    ~Option() { reset(); }

    static constexpr bool kNeedsDestruction = !std::is_trivially_destructible_v<T>;
    static constexpr bool kIsCopyable = std::is_copy_constructible_v<T>;
    static constexpr bool kIsMovable = std::is_move_constructible_v<T>;

    constexpr static auto none() -> Option<T> { return Option<T>(); }
    constexpr static auto some(const T& value) -> Option<T> { return Option<T>(value); }
    constexpr static auto some(T&& value) -> Option<T> { return Option<T>(std::forward<T>(value)); }

    template <typename F> constexpr static auto someIf(bool cond, F&& f) -> Option<T>
        requires std::invocable<F> && std::same_as<std::invoke_result_t<F>, T>
    {
        if (cond) return some(f());
        return none();
    }

    constexpr Option(const Option& other) requires kIsCopyable {
        if (other.isSome()) {
            storeObj(*other.ptr());
        } else {
            storeNone();
        }
    }
    constexpr Option& operator=(const Option& other) requires kIsCopyable {
        if (this == &other) return *this;
        reset();
        if (other.isSome()) {
            storeObj(*other.ptr());
        } else {
            storeNone();
        }
        return *this;
    }
    constexpr Option(Option&& other) requires kIsMovable {
        if (other.isSome()) {
            storeObj(std::move(*other.ptr()));
            if constexpr (kNeedsDestruction) other.ptr()->~T();
            other.storeNone();
        } else {
            storeNone();
        }
    }
    constexpr Option& operator=(Option&& other) requires kIsMovable {
        if (this == &other) return *this;
        reset();
        if (other.isSome()) {
            storeObj(std::move(*other.ptr()));
            if constexpr (kNeedsDestruction) other.ptr()->~T();
            other.storeNone();
        }
        return *this;
    }

    constexpr Option(const Option&) requires (!kIsCopyable) = delete;
    constexpr Option& operator=(const Option&) requires (!kIsCopyable) = delete;
    constexpr Option(Option&&) requires (!kIsMovable) = delete;
    constexpr Option& operator=(Option&&) requires (!kIsMovable) = delete;


    constexpr auto isSome() const -> bool {
        if constexpr (HasNiche<T>) return !NicheValue<T>::matchesNiche(m_storage);
        else return m_isSome;
    }
    constexpr auto isNone() const -> bool { return !isSome(); }
    constexpr explicit operator bool() const { return isSome(); }

    constexpr auto unwrap() const& -> const T& {
        if (isNone()) __builtin_trap();
        return *ptr();
    }
    constexpr auto unwrap() & -> T& {
        if (isNone()) __builtin_trap();
        return *ptr();
    }
    constexpr auto unwrap() && -> T {
        if (isNone()) __builtin_trap();
        return std::move(*ptr());
    }

    constexpr auto unwrapOr(const T& other) const -> const T& {
        if (isNone()) return other;
        return *ptr();
    }
    constexpr auto unwrapOr(T&& other) -> T& {
        if (isNone()) return other;
        return *ptr();
    }

    constexpr auto reset() -> void {
        if (isSome()) {
            if constexpr (kNeedsDestruction) ptr()->~T();
            storeNone();
        }
    }

private:
    constexpr Option() { storeNone(); }
    constexpr Option(const T& value) { storeObj(value); }
    constexpr Option(T&& value) { storeObj(std::forward<T>(value)); }

    constexpr auto ptr() -> T* { return reinterpret_cast<T*>(&m_storage); }
    constexpr auto ptr() const -> const T* { return reinterpret_cast<const T*>(&m_storage); }

    constexpr auto storeNone() {
        if constexpr (HasNiche<T>) NicheValue<T>::setNiche(m_storage);
        else m_isSome = false;
    }

    constexpr auto storeObj(auto&&... args) {
        new (ptr()) T(std::forward<decltype(args)>(args)...);
        if constexpr (!HasNiche<T>) m_isSome = true;
    }

    using IsSome = std::conditional_t<HasNiche<T>, std::monostate, bool>;

    alignas(T) u8 m_storage[sizeof(T)];
    [[no_unique_address]] IsSome m_isSome;
};