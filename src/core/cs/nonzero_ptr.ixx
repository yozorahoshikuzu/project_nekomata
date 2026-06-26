export module nekomata2:core.cs.nonzero_ptr;
import :core.cs.niche;

export template <typename T> class NonZeroPtr {
public:
    NonZeroPtr() = delete;
    explicit NonZeroPtr(T* ptr) {
        if (ptr == nullptr) __builtin_trap();
        m_ptr = ptr;
    }

    NonZeroPtr(const NonZeroPtr&) = default;
    NonZeroPtr& operator=(const NonZeroPtr&) = default;
    NonZeroPtr(NonZeroPtr&&) = default;
    NonZeroPtr& operator=(NonZeroPtr&&) = default;

    constexpr auto get()        const noexcept -> T* { return m_ptr; }
    constexpr auto operator*()  const noexcept -> T& { return *m_ptr; }
    constexpr auto operator->() const noexcept -> T* { return m_ptr; }

    constexpr auto operator==(const NonZeroPtr& other) const noexcept -> bool { return m_ptr == other.m_ptr; }
    constexpr auto operator!=(const NonZeroPtr& other) const noexcept -> bool { return m_ptr != other.m_ptr; }
    constexpr auto operator< (const NonZeroPtr& other) const noexcept -> bool { return m_ptr <  other.m_ptr; }

private:
    T* m_ptr;

    friend struct NicheValue<NonZeroPtr<T>>;
    explicit constexpr NonZeroPtr(std::nullptr_t) : m_ptr(nullptr) {}
};

template <typename T> struct NicheValue<NonZeroPtr<T>> {
    static NonZeroPtr<T> niche() { return NonZeroPtr<T>(nullptr); }
    static bool isNiche(const NonZeroPtr<T>& ptr) { return ptr.m_ptr == nullptr; }
};