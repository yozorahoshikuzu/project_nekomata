export module nekomata2:core.ui.components.ui_rect;
import :core.math;

export namespace nekomata2::ui {

struct UiRect {
    UiRect() = default;
    explicit UiRect(math::Vector4f color) : color(color) {}

    math::Vector4f color;
};

}