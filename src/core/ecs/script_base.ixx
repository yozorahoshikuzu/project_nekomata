export module nekomata2.core.ecs.script_base;
import nekomata2.core.ecs.entity;

export namespace nekomata2::ecs {

class ScriptBase {
public:
    Entity       m_workingEntity;
    class World* m_workingWorld{};

    virtual ~ScriptBase() = default;

    virtual void onCreate()  {}
    virtual void onDestroy() {}

    virtual void onUpdate(float dt) {}
};

}