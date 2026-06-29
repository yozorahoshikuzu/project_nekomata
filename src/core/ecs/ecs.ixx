export module nekomata2:core.ecs;
import std;
export import :core.ecs.component_pool;
export import :core.ecs.entity;
export import :core.ecs.script_base;
import :core.platform.int_def;
import :core.platform.assert;
import :core.cs.hashmap;

export namespace nekomata2::ecs {

class World {
public:
    World() = default;
    auto createEntity() -> Entity {
        u32 index;
        u32 generation;

        if (!m_entityIndexFreelist.empty()) {
            index = m_entityIndexFreelist.front();
            m_entityIndexFreelist.pop();
            // NOTE: destroying an entity already incremented that generation
            generation = m_generations[index];
        } else {
            index = static_cast<u32>(m_generations.size());
            m_generations.emplace(1);
            generation = 1;
        }

        Entity ent = Entity::create(generation, index);
        m_aliveEntities.emplace(ent);
        return ent;
    }

    void destroyEntity(Entity ent) {
        if (!isEntityValid(ent))
            return;

        for (auto& cset : m_components.values())
            cset->removeForEntity(ent);

        removeAllScripts(ent);

        u32 index = ent.index();
        m_generations[index]++;
        m_entityIndexFreelist.push(index);

        m_aliveEntities.retain([&](const auto& other) -> bool { return other != ent; });
    }

    bool isEntityValid(Entity ent) const {
        u32 idx = ent.index();
        return idx < m_generations.size() && m_generations[idx] == ent.generation();
    }

    const Vec<Entity>& entities() const { return m_aliveEntities; }
    usize entityCount() const { return m_aliveEntities.size(); }

    template <typename T, typename... Args> T& emplace(Entity e, Args&&... args) {
        debug_assert(isEntityValid(e), "attempted to add component to entity that does not exist");
        return components<T>().emplace(e, std::forward<Args>(args)...);
    }

    template <typename T> void remove(Entity e) { components<T>().remove(e); }

    template <typename T> bool has(Entity e) {
        auto vect = m_components.get(std::type_index(typeid(T)));
        if (!vect.has_value()) return false;
        return vect->get()->containsEntity(e);
    }

    template <typename T> T& get(Entity e) { return components<T>().get(e); }

    template <typename T> const T& get(Entity e) const { return components<T>().get(e); }

    template <typename T> T* tryGet(Entity e) {
        auto vect = m_components.get(std::type_index(typeid(T)));
        if (!vect.has_value()) return nullptr;
        return static_cast<ComponentSet<T>*>(vect->get())->try_get(e);
    }

    template <typename T> ComponentSet<T>& components() {
        auto key = std::type_index(typeid(T));
        auto vect = m_components.get(key);
        if (!vect.has_value()) {
            auto set = std::make_unique<ComponentSet<T>>();
            auto& p = m_components.insert(std::move(key), std::move(set));
            return *static_cast<ComponentSet<T>*>(p.get());
        }
        return *static_cast<ComponentSet<T>*>(vect.value().get().get());
    }

    // Attach a script of the given type to the given entity.
    // The given script must derive from ScriptBase.
    template <typename ScriptType, typename... Args> ScriptType& addScript(Entity e, Args&&... args) {
        static_assert(std::is_base_of_v<ScriptBase, ScriptType>, "S must derive from ScriptBase");
        debug_assert(isEntityValid(e), "attempted to add script to entity that does not exist");

        auto s = std::make_unique<ScriptType>(std::forward<Args>(args)...);
        s->m_workingEntity = e;
        s->m_workingWorld = this;

        ScriptType* raw = s.get();
        if (!m_scripts.contains(e)) m_scripts.insert(e, Vec<std::unique_ptr<ScriptBase>>::create());
        m_scripts[e].emplace(std::move(s));
        raw->onCreate();
        return *raw;
    }

    // Remove the given script type from the given entity.
    template <typename ScriptType> void removeScript(Entity ent) {
        auto vec = m_scripts.get(ent);
        if (!vec.has_value()) return;

        auto& cont = vec->get();
        cont.retain([](const auto& s) -> bool { return dynamic_cast<ScriptType*>(s.get()) == nullptr; });
    }

    void removeAllScripts(Entity ent) { m_scripts.remove(ent); }

    // Get first script of given type of given entity. Returns nullptr if absent.
    template <typename ScriptType> ScriptType* getScript(Entity ent) {
        auto vec = m_scripts.get(ent);
        if (!vec.has_value()) return nullptr;

        for (auto& s : vec->get())
            if (auto* cast = dynamic_cast<ScriptType*>(s.get()))
                return cast;
        return nullptr;
    }

    void scriptsUpdate(float dt) {
        for (auto& vec : m_scripts.values())
            for (auto& s : vec)
                s->onUpdate(dt);
    }

private:
    Vec<u32> m_generations;
    std::queue<u32> m_entityIndexFreelist;
    Vec<Entity> m_aliveEntities;

    HashMap<std::type_index, std::unique_ptr<IComponentSet>> m_components   = HashMap<std::type_index, std::unique_ptr<IComponentSet>>::create();


    HashMap<Entity, Vec<std::unique_ptr<ScriptBase>>, EntityHash> m_scripts = HashMap<Entity, Vec<std::unique_ptr<ScriptBase>>, EntityHash>::create();
};

} // namespace nekomata2::ecs