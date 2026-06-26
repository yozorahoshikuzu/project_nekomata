export module nekomata2:core.cs.option;
import std;
import :core.platform.int_def;
import :core.cs.niche;

export template <typename T> class Option {
public:
    ~Option() { reset(); }

    static constexpr bool kNeedsDestruction = !std::is_trivially_destructible_v<T>;
    static constexpr bool kIsCopyable = std::is_copy_constructible_v<T>;
    static constexpr bool kIsMovable = std::is_move_constructible_v<T>;

    static auto none() -> Option<T> { return Option<T>(); }
    static auto some(const T& value) -> Option<T> { return Option<T>(value); }
    static auto some(T&& value) -> Option<T> { return Option<T>(std::forward<T>(value)); }

    Option(const Option& other) requires kIsCopyable {
        if (other.m_isSome) {
            storeObj(*other.ptr());
        } else {
            storeNone();
        }
    }
    Option& operator=(const Option& other) requires kIsCopyable {
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
        if constexpr (HasNiche<T>) return !NicheValue<T>::isNiche(*ptr());
        else return m_isSome;
    }
    constexpr auto isNone() const -> bool { return !isSome(); }
    constexpr explicit operator bool() const { return isSome(); }

    constexpr auto unwrap() const -> const T& {
        if (isNone()) __builtin_trap();
        return *ptr();
    }
    constexpr auto unwrap() -> T& {
        if (isNone()) __builtin_trap();
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
    constexpr explicit Option(const T& value) { storeObj(value); }
    constexpr explicit Option(T&& value) { storeObj(std::move(value)); }

    constexpr auto ptr() -> T* { return reinterpret_cast<T*>(&m_storage); }
    constexpr auto ptr() const -> const T* { return reinterpret_cast<const T*>(&m_storage); }

    constexpr auto storeNone() {
        if constexpr (HasNiche<T>) new (ptr()) T(NicheValue<T>::niche());
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