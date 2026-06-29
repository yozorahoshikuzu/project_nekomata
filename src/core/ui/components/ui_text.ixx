export module projnekomata:core.ui.components.ui_text;
import std;
import :graphics.fontsystem.font_face;

export namespace projnekomata::ui {

struct UiText {
    UiText() = default;
    explicit UiText(std::string_view text, float size, graphics::fonts::FontFace&& fontFace)
        : size(size), fontFace(std::move(fontFace)), text(text) {}

    float size;
    graphics::fonts::FontFace fontFace;
    std::string text;
};

}