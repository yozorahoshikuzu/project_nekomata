export module projnekomata:core.platform.assert;
import std;
import :core.log;
import :core.cs.panic;

export constexpr void debug_assert(bool condition, std::string_view message) {
#ifdef NDEBUG
    return;
#endif
    if (!condition) {
        panic("assertion failed: {}", message);
    }
}
