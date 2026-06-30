export module projnekomata:core.ui.ui_system;
import std;
import :core.ui.ui_node;

export namespace projnekomata::ui {

inline class UiSystem* g_uiSystem = nullptr;

class UiSystem {
public:
    UiSystem(std::nullptr_t);

    static auto get() -> UiSystem& { return *g_uiSystem; }
    static auto create() -> std::unique_ptr<UiSystem>;

    auto getRoot() const -> ui::UiNode& { return *m_uiRoot; }

private:
    std::unique_ptr<ui::UiNode> m_uiRoot = nullptr;
};


}