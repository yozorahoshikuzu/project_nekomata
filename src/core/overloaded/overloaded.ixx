export module projnekomata:core.overloaded;
import std;

export namespace projnekomata {

template <class... Arms>
struct overloaded : Arms... {
    using Arms::operator()...;
};

template <class Variant, class... Arms>
constexpr auto match(Variant&& v, Arms&&... arms) -> decltype(auto) {
    return std::visit(
        overloaded{ std::forward<Arms>(arms)... },
        std::forward<Variant>(v)
    );
}

} // namespace projnekomata