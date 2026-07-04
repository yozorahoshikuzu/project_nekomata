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
    auto buildUi(Vec<ui::UiDrawCmd>& drawcmds, math::Vector2f screenLogicalSize) -> void;

    auto testMouseDownHit(math::Vector2f pos) -> void;
    auto testMouseUpHit(math::Vector2f pos) -> void;

    auto testMouseHover(math::Vector2f pos) -> void;

private:
    Vec<ui::UiMouseHitRegion>   m_lastFrameMouseHitRegions = Vec<ui::UiMouseHitRegion>::create();
    ui::UiNode* m_pressedElement = nullptr;
    ui::UiNode* m_hoveredElement = nullptr;

    std::unique_ptr<ui::UiNode> m_uiRoot = nullptr;
};


}