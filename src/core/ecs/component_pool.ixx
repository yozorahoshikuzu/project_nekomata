export module nekomata2:core.ecs.component_pool;
import std;
import :core.ecs.entity;
import :core.platform.int_def;
import :core.platform.assert;

export namespace nekomata2::ecs {

constexpr u32 INVALID_INDEX = 0xffffffff;

template <typename T> struct ComponentSetSnapshot {
    std::vector<T> m_storage;
    std::vector<u32> m_sparseToStorage;
    std::vector<Entity> m_storageToEntity;

    [[nodiscard]] bool containsEntity(Entity ent) const {
        u32 idx = ent.index();
        return idx < m_sparseToStorage.size() && m_sparseToStorage[idx] != INVALID_INDEX;
    }

    T& get(Entity ent) {
        debug_assert(containsEntity(ent), "attempted to get component from entity that does not have that component");
        return m_storage[m_sparseToStorage[ent.index()]];
    }
};

class IComponentSet {
public:
    virtual ~IComponentSet() = default;

    [[nodiscard]] virtual bool containsEntity(Entity ent) const = 0;
    virtual void removeForEntity(Entity ent) = 0;
    [[nodiscard]] virtual usize size() const = 0;
};

template <typename T> class ComponentSet : public IComponentSet {
public:
    void resizeSparseToFit(u32 index) {
        if (index >= m_storage.size()) 
            m_sparseToStorage.resize(index + 1, INVALID_INDEX);
    }

    template <typename... Args> T& emplace(Entity e, Args&&... args) {
        u32 index = e.index();
        resizeSparseToFit(index);

        if (m_sparseToStorage[index] != INVALID_INDEX)
            throw std::runtime_error("attempted to add component to entity that already has that component");

        m_sparseToStorage[index] = static_cast<u32>(m_storage.size());
        m_storage.emplace_back(std::forward<Args>(args)...);

        m_storageToEntity.push_back(e);
        return m_storage.back();
    }

    void removeForEntity(Entity ent) override {
        if (!containsEntity(ent)) return;

        u32 storageIndex = m_sparseToStorage[ent.index()];
        u32 lastStorageIndex = static_cast<u32>(m_storage.size()) - 1;
        
        if (storageIndex != lastStorageIndex) {
            // NOTE: the component is not the last component in the dense vector, move the last element to this component
            m_storage[storageIndex] = std::move(m_storage[lastStorageIndex]);
            m_storageToEntity[storageIndex] = m_storageToEntity[lastStorageIndex];

            // NOTE: for the entity whose component was moved to correctly index it again
            m_sparseToStorage[m_storageToEntity[storageIndex].index()] = storageIndex;
        }

        m_storage.pop_back();
        m_storageToEntity.pop_back();
        m_sparseToStorage[ent.index()] = INVALID_INDEX;
    }

    [[nodiscard]] bool containsEntity(Entity ent) const override {
        u32 idx = ent.index();
        return idx < m_sparseToStorage.size() && m_sparseToStorage[idx] != INVALID_INDEX;
    }

    T& get(Entity ent) {
        debug_assert(containsEntity(ent), "attempted to get component from entity that does not have that component");
        return m_storage[m_sparseToStorage[ent.index()]];
    }

    [[nodiscard]] const T& get(Entity ent) const {
        debug_assert(containsEntity(ent), "attempted to get component from entity that does not have that component");
        return m_storage[m_sparseToStorage[ent.index()]];
    }

    T* tryGet(Entity ent) {
        if (!containsEntity(ent)) return nullptr;
        return &m_storage[m_sparseToStorage[ent.index()]];
    }

    [[nodiscard]] usize size() const override {
        return m_storage.size();
    }

    void copyTo(ComponentSetSnapshot<T>& buf) const {
        if (buf.m_storage.size() < m_storage.size())
            buf.m_storage.resize(m_storage.size());

        if (buf.m_sparseToStorage.size() < m_sparseToStorage.size())
            buf.m_sparseToStorage.resize(m_sparseToStorage.size());

        if (buf.m_storageToEntity.size() < m_storageToEntity.size())
            buf.m_storageToEntity.resize(m_storageToEntity.size());

        std::memcpy(buf.m_storage.data(),           m_storage.data(),           sizeof(T) * m_storage.size());
        std::memcpy(buf.m_sparseToStorage.data(), m_sparseToStorage.data(), sizeof(u32) * m_sparseToStorage.size());
        std::memcpy(buf.m_storageToEntity.data(), m_storageToEntity.data(), sizeof(Entity) * m_storageToEntity.size());
    }

private:

    std::vector<T>      m_storage;
    std::vector<u32>    m_sparseToStorage;
    std::vector<Entity> m_storageToEntity;
};

}