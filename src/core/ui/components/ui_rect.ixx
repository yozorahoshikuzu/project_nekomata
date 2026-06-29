export module projnekomata:core.ui.components.ui_rect;
import :core.math;

export namespace projnekomata::ui {

struct UiRect {
    UiRect() = default;
    explicit UiRect(math::Vector4f color) : color(color) {}

    math::Vector4f color;
};

}