module projnekomata;
import :core.ui.ui_system;

namespace projnekomata::ui {

UiSystem::UiSystem(std::nullptr_t) {  }

auto UiSystem::create() -> std::unique_ptr<UiSystem> {
    debug_assert(g_uiSystem == nullptr, "UiSystem already exists");
    auto inst = std::make_unique<UiSystem>(nullptr);
    g_uiSystem = inst.get();

    auto viewportElem = UiNode::builder()
        .position({0.0f, 0.0f})
        .extentPercent({100.0f, 100.0f})
        .build();

    inst->m_uiRoot = std::move(viewportElem);

    return inst;
}

auto UiSystem::buildUi(Vec<ui::UiDrawCmd>& drawcmds, math::Vector2f screenLogicalSize) -> void {
    m_lastFrameMouseHitRegions.clear();
    m_uiRoot->buildDrawCmds(drawcmds, m_lastFrameMouseHitRegions, screenLogicalSize, math::Vector2f(0.0f), screenLogicalSize);
}

auto UiSystem::testMouseDownHit(math::Vector2f pos) -> void {
    for (auto& [position, extent, ref, clickCallback] : m_lastFrameMouseHitRegions.iterRev()) {
        // todo: Make a math box/aabb type to do this
        if (position.x() <= pos.x() && pos.x() <= position.x() + extent.x()
            && position.y() <= pos.y() && pos.y() <= position.y() + extent.y())
        {
            m_pressedElement = ref;
        }
    }
}
auto UiSystem::testMouseUpHit(math::Vector2f pos) -> void {
    for (auto& [position, extent, ref, clickCallback] : m_lastFrameMouseHitRegions.iterRev()) {
        if (
            ref == m_pressedElement
             && position.x() <= pos.x() && pos.x() <= position.x() + extent.x()
             && position.y() <= pos.y() && pos.y() <= position.y() + extent.y()
        ) {
            clickCallback(pos);
        }
    }
    m_pressedElement = nullptr;
}

} // namespace projnekomata::ui