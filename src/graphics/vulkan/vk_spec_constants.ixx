export module projnekomata:graphics.vulkan.vk_spec_constants;
import std;
import vulkan;
import projnekomata.cs;

export namespace projnekomata {

template <typename Derived> class SpecializationConstsReflect {
public:
    [[nodiscard]] auto specializationInfo() const -> vk::SpecializationInfo {
        vk::SpecializationInfo info{};
        info.mapEntryCount = static_cast<u32>(entriesMap().len());
        info.pMapEntries = entriesMap().data();
        info.dataSize = sizeof(Derived);
        info.pData = reinterpret_cast<const void*>(this);
        return info;
    }
protected:
    constexpr static auto registerConstU32(usize offsetToMember, u32 constantId, u32 defaultVal) -> u32 {
        Derived* dummy = nullptr;
        auto entry = vk::SpecializationMapEntry{}
            .setConstantID(constantId)
            .setOffset(offsetToMember)
            .setSize(sizeof(u32));
        entriesMap().emplace(std::move(entry));
        return defaultVal;
    }
    constexpr static auto registerConstF32(usize offsetToMember, u32 constantId, f32 defaultVal) -> u32 {
        Derived* dummy = nullptr;
        auto entry = vk::SpecializationMapEntry{}
            .setConstantID(constantId)
            .setOffset(offsetToMember)
            .setSize(sizeof(f32));
        entriesMap().emplace(std::move(entry));
        return defaultVal;
    }
    template <typename E> requires (std::is_enum_v<E> && std::is_same_v<std::underlying_type_t<E>, u32>)
    constexpr static auto registerConstEnum(usize offsetToMember, u32 constantId, E defaultVal) -> E {
        return static_cast<E>(registerConstU32(offsetToMember, constantId, static_cast<u32>(defaultVal)));
    }
private:
    constexpr static auto entriesMap() -> Vec<vk::SpecializationMapEntry>& {
        static Vec<vk::SpecializationMapEntry> entries = Vec<vk::SpecializationMapEntry>::create();
        return entries;
    }
};

template <typename T> concept HasSpecializationInfo = requires(T t) { { t.specializationInfo() } -> std::convertible_to<vk::SpecializationInfo>; };

}