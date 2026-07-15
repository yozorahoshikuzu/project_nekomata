export module projnekomata.cs:assertions;
import :panic;

export constexpr void debug_assert(bool condition, std::string_view message, std::source_location loc = std::source_location::current()) {
#ifdef NDEBUG
    return;
#endif
    if (!condition) {
        panic("assertion failed at {}:{}: {}", loc.file_name(), loc.line(), message);
    }
}