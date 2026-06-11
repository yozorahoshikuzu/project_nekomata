export module nekomata2:core.ecs;
import std;
export import :core.ecs.component_pool;
export import :core.ecs.entity;
export import :core.ecs.script_base;
import :core.platform.int_def;
import :core.platform.assert;

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
            m_generations.emplace_back(1);
            generation = 1;
        }

        Entity ent = Entity::create(generation, index);
        m_aliveEntities.push_back(ent);
        return ent;
    }

    void destroyEntity(Entity ent) {
        if (!isEntityValid(ent))
            return;

        for (auto& cset : m_components | std::views::values)
            cset->removeForEntity(ent);

        removeAllScripts(ent);

        u32 index = ent.index();
        m_generations[index]++;
        m_entityIndexFreelist.push(index);

        std::erase_if(m_aliveEntities, [&](Entity other) { return other == ent; });
    }

    bool isEntityValid(Entity ent) const {
        u32 idx = ent.index();
        return idx < m_generations.size() && m_generations[idx] == ent.generation();
    }

    const std::vector<Entity>& entities() const { return m_aliveEntities; }
    usize entityCount() const { return m_aliveEntities.size(); }

    template <typename T, typename... Args> T& emplace(Entity e, Args&&... args) {
        debug_assert(isEntityValid(e), "attempted to add component to entity that does not exist");
        return components<T>().emplace(e, std::forward<Args>(args)...);
    }

    template <typename T> void remove(Entity e) { components<T>().remove(e); }

    template <typename T> bool has(Entity e) const {
        auto it = m_components.find(std::type_index(typeid(T)));
        if (it == m_components.end())
            return false;
        return it->second->containsEntity(e);
    }

    template <typename T> T& get(Entity e) { return components<T>().get(e); }

    template <typename T> const T& get(Entity e) const { return components<T>().get(e); }

    template <typename T> T* tryGet(Entity e) {
        auto it = m_components.find(std::type_index(typeid(T)));
        if (it == m_components.end())
            return nullptr;
        return static_cast<ComponentSet<T>*>(it->second.get())->try_get(e);
    }

    template <typename T> ComponentSet<T>& components() {
        auto key = std::type_index(typeid(T));
        auto it = m_components.find(key);
        if (it == m_components.end()) {
            auto p = std::make_unique<ComponentSet<T>>();
            auto* raw = p.get();
            m_components.emplace(key, std::move(p));
            return *raw;
        }
        return *static_cast<ComponentSet<T>*>(it->second.get());
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
        m_scripts[e].push_back(std::move(s));
        raw->onCreate();
        return *raw;
    }

    // Remove the given script type from the given entity.
    template <typename ScriptType> void removeScript(Entity ent) {
        auto it = m_scripts.find(ent);
        if (it == m_scripts.end())
            return;
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(), [](const auto& s) { return dynamic_cast<ScriptType*>(s.get()) != nullptr; }), vec.end());
    }

    void removeAllScripts(Entity ent) { m_scripts.erase(ent); }

    // Get first script of given type of given entity. Returns nullptr if absent.
    template <typename ScriptType> ScriptType* getScript(Entity ent) {
        auto it = m_scripts.find(ent);
        if (it == m_scripts.end())
            return nullptr;
        for (auto& s : it->second)
            if (auto* cast = dynamic_cast<ScriptType*>(s.get()))
                return cast;
        return nullptr;
    }

    void scriptsUpdate(float dt) {
        for (auto& vec : m_scripts | std::views::values)
            for (auto& s : vec)
                s->onUpdate(dt);
    }

private:
    std::vector<u32> m_generations;
    std::queue<u32> m_entityIndexFreelist;
    std::vector<Entity> m_aliveEntities;

    std::unordered_map<std::type_index, std::unique_ptr<IComponentSet>> m_components;
    std::unordered_map<Entity, std::vector<std::unique_ptr<ScriptBase>>, EntityHash> m_scripts;
};

} // namespace nekomata2::ecs