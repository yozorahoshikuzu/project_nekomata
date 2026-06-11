export module nekomata2.core.containers.double_buffer;
import std;

export template <typename T> class DoubleBuffer {
public:
    DoubleBuffer() = default;
    DoubleBuffer(T&& primary, T&& secondary) : m_primary(std::move(primary)), m_secondary(std::move(secondary)) {}

    T& getPrimary() {
        if (m_isSwapped) {
            return m_secondary;
        }
        return m_primary;
    }

    T& getSecondary() {
        if (m_isSwapped) {
            return m_primary;
        }
        return m_secondary;
    }

    bool isSwapped() {
        return m_isSwapped;
    }

    void swap() {
        m_isSwapped = !m_isSwapped;
    }
    
private:
    T m_primary;
    T m_secondary;

    bool m_isSwapped = false;
};