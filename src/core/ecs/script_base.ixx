export module projnekomata:core.ecs.script_base;
import :core.ecs.entity;

export namespace projnekomata::ecs {

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