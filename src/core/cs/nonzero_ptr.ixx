export module nekomata2:core.cs.nonzero_ptr;
import :core.cs.niche;

export template <typename T> class NonNullPtr {
public:
    NonNullPtr() = delete;
    explicit NonNullPtr(T* ptr) {
        if (ptr == nullptr) __builtin_trap();
        m_ptr = ptr;
    }

    NonNullPtr(const NonNullPtr&) = default;
    NonNullPtr& operator=(const NonNullPtr&) = default;
    NonNullPtr(NonNullPtr&&) = default;
    NonNullPtr& operator=(NonNullPtr&&) = default;

    constexpr auto get()        const noexcept -> T* { return m_ptr; }
    constexpr auto operator*()  const noexcept -> T& { return *m_ptr; }
    constexpr auto operator->() const noexcept -> T* { return m_ptr; }

    constexpr auto operator==(const NonNullPtr& other) const noexcept -> bool { return m_ptr == other.m_ptr; }
    constexpr auto operator!=(const NonNullPtr& other) const noexcept -> bool { return m_ptr != other.m_ptr; }
    constexpr auto operator< (const NonNullPtr& other) const noexcept -> bool { return m_ptr <  other.m_ptr; }

private:
    T* m_ptr;

    friend struct NicheValue<NonNullPtr<T>>;
    explicit constexpr NonNullPtr(std::nullptr_t) : m_ptr(nullptr) {}
};

template <typename T> struct NicheValue<NonNullPtr<T>> {
    static NonNullPtr<T> niche() { return NonNullPtr<T>(nullptr); }
    static bool isNiche(const NonNullPtr<T>& ptr) { return ptr.m_ptr == nullptr; }
};