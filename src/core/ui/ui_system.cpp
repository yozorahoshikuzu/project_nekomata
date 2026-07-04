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
    m_uiRoot->buildDrawCmds(drawcmds, m_lastFrameMouseHitRegions, screenLogicalSize, math::Vector2f(0.0f), screenLogicalSize, m_pressedElement, m_hoveredElement, false, false);
}

auto UiSystem::testMouseDownHit(math::Vector2f pos) -> void {
    for (auto& [position, extent, ref, capturesClicks, _, _, _] : m_lastFrameMouseHitRegions.iterRev()) {
        // todo: Make a math box/aabb type to do this
        if (position.x() <= pos.x() && pos.x() <= position.x() + extent.x()
            && position.y() <= pos.y() && pos.y() <= position.y() + extent.y()
            && capturesClicks)
        {
            m_pressedElement = ref;
        }
    }
}
auto UiSystem::testMouseUpHit(math::Vector2f pos) -> void {
    for (auto& [position, extent, ref, capturesClicks, _, clickCallback, _] : m_lastFrameMouseHitRegions.iterRev()) {
        if (
            ref == m_pressedElement
             && position.x() <= pos.x() && pos.x() <= position.x() + extent.x()
             && position.y() <= pos.y() && pos.y() <= position.y() + extent.y()
             && capturesClicks
             && clickCallback.isSome()
        ) {
            auto& callback = clickCallback.unwrap();
            callback(pos);
        }
    }
    m_pressedElement = nullptr;
}
auto UiSystem::testMouseHover(math::Vector2f pos) -> void {
    m_hoveredElement = nullptr;
    for (auto& [position, extent, ref, _, capturesHover, _, hoverCallback] : m_lastFrameMouseHitRegions.iterRev()) {
        if (
            position.x() <= pos.x() && pos.x() <= position.x() + extent.x()
            && position.y() <= pos.y() && pos.y() <= position.y() + extent.y()
            && capturesHover
        ) {
            m_hoveredElement = ref;

            if (hoverCallback.isSome()) {
                auto& callback = hoverCallback.unwrap();
                callback(pos);
            }
        }
    }
}

} // namespace projnekomata::ui