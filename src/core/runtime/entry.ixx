export module projnekomata:core.runtime.entry;
import std;
import :core.ecs;

export namespace projnekomata {

auto entry(const std::function<void(Unique<ecs::World>&)>& initFn) -> void;
    
auto entryAfterSdlInit(const std::function<void(Unique<ecs::World>&)>& initFn) -> void;

}