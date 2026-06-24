export module nekomata2:core.platform.assert;
import std;
import :core.log;

export constexpr void debug_assert(bool condition, std::string_view message) {
#ifdef NDEBUG
    return;
#endif
    if (!condition) {
        nekomata2::log::crit("Assertion failed: {}", message);
        std::terminate();
    }
}
