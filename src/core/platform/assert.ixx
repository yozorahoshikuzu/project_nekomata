export module nekomata2.core.platform.assert;
import std;
import nekomata2.core.log;

export constexpr void debug_assert(bool condition, std::string_view message) {
#ifdef NDEBUG
    return;
#endif
    if (!condition) {
        log::crit("Assertion failed: {}", message);
    }
}
