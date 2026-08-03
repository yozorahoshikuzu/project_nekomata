export module projnekomata.cs:type_traits;
import std;

export template <typename T> struct TTriviallyRelocatable : std::bool_constant<__builtin_is_cpp_trivially_relocatable(T)> {};
template <typename T> inline constexpr bool TTriviallyRelocatableValue = TTriviallyRelocatable<T>::value;
