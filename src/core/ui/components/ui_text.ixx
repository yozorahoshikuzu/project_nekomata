export module projnekomata:core.ui.components.ui_text;
import std;
import :core.math;
import :graphics.fontsystem.font_face;

export namespace projnekomata::ui {

struct UiText {
    UiText() = default;
    explicit UiText(std::string_view text, float size, graphics::fonts::FontFace&& fontFace, math::Vector4f color = math::Vector4f(1.f))
        : size(size), fontFace(std::move(fontFace)), text(text), color(color) {}

    float size;
    graphics::fonts::FontFace fontFace;
    std::string text;

    math::Vector4f color;
};

}