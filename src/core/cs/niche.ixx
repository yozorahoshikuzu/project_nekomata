export module nekomata2:core.cs.niche;
import std;

export template <typename T, typename = void> struct NicheValue {};

template <typename T> concept HasNiche = requires {
    { NicheValue<T>::niche() }                           -> std::same_as<T>;
    { NicheValue<T>::isNiche(std::declval<const T&>()) } -> std::same_as<bool>;
};