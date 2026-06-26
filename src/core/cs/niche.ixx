export module nekomata2:core.cs.niche;
import std;

/// Niche values are used to optimize the representation of objects potentially containing an object that has a niche value, to avoid having to store a
/// discriminator to identify whether the object is present or not.
///
/// An example of an object that makes use of niche values is an `Option<T>`. If T has a niche value, then it will be used to represent a none value.
/// One notable object that has a niche value is `NonNullPtr<T>`. One of its invariants is that its stored pointer is never `nullptr`. Constructing an
/// `Option<NonNullPtr<T>>` will use `NonNullPtr<T>` with its internal pointer set to `nullptr` to identify a none value.
///
/// Keep in mind that constructing and destructing instances of niche values is undefined behavior. Niche values are intended to be constructed in byte arrays
/// directly.
export template <typename T, typename = void> struct NicheValue {};

template <typename T> concept HasNiche = requires {
    { NicheValue<T>::niche() }                           -> std::same_as<T>;
    { NicheValue<T>::isNiche(std::declval<const T&>()) } -> std::same_as<bool>;
};