module nekomata2;
import :core.ui.ui_system;

namespace nekomata2::ui {

UiSystem::UiSystem(std::nullptr_t) {  }
auto UiSystem::create(math::Vector2f initialViewport) -> std::unique_ptr<UiSystem> {
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
auto UiSystem::handleViewportResize(math::Vector2f newViewportSize) -> void {
}

} // namespace nekomata2::ui