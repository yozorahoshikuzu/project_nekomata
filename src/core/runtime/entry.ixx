export module nekomata2:core.runtime.entry;
import std;
import :core.ecs;

export namespace nekomata2 {

auto entry(const std::function<void(std::unique_ptr<ecs::World>&)>& initFn) -> void;
    
auto entryAfterSdlInit(const std::function<void(std::unique_ptr<ecs::World>&)>& initFn) -> void;

}