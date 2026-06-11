export module nekomata2.core.containers.sparse_set_storage;
import std;

export namespace nekomata2 {

template<typename T> class SparseSetStorage {
public:
    const size_t INVALID_INDEX = std::numeric_limits<usize>::max();

    auto allocate_one(const T& element) -> usize { return internal_allocate_one(element); }
    auto allocate_one(T&& element) -> usize { return internal_allocate_one(std::move(element)); }

    auto allocate_one_with_index(const T& element, usize index) -> usize { return internal_allocate_one_with_index(element, index); }
    auto allocate_one_with_index(T&& element, usize index) -> usize { return internal_allocate_one_with_index(std::move(element), index); }

    auto remove(usize index) -> std::optional<T> {
        if (index >= m_index_to_storage.size())
            return std::nullopt;

        usize maybe_storage_index = m_index_to_storage[index];
        if (maybe_storage_index == INVALID_INDEX)
            return std::nullopt;

        m_index_to_storage[index] = INVALID_INDEX;
        usize storage_index = maybe_storage_index;
        usize last_storage_index = m_storage.size() - 1;

        m_index_freelist.push_back(index);

        if (storage_index != last_storage_index) {
            std::swap(m_storage[storage_index], m_storage[last_storage_index]);
            std::swap(m_storage_to_index[storage_index], m_storage_to_index[last_storage_index]);

            size_t moved_index = m_storage_to_index[storage_index];
            m_index_to_storage[moved_index] = storage_index;
        }

        m_storage_to_index.pop_back();

        auto removed = std::move(m_storage.back());
        m_storage.pop_back();

        return removed;
    }

    auto get(usize index) -> T* {
        if (index >= m_index_to_storage.size()) {
            return nullptr;
        }

        auto storage_index = m_index_to_storage[index];
        if (storage_index == INVALID_INDEX) {
            return nullptr;
        }

        return &m_storage[storage_index];
    }

    auto get(usize index) const -> const T* {
        if (index >= m_index_to_storage.size())
            return nullptr;

        auto storage_index = m_index_to_storage[index];
        if (storage_index == INVALID_INDEX)
            return nullptr;

        return &m_storage[storage_index];
    }

    auto contains(usize index) const -> bool {
        return index < m_index_to_storage.size() && m_index_to_storage[index] != INVALID_INDEX;
    }

    auto len() const -> usize {
        return m_storage.size();
    }

    auto empty() const -> bool {
        return m_storage.empty();
    }

    auto begin() { return m_storage.begin(); }
    auto end() { return m_storage.end(); }

    auto begin() const { return m_storage.begin(); }
    auto end() const { return m_storage.end(); }

    const std::vector<T>& dense() const
    {
        return m_storage;
    }

    const std::vector<size_t>& dense_indices() const
    {
        return m_storage_to_index;
    }
private:
    template <typename U> auto internal_allocate_one(U&& element) -> usize {
        usize index;

        if (!m_index_freelist.empty()) {
            index = m_index_freelist.back();
            m_index_freelist.pop_back();
        } else {
            index = m_index_to_storage.size();
            m_index_to_storage.push_back(INVALID_INDEX);
        }

        usize storage_index = m_storage.size();
        
        m_storage.emplace_back(std::forward<U>(element));
        m_storage_to_index.push_back(index);
        m_index_to_storage[index] = storage_index;

        return index;
    }

    template <typename U> auto internal_allocate_one_with_index(U&& element, usize index) -> void {
        if (index >= m_index_to_storage.size())
            m_index_to_storage.resize(index + 1);

        if (m_index_to_storage[index] != INVALID_INDEX) {
            m_storage[m_index_to_storage[index]] = std::move(element);
            return;
        }

        usize storage_index = m_storage.size();
        m_storage.push_back(element);
        m_storage_to_index.push_back(index);
        m_index_to_storage[index] = storage_index;
    }

    std::vector<T> m_storage;
    std::vector<usize> m_storage_to_index;
    std::vector<usize> m_index_to_storage;
    std::vector<usize> m_index_freelist;
};

}