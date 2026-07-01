export module projnekomata:core.cs.result;
import std;
import :core.cs.panic;
import :core.cs.option;
import :core.cs.invoke_traits;

template <typename T> struct OkT { T value; };
template <typename T> constexpr auto Ok(T&& value) -> OkT<std::decay_t<T>> { return OkT<std::decay_t<T>>{ std::forward<T>(value) }; }

template <typename T> struct ErrT { T value; };
template <typename T> constexpr auto Err(T&& value) -> ErrT<std::decay_t<T>> { return ErrT<std::decay_t<T>>{ std::forward<T>(value) }; }

export template <typename T, typename E> class Result {
public:
    constexpr Result(OkT<T> other) : m_variant(std::move(other.value)) {}
    constexpr Result(ErrT<E> other) : m_variant(std::move(other.value)) {}

    constexpr static auto Ok(const T& value) -> Result<T, E> { return Result<T, E>(value); }
    constexpr static auto Err(const E& error) -> Result<T, E> { return Result<T, E>(error); }
    constexpr static auto Ok(T&& value) -> Result<T, E> { return Result<T, E>(std::move(value)); }
    constexpr static auto Err(E&& error) -> Result<T, E> { return Result<T, E>(std::move(error)); }

    constexpr auto isOk() const -> bool { return std::holds_alternative<T>(m_variant); }
    constexpr auto isErr() const -> bool { return std::holds_alternative<E>(m_variant); }

    template <typename P> requires TypedInvocable<P, bool, const T&>
    constexpr auto isOkAnd(P&& pred) const -> bool { return isOk() && pred(std::get<T>(m_variant)); }

    template <typename P> requires TypedInvocable<P, bool, const E&>
    constexpr auto isErrAnd(P&& pred) const -> bool { return isErr() && pred(std::get<E>(m_variant)); }

    constexpr auto unwrap() const& -> const T& {
        if (isErr()) panic("called `Result::unwrap()` on an Err value");
        return std::get<T>(m_variant);
    }
    constexpr auto unwrap() & -> T& {
        if (isErr()) panic("called `Result::unwrap()` on an Err value");
        return std::get<T>(m_variant);
    }
    constexpr auto unwrap() && -> T {
        if (isErr()) panic("called `Result::unwrap()` on an Err value");
        return std::move(std::get<T>(m_variant));
    }

    constexpr auto unwrapErr() const& -> const E& {
        if (isOk()) panic("called `Result::unwrapErr()` on an Ok value");
        return std::get<E>(m_variant);
    }
    constexpr auto unwrapErr() & -> E& {
        if (isOk()) panic("called `Result::unwrapErr()` on an Ok value");
        return std::get<E>(m_variant);
    }
    constexpr auto unwrapErr() && -> E {
        if (isOk()) panic("called `Result::unwrapErr()` on an Ok value");
        return std::move(std::get<E>(m_variant));
    }

    constexpr auto ok() -> Option<T> {
        if (isErr()) return None;
        return Some(std::move(std::get<T>(m_variant)));
    }
    constexpr auto err() -> Option<E> {
        if (isOk()) return None;
        return Some(std::move(std::get<E>(m_variant)));
    }

private:
    constexpr explicit Result(const T& value) : m_variant(value) {}
    constexpr explicit Result(const E& error) : m_variant(error) {}
    constexpr explicit Result(T&& value) : m_variant(std::move(value)) {}
    constexpr explicit Result(E&& error) : m_variant(std::move(error)) {}

    std::variant<T, E> m_variant;
};